#!/usr/bin/env python3

import argparse
import subprocess
from pathlib import Path


def run(*args: str) -> str:
    result = subprocess.run(
        args,
        cwd=PROJECT_ROOT,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    return result.stdout + result.stderr


parser = argparse.ArgumentParser(
    description="Create a review dump for the current ProjW milestone."
)
parser.add_argument(
    "milestone",
    help="Milestone name, e.g. 5b, 5c, 6a",
)
args = parser.parse_args()

PROJECT_ROOT = Path(__file__).resolve().parent.parent

milestone = args.milestone.lower()
output_path = PROJECT_ROOT / "Docs" / f"Changelog{milestone}.txt"

sections: list[str] = []

# ---------------------------------------------------------------------
# Git status
# ---------------------------------------------------------------------

sections.append("===== GIT STATUS =====")
sections.append(run("git", "status").rstrip())


# ---------------------------------------------------------------------
# Tracked changes
# ---------------------------------------------------------------------

sections.append("\n===== TRACKED DIFF =====")
sections.append(run("git", "diff").rstrip())


# ---------------------------------------------------------------------
# Untracked source/document/config files
# ---------------------------------------------------------------------

sections.append("\n===== UNTRACKED SOURCE/DOC FILES =====")

untracked = run(
    "git",
    "ls-files",
    "--others",
    "--exclude-standard",
).splitlines()

for relative_name in untracked:
    relative_name = relative_name.strip()

    if not relative_name:
        continue

    path = Path(relative_name)

    if not path.parts or path.parts[0] not in {"Source", "Docs", "Config"}:
        continue

    full_path = PROJECT_ROOT / path

    sections.append(f"\n===== FILE: {relative_name} =====")

    try:
        sections.append(
            full_path.read_text(
                encoding="utf-8",
                errors="replace",
            ).rstrip()
        )
    except OSError as exc:
        sections.append(f"[ERROR READING FILE: {exc}]")


output_path.write_text(
    "\n".join(sections) + "\n",
    encoding="utf-8",
)

print(f"Review written to: {output_path}")
