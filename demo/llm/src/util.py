import time
from dataclasses import dataclass
from datetime import datetime
from queue import Empty, Queue
from threading import Thread
from typing import *

from rich.console import Group
from rich.live import Live
from rich.padding import Padding
from rich.panel import Panel
from rich.text import Text


@dataclass
class Colors:
    GREEN = "\033[92m"
    DARK_GREEN = "\u001B[32m"
    GREY = "\033[90m"
    RESET = "\033[0m"


@dataclass
class Timer:
    time_llm_request: float = -1.0
    time_first_llm_token: float = -1.0
    time_last_llm_token: float = -1.0
    time_first_synthesis_request: float = -1.0
    time_first_audio: float = -1.0
    initial_audio_delay: float = 0.0

    is_first_audio: bool = True
    _is_first_synthesis_request: bool = True

    _num_tokens: int = 0

    @staticmethod
    def _get_time() -> float:
        return time.time()

    def log_time_llm_request(self) -> None:
        self.time_llm_request = self._get_time()

    def log_time_first_llm_token(self) -> None:
        self.time_first_llm_token = self._get_time()

    def log_time_last_llm_token(self) -> None:
        self.time_last_llm_token = self._get_time()

    def maybe_log_time_first_synthesis_request(self) -> None:
        if self._is_first_synthesis_request:
            self.time_first_synthesis_request = self._get_time()
            self._is_first_synthesis_request = False

    def maybe_log_time_first_audio(self) -> None:
        if self.is_first_audio:
            self.time_first_audio = self._get_time()
            self.is_first_audio = False

    def increment_num_tokens(self) -> None:
        self._num_tokens += 1
        if self._num_tokens == 1:
            self.log_time_first_llm_token()

    @property
    def is_first_token(self) -> bool:
        return self._num_tokens == 0

    def set_initial_audio_delay(self, delay: float) -> None:
        self.initial_audio_delay = delay

    def reset(self) -> None:
        self.time_llm_request = -1.0
        self.time_first_llm_token = -1.0
        self.time_last_llm_token = -1.0
        self.time_first_synthesis_request = -1.0
        self.time_first_audio = -1.0
        self.initial_audio_delay = 0.0

        self._is_first_synthesis_request = True
        self.is_first_audio = True

        self._num_tokens = 0

    @staticmethod
    def _to_ms(t: float) -> float:
        return round(t * 1_000, -1)

    def get_time_to_first_audio(self) -> float:
        return self._to_ms(self.time_first_audio - self.time_first_llm_token)

    def pretty_print_diffs(self) -> None:
        print(Colors.DARK_GREEN)

        print(f"Delay for first text token: {self._to_ms(self.time_first_llm_token - self.time_llm_request):.0f}ms")
        print(
            f"Time to generate text: {self._to_ms(self.time_last_llm_token - self.time_first_llm_token):.0f}ms ",
            end="")
        print(f"(tokens / second = ~{self._num_tokens / (self.time_last_llm_token - self.time_first_llm_token):.0f})")
        print(
            f"Time to first audio after first token: "
            f"{Colors.GREEN}{self.get_time_to_first_audio():.0f}ms{Colors.DARK_GREEN}",
            end="")
        if self.initial_audio_delay > 0.1:
            print(
                f" (added delay of `{self._to_ms(self.initial_audio_delay):.0f}ms` "
                "to ensure continuous audio)")

        else:
            print()
        print(Colors.RESET)

    def debug_print(self) -> None:
        def to_hms(t: float) -> str:
            date_object = datetime.fromtimestamp(t)
            return date_object.strftime("%H:%M:%S.%f")[:-3]

        print(f"time LLM request {to_hms(self.time_llm_request)}")
        print(f"time first LLM token {to_hms(self.time_first_llm_token)}")
        print(f"time last LLM token {to_hms(self.time_last_llm_token)}")
        print(f"time first synthesis request {to_hms(self.time_first_synthesis_request)}")
        print(f"time first audio {to_hms(self.time_first_audio)}")


