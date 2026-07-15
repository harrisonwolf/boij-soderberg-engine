#!/usr/bin/env python3
"""Run task-matched C++/Macaulay2 comparisons into immutable evidence bundles."""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import hashlib
import json
import math
import os
import platform
import re
import shutil
import signal
import statistics
import subprocess
import sys
import time
from collections import Counter
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1
# Exact B_4 is 14,043,766,264,500,157,900, so the final value exceeds int64.
KNOWN_REGRESSION = [0, 1, 2, 3, 4, 5, 11, 12]
SAFE_PROBE = "0,2,7,8"
OVERFLOW_PROBE = "0,127,357,426,456,490,799,932"
SUMMARY_CSV_FIELDS = [
    "case_id", "codimension", "max_degree", "lowbound", "planned_repetitions",
    "successful_repetitions", "bad_count", "cpp_external_wall_median_seconds",
    "m2_external_wall_median_seconds", "wall_speedup_m2_over_cpp",
    "cpp_max_rss_median_kb", "m2_max_rss_median_kb",
]


def utc_now() -> str:
    return dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def json_bytes(value: Any) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(json_bytes(value))


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def command_output(command: list[str], cwd: Path | None = None) -> str | None:
    try:
        return subprocess.check_output(
            command, cwd=cwd, text=True, stderr=subprocess.STDOUT, timeout=15
        ).strip()
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired):
        return None


def logical_path(path: Path, repo: Path, external_label: str) -> str:
    resolved = path.resolve()
    try:
        return resolved.relative_to(repo.resolve()).as_posix()
    except ValueError:
        return f"${external_label}/{resolved.name}"


def git_metadata(repo: Path) -> dict[str, Any]:
    commit = command_output(["git", "rev-parse", "HEAD"], repo) or "unknown"
    branch = command_output(["git", "rev-parse", "--abbrev-ref", "HEAD"], repo) or "unknown"
    status = command_output(["git", "status", "--porcelain=v1", "--untracked-files=all"], repo) or ""
    diff = command_output(["git", "diff", "--binary", "HEAD"], repo) or ""
    return {
        "commit": commit,
        "branch": branch,
        "dirty": bool(status),
        "working_diff_sha256": sha256_bytes(diff.encode("utf-8")) if diff else None,
    }


def memory_total_kb() -> int | None:
    try:
        for line in Path("/proc/meminfo").read_text(encoding="utf-8").splitlines():
            if line.startswith("MemTotal:"):
                return int(line.split()[1])
    except (OSError, ValueError):
        return None
    return None


def cpu_model() -> str | None:
    try:
        for line in Path("/proc/cpuinfo").read_text(encoding="utf-8").splitlines():
            if line.lower().startswith("model name"):
                return line.split(":", 1)[1].strip()
    except OSError:
        return None
    return None


def collect_environment(compiler: str, m2_command: str) -> dict[str, Any]:
    return {
        "schema_version": SCHEMA_VERSION,
        "captured_at_utc": utc_now(),
        "platform": {
            "system": platform.system(),
            "release": platform.release(),
            "version": platform.version(),
            "machine": platform.machine(),
            "python": platform.python_version(),
            "wsl_detected": (
                "microsoft" in platform.release().lower()
                or "microsoft" in platform.version().lower()
            ),
        },
        "hardware": {
            "cpu_model": cpu_model(),
            "logical_cpu_count": os.cpu_count(),
            "memory_total_kb": memory_total_kb(),
            "lscpu": command_output(["lscpu"]),
        },
        "tools": {
            "compiler_command": compiler,
            "compiler_version": command_output([compiler, "--version"]),
            "macaulay2_command": "M2",
            "macaulay2_version": command_output([m2_command, "--version"]),
            "gnu_time_version": command_output(["/usr/bin/time", "--version"]),
        },
        "environment_variables": {
            key: os.environ.get(key) for key in ("LANG", "LC_ALL", "TZ", "OMP_NUM_THREADS")
        },
        "repository_path": "$REPO_ROOT",
        "path_capture_policy": "PATH, usernames, host name, and absolute workstation paths are not recorded",
    }


