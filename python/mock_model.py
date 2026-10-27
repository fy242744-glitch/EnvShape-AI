def generate_answer(task, group="A"):
    exp = task.get("expected", {})
    tid = task["id"]

    if "slope" in exp:
        if group == "A" and tid == "func_004":
            return {"slope": -3, "intercept": -2}
        return {"slope": exp["slope"], "intercept": exp["intercept"]}

    if "intersection" in exp:
        if group == "B" and tid == "func_005":
            return {"intersection": [3, 4]}
        return {"intersection": list(exp["intersection"])}

    if "quadrants" in exp:
        if group == "A" and tid == "func_012":
            return {"quadrants": [1, 2, 4]}
        return {"quadrants": list(exp["quadrants"])}

    if "slope_sign" in exp:
        return {"slope_sign": exp["slope_sign"]}

    if "relation" in exp:
        if group == "B" and tid == "func_017":
            return {"relation": "intersect"}
        return {"relation": exp["relation"]}

    return {}