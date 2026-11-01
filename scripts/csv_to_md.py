"""Render results/test_results.csv as a Markdown table (docs/TEST_RESULTS.md).

Usage (from the project folder):  python scripts/csv_to_md.py
Re-run after run_tests.bat so the document always reflects the latest actual results.
"""
import csv
import datetime
import os
import sys

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
src = os.path.join(root, "results", "test_results.csv")
dst = os.path.join(root, "docs", "TEST_RESULTS.md")

if not os.path.exists(src):
    sys.exit("results/test_results.csv not found - run run_tests.bat first")

with open(src, newline="", encoding="utf-8") as f:
    rows = list(csv.DictReader(f))


def cell(text):
    return text.replace("|", "\\|").replace("\n", " ")


passed = sum(r["status"] == "PASS" for r in rows)
stamp = datetime.datetime.fromtimestamp(os.path.getmtime(src)).strftime("%Y-%m-%d %H:%M")
lines = [
    "# Test results",
    "",
    f"Generated from `results/test_results.csv` (written {stamp}). "
    f"**{passed}/{len(rows)} tests passed.** The Actual column is copied from the "
    "test program's output. Any test that was not executed is marked \"Not run\".",
    "",
    "| ID | Category | Scenario | Input / setup | Expected | Actual | Status |",
    "|---|---|---|---|---|---|---|",
]
for r in rows:
    lines.append("| " + " | ".join(cell(r[k]) for k in
                 ["test_id", "category", "scenario", "setup", "expected", "actual", "status"]) + " |")

os.makedirs(os.path.dirname(dst), exist_ok=True)
with open(dst, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")
print(f"wrote {dst} ({passed}/{len(rows)} passed)")
