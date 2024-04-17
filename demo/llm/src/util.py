import time
from dataclasses import dataclass
from datetime import datetime
from queue import Empty, Queue
from threading import Thread
from typing import Optional

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
    BLUE = "\033[94m"
    RESET = "\033[0m"
    ORANGE = "\033[93m"
    RED = "\033[91m"


@dataclass
class Timer:
    time_llm_request: float = -1.0
    time_first_llm_token: float = -1.0
    time_last_llm_token: float = -1.0
    time_first_synthesis_request: float = -1.0
    time_first_audio: float = -1.0
    initial_audio_delay: float = 0.0

    before_first_audio: bool = True
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
        if self.before_first_audio:
            self.time_first_audio = self._get_time()
            self.before_first_audio = False

    def increment_num_tokens(self) -> None:
        self._num_tokens += 1

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
        self.before_first_audio = True

        self._num_tokens = 0

    @staticmethod
    def _to_ms(t: float) -> float:
        return round(t * 1_000, -1)

    def get_time_to_first_audio(self) -> float:
        return self._to_ms(self.time_first_audio - self.time_first_llm_token)

    def get_time_to_first_token(self) -> float:
        return self._to_ms(self.time_first_llm_token - self.time_llm_request)

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


def clean_console_line() -> None:
    print("\033[K", end="\r", flush=True)


