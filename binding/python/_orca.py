import os
from collections import namedtuple
from ctypes import *
from enum import Enum
from typing import (
    Optional,
    Sequence,
    Set,
    Tuple)


class OrcaError(Exception):
    def __init__(self, message: str = "", message_stack: Sequence[str] = None) -> None:
        super().__init__(message)

        self._message = message
        self._message_stack = list() if message_stack is None else message_stack

    def __str__(self) -> str:
        message = self._message
        if len(self._message_stack) > 0:
            message += ":"
            for i in range(len(self._message_stack)):
                message += "\n  [%d] %s" % (i, self._message_stack[i])
        return message

    @property
    def message(self) -> str:
        return self._message

    @property
    def message_stack(self) -> Sequence[str]:
        return self._message_stack


class OrcaMemoryError(OrcaError):
    pass


class OrcaIOError(OrcaError):
    pass


class OrcaInvalidArgumentError(OrcaError):
    pass


class OrcaStopIterationError(OrcaError):
    pass


class OrcaKeyError(OrcaError):
    pass


class OrcaInvalidStateError(OrcaError):
    pass


class OrcaRuntimeError(OrcaError):
    pass


class OrcaActivationError(OrcaError):
    pass


class OrcaActivationLimitError(OrcaError):
    pass


class OrcaActivationThrottledError(OrcaError):
    pass


class OrcaActivationRefusedError(OrcaError):
    pass


class PicovoiceStatuses(Enum):
    SUCCESS = 0
    OUT_OF_MEMORY = 1
    IO_ERROR = 2
    INVALID_ARGUMENT = 3
    STOP_ITERATION = 4
    KEY_ERROR = 5
    INVALID_STATE = 6
    RUNTIME_ERROR = 7
    ACTIVATION_ERROR = 8
    ACTIVATION_LIMIT_REACHED = 9
    ACTIVATION_THROTTLED = 10
    ACTIVATION_REFUSED = 11


_PICOVOICE_STATUS_TO_EXCEPTION = {
    PicovoiceStatuses.OUT_OF_MEMORY: OrcaMemoryError,
    PicovoiceStatuses.IO_ERROR: OrcaIOError,
    PicovoiceStatuses.INVALID_ARGUMENT: OrcaInvalidArgumentError,
    PicovoiceStatuses.STOP_ITERATION: OrcaStopIterationError,
    PicovoiceStatuses.KEY_ERROR: OrcaKeyError,
    PicovoiceStatuses.INVALID_STATE: OrcaInvalidStateError,
    PicovoiceStatuses.RUNTIME_ERROR: OrcaRuntimeError,
    PicovoiceStatuses.ACTIVATION_ERROR: OrcaActivationError,
    PicovoiceStatuses.ACTIVATION_LIMIT_REACHED: OrcaActivationLimitError,
    PicovoiceStatuses.ACTIVATION_THROTTLED: OrcaActivationThrottledError,
    PicovoiceStatuses.ACTIVATION_REFUSED: OrcaActivationRefusedError,
}


class COrcaPhonemeAlignment(Structure):
    _fields_ = [
        ("phoneme", c_char_p),
        ("start_sec", c_float),
        ("end_sec", c_float),
    ]


class COrcaWordAlignment(Structure):
    _fields_ = [
        ("word", c_char_p),
        ("start_sec", c_float),
        ("end_sec", c_float),
        ("num_phonemes", c_int32),
        ("phonemes", POINTER(POINTER(COrcaPhonemeAlignment))),
    ]


