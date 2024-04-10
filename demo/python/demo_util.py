import json
import os
import random
import time
from typing import Generator, Optional, Sequence


def load_demo_sentences() -> Sequence[str]:
    data_file_path = os.path.join(os.path.dirname(__file__), "../../resources/demo/demo_data.json")
    with open(data_file_path, encoding="utf8") as data_file:
        demo_data = json.loads(data_file.read())
    return demo_data["demo_sentences"]


def get_text_generator(token_delay: float = 0.1, text_index: Optional[int] = None) -> Generator[str, None, None]:
    sentences = load_demo_sentences()
    text_index = text_index if text_index is not None else random.randint(0, len(sentences) - 1)

    text = ""
    for char in sentences[text_index]:
        if char == " " or char in {".", ",", "!", "?"}:
            token = f"{text}{char}"
            time.sleep(token_delay)
            yield token
            text = ""
        else:
            text += char
    if text != "":
        yield text


__all__ = [
    "get_text_generator",
]