def compiler_version_line(compiler: str) -> str | None:
    output = command_output([compiler, "--version"])
    return output.splitlines()[0].strip() if output else None


def inspect_driver(binary: Path, repo: Path) -> dict[str, Any]:
    completed = subprocess.run(
        [str(binary), "--build-info"], cwd=repo, text=True, capture_output=True, timeout=15
    )
    if completed.returncode != 0:
        raise ValueError(f"driver build-info failed: {completed.stderr.strip()}")
    payload = json.loads(completed.stdout)
    if payload.get("status") != "build_info":
        raise ValueError("driver did not emit build_info status")
    return payload.get("build", {})


def parse_time_file(path: Path) -> dict[str, float | int | None]:
    values: dict[str, float | int | None] = {
        "elapsed_seconds": None,
        "user_seconds": None,
        "system_seconds": None,
        "max_rss_kb": None,
    }
    if not path.is_file():
        return values
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if "=" not in line:
            continue
        key, raw = line.split("=", 1)
        if key not in values:
            continue
        try:
            values[key] = int(raw) if key == "max_rss_kb" else float(raw)
        except ValueError:
            values[key] = None
    return values


def shared_cgroup_oom_kill_count() -> int | None:
    """Return the shared cgroup-v2 OOM-kill counter when the host exposes it."""
    try:
        for line in Path("/sys/fs/cgroup/memory.events").read_text(encoding="utf-8").splitlines():
            key, raw = line.split()
            if key == "oom_kill":
                return int(raw)
    except (OSError, ValueError):
        return None
    return None


def run_measured(
    command: list[str], cwd: Path, timeout_seconds: float,
    stdout_path: Path, stderr_path: Path, time_path: Path,
) -> tuple[int | None, bool, int | None, float]:
    """Measure the complete GNU-time-wrapped child invocation.

    The returned external wall value comes from Python time.perf_counter around
    process launch through collection. GNU time coarser %e wall value and %M
    peak RSS are written to time_path.
    """
    measured = [
        "/usr/bin/time", "-f",
        "elapsed_seconds=%e\nuser_seconds=%U\nsystem_seconds=%S\nmax_rss_kb=%M\nexit_code=%x",
        "-o", str(time_path), "--", *command,
    ]
    environment = os.environ.copy()
    environment["LC_ALL"] = "C"
    oom_before = shared_cgroup_oom_kill_count()
    started = time.perf_counter()
    process = subprocess.Popen(
        measured, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        start_new_session=True, env=environment,
    )
    timed_out = False
    try:
        stdout, stderr = process.communicate(timeout=timeout_seconds)
    except subprocess.TimeoutExpired:
        timed_out = True
        os.killpg(process.pid, signal.SIGTERM)
        try:
            stdout, stderr = process.communicate(timeout=3)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            stdout, stderr = process.communicate()
    external_wall = time.perf_counter() - started
    stdout_path.write_text(stdout, encoding="utf-8")
    oom_after = shared_cgroup_oom_kill_count()
    oom_kill_delta = (
        oom_after - oom_before if oom_before is not None and oom_after is not None else None
    )
    stderr_path.write_text(stderr, encoding="utf-8")
    if not time_path.exists():
        time_path.write_text("", encoding="utf-8")
    return ((None if timed_out else process.returncode), timed_out,
            oom_kill_delta, external_wall)


def canonical_sequences(raw: Any, label: str) -> list[list[int]]:
    if not isinstance(raw, list):
        raise ValueError(f"{label} is not a list")
    sequences: list[tuple[int, ...]] = []
    for sequence in raw:
        if not isinstance(sequence, list) or not sequence or not all(isinstance(v, int) for v in sequence):
            raise ValueError(f"{label} contains a malformed sequence")
        if sequence[0] != 0 or any(left >= right for left, right in zip(sequence, sequence[1:])):
            raise ValueError(f"{label} contains a non-degree sequence")
        sequences.append(tuple(sequence))
    if len(set(sequences)) != len(sequences):
        raise ValueError(f"{label} contains duplicates")
    return [list(sequence) for sequence in sorted(sequences)]