class ListeningAnimation(Thread):
    def __init__(self, message: str, sleep_time_sec=0.2):
        self._message = message
        self._sleep_time_sec = sleep_time_sec
        self._frames = [
            " .  ",
            " .. ",
            " ...",
            "  ..",
            "   .",
            "    "
        ]
        self._done = False
        super().__init__()

    def run(self):
        self._done = False
        while not self._done:
            for frame in self._frames:
                if self._done:
                    break
                print(f"{self._message}{frame}", end="\r", flush=True)
                time.sleep(self._sleep_time_sec)

    def change_message(self, message: str):
        self._message = message

    def stop(self, message: str):
        clean_console_line()
        print(message, flush=True)
        self._done = True


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
            show_llm_response: bool = False,
            show_live_progress_bar: bool = False,
            timer_tts_init_message: str = "",
            timer_llm_init_message: str = "",
            progress_bar_print_interval_seconds: float = PROGRESS_BAR_PRINT_INTERVAL_SECONDS,
            progress_bar_symbol: str = PROGRESS_BAR_SYMBOL,
    ) -> None:
        self._progress_bar_print_interval_seconds = progress_bar_print_interval_seconds
        self._progress_bar_symbol = progress_bar_symbol

        self._show_live_progress_bar = show_live_progress_bar

        self._thread_dashboard: Optional[Thread] = None

        self._llm_response_init_message = llm_response_init_message
        self._show_llm_response = show_llm_response
        self._timer_tts_init_message = timer_tts_init_message
        self._timer_llm_init_message = timer_llm_init_message
        self._text_message = None
        self._text_timer_tts = None
        self._text_timer_llm = None
        self._queue_message = None
        self._queue_timer_tts = None
        self._queue_timer_llm = None

        self._reset()

    def _reset(self) -> None:
        self._queue_message: Queue[ProgressPrinter.MessageEvent] = Queue()
        self._queue_timer_tts: Queue[ProgressPrinter.TimerEvent] = Queue()
        self._queue_timer_llm: Queue[ProgressPrinter.TimerEvent] = Queue()
        self._text_message = Text.from_ansi(text=self._llm_response_init_message, no_wrap=False, overflow="ellipsis")
        self._text_timer_tts = Text.from_ansi(text=self._timer_tts_init_message)
        self._text_timer_llm = Text.from_ansi(text=self._timer_llm_init_message)

    def _worker_message(self) -> None:
        while True:
            message_event = self._queue_message.get()
            if message_event.stop:
                break

            self._text_message.append(message_event.token)

    def _worker_timer_tts(self) -> None:
        active = False
        progress_bar_text = ""
        while True:
            try:
                timer_event = self._queue_timer_tts.get_nowait()
                if timer_event.start:
                    active = True
                elif timer_event.num_milliseconds is not None:
                    proc_time_text = f" {round(timer_event.num_milliseconds, -1):.0f}ms"
                    if self._show_live_progress_bar:
                        self._text_timer_tts.append(proc_time_text)
                    else:
                        progress_bar_text += proc_time_text
                        self._text_timer_tts.append(progress_bar_text)
                    self._text_timer_tts.stylize("bold")
                    active = False
                    break
            except Empty:
                pass
            finally:
                if active:
                    progress_bar_text += self._progress_bar_symbol
                    if self._show_live_progress_bar:
                        self._text_timer_tts.append(self._progress_bar_symbol)
                    time.sleep(self._progress_bar_print_interval_seconds)

    def _worker_timer_llm(self) -> None:
        active = False
        progress_bar_text = ""
        while True:
            try:
                timer_event = self._queue_timer_llm.get_nowait()
                if timer_event.start:
                    active = True
                elif timer_event.num_milliseconds is not None:
                    proc_time_text = f" {round(timer_event.num_milliseconds, -1):.0f}ms"
                    if self._show_live_progress_bar:
                        self._text_timer_llm.append(proc_time_text)
                    else:
                        progress_bar_text += proc_time_text
                        self._text_timer_llm.append(progress_bar_text)
                    active = False
                    break
            except Empty:
                pass
            finally:
                if active:
                    progress_bar_text += self._progress_bar_symbol
                    if self._show_live_progress_bar:
                        self._text_timer_tts.append(self._progress_bar_symbol)
                    time.sleep(self._progress_bar_print_interval_seconds)

    def _worker(self, title: str) -> None:
        thread_timer_llm = Thread(target=self._worker_timer_llm)
        thread_timer_tts = Thread(target=self._worker_timer_tts)
        thread_message = Thread(target=self._worker_message)

        if self._show_live_progress_bar:
            print()
            pad_text = Text("\n")
            inner_group = Group(
                title,
                Padding(self._text_timer_llm, pad=(1, 0, 0, 1)),
                Padding(self._text_timer_tts, pad=(0, 0, 0, 1)),
                Padding(self._text_message, pad=(0, 0, 1, 1)),
            )
            if not self._show_llm_response:
                inner_group = Group(
                    title,
                    self._text_timer_llm,
                    self._text_timer_tts,
                )
            rows = Group(
                Panel(
                    inner_group,
                ),
                pad_text)

            with Live(rows, refresh_per_second=1 / self._progress_bar_print_interval_seconds):
                thread_timer_llm.start()
                thread_timer_tts.start()
                thread_message.start()

                thread_timer_llm.join()
                thread_timer_tts.join()
                thread_message.join()
                pad_text.remove_suffix("\n")
        else:
            print()
            thread_message.start()
            animation = ListeningAnimation(message=f"Generating answer with {title}")
            animation.start()
            thread_timer_llm.start()
            thread_timer_tts.start()
            thread_timer_llm.join()
            thread_timer_tts.join()
            clean_console_line()
            print(f"{Colors.DARK_GREEN}{self._text_timer_llm.plain}{Colors.RESET}")
            print(f"{Colors.GREEN}{self._text_timer_tts.plain}{Colors.RESET}")
            animation.change_message(f"Answering with {title}")
            if self._show_llm_response:
                print(self._text_message.plain)
            thread_message.join()
            animation.stop(f"Answered with {title}!")
            print()

    def start(self, title: str) -> None:
        self._reset()
        self._thread_dashboard = Thread(target=self._worker, args=(title,))
        self._thread_dashboard.start()

    def update_llm_response(self, message: str) -> None:
        if self._thread_dashboard is None:
            raise ValueError("Dashboard must be started first")

        self._queue_message.put_nowait(self.MessageEvent(message))

    def update_timer_tts(self, timer_event: TimerEvent) -> None:
        if self._thread_dashboard is None:
            raise ValueError("Dashboard must be started first")

        self._queue_timer_tts.put_nowait(timer_event)

    def update_timer_llm(self, timer_event: TimerEvent) -> None:
        if self._thread_dashboard is None:
            raise ValueError("Dashboard must be started first")

        self._queue_timer_llm.put_nowait(timer_event)

    def stop(self) -> None:
        self._queue_message.put_nowait(self.MessageEvent(stop=True))
        self._thread_dashboard.join()
        self._thread_dashboard = None


__all__ = [
    "Colors",
    "ListeningAnimation",
    "ProgressPrinter",
    "Timer",
]
