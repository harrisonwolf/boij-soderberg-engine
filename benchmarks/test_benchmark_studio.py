#!/usr/bin/env python3
"""Regression tests for benchmark correctness and evidence validation."""

from __future__ import annotations

import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from run_benchmarks import (
    KNOWN_REGRESSION,
    OVERFLOW_PROBE,
    SAFE_PROBE,
    normalize_cpp,
    normalize_m2,
    render_m2_script,
    sequence_hash,
)
from validate_bundle import PRIVATE_TEXT, ValidationError, expected_candidate_count, validate_record


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
        self.assertEqual(json.loads(overflow.stdout)["status"], "arithmetic_overflow")

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
                "max_rss_kb": 100, "cgroup_oom_kill_delta": 0,
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


if __name__ == "__main__":
    unittest.main()
