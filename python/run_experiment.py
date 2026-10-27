import argparse
import csv
import json
import time
from datetime import datetime
from pathlib import Path

from model_interface import generate_answer
from validator import validate

ROOT = Path(__file__).resolve().parent.parent
TASKS_PATH = ROOT / "tasks" / "tasks.json"
ANSWERS_DIR = ROOT / "answers"
LOGS_DIR = ROOT / "logs"

ANSWERS_DIR.mkdir(exist_ok=True)
LOGS_DIR.mkdir(exist_ok=True)


def load_tasks():
    with open(TASKS_PATH, encoding="utf-8") as f:
        return json.load(f)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--group", choices=["A", "B", "all"], default="A")
    parser.add_argument("--task", default=None)
    args = parser.parse_args()

    tasks = load_tasks()
    if args.task:
        tasks = [t for t in tasks if t["id"] == args.task]
        if not tasks:
            print(f"task {args.task} not found")
            return

    groups = ["A", "B"] if args.group == "all" else [args.group]

    rows = []
    for group in groups:
        for task in tasks:
            t0 = time.perf_counter()
            answer = generate_answer(task, group)
            latency_ms = int((time.perf_counter() - t0) * 1000)

            answer_path = ANSWERS_DIR / f"answer_{group}_{task['id']}.json"
            answer_path.write_text(
                json.dumps(answer, ensure_ascii=False, indent=2),
                encoding="utf-8",
            )

            ok, reason = validate(task, answer)
            print(f"[{group}] {task['id']}: {'PASS' if ok else 'FAIL'} - {reason}")

            rows.append({
                "timestamp": datetime.now().isoformat(timespec="seconds"),
                "task_id": task["id"],
                "group": group,
                "model": "mock",
                "correct": ok,
                "reason": reason,
                "latency_ms": latency_ms,
                "tokens": 0,
            })

    csv_path = LOGS_DIR / "results.csv"
    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=[
            "timestamp", "task_id", "group", "model",
            "correct", "reason", "latency_ms", "tokens",
        ])
        writer.writeheader()
        writer.writerows(rows)

    print(f"\nwrote {len(rows)} rows to {csv_path}")


if __name__ == "__main__":
    main()