def sequence_hash(sequences: list[list[int]]) -> str:
    payload = json.dumps(sequences, sort_keys=True, separators=(",", ":")).encode("ascii")
    return sha256_bytes(payload)


def normalize_cpp(stdout: str) -> dict[str, Any]:
    payload = json.loads(stdout)
    if payload.get("status") != "ok":
        raise ValueError(f"C++ status is {payload.get('status')}")
    bad = canonical_sequences(payload.get("bad_sequences"), "C++ bad_sequences")
    rinsed = canonical_sequences(payload.get("gcd_rinsed_sequences"), "C++ gcd_rinsed_sequences")
    counts = payload.get("counts", {})
    if counts.get("bad") != len(bad) or counts.get("gcd_rinsed") != len(rinsed):
        raise ValueError("C++ counts disagree with exact result arrays")
    return {
        "schema_version": SCHEMA_VERSION,
        "engine": "cpp",
        "build": payload.get("build", {}),
        "task": payload.get("task", {}),
        "counts": counts,
        "phase_seconds": payload.get("timing_seconds", {}),
        "bad_sequences": bad,
        "gcd_rinsed_sequences": rinsed,
        "bad_sha256": sequence_hash(bad),
        "gcd_rinsed_sha256": sequence_hash(rinsed),
    }


SEQUENCE_LINE = re.compile(r"^\{\s*([0-9]+(?:\s*,\s*[0-9]+)*)\s*\}$")


def normalize_m2(stdout: str, case: dict[str, Any]) -> dict[str, Any]:
    metadata: dict[str, str] = {}
    bad: list[list[int]] = []
    rinsed: list[list[int]] = []
    mode: str | None = None
    for line in stdout.splitlines():
        if line.startswith("BOIJ_META "):
            metadata.update(item.split("=", 1) for item in line.split()[1:])
        elif line == "BOIJ_BAD_BEGIN":
            mode = "bad"
        elif line == "BOIJ_BAD_END":
            mode = None
        elif line == "BOIJ_GCD_RINSED_BEGIN":
            mode = "rinsed"
        elif line == "BOIJ_GCD_RINSED_END":
            mode = None
        elif mode:
            match = SEQUENCE_LINE.fullmatch(line.strip())
            if not match:
                raise ValueError(f"malformed M2 sequence line: {line}")
            sequence = [int(value.strip()) for value in match.group(1).split(",")]
            (bad if mode == "bad" else rinsed).append(sequence)
    required = {"package_version", "candidate_count", "bad_count", "gcd_rinsed_count"}
    if not required.issubset(metadata):
        raise ValueError("M2 output is missing required metadata")
    bad = canonical_sequences(bad, "M2 bad sequences")
    rinsed = canonical_sequences(rinsed, "M2 gcd-rinsed sequences")
    counts = {
        "candidates": int(metadata["candidate_count"]),
        "bad": int(metadata["bad_count"]),
        "gcd_rinsed": int(metadata["gcd_rinsed_count"]),
    }
    if counts["bad"] != len(bad) or counts["gcd_rinsed"] != len(rinsed):
        raise ValueError("M2 counts disagree with exact result arrays")
    return {
        "schema_version": SCHEMA_VERSION,
        "engine": "macaulay2_builtin",
        "task": case,
        "oracle": {"package": "BoijSoederberg", "package_version": metadata["package_version"]},
        "counts": counts,
        "phase_cpu_seconds": {
            "generation": float(metadata.get("generation_cpu_seconds", "nan")),
            "testing": float(metadata.get("testing_cpu_seconds", "nan")),
            "gcd_rinse": float(metadata.get("gcd_rinse_cpu_seconds", "nan")),
        },
        "bad_sequences": bad,
        "gcd_rinsed_sequences": rinsed,
        "bad_sha256": sequence_hash(bad),
        "gcd_rinsed_sha256": sequence_hash(rinsed),
    }


