#!/usr/bin/env python3
"""Fail closed unless a Boij benchmark bundle is internally reproducible."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import re
import sys
from collections import Counter
from pathlib import Path
from typing import Any
from run_benchmarks import normalize_cpp, normalize_m2, parse_time_file



SCHEMA_VERSION = 1
KNOWN_REGRESSION = [0, 1, 2, 3, 4, 5, 11, 12]
REQUIRED_TOP_LEVEL = {
    "checksums.sha256", "environment.json", "manifest.json", "runs.jsonl",
    "summary.csv", "summary.json", "validation.json",
}
PRIVATE_TEXT = re.compile(
    r"(?i)(?:/home/[^/\s]+|/mnt/[a-z]/users/[^/\s]+|[a-z]:[\\/]+users[\\/]+[^\\/\s]+|/users/[^/\s]+|WIN-[A-Z0-9-]{6,})"
)
SHA256 = re.compile(r"[0-9a-f]{64}")
COMMIT = re.compile(r"[0-9a-f]{40}")


class ValidationError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValidationError(message)


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValidationError(f"cannot read {path.name}: {error}") from error


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def sequence_hash(sequences: list[list[int]]) -> str:
    payload = json.dumps(sequences, sort_keys=True, separators=(",", ":")).encode("ascii")
    return hashlib.sha256(payload).hexdigest()


def safe_logical_path(value: Any, label: str) -> str:
    require(isinstance(value, str) and value, f"{label} is not a nonempty path")
    require("\\" not in value and "\x00" not in value, f"{label} uses an unsafe separator")
    require(not value.startswith("/") and not re.match(r"^[A-Za-z]:", value), f"{label} is absolute")
    require(all(part not in ("", ".", "..") for part in value.split("/")), f"{label} is not normalized")
    if value.startswith("$"):
        require(re.match(r"^\$(?:REPO_ROOT|OUTPUT_ROOT|EXTERNAL_BINARY)(?:/[^/]+)*$", value) is not None,
                f"{label} uses an unknown logical root")
    return value


def bundle_file(bundle: Path, value: Any, label: str) -> Path:
    relative = safe_logical_path(value, label)
    require(not relative.startswith("$"), f"{label} must refer to an in-bundle artifact")
    path = bundle / relative
    require(path.is_file(), f"{label} artifact is missing: {relative}")
    return path


def canonical_sequences(raw: Any, case: dict[str, Any], label: str) -> list[list[int]]:
    require(isinstance(raw, list), f"{label} is not a list")
    canonical: list[tuple[int, ...]] = []
    codimension = case["codimension"]
    max_degree = case["max_degree"]
    lowbound = max(1, case["lowbound"])
    for sequence in raw:
        require(isinstance(sequence, list), f"{label} contains a non-list")
        require(len(sequence) == codimension + 1, f"{label} contains a wrong-length sequence")
        require(all(type(value) is int for value in sequence), f"{label} contains a non-integer")
        require(sequence[0] == 0, f"{label} contains a sequence not starting at zero")
        require(all(left < right for left, right in zip(sequence, sequence[1:])),
                f"{label} contains a non-increasing sequence")
        require(sequence[-1] <= max_degree and sequence[-1] >= lowbound,
                f"{label} contains a sequence outside the case bounds")
        canonical.append(tuple(sequence))
    require(len(canonical) == len(set(canonical)), f"{label} contains duplicates")
    return [list(sequence) for sequence in sorted(canonical)]


def expected_candidate_count(case: dict[str, Any]) -> int:
    c, d, low = case["codimension"], case["max_degree"], max(1, case["lowbound"])
    return math.comb(d, c) - (math.comb(low - 1, c) if low - 1 >= c else 0)


def validate_numeric_tree(value: Any, label: str) -> None:
    if isinstance(value, dict):
        for key, child in value.items():
            validate_numeric_tree(child, f"{label}.{key}")
    elif value is not None:
        require(type(value) in (int, float), f"{label} is not numeric")
        require(math.isfinite(float(value)) and float(value) >= 0, f"{label} is not finite/nonnegative")


def validate_parsed(bundle: Path, engine: str, entry: dict[str, Any], case: dict[str, Any],
                    expected_build: dict[str, Any], label: str) -> dict[str, Any]:
    require(entry.get("status") == "ok", f"{label} engine status is not ok")
    require(entry.get("exit_code") == 0 and entry.get("timed_out") is False,
            f"{label} did not exit normally")
    require(entry.get("parse_error") is None, f"{label} has a parse error")
    command = entry.get("command")
    require(isinstance(command, list) and command, f"{label} command is missing")
    for index, token in enumerate(command):
        require(isinstance(token, str), f"{label} command token is not text")
        if index == 0 or token.endswith((".m2", "/benchmark_driver")):
            safe_logical_path(token, f"{label} command token")
    measurements = entry.get("measurements")
    require(isinstance(measurements, dict), f"{label} measurements are missing")
    validate_numeric_tree(measurements, f"{label}.measurements")
    artifacts = entry.get("artifacts")
    require(isinstance(artifacts, dict), f"{label} artifacts are missing")
    stdout_path = bundle_file(bundle, artifacts.get("stdout"), f"{label}.stdout")
    bundle_file(bundle, artifacts.get("stderr"), f"{label}.stderr")
    time_path = bundle_file(bundle, artifacts.get("resource_time"), f"{label}.resource_time")
    parsed = load_json(bundle_file(bundle, artifacts.get("parsed_exact_results"), f"{label}.parsed"))
    try:
        raw_stdout = stdout_path.read_text(encoding="utf-8")
        normalized_raw = normalize_cpp(raw_stdout) if engine == "cpp" else normalize_m2(raw_stdout, case)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        raise ValidationError(f"{label} raw stdout cannot be normalized: {error}") from error
    require(normalized_raw == parsed, f"{label} parsed result is not derived from raw stdout")
    raw_time = parse_time_file(time_path)
    for key in ("gnu_time_elapsed_seconds", "user_seconds", "system_seconds", "max_rss_kb"):
        raw_key = "elapsed_seconds" if key == "gnu_time_elapsed_seconds" else key
        require(measurements.get(key) == raw_time.get(raw_key), f"{label} GNU time measurement differs")
    require("exit_code=0" in time_path.read_text(encoding="utf-8").splitlines(),
            f"{label} GNU time exit code is not zero")
    require(parsed.get("schema_version") == SCHEMA_VERSION, f"{label} parsed schema differs")
    require(parsed.get("engine") == ("cpp" if engine == "cpp" else "macaulay2_builtin"),
            f"{label} parsed engine differs")
    task = parsed.get("task")
    require(isinstance(task, dict), f"{label} parsed task is missing")
    for key in ("codimension", "max_degree", "lowbound"):
        require(task.get(key) == case[key], f"{label} parsed task differs on {key}")
    bad = canonical_sequences(parsed.get("bad_sequences"), case, f"{label}.bad")
    rinsed = canonical_sequences(parsed.get("gcd_rinsed_sequences"), case, f"{label}.rinsed")
    counts = parsed.get("counts")
    require(isinstance(counts, dict) and counts == entry.get("counts"), f"{label} counts differ")
    require(counts.get("candidates") == expected_candidate_count(case), f"{label} candidate count differs")
    require(counts.get("bad") == len(bad) and counts.get("gcd_rinsed") == len(rinsed),
            f"{label} arrays/counts differ")
    bad_hash, rinsed_hash = sequence_hash(bad), sequence_hash(rinsed)
    require(parsed.get("bad_sha256") == bad_hash == entry.get("bad_sha256"), f"{label} bad hash differs")
    require(parsed.get("gcd_rinsed_sha256") == rinsed_hash == entry.get("gcd_rinsed_sha256"),
            f"{label} rinsed hash differs")
    phases = parsed.get("phase_seconds" if engine == "cpp" else "phase_cpu_seconds")
    validate_numeric_tree(phases, f"{label}.phases")
    if engine == "cpp":
        require(parsed.get("build") == expected_build == entry.get("build"), f"{label} build differs")
    else:
        parsed_oracle = parsed.get("oracle")
        require(isinstance(parsed_oracle, dict) and parsed_oracle.get("package") == "BoijSoederberg"
                and isinstance(parsed_oracle.get("package_version"), str)
                and parsed_oracle.get("package_version"), f"{label} oracle provenance differs")
    return {"bad": bad, "rinsed": rinsed, "counts": counts, "oracle": parsed.get("oracle")}


def validate_record(bundle: Path, record: dict[str, Any], case: dict[str, Any], repetition: int,
                    expected_build: dict[str, Any], label: str) -> dict[str, Any]:
    require(record.get("schema_version") == SCHEMA_VERSION, f"{label} schema differs")
    require(record.get("case_id") == case["id"] and record.get("case") == case, f"{label} case differs")
    require(record.get("repetition") == repetition, f"{label} repetition differs")
    expected_order = ["cpp", "m2"] if repetition % 2 else ["m2", "cpp"]
    if repetition == 0:
        expected_order = ["cpp", "m2"]
    require(record.get("execution_order") == expected_order, f"{label} execution order differs")
    require(record.get("status") == "ok", f"{label} pair status is not ok")
    require(record.get("expected_candidate_count") == expected_candidate_count(case),
            f"{label} expected candidate count differs")
    require(record.get("equivalence") == {
        "candidate_count_matches_contract": True,
        "bad_set_exact_match": True,
        "gcd_rinsed_set_exact_match": True,
    }, f"{label} equivalence flags differ")
    engines = record.get("engines")
    require(isinstance(engines, dict) and set(engines) == {"cpp", "m2"}, f"{label} engines differ")
    cpp = validate_parsed(bundle, "cpp", engines["cpp"], case, expected_build, f"{label}.cpp")
    m2 = validate_parsed(bundle, "m2", engines["m2"], case, expected_build, f"{label}.m2")
    require(cpp["bad"] == m2["bad"], f"{label} exact bad sets differ")
    require(cpp["rinsed"] == m2["rinsed"], f"{label} exact rinsed sets differ")
    require(cpp["counts"] == m2["counts"], f"{label} engine counts differ")
    cpp["m2_oracle"] = m2["oracle"]
    return cpp


def validate_checksums(bundle: Path) -> None:
    checksum_path = bundle / "checksums.sha256"
    require(checksum_path.is_file(), "checksums.sha256 is missing")
    recorded: dict[str, str] = {}
    for line in checksum_path.read_text(encoding="utf-8").splitlines():
        match = re.fullmatch(r"([0-9a-f]{64})  (.+)", line)
        require(match is not None, "checksum inventory has a malformed line")
        digest, relative = match.groups()
        safe_logical_path(relative, "checksum path")
        require(relative not in recorded, f"duplicate checksum path: {relative}")
        recorded[relative] = digest
    actual = {
        path.relative_to(bundle).as_posix(): sha256_file(path)
        for path in bundle.rglob("*") if path.is_file() and path != checksum_path
    }
    require(recorded == actual, "checksum inventory or digest differs from bundle contents")


def validate_privacy(bundle: Path) -> None:
    for path in bundle.rglob("*"):
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        require(PRIVATE_TEXT.search(text) is None,
                f"private workstation identifier in {path.relative_to(bundle)}")
        require("\"PATH\"" not in text and "\"HOSTNAME\"" not in text,
                f"unsafe environment variable captured in {path.relative_to(bundle)}")


def validate(bundle: Path, allow_dirty: bool) -> None:
    require(bundle.is_dir(), "bundle path is not a directory")
    present = {path.name for path in bundle.iterdir() if path.is_file()}
    require(REQUIRED_TOP_LEVEL <= present, "required top-level artifacts are missing")
    validate_checksums(bundle)
    validate_privacy(bundle)

    manifest = load_json(bundle / "manifest.json")
    environment = load_json(bundle / "environment.json")
    summary = load_json(bundle / "summary.json")
    validation = load_json(bundle / "validation.json")
    require(manifest.get("schema_version") == SCHEMA_VERSION, "manifest schema differs")
    require(manifest.get("bundle_kind") == "boij_task_matched_benchmark_bundle", "bundle kind differs")
    require(manifest.get("run_id") == bundle.name, "run id and directory name differ")
    require(environment.get("schema_version") == SCHEMA_VERSION, "environment schema differs")
    require(environment.get("repository_path") == "$REPO_ROOT", "environment path is unsafe")
    require("PATH" not in environment.get("environment_variables", {}), "PATH was captured")
    tools = environment.get("tools")
    require(isinstance(tools, dict) and tools.get("macaulay2_version"),
            "environment Macaulay2 version is missing")

    repository = manifest.get("repository")
    require(isinstance(repository, dict) and COMMIT.fullmatch(str(repository.get("commit"))) is not None,
            "repository commit is not a full SHA")
    dirty = repository.get("dirty")
    require(type(dirty) is bool, "repository dirty flag is not boolean")
    require(allow_dirty or not dirty, "dirty bundle rejected; use --allow-dirty only for scratch validation")
    require((repository.get("working_diff_sha256") is None) if not dirty else
            SHA256.fullmatch(str(repository.get("working_diff_sha256"))) is not None,
            "working diff provenance is inconsistent")
    binary = manifest.get("binary")
    require(isinstance(binary, dict), "binary provenance is missing")
    safe_logical_path(binary.get("path"), "binary path")
    require(SHA256.fullmatch(str(binary.get("sha256"))) is not None, "binary SHA is malformed")

    build = manifest.get("build_contract")
    require(isinstance(build, dict) and build.get("passed") is True, "build contract failed")
    expected_build = build.get("expected")
    require(expected_build == build.get("preflight_driver_build"), "driver build differs from contract")
    require(expected_build.get("commit") == repository["commit"] and expected_build.get("dirty") == dirty,
            "build/repository provenance differs")
    require(manifest.get("working_directory") == "$REPO_ROOT", "working directory is not logical")
    for token in manifest.get("runner_command", []):
        require(isinstance(token, str), "runner command token is not text")
        if token.endswith(".py") or token.startswith(("--binary=", "--output-root=")):
            safe_logical_path(token.split("=", 1)[-1], "runner command path")
    oracle = manifest.get("macaulay2_oracle")
    require(isinstance(oracle, dict) and oracle.get("package") == "BoijSoederberg"
            and oracle.get("legacy_transcription_used") is False, "oracle provenance differs")
    require(oracle.get("version") == tools["macaulay2_version"], "Macaulay2 versions differ")
    require(isinstance(oracle.get("package_version"), str) and oracle.get("package_version"),
            "BoijSoederberg package version is missing")
    require(oracle.get("template") == "research/macaulay2/benchmark_builtin_task.m2.in",
            "oracle template path differs")
    require(SHA256.fullmatch(str(oracle.get("template_sha256"))) is not None,
            "oracle template SHA is malformed")
    require(oracle.get("per_run_command") == ["M2", "--script", "scripts/<case>-r<repetition>.m2"],
            "oracle command template differs")

    profile = manifest.get("profile_definition")
    require(isinstance(profile, dict) and isinstance(profile.get("cases"), list), "profile is malformed")
    repetitions = profile.get("repetitions")
    require(type(repetitions) is int and repetitions >= 3, "profile has too few repetitions")
    try:
        records = [json.loads(line) for line in (bundle / "runs.jsonl").read_text(encoding="utf-8").splitlines() if line]
    except json.JSONDecodeError as error:
        raise ValidationError(f"runs.jsonl is malformed: {error}") from error
    require(len(records) == len(profile["cases"]) * repetitions == manifest.get("run_count"),
            "record count differs from profile")
    seen = Counter((record.get("case_id"), record.get("repetition")) for record in records)
    for case in profile["cases"]:
        require(set(case) == {"id", "codimension", "max_degree", "lowbound"}, "case schema differs")
        for repetition in range(1, repetitions + 1):
            require(seen[(case["id"], repetition)] == 1, "case/repetition coverage differs")
            record = next(r for r in records if r.get("case_id") == case["id"] and r.get("repetition") == repetition)
            result = validate_record(bundle, record, case, repetition, expected_build,
                                     f"{case['id']}-r{repetition}")
            require(result["m2_oracle"]["package_version"] == oracle["package_version"], "oracle package versions differ")
    require(manifest.get("status_counts") == {"ok": len(records)}, "manifest status accounting differs")

    preflight = manifest.get("correctness_preflight")
    require(isinstance(preflight, dict) and preflight.get("passed") is True, "correctness preflight failed")
    require(preflight.get("known_missed_by_legacy_transcription") == KNOWN_REGRESSION,
            "known regression sentinel differs")
    preflight_result = validate_record(bundle, preflight["record"], preflight["case"], 0,
                                       expected_build, "preflight")
    require(preflight_result["counts"] == {"candidates": 792, "bad": 28, "gcd_rinsed": 26},
            "preflight reference counts differ")
    require(KNOWN_REGRESSION in preflight_result["bad"], "known regression sequence is absent")
    require(preflight_result["m2_oracle"]["package_version"] == oracle["package_version"],
            "preflight oracle package version differs")

    require(validation.get("all_pairs_ok") is True
            and validation.get("all_exact_bad_sets_match") is True
            and validation.get("all_exact_gcd_rinsed_sets_match") is True
            and validation.get("correctness_preflight_passed") is True,
            "validation summary is not all-pass")
    probes = validation.get("safe_and_overflow_probes")
    require(isinstance(probes, dict) and set(probes) == {"safe", "overflow"}, "probe evidence differs")
    require(all(probe.get("passed") is True for probe in probes.values()), "an arithmetic probe failed")
    for name, probe in probes.items():
        bundle_file(bundle, probe.get("stdout"), f"{name} probe stdout")
        bundle_file(bundle, probe.get("stderr"), f"{name} probe stderr")

    require(summary.get("schema_version") == SCHEMA_VERSION, "summary schema differs")
    require(summary.get("sample_accounting") == {
        "planned_pair_count": len(records), "observed_pair_count": len(records),
        "successful_pair_count": len(records),
    }, "summary sample accounting differs")
    summary_cases = summary.get("cases")
    require(isinstance(summary_cases, list) and len(summary_cases) == len(profile["cases"]),
            "summary case coverage differs")
    for item in summary_cases:
        require(item.get("planned_repetitions") == repetitions
                and item.get("successful_repetitions") == repetitions
                and item.get("status_counts") == {"ok": repetitions}, "summary repetitions differ")
        validate_numeric_tree({key: value for key, value in item.items() if key.endswith(("seconds", "kb"))},
                              f"summary.{item.get('case_id')}")
    with (bundle / "summary.csv").open(newline="", encoding="utf-8") as handle:
        require(len(list(csv.DictReader(handle))) == len(profile["cases"]), "summary CSV rows differ")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundle", type=Path)
    parser.add_argument("--allow-dirty", action="store_true")
    args = parser.parse_args()
    try:
        validate(args.bundle.resolve(), args.allow_dirty)
    except ValidationError as error:
        print(f"INVALID: {error}", file=sys.stderr)
        return 1
    print(f"VALID: {args.bundle}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
