#!/usr/bin/env python3
"""Regression tests for benchmark correctness and evidence validation."""

from __future__ import annotations

import json
import shutil
import subprocess
import tempfile
import sys
import unittest
from pathlib import Path

from run_benchmarks import (
    KNOWN_REGRESSION,
    OVERFLOW_PROBE,
    SAFE_PROBE,
    create_summary,
    normalize_cpp,
    finalize_staged_bundle,
    normalize_m2,
    render_m2_script,
    sequence_hash,
    write_summary_csv,
)
from validate_bundle import (
    PRIVATE_TEXT,
    ValidationError,
    expected_candidate_count,
    validate_record,
    validate_summary_artifacts,
)


REPO = Path(__file__).resolve().parent.parent
BINARY = REPO / "build/bin/benchmark_driver"
BUILTIN_TEMPLATE = REPO / "research/macaulay2/benchmark_builtin_task.m2.in"
LEGACY_TEMPLATE = REPO / "research/macaulay2/benchmark_bad_ones.m2.in"


class EngineRegressionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if not BINARY.is_file():
            raise unittest.SkipTest("build/bin/benchmark_driver is not built")
        cls.m2 = shutil.which("M2") or shutil.which("macaulay2")

    def run_driver(self, *arguments: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [str(BINARY), *arguments], cwd=REPO, text=True,
            capture_output=True, timeout=30, check=False,
        )

    def test_safe_and_overflow_probes_are_explicit(self) -> None:
        safe = self.run_driver(f"--probe-sequence={SAFE_PROBE}")
        self.assertEqual(safe.returncode, 0, safe.stderr)
        self.assertEqual(json.loads(safe.stdout)["pure_betti"], [15, 28, 48, 35])
        overflow = self.run_driver(f"--probe-sequence={OVERFLOW_PROBE}")
        self.assertEqual(overflow.returncode, 4, overflow.stderr)
        overflow_payload = json.loads(overflow.stdout)
        self.assertEqual(overflow_payload["status"], "arithmetic_overflow")
        self.assertIn("pure_betti value", overflow_payload["error"])

    def test_scaled_degree_sequences_preserve_exact_betti(self) -> None:
        expected = [1, 7, 21, 35, 35, 21, 7, 1]
        base = self.run_driver("--probe-sequence=0,1,2,3,4,5,6,7")
        formerly_false_overflow = self.run_driver(
            "--probe-sequence=0,1000,2000,3000,4000,5000,6000,7000"
        )
        larger_scale = self.run_driver(
            "--probe-sequence=0,1000000,2000000,3000000,4000000,5000000,6000000,7000000"
        )
        for completed in (base, formerly_false_overflow, larger_scale):
            self.assertEqual(completed.returncode, 0, completed.stderr)
            payload = json.loads(completed.stdout)
            self.assertEqual(payload["status"], "ok")
            self.assertEqual(payload["pure_betti"], expected)
        self.assertEqual(
            json.loads(base.stdout)["pure_betti"],
            json.loads(formerly_false_overflow.stdout)["pure_betti"],
        )

    def test_benchmark_cli_rejects_unsupported_codimension(self) -> None:
        completed = self.run_driver(
            "--codim=21", "--max-degree=21", "--lowbound=1"
        )
        self.assertEqual(completed.returncode, 2, completed.stderr)
        payload = json.loads(completed.stdout)
        self.assertEqual(payload["status"], "error")
        self.assertIn("--codim must be in [2,20]", payload["error"])

    def test_candidate_count_overflow_is_explicit(self) -> None:
        completed = self.run_driver(
            "--codim=20", "--max-degree=1000", "--lowbound=1"
        )
        self.assertEqual(completed.returncode, 4, completed.stderr)
        payload = json.loads(completed.stdout)
        self.assertEqual(payload["status"], "arithmetic_overflow")
        self.assertIn("degree-sequence count", payload["error"])

    def test_builtin_m2_and_cpp_exact_sets_match_on_legacy_miss(self) -> None:
        if not self.m2:
            self.skipTest("Macaulay2 is not installed")
        case = {"id": "c7-d12-l1", "codimension": 7, "max_degree": 12, "lowbound": 1}
        cpp_run = self.run_driver("--codim=7", "--max-degree=12", "--lowbound=1")
        self.assertEqual(cpp_run.returncode, 0, cpp_run.stderr)
        cpp = normalize_cpp(cpp_run.stdout)
        script = render_m2_script(BUILTIN_TEMPLATE.read_text(encoding="utf-8"), case)
        with tempfile.TemporaryDirectory() as directory:
            script_path = Path(directory) / "oracle.m2"
            script_path.write_text(script, encoding="utf-8")
            m2_run = subprocess.run(
                [self.m2, "--script", str(script_path)], cwd=REPO, text=True,
                capture_output=True, timeout=30, check=False,
            )
        self.assertEqual(m2_run.returncode, 0, m2_run.stderr)
        m2 = normalize_m2(m2_run.stdout, case)
        self.assertEqual(cpp["counts"], {"candidates": 792, "bad": 28, "gcd_rinsed": 26})
        self.assertEqual(cpp["counts"], m2["counts"])
        self.assertEqual(cpp["bad_sequences"], m2["bad_sequences"])
        self.assertEqual(cpp["gcd_rinsed_sequences"], m2["gcd_rinsed_sequences"])
        self.assertIn(KNOWN_REGRESSION, cpp["bad_sequences"])
        self.assertEqual(m2["oracle"]["package"], "BoijSoederberg")
        self.assertTrue(m2["oracle"]["package_version"])

    def test_repaired_standalone_m2_path_matches_cpp_exact_sets(self) -> None:
        if not self.m2:
            self.skipTest("Macaulay2 is not installed")
        cpp_run = self.run_driver("--codim=7", "--max-degree=12", "--lowbound=1")
        cpp = normalize_cpp(cpp_run.stdout)
        script = (LEGACY_TEMPLATE.read_text(encoding="utf-8")
                  .replace("__CODIM__", "7").replace("__MAXDEG__", "12")
                  .replace("__LOWBOUND__", "1").replace("__MILESTONE_FILE__", ""))
        with tempfile.TemporaryDirectory() as directory:
            script_path = Path(directory) / "standalone.m2"
            script_path.write_text(script, encoding="utf-8")
            completed = subprocess.run(
                [self.m2, "--script", str(script_path)], cwd=REPO, text=True,
                capture_output=True, timeout=30, check=False,
            )
        self.assertEqual(completed.returncode, 0, completed.stderr)
        parsed = self.parse_legacy_lists(completed.stdout)
        self.assertEqual(cpp["bad_sequences"], parsed["bad"])
        self.assertEqual(cpp["gcd_rinsed_sequences"], parsed["rinsed"])

    @staticmethod
    def parse_legacy_lists(output: str) -> dict[str, list[list[int]]]:
        result: dict[str, list[list[int]]] = {"bad": [], "rinsed": []}
        mode: str | None = None
        for raw in output.splitlines():
            line = raw.strip()
            if line == "badOnesList:":
                mode = "bad"
            elif line == "gcdRinsedList:":
                mode = "rinsed"
            elif mode and line.startswith("{"):
                result[mode].append(json.loads(line.replace("{", "[").replace("}", "]")))
        for key in result:
            result[key] = sorted(result[key])
        return result