def render_m2_script(template: str, case: dict[str, Any]) -> str:
    return (template
        .replace("__CODIM__", str(case["codimension"]))
        .replace("__MAXDEG__", str(case["max_degree"]))
        .replace("__LOWBOUND__", str(case["lowbound"])))


def expected_candidate_count(case: dict[str, Any]) -> int:
    c = int(case["codimension"])
    d = int(case["max_degree"])
    lowbound = max(1, int(case["lowbound"]))
    return math.comb(d, c) - (math.comb(lowbound - 1, c) if lowbound - 1 >= c else 0)


def run_engine(
    engine: str, case: dict[str, Any], stem: str, partial: Path, repo: Path,
    binary: Path, binary_logical: str, m2_binary: str, template: str,
    timeout_seconds: float, expected_build: dict[str, Any],
) -> dict[str, Any]:
    logs = partial / "logs"
    parsed_dir = partial / "parsed"
    scripts_dir = partial / "scripts"
    stdout_path = logs / f"{stem}-{engine}.stdout.txt"
    stderr_path = logs / f"{stem}-{engine}.stderr.txt"
    time_path = logs / f"{stem}-{engine}.time.txt"
    if engine == "cpp":
        command = [
            str(binary), f"--codim={case['codimension']}",
            f"--max-degree={case['max_degree']}", f"--lowbound={case['lowbound']}",
        ]
        recorded_command = [binary_logical, *command[1:]]
    else:
        script_path = scripts_dir / f"{stem}.m2"
        script_path.write_text(render_m2_script(template, case), encoding="utf-8")
        command = [m2_binary, "--script", str(script_path)]
        recorded_command = ["M2", "--script", script_path.relative_to(partial).as_posix()]
    exit_code, timed_out, oom_kill_delta, wall = run_measured(
        command, repo, timeout_seconds, stdout_path, stderr_path, time_path
    )
    time_values = parse_time_file(time_path)
    status = "timeout" if timed_out else "ok" if exit_code == 0 else "error"
    parse_error = None
    normalized = None
    if status == "ok":
        try:
            raw_stdout = stdout_path.read_text(encoding="utf-8")
            normalized = normalize_cpp(raw_stdout) if engine == "cpp" else normalize_m2(raw_stdout, case)
            if engine == "cpp" and normalized.get("build") != expected_build:
                raise ValueError("C++ per-run build provenance differs from preflight")
            parsed_path = parsed_dir / f"{stem}-{engine}.json"
            write_json(parsed_path, normalized)
        except (ValueError, json.JSONDecodeError) as error:
            status = "parse_error"
            parse_error = str(error)
            parsed_path = None
    else:
        parsed_path = None
    return {
        "status": status,
        "exit_code": exit_code,
        "timed_out": timed_out,
        "command": recorded_command,
        "measurements": {
            "external_wall_seconds": wall,
            "gnu_time_elapsed_seconds": time_values["elapsed_seconds"],
            "user_seconds": time_values["user_seconds"],
            "system_seconds": time_values["system_seconds"],
            "max_rss_kb": time_values["max_rss_kb"],
            "shared_cgroup_oom_kill_delta": oom_kill_delta,
        },
        "parse_error": parse_error,
        "shared_cgroup_oom_observation": (
            "shared cgroup oom_kill event observed; process attribution is unavailable"
            if oom_kill_delta is not None and oom_kill_delta > 0 else None
        ),
        "counts": normalized.get("counts") if normalized else None,
        "bad_sha256": normalized.get("bad_sha256") if normalized else None,
        "gcd_rinsed_sha256": normalized.get("gcd_rinsed_sha256") if normalized else None,
        "build": normalized.get("build") if engine == "cpp" and normalized else None,
        "failure_scope": "this recorded machine/run only" if status != "ok" else None,
        "program_phase_seconds": (
            normalized.get("phase_seconds") if engine == "cpp" and normalized
            else normalized.get("phase_cpu_seconds") if normalized else None
        ),
        "artifacts": {
            "stdout": stdout_path.relative_to(partial).as_posix(),
            "stderr": stderr_path.relative_to(partial).as_posix(),
            "resource_time": time_path.relative_to(partial).as_posix(),
            "parsed_exact_results": parsed_path.relative_to(partial).as_posix() if parsed_path else None,
        },
    }


