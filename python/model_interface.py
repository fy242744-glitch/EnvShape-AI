from mock_model import generate_answer as _mock_generate


def generate_answer(task, group="A"):
    """
    统一回答生成接口。
    sk-OyWCoub8ocrQrrH3253LUB88qdf1NLSHKauU2fkH5FRCNGQge2Z2znB1kryrPJug
    未来接 API 时，只改这个函数内部，调用方不动。
    """
    return _mock_generate(task, group)