class PublicationStagingTests(unittest.TestCase):
    def test_only_validator_approved_eligible_bundles_are_published(self) -> None:
        cases = (
            ("approved", 0, True, True),
            ("validator-rejected", 1, True, False),
            ("failed-pair", 0, False, False),
        )
        for run_id, validator_exit, eligible, expected_published in cases:
            with self.subTest(run_id=run_id), tempfile.TemporaryDirectory() as directory:
                output_root = Path(directory)
                staged = output_root / f".staging-{run_id}" / run_id
                staged.mkdir(parents=True)
                (staged / "marker.txt").write_text("complete", encoding="utf-8")
                final = output_root / run_id
                quarantine = output_root / "quarantine" / run_id
                command = [
                    sys.executable, "-c",
                    f"import sys; sys.exit({validator_exit})",
                ]

                destination, published, observed_exit = finalize_staged_bundle(
                    staged, final, quarantine, command, REPO, eligible
                )

                expected_destination = final if expected_published else quarantine
                self.assertEqual(destination, expected_destination)
                self.assertEqual(published, expected_published)
                self.assertEqual(observed_exit, validator_exit)
                self.assertTrue((expected_destination / "marker.txt").is_file())
                self.assertEqual(final.exists(), expected_published)
                self.assertEqual(quarantine.exists(), not expected_published)
                self.assertFalse(staged.parent.exists())


