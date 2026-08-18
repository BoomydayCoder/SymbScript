#!/usr/bin/env python3

import argparse
import subprocess
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


@dataclass(frozen=True)
class Case:
    name: str
    program: str
    input_data: str = ""


CASES = [
    Case("fibonacci", "examples/fibonacci.txt", "20\n"),
    Case("eratosthenes", "examples/eratosthenes.txt", "100\n"),
    Case(
        "dijkstra",
        "examples/dijkstra.txt",
        "5\n6\n0\n1\n4\n0\n2\n1\n2\n1\n2\n1\n3\n1\n2\n3\n5\n3\n4\n3\n",
    ),
    Case("bubblesort", "examples/sorting/bubblesort.txt"),
    Case("heapsort", "examples/sorting/heapsort.txt"),
    Case("mergesort", "examples/sorting/mergesort.txt"),
]


def run(binary: Path, case: Case) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(binary), str(ROOT / case.program)],
        input=case.input_data,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    args = parser.parse_args()

    failed = False
    for case in CASES:
        reference = run(args.reference.resolve(), case)
        candidate = run(args.candidate.resolve(), case)
        matches = (
            reference.returncode == candidate.returncode
            and reference.stdout == candidate.stdout
            and reference.stderr == candidate.stderr
        )
        print(f"{'PASS' if matches else 'FAIL'} {case.name}")
        if not matches:
            failed = True
            print(f"  reference return={reference.returncode}, stderr={reference.stderr!r}")
            print(f"  candidate return={candidate.returncode}, stderr={candidate.stderr!r}")

    if failed:
        raise SystemExit(1)


if __name__ == "__main__":
    main()

