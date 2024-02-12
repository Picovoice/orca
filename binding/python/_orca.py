import os
from ctypes import *
from enum import Enum
from typing import (
    Optional,
    Sequence,
    Set)


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


class Orca(object):
    """
    Python binding for Orca Text-to-Speech engine.
    """

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

    class COrca(Structure):
        pass

    class COrcaSynthesizeParams(Structure):
        pass

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
        self._get_error_stack_func.restype = self.PicovoiceStatuses

        self._free_error_stack_func = library.pv_free_error_stack
        self._free_error_stack_func.argtypes = [POINTER(c_char_p)]
        self._free_error_stack_func.restype = None

        init_func = library.pv_orca_init
        init_func.argtypes = [c_char_p, c_char_p, POINTER(POINTER(self.COrca))]
        init_func.restype = self.PicovoiceStatuses

        self._handle = POINTER(self.COrca)()
        status = init_func(access_key.encode(), model_path.encode(), byref(self._handle))
        if status is not self.PicovoiceStatuses.SUCCESS:
            raise self._PICOVOICE_STATUS_TO_EXCEPTION[status](
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
        self._valid_characters_func.restype = self.PicovoiceStatuses

        self._valid_characters_delete_func = library.pv_orca_valid_characters_delete
        self._valid_characters_delete_func.argtypes = [POINTER(POINTER(c_char_p))]
        self._valid_characters_delete_func.restype = None

        self._sample_rate_func = library.pv_orca_sample_rate
        self._sample_rate_func.argtypes = [POINTER(self.COrca), POINTER(c_int32)]
        self._sample_rate_func.restype = self.PicovoiceStatuses

        self._max_character_limit = library.pv_orca_max_character_limit()

        self._synthesize_params_init_func = library.pv_orca_synthesize_params_init
        self._synthesize_params_init_func.argtypes = [POINTER(POINTER(self.COrcaSynthesizeParams))]
        self._synthesize_params_init_func.restype = self.PicovoiceStatuses

        self._synthesize_params_delete_func = library.pv_orca_synthesize_params_delete
        self._synthesize_params_delete_func.argtypes = [POINTER(self.COrcaSynthesizeParams)]
        self._synthesize_params_delete_func.restype = None

        self._synthesize_params_set_speech_rate_func = library.pv_orca_synthesize_params_set_speech_rate
        self._synthesize_params_set_speech_rate_func.argtypes = [POINTER(self.COrcaSynthesizeParams), c_float]
        self._synthesize_params_set_speech_rate_func.restype = self.PicovoiceStatuses

        self._synthesize_func = library.pv_orca_synthesize
        self._synthesize_func.argtypes = [
            POINTER(self.COrca),
            c_char_p,
            POINTER(self.COrcaSynthesizeParams),
            POINTER(c_int32),
            POINTER(POINTER(c_int16)),
        ]
        self._synthesize_func.restype = self.PicovoiceStatuses

        self._synthesize_to_file_func = library.pv_orca_synthesize_to_file
        self._synthesize_to_file_func.argtypes = [
            POINTER(self.COrca),
            c_char_p,
            POINTER(self.COrcaSynthesizeParams),
            c_char_p,
        ]
        self._synthesize_to_file_func.restype = self.PicovoiceStatuses

        self._delete_pcm_func = library.pv_orca_delete_pcm
        self._delete_pcm_func.argtypes = [POINTER(c_int16)]
        self._delete_pcm_func.restype = None

        version_func = library.pv_orca_version
        version_func.argtypes = []
        version_func.restype = c_char_p
        self._version = version_func().decode("utf-8")

    def delete(self) -> None:
        """Releases resources acquired by Orca."""

        self._delete_func(self._handle)

    @property
    def valid_characters(self) -> Set[str]:
        """Set of characters supported by Orca."""

        c_num_characters = c_int32()
        c_characters = POINTER(POINTER(c_char_p))()

        status = self._valid_characters_func(self._handle, byref(c_num_characters), byref(c_characters))
        if status is not self.PicovoiceStatuses.SUCCESS:
            raise self._PICOVOICE_STATUS_TO_EXCEPTION[status](
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
        if status is not self.PicovoiceStatuses.SUCCESS:
            raise self._PICOVOICE_STATUS_TO_EXCEPTION[status](
                message="Unable to get Orca sample rate",
                message_stack=self._get_error_stack())

        return c_sample_rate.value

    @property
    def max_character_limit(self) -> int:
        """Maximum number of characters allowed in a single synthesis request."""

        return self._max_character_limit

    def synthesize(
            self,
            text: str,
            speech_rate: Optional[float] = None) -> Sequence[int]:
        """
        Generates audio from text. The returned audio contains the speech representation of the text.

        :param text: Text to be converted to audio. The maximum number of characters per call to `.synthesize()` is
        `self.max_character_limit`. Allowed characters can be retrieved by calling `self.pv_orca_valid_characters`.
        Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
        The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
        :param speech_rate: Rate of speech of the synthesized audio.
        :return: The generated audio, stored as a sequence of 16-bit linearly-encoded integers.
        """

        c_synthesize_params = self._get_c_synthesize_params(speech_rate=speech_rate)

        c_num_samples = c_int32()
        c_pcm = POINTER(c_int16)()
        status = self._synthesize_func(
            self._handle,
            text.encode("utf-8"),
            c_synthesize_params,
            byref(c_num_samples),
            byref(c_pcm))
        if status is not self.PicovoiceStatuses.SUCCESS:
            raise self._PICOVOICE_STATUS_TO_EXCEPTION[status](
                message="Unable to synthesize speech",
                message_stack=self._get_error_stack())

        pcm = [c_pcm[i] for i in range(c_num_samples.value)]

        self._delete_pcm_func(c_pcm)
        self._synthesize_params_delete_func(c_synthesize_params)

        return pcm

    def synthesize_to_file(
            self,
            text: str,
            output_path: str,
            speech_rate: Optional[float] = None) -> None:
        """
        Generates audio from text. The returned audio contains the speech representation of the text.

        :param text: Text to be converted to audio. The maximum number of characters per call to `.synthesize()` is
        `self.max_character_limit`. Allowed characters can be retrieved by calling `self.pv_orca_valid_characters`.
        Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
        The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
        :param output_path: Absolute path to the output audio file. The output file is saved as `WAV (.wav)`
        and consists of a single mono channel.
        :param speech_rate: Rate of speech of the generated audio.
        """

        c_synthesize_params = self._get_c_synthesize_params(speech_rate=speech_rate)

        status = self._synthesize_to_file_func(
            self._handle,
            text.encode("utf-8"),
            c_synthesize_params,
            output_path.encode("utf-8"))
        if status is not self.PicovoiceStatuses.SUCCESS:
            raise self._PICOVOICE_STATUS_TO_EXCEPTION[status](
                message="Unable to synthesize speech",
                message_stack=self._get_error_stack())

        self._synthesize_params_delete_func(c_synthesize_params)

    @property
    def version(self) -> str:
        """Version."""

        return self._version

    def _get_c_synthesize_params(self, speech_rate: Optional[float] = None) -> POINTER(COrcaSynthesizeParams):
        c_params = POINTER(self.COrcaSynthesizeParams)()

        status = self._synthesize_params_init_func(byref(c_params))
        if status is not self.PicovoiceStatuses.SUCCESS:
            raise self._PICOVOICE_STATUS_TO_EXCEPTION[status](
                message="Unable to create Orca synthesize params object",
                message_stack=self._get_error_stack())

        if speech_rate is not None:
            status = self._synthesize_params_set_speech_rate_func(c_params, c_float(speech_rate))
            if status is not self.PicovoiceStatuses.SUCCESS:
                raise self._PICOVOICE_STATUS_TO_EXCEPTION[status](
                    message="Unable to set Orca speech rate",
                    message_stack=self._get_error_stack())

        return c_params

    def _get_error_stack(self) -> Sequence[str]:
        message_stack_ref = POINTER(c_char_p)()
        message_stack_depth = c_int()

        status = self._get_error_stack_func(byref(message_stack_ref), byref(message_stack_depth))
        if status is not self.PicovoiceStatuses.SUCCESS:
            raise self._PICOVOICE_STATUS_TO_EXCEPTION[status](message="Unable to get Orca error state")

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