def run_pair(
    case: dict[str, Any], repetition: int, order: list[str], stem: str,
    partial: Path, repo: Path, binary: Path, binary_logical: str,
    m2_binary: str, template: str, timeout_seconds: float,
    expected_build: dict[str, Any],
) -> dict[str, Any]:
    engines = {}
    for engine in order:
        engines[engine] = run_engine(
            engine, case, stem, partial, repo, binary, binary_logical,
            m2_binary, template, timeout_seconds, expected_build,
        )
    cpp = engines["cpp"]
    m2 = engines["m2"]
    expected_count = expected_candidate_count(case)
    equivalence = {
        "candidate_count_matches_contract": (
            cpp.get("counts", {}).get("candidates") == expected_count
            if cpp.get("counts") else False
        ) and (
            m2.get("counts", {}).get("candidates") == expected_count
            if m2.get("counts") else False
        ),
        "bad_set_exact_match": cpp.get("bad_sha256") == m2.get("bad_sha256") and cpp.get("bad_sha256") is not None,
        "gcd_rinsed_set_exact_match": (
            cpp.get("gcd_rinsed_sha256") == m2.get("gcd_rinsed_sha256")
            and cpp.get("gcd_rinsed_sha256") is not None
        ),
    }
    status = "ok" if (
        cpp["status"] == "ok" and m2["status"] == "ok" and all(equivalence.values())
    ) else "validation_failed"
    return {
        "schema_version": SCHEMA_VERSION,
        "case_id": case["id"],
        "case": case,
        "repetition": repetition,
        "execution_order": order,
        "timeout_seconds_per_engine": timeout_seconds,
        "status": status,
        "expected_candidate_count": expected_count,
        "equivalence": equivalence,
        "engines": engines,
    }


def run_probes(binary: Path, binary_logical: str, repo: Path, partial: Path) -> dict[str, Any]:
    results = {}
    for name, sequence in (("safe", SAFE_PROBE), ("overflow", OVERFLOW_PROBE)):
        command = [str(binary), f"--probe-sequence={sequence}"]
        completed = subprocess.run(command, cwd=repo, text=True, capture_output=True, timeout=15)
        stdout_path = partial / "preflight" / f"{name}-probe.stdout.json"
        stderr_path = partial / "preflight" / f"{name}-probe.stderr.txt"
        stdout_path.write_text(completed.stdout, encoding="utf-8")
        stderr_path.write_text(completed.stderr, encoding="utf-8")
        payload = json.loads(completed.stdout)
        passed = (
            completed.returncode == 0 and payload.get("status") == "ok"
            and payload.get("pure_betti") == [15, 28, 48, 35]
        ) if name == "safe" else (
            completed.returncode == 4 and payload.get("status") == "arithmetic_overflow"
        )
        results[name] = {
            "passed": passed,
            "exit_code": completed.returncode,
            "status": payload.get("status"),
            "command": [binary_logical, f"--probe-sequence={sequence}"],
            "stdout": stdout_path.relative_to(partial).as_posix(),
            "stderr": stderr_path.relative_to(partial).as_posix(),
        }
    return results


def median(values: list[float | int | None]) -> float | None:
    observed = [float(value) for value in values if value is not None]
    return statistics.median(observed) if observed else None