class ValidatorNegativeTests(unittest.TestCase):
    CASE = {"id": "c2-d4-l1", "codimension": 2, "max_degree": 4, "lowbound": 1}
    BUILD = {
        "commit": "a" * 40, "branch": "test", "dirty": False,
        "profile": "benchmark-release", "compiler_command": "g++",
        "compiler_version": "g++ test", "compiler_flags": "-O2",
    }

    def make_engine(self, bundle: Path, engine: str, suffix: str,
                    bad: list[list[int]], rinsed: list[list[int]]) -> dict:
        counts = {
            "candidates": expected_candidate_count(self.CASE),
            "bad": len(bad), "gcd_rinsed": len(rinsed),
        }
        parsed = {
            "schema_version": 1,
            "engine": "cpp" if engine == "cpp" else "macaulay2_builtin",
            "task": self.CASE,
            "counts": counts,
            "bad_sequences": sorted(bad),
            "gcd_rinsed_sequences": sorted(rinsed),
            "bad_sha256": sequence_hash(sorted(bad)),
            "gcd_rinsed_sha256": sequence_hash(sorted(rinsed)),
        }
        if engine == "cpp":
            parsed["build"] = self.BUILD
            parsed["phase_seconds"] = {"generation": 0.1, "testing": 0.1, "gcd_rinse": 0.1, "total": 0.3}
        else:
            parsed["oracle"] = {"package": "BoijSoederberg", "package_version": "1.5"}
            parsed["phase_cpu_seconds"] = {"generation": 0.1, "testing": 0.1, "gcd_rinse": 0.1}
        if engine == "cpp":
            raw_stdout = json.dumps({
                "schema_version": 1, "status": "ok", "build": self.BUILD,
                "task": self.CASE, "counts": counts,
                "timing_seconds": parsed["phase_seconds"],
                "bad_sequences": sorted(bad),
                "gcd_rinsed_sequences": sorted(rinsed),
            }) + "\n"
        else:
            format_sequence = lambda sequence: "{" + ",".join(str(value) for value in sequence) + "}"
            raw_stdout = (
                f"BOIJ_META package_version=1.5 candidate_count={counts['candidates']} "
                f"bad_count={counts['bad']} gcd_rinsed_count={counts['gcd_rinsed']} "
                "generation_cpu_seconds=0.1 testing_cpu_seconds=0.1 gcd_rinse_cpu_seconds=0.1\n"
                "BOIJ_BAD_BEGIN\n"
                + "\n".join(format_sequence(sequence) for sequence in sorted(bad))
                + "\nBOIJ_BAD_END\nBOIJ_GCD_RINSED_BEGIN\n"
                + "\n".join(format_sequence(sequence) for sequence in sorted(rinsed))
                + "\nBOIJ_GCD_RINSED_END\n"
            )
        parsed_path = bundle / f"parsed/{suffix}-{engine}.json"
        parsed_path.parent.mkdir(parents=True, exist_ok=True)
        parsed_path.write_text(json.dumps(parsed), encoding="utf-8")
        artifacts = {}
        contents = {
            "stdout": raw_stdout,
            "stderr": "",
            "resource_time": (
                "elapsed_seconds=0.1\nuser_seconds=0.1\nsystem_seconds=0.0\n"
                "max_rss_kb=100\nexit_code=0\n"
            ),
        }
        for name in ("stdout", "stderr", "resource_time"):
            path = bundle / f"logs/{suffix}-{engine}.{name}.txt"
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(contents[name], encoding="utf-8")
            artifacts[name] = path.relative_to(bundle).as_posix()
        artifacts["parsed_exact_results"] = parsed_path.relative_to(bundle).as_posix()
        return {
            "status": "ok", "exit_code": 0, "timed_out": False,
            "command": (["build/bin/benchmark_driver"] if engine == "cpp" else ["M2", "--script", "scripts/case.m2"]),
            "measurements": {
                "external_wall_seconds": 0.1, "gnu_time_elapsed_seconds": 0.1,
                "user_seconds": 0.1, "system_seconds": 0.0,
                "max_rss_kb": 100, "shared_cgroup_oom_kill_delta": 0,
            },
            "failure_scope": None, "parse_error": None, "counts": counts,
            "bad_sha256": parsed["bad_sha256"],
            "gcd_rinsed_sha256": parsed["gcd_rinsed_sha256"],
            "build": self.BUILD if engine == "cpp" else None,
            "program_phase_seconds": parsed.get("phase_seconds", parsed.get("phase_cpu_seconds")),
            "artifacts": artifacts,
        }

    def assert_pair_rejected(self, cpp_bad: list[list[int]], m2_bad: list[list[int]],
                             cpp_rinsed: list[list[int]], m2_rinsed: list[list[int]],
                             expected_message: str) -> None:
        with tempfile.TemporaryDirectory() as directory:
            bundle = Path(directory)
            cpp = self.make_engine(bundle, "cpp", "case", cpp_bad, cpp_rinsed)
            m2 = self.make_engine(bundle, "m2", "case", m2_bad, m2_rinsed)
            record = {
                "schema_version": 1, "case_id": self.CASE["id"], "case": self.CASE,
                "repetition": 1, "execution_order": ["cpp", "m2"], "status": "ok",
                "expected_candidate_count": expected_candidate_count(self.CASE),
                "equivalence": {
                    "candidate_count_matches_contract": True,
                    "bad_set_exact_match": True,
                    "gcd_rinsed_set_exact_match": True,
                },
                "engines": {"cpp": cpp, "m2": m2},
            }
            with self.assertRaisesRegex(ValidationError, expected_message):
                validate_record(bundle, record, self.CASE, 1, self.BUILD, "negative")

    def test_equal_bad_count_with_one_different_member_is_rejected(self) -> None:
        common, cpp_only, m2_only = [0, 1, 4], [0, 1, 2], [0, 2, 3]
        self.assert_pair_rejected(
            [common, cpp_only], [common, m2_only], [common], [common], "exact bad sets differ"
        )

    def test_equal_rinsed_count_with_one_different_member_is_rejected(self) -> None:
        first, second = [0, 1, 4], [0, 1, 2]
        self.assert_pair_rejected(
            [first, second], [first, second], [first], [second], "exact rinsed sets differ"
        )

    def test_private_workstation_identifiers_are_detected(self) -> None:
        for value in ("/home/alice/project", "/mnt/c/Users/alice/project", "C:\\Users\\alice\\repo", "WIN-ABCDEF123"):
            with self.subTest(value=value):
                self.assertIsNotNone(PRIVATE_TEXT.search(value))