class ProgressPrinter:
    PROGRESS_BAR_PRINT_INTERVAL_SECONDS = 0.02
    PROGRESS_BAR_SYMBOL = "#"

    @dataclass
    class TimerEvent:
        num_milliseconds: Optional[float] = None
        start: bool = False

    @dataclass
    class MessageEvent:
        token: str = ""
        stop: bool = False

    def __init__(
            self,
            llm_response_init_message: str = "",
            timer_init_message: str = "",
            progress_bar_print_interval_seconds: float = PROGRESS_BAR_PRINT_INTERVAL_SECONDS,
            progress_bar_symbol: str = PROGRESS_BAR_SYMBOL,
    ) -> None:
        self._progress_bar_print_interval_seconds = progress_bar_print_interval_seconds
        self._progress_bar_symbol = progress_bar_symbol

        self._thread_dashboard: Optional[Thread] = None

        self._llm_response_init_message = llm_response_init_message
        self._timer_init_message = timer_init_message
        self._text_message = None
        self._text_timer = None
        self._queue_message = None
        self._queue_timer = None

        self._reset()

    def _reset(self) -> None:
        self._queue_message: Queue[ProgressPrinter.MessageEvent] = Queue()
        self._queue_timer: Queue[ProgressPrinter.TimerEvent] = Queue()
        self._text_message = Text(text=self._llm_response_init_message, no_wrap=False, overflow="ellipsis")
        self._text_timer = Text(text=self._timer_init_message)

    def _worker_message(self) -> None:
        while True:
            message_event = self._queue_message.get()
            if message_event.stop:
                break

            self._text_message.append(message_event.token)

    def _worker_timer(self) -> None:
        active = False
        while True:
            try:
                timer_event = self._queue_timer.get_nowait()
                if timer_event.start:
                    active = True
                elif timer_event.num_milliseconds is not None:
                    self._text_timer.append(f" {round(timer_event.num_milliseconds, -1):.0f}ms")
                    self._text_timer.stylize("bold")
                    active = False
                    break
            except Empty:
                pass
            finally:
                if active:
                    self._text_timer.append(self._progress_bar_symbol)
                    time.sleep(self._progress_bar_print_interval_seconds)

    def _worker(self, title: str) -> None:
        print()
        pad_text = Text("\n")
        rows = Group(
            Panel(
                Group(
                    title,
                    Padding(self._text_timer, pad=(1, 0, 0, 1)),
                    Padding(self._text_message, pad=(1, 0, 1, 1)),
                ),
            ),
            pad_text)

        thread_message = Thread(target=self._worker_message)
        thread_timer = Thread(target=self._worker_timer)

        with Live(rows, refresh_per_second=1 / self._progress_bar_print_interval_seconds):
            thread_message.start()
            thread_timer.start()

            thread_message.join()
            thread_timer.join()
            pad_text.remove_suffix("\n")

    def start(self, title: str) -> None:
        self._reset()
        self._thread_dashboard = Thread(target=self._worker, args=(title,))
        self._thread_dashboard.start()

    def update_llm_response(self, message: str) -> None:
        if self._thread_dashboard is None:
            raise ValueError("Dashboard must be started first")

        self._queue_message.put_nowait(self.MessageEvent(message))

    def update_timer(self, start: Optional[bool] = None, num_milliseconds: Optional[float] = None) -> None:
        if self._thread_dashboard is None:
            raise ValueError("Dashboard must be started first")

        if start is not None:
            self._queue_timer.put_nowait(self.TimerEvent(start=start))
        elif num_milliseconds is not None:
            self._queue_timer.put_nowait(self.TimerEvent(num_milliseconds=num_milliseconds))
        else:
            raise ValueError("Either start or num_milliseconds must be provided")

    def stop(self) -> None:
        self._queue_message.put_nowait(self.MessageEvent(stop=True))
        self._thread_dashboard.join()
        self._thread_dashboard = None


__all__ = [
    "Colors",
    "ProgressPrinter",
    "Timer",
]