def create_summary(profile: dict[str, Any], records: list[dict[str, Any]]) -> dict[str, Any]:
    cases = []
    for case in profile["cases"]:
        case_records = [record for record in records if record["case_id"] == case["id"]]
        successful = [record for record in case_records if record["status"] == "ok"]
        cpp_wall = median([r["engines"]["cpp"]["measurements"]["external_wall_seconds"] for r in successful])
        m2_wall = median([r["engines"]["m2"]["measurements"]["external_wall_seconds"] for r in successful])
        cases.append({
            "case_id": case["id"],
            "input": case,
            "planned_repetitions": profile["repetitions"],
            "successful_repetitions": len(successful),
            "status_counts": dict(sorted(Counter(r["status"] for r in case_records).items())),
            "cpp_external_wall_median_seconds": cpp_wall,
            "m2_external_wall_median_seconds": m2_wall,
            "wall_speedup_m2_over_cpp": (m2_wall / cpp_wall) if cpp_wall and m2_wall else None,
            "cpp_max_rss_median_kb": median([r["engines"]["cpp"]["measurements"]["max_rss_kb"] for r in successful]),
            "m2_max_rss_median_kb": median([r["engines"]["m2"]["measurements"]["max_rss_kb"] for r in successful]),
            "bad_count": successful[0]["engines"]["cpp"]["counts"]["bad"] if successful else None,
        })
    return {
        "schema_version": SCHEMA_VERSION,
        "statistic_policy": (
            "median over successful paired repetitions; execution order alternates; "
            "external_wall_seconds is Python time.perf_counter around the complete "
            "GNU-time-wrapped child command; gnu_time_elapsed_seconds is the separate "
            "coarser GNU %e field; max_rss_kb is GNU %M"
        ),
        "sample_accounting": {
            "planned_pair_count": len(profile["cases"]) * int(profile["repetitions"]),
            "observed_pair_count": len(records),
            "successful_pair_count": sum(r["status"] == "ok" for r in records),
        },
        "cases": cases,
    }


def write_summary_csv(path: Path, summary: dict[str, Any]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=SUMMARY_CSV_FIELDS, lineterminator="\n")
        writer.writeheader()
        for case in summary["cases"]:
            row = {key: case.get(key) for key in SUMMARY_CSV_FIELDS}
            row.update({key: case["input"][key] for key in ("codimension", "max_degree", "lowbound")})
            writer.writerow(row)


def write_checksums(bundle: Path) -> None:
    files = sorted(path for path in bundle.rglob("*") if path.is_file() and path.name != "checksums.sha256")
    (bundle / "checksums.sha256").write_text(
        "".join(f"{sha256_file(path)}  {path.relative_to(bundle).as_posix()}\n" for path in files),
        encoding="utf-8",
    )