class Orca:
    """
    Python binding for Orca Text-to-Speech engine.
    """

    class COrca(Structure):
        pass

    class COrcaSynthesizeParams(Structure):
        pass

    class COrcaStream(Structure):
        pass

    class Stream:
        """
        TODO
        """
        def __init__(self, handle: POINTER('Orca.COrcaStream'), orca: 'Orca') -> None:
            self._handle = handle
            self._orca = orca

        def synthesize(self, text: str) -> Optional[Sequence[int]]:
            """
            TODO
            """
            c_num_samples = c_int32()
            c_pcm = POINTER(c_int16)()

            status = self._orca._stream_synthesize_func(
                self._handle,
                text.encode("utf-8"),
                byref(c_num_samples),
                byref(c_pcm)
            )
            if status is not PicovoiceStatuses.SUCCESS:
                raise _PICOVOICE_STATUS_TO_EXCEPTION[status](
                    message="Unable to synthesize text in Orca stream",
                    message_stack=self._orca._get_error_stack())

            pcm = [c_pcm[i] for i in range(c_num_samples.value)]
            self._orca._pcm_delete_func(c_pcm)

            return pcm

        def flush(self) -> Optional[Sequence[int]]:
            """
            TODO
            """
            c_num_samples = c_int32()
            c_pcm = POINTER(c_int16)()

            status = self._orca._stream_flush_func(
                self._handle,
                byref(c_num_samples),
                byref(c_pcm)
            )
            if status is not PicovoiceStatuses.SUCCESS:
                raise _PICOVOICE_STATUS_TO_EXCEPTION[status](
                    message="Unable to flush Orca stream",
                    message_stack=self._orca._get_error_stack())

            pcm = [c_pcm[i] for i in range(c_num_samples.value)]
            self._orca._pcm_delete_func(c_pcm)

            return pcm

        def close(self) -> None:
            """
            TODO
            """
            self._orca._stream_close_func(self._handle)

    def __init__(self, access_key: str, model_path: str, library_path: str) -> None:
        """
        Constructor.

        :param access_key: AccessKey obtained from Picovoice Console (https://console.picovoice.ai/)
        :param model_path: Absolute path to the file containing model parameters.
        :param library_path: Absolute path to Orca's dynamic library.
        """

        if not isinstance(access_key, str) or len(access_key) == 0:
            raise OrcaInvalidArgumentError("`access_key` should be a non-empty string.")

        if not os.path.exists(model_path):
            raise OrcaIOError("Could not find model file at `%s`." % model_path)

        if not os.path.exists(library_path):
            raise OrcaIOError("Could not find Orca's dynamic library at `%s`." % library_path)

        library = cdll.LoadLibrary(library_path)

        set_sdk_func = library.pv_set_sdk
        set_sdk_func.argtypes = [c_char_p]
        set_sdk_func.restype = None

        set_sdk_func('python'.encode("utf-8"))

        self._get_error_stack_func = library.pv_get_error_stack
        self._get_error_stack_func.argtypes = [POINTER(POINTER(c_char_p)), POINTER(c_int)]
        self._get_error_stack_func.restype = PicovoiceStatuses

        self._free_error_stack_func = library.pv_free_error_stack
        self._free_error_stack_func.argtypes = [POINTER(c_char_p)]
        self._free_error_stack_func.restype = None

        init_func = library.pv_orca_init
        init_func.argtypes = [c_char_p, c_char_p, POINTER(POINTER(self.COrca))]
        init_func.restype = PicovoiceStatuses

        self._handle = POINTER(self.COrca)()
        status = init_func(access_key.encode(), model_path.encode(), byref(self._handle))
        if status is not PicovoiceStatuses.SUCCESS:
            raise _PICOVOICE_STATUS_TO_EXCEPTION[status](
                message='Initialization failed',
                message_stack=self._get_error_stack())

        self._delete_func = library.pv_orca_delete
        self._delete_func.argtypes = [POINTER(self.COrca)]
        self._delete_func.restype = None

        self._valid_characters_func = library.pv_orca_valid_characters
        self._valid_characters_func.argtypes = [
            POINTER(self.COrca),
            POINTER(c_int32),
            POINTER(POINTER(POINTER(c_char_p))),
        ]
        self._valid_characters_func.restype = PicovoiceStatuses

        self._valid_characters_delete_func = library.pv_orca_valid_characters_delete
        self._valid_characters_delete_func.argtypes = [POINTER(POINTER(c_char_p))]
        self._valid_characters_delete_func.restype = None

        self._sample_rate_func = library.pv_orca_sample_rate
        self._sample_rate_func.argtypes = [POINTER(self.COrca), POINTER(c_int32)]
        self._sample_rate_func.restype = PicovoiceStatuses

        self._max_character_limit_func = library.pv_orca_max_character_limit
        self._max_character_limit_func.argtypes = [POINTER(self.COrca), POINTER(c_int32)]
        self._max_character_limit_func.restype = PicovoiceStatuses

        self._synthesize_params_init_func = library.pv_orca_synthesize_params_init
        self._synthesize_params_init_func.argtypes = [POINTER(POINTER(self.COrcaSynthesizeParams))]
        self._synthesize_params_init_func.restype = PicovoiceStatuses

        self._synthesize_params_delete_func = library.pv_orca_synthesize_params_delete
        self._synthesize_params_delete_func.argtypes = [POINTER(self.COrcaSynthesizeParams)]
        self._synthesize_params_delete_func.restype = None

        self._synthesize_params_set_speech_rate_func = library.pv_orca_synthesize_params_set_speech_rate
        self._synthesize_params_set_speech_rate_func.argtypes = [POINTER(self.COrcaSynthesizeParams), c_float]
        self._synthesize_params_set_speech_rate_func.restype = PicovoiceStatuses

        self._synthesize_params_set_random_state_func = library.pv_orca_synthesize_params_set_random_state
        self._synthesize_params_set_random_state_func.argtypes = [POINTER(self.COrcaSynthesizeParams), c_int64]
        self._synthesize_params_set_random_state_func.restype = PicovoiceStatuses

        self._synthesize_func = library.pv_orca_synthesize
        self._synthesize_func.argtypes = [
            POINTER(self.COrca),
            c_char_p,
            POINTER(self.COrcaSynthesizeParams),
            POINTER(c_int32),
            POINTER(POINTER(c_int16)),
            POINTER(c_int32),
            POINTER(POINTER(POINTER(COrcaWordAlignment))),
        ]
        self._synthesize_func.restype = PicovoiceStatuses

        self._synthesize_to_file_func = library.pv_orca_synthesize_to_file
        self._synthesize_to_file_func.argtypes = [
            POINTER(self.COrca),
            c_char_p,
            POINTER(self.COrcaSynthesizeParams),
            c_char_p,
            POINTER(c_int32),
            POINTER(POINTER(POINTER(COrcaWordAlignment))),
        ]
        self._synthesize_to_file_func.restype = PicovoiceStatuses

        self._word_alignments_delete_func = library.pv_orca_word_alignments_delete
        self._word_alignments_delete_func.argtypes = [c_int32, POINTER(POINTER(COrcaWordAlignment))]
        self._word_alignments_delete_func.restype = PicovoiceStatuses

        self._pcm_delete_func = library.pv_orca_pcm_delete
        self._pcm_delete_func.argtypes = [POINTER(c_int16)]
        self._pcm_delete_func.restype = None

        self._stream_open_func = library.pv_orca_stream_open
        self._stream_open_func.argtypes = [
            POINTER(self.COrca),
            POINTER(self.COrcaSynthesizeParams),
            POINTER(POINTER(self.COrcaStream))
        ]
        self._stream_open_func.restype = PicovoiceStatuses

        self._stream_synthesize_func = library.pv_orca_stream_synthesize
        self._stream_synthesize_func.argtypes = [
            POINTER(self.COrcaStream),
            c_char_p,
            POINTER(c_int32),
            POINTER(POINTER(c_int16))
        ]
        self._stream_synthesize_func.restype = PicovoiceStatuses

        self._stream_flush_func = library.pv_orca_stream_flush
        self._stream_flush_func.argtypes = [
            POINTER(self.COrcaStream),
            POINTER(c_int32),
            POINTER(POINTER(c_int16))
        ]
        self._stream_flush_func.restype = PicovoiceStatuses

        self._stream_close_func = library.pv_orca_stream_close
        self._stream_close_func.argtypes = [POINTER(self.COrcaStream)]
        self._stream_close_func.restype = None

        version_func = library.pv_orca_version
        version_func.argtypes = []
        version_func.restype = c_char_p
        self._version = version_func().decode("utf-8")

    PhonemeAlignment = namedtuple('Phoneme', ['phoneme', 'start_sec', 'end_sec'])
    WordAlignment = namedtuple('Word', ['word', 'start_sec', 'end_sec', 'phonemes'])

    def delete(self) -> None:
        """Releases resources acquired by Orca."""

        self._delete_func(self._handle)

    @property
    def valid_characters(self) -> Set[str]:
        """Set of characters supported by Orca."""

        c_num_characters = c_int32()
        c_characters = POINTER(POINTER(c_char_p))()

        status = self._valid_characters_func(self._handle, byref(c_num_characters), byref(c_characters))
        if status is not PicovoiceStatuses.SUCCESS:
            raise _PICOVOICE_STATUS_TO_EXCEPTION[status](
                message="Unable to get Orca valid characters",
                message_stack=self._get_error_stack())

        num_characters = c_num_characters.value
        characters_array_pointer = cast(c_characters, POINTER(c_char_p * num_characters))
        characters = set([symbol.decode('utf-8') for symbol in list(characters_array_pointer.contents)])

        self._valid_characters_delete_func(c_characters)

        return characters

    @property
    def sample_rate(self) -> int:
        """Audio sample rate of generated audio."""

        c_sample_rate = c_int32()

        status = self._sample_rate_func(self._handle, byref(c_sample_rate))
        if status is not PicovoiceStatuses.SUCCESS:
            raise _PICOVOICE_STATUS_TO_EXCEPTION[status](
                message="Unable to get Orca sample rate",
                message_stack=self._get_error_stack())

        return c_sample_rate.value

    @property
    def max_character_limit(self) -> int:
        """Maximum number of characters allowed in a single synthesis request."""

        c_max_character_limit = c_int32()

        status = self._max_character_limit_func(self._handle, byref(c_max_character_limit))
        if status is not PicovoiceStatuses.SUCCESS:
            raise _PICOVOICE_STATUS_TO_EXCEPTION[status](
                message="Unable to get Orca maximum character limit",
                message_stack=self._get_error_stack())

        return c_max_character_limit.value

    def synthesize(
            self,
            text: str,
            speech_rate: Optional[float] = None,
            random_state: Optional[int] = None) -> Tuple[Sequence[int], Sequence[WordAlignment]]:
        """
        Generates audio from text. The returned audio contains the speech representation of the text.

        :param text: Text to be converted to audio. The maximum number of characters per call to `.synthesize()` is
        `self.max_character_limit`. Allowed characters can be retrieved by calling `self.pv_orca_valid_characters`.
        Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
        The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
        :param speech_rate: Rate of speech of the synthesized audio.
        :param random_state: Random seed for the synthesis process.
        :return: A tuple containing the generated audio as a sequence of 16-bit linearly-encoded integers
        and a sequence of OrcaWordAlignment objects representing the word alignments.
        """

        c_synthesize_params = self._get_c_synthesize_params(speech_rate=speech_rate, random_state=random_state)

        c_num_samples = c_int32()
        c_pcm = POINTER(c_int16)()
        c_num_alignments = c_int32()
        c_alignments = POINTER(POINTER(COrcaWordAlignment))()

        status = self._synthesize_func(
            self._handle,
            text.encode("utf-8"),
            c_synthesize_params,
            byref(c_num_samples),
            byref(c_pcm),
            byref(c_num_alignments),
            byref(c_alignments))
        if status is not PicovoiceStatuses.SUCCESS:
            raise _PICOVOICE_STATUS_TO_EXCEPTION[status](
                message="Unable to synthesize speech",
                message_stack=self._get_error_stack())

        pcm = [c_pcm[i] for i in range(c_num_samples.value)]
        self._pcm_delete_func(c_pcm)

        alignments = []
        for i in range(c_num_alignments.value):
            word_alignment = c_alignments[i].contents
            word = word_alignment.word.decode("utf-8")
            start_sec = word_alignment.start_sec
            end_sec = word_alignment.end_sec
            num_phonemes = word_alignment.num_phonemes
            phoneme_alignments = []
            for j in range(num_phonemes):
                phoneme_alignment = word_alignment.phonemes[j].contents
                phoneme = phoneme_alignment.phoneme.decode("utf-8")
                phoneme_start_sec = phoneme_alignment.start_sec
                phoneme_end_sec = phoneme_alignment.end_sec
                phoneme_alignment = self.PhonemeAlignment(
                    phoneme=phoneme,
                    start_sec=phoneme_start_sec,
                    end_sec=phoneme_end_sec)
                phoneme_alignments.append(phoneme_alignment)
            word_alignment = self.WordAlignment(
                word=word,
                start_sec=start_sec,
                end_sec=end_sec,
                phonemes=phoneme_alignments)
            alignments.append(word_alignment)

        status = self._word_alignments_delete_func(c_num_alignments.value, c_alignments)
        if status is not PicovoiceStatuses.SUCCESS:
            raise _PICOVOICE_STATUS_TO_EXCEPTION[status](
                message="Unable to delete Orca word alignments",
                message_stack=self._get_error_stack())

        self._synthesize_params_delete_func(c_synthesize_params)

        return pcm, alignments

    def synthesize_to_file(
            self,
            text: str,
            output_path: str,
            speech_rate: Optional[float] = None,
            random_state: Optional[int] = None) -> Sequence[WordAlignment]:
        """
        Generates audio from text. The returned audio contains the speech representation of the text.

        :param text: Text to be converted to audio. The maximum number of characters per call to `.synthesize()` is
        `self.max_character_limit`. Allowed characters can be retrieved by calling `self.pv_orca_valid_characters`.
        Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
        The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
        :param output_path: Absolute path to the output audio file. The output file is saved as `WAV (.wav)`
        and consists of a single mono channel.
        :param speech_rate: Rate of speech of the generated audio.
        :param random_state: Random seed for the synthesis process.
        :return: A sequence of OrcaWordAlignment objects representing the word alignments.
        """

        c_synthesize_params = self._get_c_synthesize_params(speech_rate=speech_rate, random_state=random_state)

        c_num_alignments = c_int32()
        c_alignments = POINTER(POINTER(COrcaWordAlignment))()

        status = self._synthesize_to_file_func(
            self._handle,
            text.encode("utf-8"),
            c_synthesize_params,
            output_path.encode("utf-8"),
            byref(c_num_alignments),
            byref(c_alignments))
        if status is not PicovoiceStatuses.SUCCESS:
            raise _PICOVOICE_STATUS_TO_EXCEPTION[status](
                message="Unable to synthesize speech",
                message_stack=self._get_error_stack())

        alignments = []
        for i in range(c_num_alignments.value):
            word_alignment = c_alignments[i].contents
            word = word_alignment.word.decode("utf-8")
            start_sec = word_alignment.start_sec
            end_sec = word_alignment.end_sec
            num_phonemes = word_alignment.num_phonemes
            phoneme_alignments = []
            for j in range(num_phonemes):
                phoneme_alignment = word_alignment.phonemes[j].contents
                phoneme = phoneme_alignment.phoneme.decode("utf-8")
                phoneme_start_sec = phoneme_alignment.start_sec
                phoneme_end_sec = phoneme_alignment.end_sec
                phoneme_alignment = self.PhonemeAlignment(
                    phoneme=phoneme,
                    start_sec=phoneme_start_sec,
                    end_sec=phoneme_end_sec)
                phoneme_alignments.append(phoneme_alignment)
            word_alignment = self.WordAlignment(
                word=word,
                start_sec=start_sec,
                end_sec=end_sec,
                phonemes=phoneme_alignments)
            alignments.append(word_alignment)

        status = self._word_alignments_delete_func(c_num_alignments.value, c_alignments)
        if status is not PicovoiceStatuses.SUCCESS:
            raise _PICOVOICE_STATUS_TO_EXCEPTION[status](
                message="Unable to delete Orca word alignments",
                message_stack=self._get_error_stack())

        self._synthesize_params_delete_func(c_synthesize_params)

        return alignments

    def open_stream(self, speech_rate: Optional[float] = None, random_state: Optional[int] = None) -> 'Orca.Stream':
        """
        Opens a stream for streaming synthesis.

        :param speech_rate: Rate of speech of the generated audio.
        :param random_state: Random seed for the synthesis process.
        :return: An instance of Orca.Stream.
        """

        c_synthesize_params = self._get_c_synthesize_params(speech_rate=speech_rate, random_state=random_state)

        stream_handle = POINTER(Orca.COrcaStream)()
        status = self._stream_open_func(
            self._handle,
            c_synthesize_params,
            byref(stream_handle))
        if status is not PicovoiceStatuses.SUCCESS:
            raise _PICOVOICE_STATUS_TO_EXCEPTION[status](
                message="Unable to open Orca stream",
                message_stack=self._get_error_stack())

        self._synthesize_params_delete_func(c_synthesize_params)

        return self.Stream(stream_handle, self)

    @property
    def version(self) -> str:
        """Version."""

        return self._version

    def _get_c_synthesize_params(
            self,
            speech_rate: Optional[float] = None,
            random_state: Optional[int] = None) -> POINTER(COrcaSynthesizeParams):
        c_params = POINTER(self.COrcaSynthesizeParams)()

        status = self._synthesize_params_init_func(byref(c_params))
        if status is not PicovoiceStatuses.SUCCESS:
            raise _PICOVOICE_STATUS_TO_EXCEPTION[status](
                message="Unable to create Orca synthesize params object",
                message_stack=self._get_error_stack())

        if speech_rate is not None:
            status = self._synthesize_params_set_speech_rate_func(c_params, c_float(speech_rate))
            if status is not PicovoiceStatuses.SUCCESS:
                raise _PICOVOICE_STATUS_TO_EXCEPTION[status](
                    message="Unable to set Orca speech rate",
                    message_stack=self._get_error_stack())

        if random_state is not None:
            status = self._synthesize_params_set_random_state_func(c_params, c_int64(random_state))
            if status is not PicovoiceStatuses.SUCCESS:
                raise _PICOVOICE_STATUS_TO_EXCEPTION[status](
                    message="Unable to set Orca random state",
                    message_stack=self._get_error_stack())

        return c_params

    def _get_error_stack(self) -> Sequence[str]:
        message_stack_ref = POINTER(c_char_p)()
        message_stack_depth = c_int()

        status = self._get_error_stack_func(byref(message_stack_ref), byref(message_stack_depth))
        if status is not PicovoiceStatuses.SUCCESS:
            raise _PICOVOICE_STATUS_TO_EXCEPTION[status](message="Unable to get Orca error state")

        message_stack = list()
        for i in range(message_stack_depth.value):
            message_stack.append(message_stack_ref[i].decode("utf-8"))

        self._free_error_stack_func(message_stack_ref)

        return message_stack


__all__ = [
    "Orca",
    "OrcaActivationError",
    "OrcaActivationLimitError",
    "OrcaActivationRefusedError",
    "OrcaActivationThrottledError",
    "OrcaError",
    "OrcaIOError",
    "OrcaInvalidArgumentError",
    "OrcaInvalidStateError",
    "OrcaKeyError",
    "OrcaMemoryError",
    "OrcaRuntimeError",
    "OrcaStopIterationError",
]
