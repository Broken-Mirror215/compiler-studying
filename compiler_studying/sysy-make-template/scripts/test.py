#!/usr/bin/env python3
"""Run course tests and verify their summary as well as their exit status."""

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("both", "koopa", "riscv"), default="both")
    parser.add_argument("--suite", choices=("all", "custom", "lv8", "lv9"), default="all")
    parser.add_argument("--log-dir", type=Path, help="Parent directory for run logs")
    args = parser.parse_args()

    project = Path(__file__).resolve().parents[1]
    autotest = shutil.which("autotest")
    if autotest is None:
        parser.exit(2, "找不到 autotest，请在课程开发环境中运行。\n")

    modes = ("koopa", "riscv") if args.mode == "both" else (args.mode,)
    suites = ("custom", "lv8", "lv9") if args.suite == "all" else (args.suite,)
    run_id = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    log_dir = (args.log_dir or project / "build/test-logs") / run_id
    log_dir.mkdir(parents=True)
    print(f"日志目录：{log_dir}", flush=True)
    results = []

    try:
        for mode in modes:
            for suite in suites:
                selection = ["-t", str(project / "tests")] if suite == "custom" else ["-s", suite]
                command = [autotest, "-" + mode, *selection, str(project)]
                log_path = log_dir / f"{mode}-{suite}.log"
                print(f"运行 {mode} / {suite} ...", flush=True)
                with log_path.open("w", encoding="utf-8") as output:
                    completed = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT)

                clean = re.sub(r"\x1b\[[0-9;]*m", "", log_path.read_text(encoding="utf-8", errors="replace"))
                summaries = re.findall(r"^PASSED \((\d+)/(\d+)\)\s*$", clean, re.MULTILINE)
                passed, total = map(int, summaries[-1]) if summaries else (0, 0)
                success = completed.returncode == 0 and total > 0 and passed == total
                results.append({
                    "mode": mode, "suite": suite,
                    "exit_code": completed.returncode,
                    "success": success, "passed": passed, "total": total,
                    "log": log_path.name,
                })
                if success:
                    print(f"通过：{mode} / {suite} {passed}/{total}", flush=True)
                else:
                    print(f"失败：{mode} / {suite}，详见 {log_path}", file=sys.stderr)
                    print("\n".join(clean.splitlines()[-30:]), file=sys.stderr)
    except KeyboardInterrupt:
        print("测试已中断。", file=sys.stderr)
        return 130
    finally:
        (log_dir / "results.json").write_text(
            json.dumps(results, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
        )

    return 0 if results and all(item["success"] for item in results) else 1


if __name__ == "__main__":
    sys.exit(main())
