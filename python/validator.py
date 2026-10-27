import json
import sys


def _to_float(x):
    try:
        return float(x)
    except (TypeError, ValueError):
        return None


def validate(task, answer):
    if not isinstance(answer, dict):
        return False, "answer is not a dict"

    exp = task.get("expected", {})

    if "slope" in exp:
        s = _to_float(answer.get("slope"))
        b = _to_float(answer.get("intercept"))
        if s is None or b is None:
            return False, "missing slope or intercept"
        if abs(s - float(exp["slope"])) > 1e-6:
            return False, f"slope mismatch: got {s}, expected {exp['slope']}"
        if abs(b - float(exp["intercept"])) > 1e-6:
            return False, f"intercept mismatch: got {b}, expected {exp['intercept']}"
        return True, "slope/intercept ok"

    if "intersection" in exp:
        got = answer.get("intersection")
        if not isinstance(got, (list, tuple)) or len(got) != 2:
            return False, "missing or malformed intersection"
        x = _to_float(got[0])
        y = _to_float(got[1])
        if x is None or y is None:
            return False, "intersection not numeric"
        ex = exp["intersection"]
        if abs(x - float(ex[0])) > 1e-6 or abs(y - float(ex[1])) > 1e-6:
            return False, f"intersection mismatch: got {got}, expected {ex}"
        return True, "intersection ok"

    if "quadrants" in exp:
        got = answer.get("quadrants")
        if not isinstance(got, (list, tuple)):
            return False, "missing quadrants"
        got_norm = sorted(int(q) for q in got if _to_float(q) is not None)
        exp_norm = sorted(int(q) for q in exp["quadrants"])
        if got_norm != exp_norm:
            return False, f"quadrants mismatch: got {got_norm}, expected {exp_norm}"
        return True, "quadrants ok"

    if "slope_sign" in exp:
        got = answer.get("slope_sign")
        if got != exp["slope_sign"]:
            return False, f"slope_sign mismatch: got {got}, expected {exp['slope_sign']}"
        return True, "slope_sign ok"

    if "relation" in exp:
        got = answer.get("relation")
        if got != exp["relation"]:
            return False, f"relation mismatch: got {got}, expected {exp['relation']}"
        return True, "relation ok"

    return False, "unknown expected type"


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("usage: python validator.py tasks.json task_id answer.json")
        sys.exit(2)

    tasks_path, task_id, answer_path = sys.argv[1], sys.argv[2], sys.argv[3]
    with open(tasks_path, encoding="utf-8") as f:
        tasks = json.load(f)
    task = next((t for t in tasks if t["id"] == task_id), None)
    if task is None:
        print(json.dumps({"correct": False, "reason": f"task {task_id} not found"}, ensure_ascii=False))
        sys.exit(1)

    with open(answer_path, encoding="utf-8") as f:
        answer = json.load(f)
    ok, reason = validate(task, answer)
    print(json.dumps({"correct": ok, "reason": reason}, ensure_ascii=False))