def finalize_staged_bundle(
    staged_bundle: Path, final: Path, quarantine: Path,
    validation_command: list[str], cwd: Path, publication_eligible: bool,
) -> tuple[Path, bool, int]:
    """Validate in staging, then atomically publish or quarantine the bundle."""
    completed = subprocess.run(validation_command, cwd=cwd)
    published = completed.returncode == 0 and publication_eligible
    destination = final if published else quarantine
    destination.parent.mkdir(parents=True, exist_ok=True)
    staged_bundle.rename(destination)
    staged_bundle.parent.rmdir()
    return destination, published, completed.returncode


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", choices=("smoke", "standard", "headline"), default="smoke")
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--compiler-flags", required=True)
    parser.add_argument("--m2", default="M2")
    parser.add_argument("--output-root", type=Path, default=Path("benchmarks/runs"))
    parser.add_argument("--run-id")
    parser.add_argument("--allow-dirty", action="store_true")
    args = parser.parse_args()

    benchmark_dir = Path(__file__).resolve().parent
    repo = benchmark_dir.parent
    profile = json.loads((benchmark_dir / "profiles.json").read_text(encoding="utf-8"))["profiles"][args.profile]
    template = (repo / "research/macaulay2/benchmark_builtin_task.m2.in").read_text(encoding="utf-8")
    binary = (args.binary if args.binary.is_absolute() else repo / args.binary).resolve()
    if not binary.is_file():
        parser.error(f"benchmark binary does not exist: {binary}")
    m2_binary = shutil.which(args.m2)
    if not m2_binary:
        parser.error("Macaulay2 executable not found")

    git = git_metadata(repo)
    if git["dirty"] and not args.allow_dirty:
        parser.error("refusing to benchmark a dirty repository; use --allow-dirty only for scratch runs")
    compiler_version = compiler_version_line(args.compiler)
    if not compiler_version:
        parser.error("compiler version probe failed")
    driver_build = inspect_driver(binary, repo)
    expected_build = {
        "commit": git["commit"], "branch": git["branch"], "dirty": git["dirty"],
        "profile": "benchmark-release", "compiler_command": args.compiler,
        "compiler_version": compiler_version, "compiler_flags": args.compiler_flags,
    }
    if driver_build != expected_build:
        parser.error("benchmark binary provenance mismatch: " + json.dumps({"expected": expected_build, "actual": driver_build}, sort_keys=True))

    binary_logical = logical_path(binary, repo, "EXTERNAL_BINARY")
    output_root = args.output_root if args.output_root.is_absolute() else repo / args.output_root
    run_id = args.run_id or f"{utc_now().replace(':', '').replace('-', '')}-{git['commit'][:8]}-{args.profile}"
    if not re.fullmatch(r"[A-Za-z0-9._-]+", run_id):
        parser.error("--run-id contains unsupported characters")
    final = output_root / run_id
    quarantine = output_root / "quarantine" / run_id
    staging_root = output_root / f".staging-{run_id}-{os.getpid()}"
    partial = staging_root / run_id
    if final.exists() or quarantine.exists() or staging_root.exists():
        parser.error("refusing to overwrite an existing bundle")
    for directory in (partial / "logs", partial / "parsed", partial / "scripts", partial / "preflight"):
        directory.mkdir(parents=True, exist_ok=True)

    started_at = utc_now()
    records: list[dict[str, Any]] = []
    try:
        write_json(partial / "environment.json", collect_environment(args.compiler, m2_binary))
        probes = run_probes(binary, binary_logical, repo, partial)
        regression_case = {"id": "preflight-c7-d12-l1", "codimension": 7, "max_degree": 12, "lowbound": 1}
        regression = run_pair(
            regression_case, 0, ["cpp", "m2"], "preflight-c7-d12-l1", partial,
            repo, binary, binary_logical, m2_binary, template, 120, expected_build,
        )
        parsed_cpp = json.loads((partial / regression["engines"]["cpp"]["artifacts"]["parsed_exact_results"]).read_text(encoding="utf-8"))
        parsed_m2 = json.loads((partial / regression["engines"]["m2"]["artifacts"]["parsed_exact_results"]).read_text(encoding="utf-8"))
        regression_passed = (
            regression["status"] == "ok"
            and KNOWN_REGRESSION in parsed_cpp["bad_sequences"]
            and regression["engines"]["cpp"]["counts"] == {"candidates": 792, "bad": 28, "gcd_rinsed": 26}
            and all(item["passed"] for item in probes.values())
        )
        if not regression_passed:
            raise RuntimeError("correctness/overflow preflight failed")

        for case in profile["cases"]:
            for repetition in range(1, int(profile["repetitions"]) + 1):
                order = ["cpp", "m2"] if repetition % 2 else ["m2", "cpp"]
                stem = f"{case['id']}-r{repetition:03d}"
                print(f"[run] {stem} order={','.join(order)}", flush=True)
                records.append(run_pair(
                    case, repetition, order, stem, partial, repo, binary, binary_logical,
                    m2_binary, template, float(profile["timeout_seconds"]), expected_build,
                ))

        with (partial / "runs.jsonl").open("w", encoding="utf-8") as handle:
            for record in records:
                handle.write(json.dumps(record, sort_keys=True, separators=(",", ":")) + "\n")
        summary = create_summary(profile, records)
        write_json(partial / "summary.json", summary)
        write_summary_csv(partial / "summary.csv", summary)
        all_pairs_ok = all(record["status"] == "ok" for record in records)
        validation = {
            "schema_version": SCHEMA_VERSION,
            "all_pairs_ok": all_pairs_ok,
            "all_exact_bad_sets_match": all(r["equivalence"]["bad_set_exact_match"] for r in records),
            "all_exact_gcd_rinsed_sets_match": all(r["equivalence"]["gcd_rinsed_set_exact_match"] for r in records),
            "correctness_preflight_passed": regression_passed,
            "known_regression_sequence": KNOWN_REGRESSION,
            "safe_and_overflow_probes": probes,
        }
        write_json(partial / "validation.json", validation)
        manifest = {
            "schema_version": SCHEMA_VERSION,
            "bundle_kind": "boij_task_matched_benchmark_bundle",
            "run_id": run_id,
            "profile": args.profile,
            "profile_definition": profile,
            "started_at_utc": started_at,
            "finished_at_utc": utc_now(),
            "repository": git,
            "binary": {"path": binary_logical, "sha256": sha256_file(binary)},
            "build_contract": {"expected": expected_build, "preflight_driver_build": driver_build, "passed": True},
            "macaulay2_oracle": {
                "command": "M2", "version": command_output([m2_binary, "--version"]),
                "package": "BoijSoederberg", "package_version": parsed_m2["oracle"]["package_version"],
                "template": "research/macaulay2/benchmark_builtin_task.m2.in",
                "template_sha256": sha256_file(repo / "research/macaulay2/benchmark_builtin_task.m2.in"),
                "per_run_command": ["M2", "--script", "scripts/<case>-r<repetition>.m2"],
                "legacy_transcription_used": False,
            },
            "task_contract": "same materialized candidate family; pure Betti; explicit BEH-or-LLBC test; gcd rinse; exact canonical bad and rinsed set equality required",
            "execution_policy": {
                "paired_repetitions": profile["repetitions"],
                "alternating_engine_order": True,
                "timeout_seconds_per_engine": profile["timeout_seconds"],
                "primary_timing": (
                    "Python time.perf_counter around the complete GNU-time-wrapped child "
                    "command, paired on one machine; GNU %e is a separate coarser wall "
                    "record and GNU %M supplies peak RSS"
                ),
                "failure_classification": "timeout and error are process outcomes; a positive shared-cgroup oom_kill delta is only an unattributed concurrent observation, never proof that this process exhausted memory",
            },
            "correctness_preflight": {
                "case": regression_case,
                "record": regression,
                "known_missed_by_legacy_transcription": KNOWN_REGRESSION,
                "passed": regression_passed,
            },
            "runner_command": [
                "python3", "benchmarks/run_benchmarks.py", f"--profile={args.profile}",
                f"--binary={binary_logical}", f"--compiler={args.compiler}",
                f"--compiler-flags={args.compiler_flags}", "--m2=M2",
                f"--output-root={logical_path(output_root, repo, 'OUTPUT_ROOT')}", f"--run-id={run_id}",
                *(["--allow-dirty"] if args.allow_dirty else []),
            ],
            "working_directory": "$REPO_ROOT",
            "run_count": len(records),
            "status_counts": dict(sorted(Counter(record["status"] for record in records).items())),
            "artifacts": {
                "environment": "environment.json", "records": "runs.jsonl",
                "summary_json": "summary.json", "summary_csv": "summary.csv",
                "validation": "validation.json", "checksums": "checksums.sha256",
            },
        }
        write_json(partial / "manifest.json", manifest)
        if sha256_file(binary) != manifest["binary"]["sha256"]:
            raise RuntimeError("benchmark binary changed during the run")
        write_checksums(partial)
        validation_command = [
            sys.executable, str(benchmark_dir / "validate_bundle.py"), str(partial)
        ]
        if args.allow_dirty:
            validation_command.append("--allow-dirty")
        destination, published, _ = finalize_staged_bundle(
            partial, final, quarantine, validation_command, repo, all_pairs_ok
        )
        print(destination)
        return 0 if published else 1
    except BaseException:
        if staging_root.exists():
            shutil.rmtree(staging_root)
        raise


if __name__ == "__main__":
    raise SystemExit(main())