class SummaryValidationTests(unittest.TestCase):
    CASE = {"id": "c3-d8-l1", "codimension": 3, "max_degree": 8, "lowbound": 1}
    PROFILE = {"cases": [CASE], "repetitions": 3, "timeout_seconds": 30}

    def records(self) -> list[dict]:
        records = []
        for repetition, cpp_wall, m2_wall, cpp_rss, m2_rss in (
            (1, 1.0, 4.0, 100, 400),
            (2, 2.0, 6.0, 200, 600),
            (3, 3.0, 8.0, 300, 800),
        ):
            def engine(wall: float, rss: int) -> dict:
                return {
                    "measurements": {"external_wall_seconds": wall, "max_rss_kb": rss},
                    "counts": {"bad": 7},
                }
            records.append({
                "case_id": self.CASE["id"], "repetition": repetition, "status": "ok",
                "engines": {"cpp": engine(cpp_wall, cpp_rss), "m2": engine(m2_wall, m2_rss)},
            })
        return records

    def write_summary(self, bundle: Path, records: list[dict]) -> dict:
        summary = create_summary(self.PROFILE, records)
        (bundle / "summary.json").write_text(json.dumps(summary), encoding="utf-8")
        write_summary_csv(bundle / "summary.csv", summary)
        return summary

    def test_recomputed_summary_accepts_exact_json_and_csv(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            bundle = Path(directory)
            records = self.records()
            summary = self.write_summary(bundle, records)
            case = summary["cases"][0]
            self.assertEqual(case["successful_repetitions"], 3)
            self.assertEqual(case["bad_count"], 7)
            self.assertEqual(case["cpp_external_wall_median_seconds"], 2.0)
            self.assertEqual(case["m2_external_wall_median_seconds"], 6.0)
            self.assertEqual(case["wall_speedup_m2_over_cpp"], 3.0)
            self.assertEqual(case["cpp_max_rss_median_kb"], 200.0)
            self.assertEqual(case["m2_max_rss_median_kb"], 600.0)
            self.assertIn("time.perf_counter", summary["statistic_policy"])
            self.assertIn("GNU %e", summary["statistic_policy"])
            self.assertIn("GNU %M", summary["statistic_policy"])
            validate_summary_artifacts(bundle, self.PROFILE, records)

    def test_recomputed_summary_rejects_tampered_published_statistics(self) -> None:
        fields = (
            "successful_repetitions", "bad_count", "cpp_external_wall_median_seconds",
            "wall_speedup_m2_over_cpp", "cpp_max_rss_median_kb",
        )
        for field in fields:
            with self.subTest(field=field), tempfile.TemporaryDirectory() as directory:
                bundle = Path(directory)
                records = self.records()
                summary = self.write_summary(bundle, records)
                summary["cases"][0][field] = 999
                (bundle / "summary.json").write_text(json.dumps(summary), encoding="utf-8")
                with self.assertRaisesRegex(ValidationError, "summary case statistics differ"):
                    validate_summary_artifacts(bundle, self.PROFILE, records)

    def test_recomputed_summary_rejects_tampered_csv(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            bundle = Path(directory)
            records = self.records()
            self.write_summary(bundle, records)
            lines = (bundle / "summary.csv").read_text(encoding="utf-8").splitlines()
            header, row = lines[0].split(","), lines[1].split(",")
            row[header.index("wall_speedup_m2_over_cpp")] = "999"
            (bundle / "summary.csv").write_text(
                ",".join(header) + "\n" + ",".join(row) + "\n", encoding="utf-8")
            with self.assertRaisesRegex(ValidationError, "summary CSV statistics differ"):
                validate_summary_artifacts(bundle, self.PROFILE, records)


if __name__ == "__main__":
    unittest.main()
