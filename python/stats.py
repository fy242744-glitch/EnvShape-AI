import csv
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CSV_PATH = ROOT / "logs" / "results.csv"
TASKS_PATH = ROOT / "tasks" / "tasks.json"


def load_task_meta():
    with open(TASKS_PATH, encoding="utf-8") as f:
        tasks = json.load(f)
    return {t["id"]: {"type": t["type"], "variant": t["variant"]} for t in tasks}


def pct(correct, total):
    if total == 0:
        return "N/A"
    return f"{correct}/{total} = {correct / total * 100:.1f}%"


def main():
    if not CSV_PATH.exists():
        print(f"missing {CSV_PATH}, run run_experiment.py first")
        return

    meta = load_task_meta()
    with open(CSV_PATH, encoding="utf-8") as f:
        rows = list(csv.DictReader(f))

    total = len(rows)
    total_correct = sum(1 for r in rows if r["correct"] == "True")

    by_group = defaultdict(lambda: [0, 0])
    by_type = defaultdict(lambda: [0, 0])
    by_variant = defaultdict(lambda: [0, 0])

    for r in rows:
        ok = r["correct"] == "True"
        group = r["group"]
        task_id = r["task_id"]
        m = meta.get(task_id, {"type": "unknown", "variant": "unknown"})

        by_group[group][0] += int(ok)
        by_group[group][1] += 1

        by_type[m["type"]][0] += int(ok)
        by_type[m["type"]][1] += 1

        by_variant[m["variant"]][0] += int(ok)
        by_variant[m["variant"]][1] += 1

    print(f"总正确率: {pct(total_correct, total)}\n")

    print("按组:")
    for k, (c, n) in sorted(by_group.items()):
        print(f"  {k}: {pct(c, n)}")

    print("\n按题型:")
    for k, (c, n) in sorted(by_type.items()):
        print(f"  {k}: {pct(c, n)}")

    print("\n按变体:")
    for k, (c, n) in sorted(by_variant.items()):
        print(f"  {k}: {pct(c, n)}")


if __name__ == "__main__":
    main()