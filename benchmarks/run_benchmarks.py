#!/usr/bin/env python3

import argparse
import re
import statistics
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PROGRAMS = {
    "bubblesort": ("examples/sorting/bubblesort.txt", ">> bubblesort(lr);", "bubblesort(lr);"),
    "heapsort": ("examples/sorting/heapsort.txt", ">> heapsort(lr);", "heapsort(lr);"),
    "mergesort": ("examples/sorting/mergesort.txt", ">> sort(lr);", "sort(lr);"),
}
TIME_PATTERN = re.compile(r"Execution time: (\d+) microseconds")


def benchmark(binary: Path, runs: int) -> None:
    print("| Algorithm | Median (ms) | Min (ms) | Runs |")
    print("|---|---:|---:|---:|")

    with tempfile.TemporaryDirectory(prefix="symbscript-bench-") as tmp:
        tmpdir = Path(tmp)
        for name, (relative_path, output_statement, replacement) in PROGRAMS.items():
            source = (ROOT / relative_path).read_text()
            if source.count(output_statement) != 1:
                raise RuntimeError(f"Expected one final output statement in {relative_path}")

            benchmark_file = tmpdir / f"{name}.txt"
            benchmark_file.write_text(source.replace(output_statement, replacement))
            timings = []

            for _ in range(runs):
                result = subprocess.run(
                    [str(binary), "-t", str(benchmark_file)],
                    text=True,
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.PIPE,
                    check=True,
                )
                match = TIME_PATTERN.search(result.stderr)
                if not match:
                    raise RuntimeError(f"No timing produced for {name}: {result.stderr}")
                timings.append(int(match.group(1)) / 1000.0)

            print(
                f"| {name} | {statistics.median(timings):.3f} | "
                f"{min(timings):.3f} | {runs} |"
            )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--runs", type=int, default=3)
    args = parser.parse_args()

    binary = args.binary.resolve()
    if args.runs < 1:
        parser.error("--runs must be positive")
    benchmark(binary, args.runs)


if __name__ == "__main__":
    main()

