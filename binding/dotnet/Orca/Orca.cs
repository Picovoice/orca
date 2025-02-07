/*
    Copyright 2025 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
    file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
    an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
    specific language governing permissions and limitations under the License.
*/

using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;

namespace Pv
{
    /// <summary>
    /// Status codes returned by Orca library
    /// </summary>
    public enum PvStatus
    {
        SUCCESS = 0,
        OUT_OF_MEMORY = 1,
        IO_ERROR = 2,
        INVALID_ARGUMENT = 3,
        STOP_ITERATION = 4,
        KEY_ERROR = 5,
        INVALID_STATE = 6,
        RUNTIME_ERROR = 7,
        ACTIVATION_ERROR = 8,
        ACTIVATION_LIMIT_REACHED = 9,
        ACTIVATION_THROTTLED = 10,
        ACTIVATION_REFUSED = 11
    }

    /// <summary>
    /// Class representing a phoneme synthesized by Orca and its associated metadata.
    /// </summary>
    public class OrcaPhoneme
    {
        /// <summary>
        /// Synthesized phoneme.
        /// </summary>
        public string Phoneme { get; }

        /// <summary>
        /// Start of phoneme in seconds.
        /// </summary>
        public float StartSec { get; }

        /// <summary>
        /// End of phoneme in seconds.
        /// </summary>
        public float EndSec { get; }

        /// <summary>
        /// Constructor for OrcaPhoneme.
        /// </summary>
        /// <param name="phoneme">Synthesized phoneme.</param>
        /// <param name="startSec">Start of phoneme in seconds.</param>
        /// <param name="endSec">End of phoneme in seconds.</param>
        public OrcaPhoneme(
            string phoneme,
            float startSec,
            float endSec)
        {
            Phoneme = phoneme;
            StartSec = startSec;
            EndSec = endSec;
        }
    }

    /// <summary>
    /// Class representing a word synthesized by Orca and its associated metadata.
    /// </summary>
    public class OrcaWord
    {
        /// <summary>
        /// Synthesized word.
        /// </summary>
        public string Word { get; }

        /// <summary>
        /// Start of word in seconds.
        /// </summary>
        public float StartSec { get; }

        /// <summary>
        /// End of word in seconds.
        /// </summary>
        public float EndSec { get; }

        /// <summary>
        /// Synthesized phonemes and their associated metadata.
        /// </summary>
        public OrcaPhoneme[] Phonemes { get; }

        /// <summary>
        /// Constructor for OrcaWord.
        /// </summary>
        /// <param name="word">Synthesized word.</param>
        /// <param name="startSec">Start of word in seconds.</param>
        /// <param name="endSec">End of word in seconds.</param>
        /// <param name="phonemes">Synthesized phonemes and their associated metadata.</param>
        public OrcaWord(
            string word,
            float startSec,
            float endSec,
            OrcaPhoneme[] phonemes)
        {
            Word = word;
            StartSec = startSec;
            EndSec = endSec;
            Phonemes = phonemes;
        }
    }

    /// <summary>
    /// Class that contains audio and word alignments returned by Orca's synthesize function.
    /// </summary>
    public class OrcaAudio
    {
        /// <summary>
        /// Synthesized audio.
        /// </summary>
        public short[] Pcm { get; }

        /// <summary>
        /// Synthesized words and their associated metadata.
        /// </summary>
        public OrcaWord[] Words { get; }

        /// <summary>
        /// Constructor for OrcaAudio.
        /// </summary>
        /// <param name="pcm">Synthesized audio.</param>
        /// <param name="words">Synthesized words and their associated metadata.</param>
        public OrcaAudio(short[] pcm, OrcaWord[] words)
        {
            Pcm = pcm;
            Words = words;
        }
    }

    /// <summary>
    /// .NET binding for Orca Streaming Text-to-Speech Engine.
    /// </summary>
    public class Orca : IDisposable
    {
        private const string LIBRARY = "libpv_orca";

        public static readonly string DEFAULT_MODEL_PATH;
        private IntPtr _libraryPointer = IntPtr.Zero;

        static Orca()
        {

#if NETCOREAPP3_0_OR_GREATER

            NativeLibrary.SetDllImportResolver(typeof(Orca).Assembly, ImportResolver);

#endif

            DEFAULT_MODEL_PATH = Utils.PvModelPath();

        }

#if NETCOREAPP3_0_OR_GREATER

        private static IntPtr ImportResolver(string libraryName, Assembly assembly, DllImportSearchPath? searchPath)
        {

#pragma warning disable IDE0058
#pragma warning disable IDE0059

            IntPtr libHandle = IntPtr.Zero;
            NativeLibrary.TryLoad(Utils.PvLibraryPath(libraryName), out libHandle);
            return libHandle;
        }

#pragma warning restore IDE0059
#pragma warning restore IDE0058

#endif

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern PvStatus pv_orca_init(
            IntPtr accessKey,
            IntPtr modelPath,
            out IntPtr handle);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern void pv_orca_delete(IntPtr handle);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern PvStatus pv_orca_valid_characters(
            IntPtr handle,
            out int numCharacters,
            out IntPtr characters);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern void pv_orca_valid_characters_delete(IntPtr characters);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern PvStatus pv_orca_sample_rate(
            IntPtr handle,
            out int sampleRate);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern PvStatus pv_orca_max_character_limit(
            IntPtr handle,
            out int sampleRate);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern PvStatus pv_orca_synthesize_params_init(out IntPtr handle);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern void pv_orca_synthesize_params_delete(IntPtr handle);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern PvStatus pv_orca_synthesize_params_set_speech_rate(
            IntPtr handle,
            float speechRate);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern PvStatus pv_orca_synthesize_params_set_random_state(
            IntPtr handle,
            long randomState);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern PvStatus pv_orca_stream_open(
            IntPtr handle,
            IntPtr synthesizeParams,
            out IntPtr streamPtr);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern PvStatus pv_orca_stream_synthesize(
            IntPtr streamPtr,
            IntPtr text,
            out int numSamples,
            out IntPtr pcm);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern PvStatus pv_orca_stream_flush(
            IntPtr streamPtr,
            out int numSamples,
            out IntPtr pcm);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern void pv_orca_stream_close(IntPtr streamPtr);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern PvStatus pv_orca_synthesize(
            IntPtr handle,
            IntPtr text,
            IntPtr synthesizeParams,
            out int numSamples,
            out IntPtr pcm,
            out int numAlignments,
            out IntPtr alignments);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern PvStatus pv_orca_synthesize_to_file(
            IntPtr handle,
            IntPtr text,
            IntPtr synthesizeParams,
            IntPtr outputPath,
            out int numAlignments,
            out IntPtr alignments);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern void pv_orca_pcm_delete(IntPtr pcm);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern void pv_orca_word_alignments_delete(
            int numAlignments,
            IntPtr alignments);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr pv_orca_version();

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern void pv_set_sdk(string sdk);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern PvStatus pv_get_error_stack(out IntPtr messageStack, out int messageStackDepth);

        [DllImport(LIBRARY, CallingConvention = CallingConvention.Cdecl)]
        private static extern void pv_free_error_stack(IntPtr messageStack);

        /// <summary>
        /// C Struct for storing phoneme alignment metadata
        /// </summary>
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        private struct CPhonemeAlignment
        {
            public readonly IntPtr phonemePtr;
            public readonly float startSec;
            public readonly float endSec;
        }

        /// <summary>
        /// C Struct for storing word alignment metadata
        /// </summary>
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        private struct CWordAlignment
        {
            public readonly IntPtr wordPtr;
            public readonly float startSec;
            public readonly float endSec;

            public readonly int numPhonemes;
            public readonly IntPtr phonemes;
        }

        /// <summary>
        /// Gets the version number of the Orca library.
        /// </summary>
        /// <returns>Version of Orca</returns>
        public string Version { get; private set; }

        /// <summary>
        /// Gets the sampling rate of the audio produced by Orca.
        /// </summary>
        /// <returns>Sampling rate of the audio produced by Orca.</returns>
        public int SampleRate { get; private set; }

        /// <summary>
        /// Gets the maximum number of characters that can be synthesized at once.
        /// </summary>
        /// <returns>Maximum number of characters that can be synthesized at once.</returns>
        public int MaxCharacterLimit { get; private set; }

        /// <summary>
        /// Gets an array of characters that are accepted as input to Orca synthesize functions.
        /// </summary>
        /// <returns>An array of valid characters for Orca.</returns>
        public string[] ValidCharacters { get; private set; }

        /// <summary>
        /// Class created by Orca that converts a stream of text to a stream of audio.
        /// </summary>
        public class OrcaStream : IDisposable
        {
            private IntPtr _streamPointer;

            internal OrcaStream(IntPtr streamPointer)
            {
                _streamPointer = streamPointer;
            }

            /// <summary>
            /// Adds a chunk of text to the OrcaStream object and generates audio if enough text has been added.
            /// This function is expected to be called multiple times with consecutive chunks of text from a text stream.
            /// The incoming text is buffered as it arrives until there is enough context to convert a chunk of the buffered
            /// text into audio. The caller needs to use `Flush()` to generate the audio chunk for
            /// the remaining text that has not yet been synthesized.
            /// </summary>
            /// <param name="text">
            /// A chunk of text from a text input stream. Characters not supported by Orca will be ignored.
            /// Valid characters can be retrieved by checking `Orca.ValidCharacters`.
            /// Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`. They need to be
            /// added in a single call to this function.The pronunciation is expressed in ARPAbet format,
            /// e.g.: `I {liv|L IH V} in {Sevilla|S EH V IY Y AH}`.
            /// </param>
            /// <returns>
            ///  The generated audio as a sequence of 16-bit linearly-encoded integers, `null` if no
            ///  audio chunk has been produced.
            /// </returns>
            public short[] Synthesize(string text)
            {
                if (_streamPointer == IntPtr.Zero)
                {
                    throw new OrcaInvalidStateException(
                        $"Unable to synthesize - stream not open"
                    );
                }

                IntPtr textPtr = Utils.GetPtrFromUtf8String(text);

                int numSamples;
                IntPtr cPcm;

                PvStatus status = pv_orca_stream_synthesize(
                    _streamPointer,
                    textPtr,
                    out numSamples,
                    out cPcm);

                Marshal.FreeHGlobal(textPtr);

                if (status != PvStatus.SUCCESS)
                {
                    HandlePvStatus(status, "Orca stream synthesize failed");
                }

                short[] pcm = null;
                if (numSamples > 0)
                {
                    pcm = new short[numSamples];
                    Marshal.Copy(cPcm, pcm, 0, numSamples);
                    pv_orca_pcm_delete(cPcm);

                }
                return pcm;
            }

            /// <summary>
            /// Generates audio for all the buffered text that has been added to the OrcaStream object
            /// via `OrcaStream.synthesize()`.
            /// </summary>
            /// <returns>
            /// The generated audio as a sequence of 16-bit linearly-encoded integers, `nil` if no
            /// audio chunk has been produced.
            /// </returns>
            public short[] Flush()
            {
                if (_streamPointer == IntPtr.Zero)
                {
                    throw new OrcaInvalidStateException(
                        $"Unable to flush - stream not open"
                    );
                }

                int numSamples;
                IntPtr cPcm;

                PvStatus status = pv_orca_stream_flush(
                    _streamPointer,
                    out numSamples,
                    out cPcm);

                if (status != PvStatus.SUCCESS)
                {
                    HandlePvStatus(status, "Orca stream flush failed");
                }

                short[] pcm = new short[numSamples];
                Marshal.Copy(cPcm, pcm, 0, numSamples);
                pv_orca_pcm_delete(cPcm);

                return pcm;
            }

            /// <summary>
            /// Releases the resources acquired by OrcaStream.
            /// </summary>
            public void Dispose()
            {
                if (_streamPointer != IntPtr.Zero)
                {
                    pv_orca_stream_close(_streamPointer);
                    _streamPointer = IntPtr.Zero;

                    // ensures finalizer doesn't trigger if already manually disposed
                    GC.SuppressFinalize(this);
                }
            }

            ~OrcaStream()
            {
                Dispose();
            }
        }

        /// <summary>
        /// Creates an instance of the Orca Streaming Text-to-Speech engine.
        /// </summary>
        /// <param name="accessKey">AccessKey obtained from Picovoice Console (https://console.picovoice.ai/).</param>
        /// <param name="modelPath">
        /// Absolute path to the file containing model parameters (`.pv`). If not set it will be set to the
        /// default location.
        /// </param>
        public static Orca Create(
            string accessKey,
            string modelPath = null)
        {
            return new Orca(
                accessKey,
                modelPath ?? DEFAULT_MODEL_PATH);
        }

        /// <summary>
        /// Creates an instance of the Orca Streaming Text-to-Speech engine.
        /// </summary>
        /// <param name="accessKey">AccessKey obtained from Picovoice Console (https://console.picovoice.ai/).</param>
        /// <param name="modelPath">
        /// Absolute path to the file containing model parameters (`.pv`). If not set it will be set to the
        /// default location.
        /// </param>
        private Orca(
            string accessKey,
            string modelPath)
        {
            if (string.IsNullOrEmpty(accessKey))
            {
                throw new OrcaInvalidArgumentException("No AccessKey provided to Orca");
            }

            if (!File.Exists(modelPath))
            {
                throw new OrcaIOException($"Couldn't find model file at '{modelPath}'");
            }

            IntPtr accessKeyPtr = Utils.GetPtrFromUtf8String(accessKey);
            IntPtr modelPathPtr = Utils.GetPtrFromUtf8String(modelPath);

            pv_set_sdk("dotnet");

            PvStatus status = pv_orca_init(
                accessKeyPtr,
                modelPathPtr,
                out _libraryPointer);

            Marshal.FreeHGlobal(accessKeyPtr);
            Marshal.FreeHGlobal(modelPathPtr);

            if (status != PvStatus.SUCCESS)
            {
                HandlePvStatus(status, "Orca init failed");
            }

            int numValidCharacters;
            IntPtr validCharactersPtr;
            status = pv_orca_valid_characters(
                _libraryPointer,
                out numValidCharacters,
                out validCharactersPtr);
            if (status != PvStatus.SUCCESS)
            {
                HandlePvStatus(status, "Failed to get valid characters");
            }
            ValidCharacters = new string[numValidCharacters];
            for (int i = 0; i < numValidCharacters; i++)
            {
                IntPtr validCharacterPtr = Marshal.ReadIntPtr(validCharactersPtr, i * Marshal.SizeOf(typeof(IntPtr)));
                ValidCharacters[i] = Utils.GetUtf8StringFromPtr(validCharacterPtr);
            }
            pv_orca_valid_characters_delete(validCharactersPtr);

            int sampleRate;
            status = pv_orca_sample_rate(_libraryPointer, out sampleRate);
            if (status != PvStatus.SUCCESS)
            {
                HandlePvStatus(status, "Failed to get sample rate");
            }
            SampleRate = sampleRate;

            int maxCharacterLimit;
            status = pv_orca_max_character_limit(_libraryPointer, out maxCharacterLimit);
            if (status != PvStatus.SUCCESS)
            {
                HandlePvStatus(status, "Failed to get max character limit");
            }
            MaxCharacterLimit = maxCharacterLimit;

            Version = Utils.GetUtf8StringFromPtr(pv_orca_version());
        }

        /// <summary>
        /// Generates audio from text. The returned audio contains the speech representation of the text.
        /// </summary>
        /// <param name="text">
        /// Text to be converted to audio. The maximum number of characters per call is
        /// `.MaxCharacterLimit`. Allowed characters can be retrieved by calling `.ValidCharacters`.
        /// Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
        /// The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
        /// </param>
        /// <param name="speechRate">Rate of speech of the generated audio. Valid values are within [0.7, 1.3].</param>
        /// <param name="randomState">Random seed for the synthesis process.</param>
        /// <returns>
        ///  An object containing the generated audio as a sequence of 16-bit linearly-encoded integers
        ///  and an array of `OrcaWord` objects representing the word alignments.
        /// </returns>
        public OrcaAudio Synthesize(
            string text,
            float? speechRate = null,
            long? randomState = null)
        {
            if (text.Length > MaxCharacterLimit)
            {
                throw new OrcaInvalidArgumentException(
                    $"Text length ({text.Length}) must be smaller than max character limit {MaxCharacterLimit}"
                );
            }

            IntPtr synthesizeParamsPtr = GetSynthesizeParams(speechRate, randomState);

            IntPtr textPtr = Utils.GetPtrFromUtf8String(text);

            int numSamples;
            IntPtr cPcm;
            int numAlignments;
            IntPtr cAlignments;

            PvStatus status = pv_orca_synthesize(
                _libraryPointer,
                textPtr,
                synthesizeParamsPtr,
                out numSamples,
                out cPcm,
                out numAlignments,
                out cAlignments);

            Marshal.FreeHGlobal(textPtr);
            pv_orca_synthesize_params_delete(synthesizeParamsPtr);

            if (status != PvStatus.SUCCESS)
            {
                HandlePvStatus(status, "Orca synthesize failed");
            }

            short[] pcm = new short[numSamples];
            Marshal.Copy(cPcm, pcm, 0, numSamples);
            pv_orca_pcm_delete(cPcm);

            OrcaWord[] alignments = GetWordAlignments(numAlignments, cAlignments);
            pv_orca_word_alignments_delete(numAlignments, cAlignments);

            return new OrcaAudio(pcm, alignments);
        }

        /// <summary>
        /// Generates audio from text. The returned audio contains the speech representation of the text.
        /// </summary>
        /// <param name="text">
        /// Text to be converted to audio. The maximum number of characters per call is
        /// `.MaxCharacterLimit`. Allowed characters can be retrieved by calling `.ValidCharacters`.
        /// Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
        /// The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
        /// </param>
        /// <param name="outputPath">
        /// Absolute path to the output audio file. The output file is saved as `WAV (.wav)`.
        /// </param>
        /// <param name="speechRate">Rate of speech of the generated audio. Valid values are within [0.7, 1.3].</param>
        /// <param name="randomState">Random seed for the synthesis process.</param>
        /// <returns>
        /// An array of `OrcaWord` objects representing the word alignments.
        /// </returns>
        public OrcaWord[] SynthesizeToFile(
            string text,
            string outputPath,
            float? speechRate = null,
            long? randomState = null)
        {
            if (text.Length > MaxCharacterLimit)
            {
                throw new OrcaInvalidArgumentException(
                    $"Text length ({text.Length}) must be smaller than max character limit {MaxCharacterLimit}"
                );
            }

            IntPtr synthesizeParams = GetSynthesizeParams(speechRate, randomState);

            IntPtr textPtr = Utils.GetPtrFromUtf8String(text);
            IntPtr outputPathPtr = Utils.GetPtrFromUtf8String(outputPath);

            int numAlignments;
            IntPtr cAlignments;

            PvStatus status = pv_orca_synthesize_to_file(
                _libraryPointer,
                textPtr,
                synthesizeParams,
                outputPathPtr,
                out numAlignments,
                out cAlignments);

            Marshal.FreeHGlobal(textPtr);
            Marshal.FreeHGlobal(outputPathPtr);
            pv_orca_synthesize_params_delete(synthesizeParams);

            if (status != PvStatus.SUCCESS)
            {
                HandlePvStatus(status, "Orca synthesize to file failed");
            }

            OrcaWord[] alignments = GetWordAlignments(numAlignments, cAlignments);
            pv_orca_word_alignments_delete(numAlignments, cAlignments);

            return alignments;
        }

        public OrcaStream StreamOpen(
            float? speechRate = null,
            long? randomState = null)
        {
            IntPtr synthesizeParams = GetSynthesizeParams(speechRate, randomState);

            IntPtr streamPtr;
            PvStatus status = pv_orca_stream_open(
                _libraryPointer,
                synthesizeParams,
                out streamPtr);

            if (status != PvStatus.SUCCESS)
            {
                HandlePvStatus(status, "Orca stream open failed");
            }

            return new OrcaStream(streamPtr);
        }

        /// <summary>
        /// Coverts status codes to relevant .NET exceptions
        /// </summary>
        /// <param name="status">Picovoice library status code.</param>
        /// <param name="message">Default error message.</param>
        /// <param name="messageStack">Error stack returned from Picovoice library.</param>
        /// <returns>.NET exception</returns>
        private static OrcaException PvStatusToException(
            PvStatus status,
            string message = "",
            string[] messageStack = null)
        {
            if (messageStack == null)
            {
                messageStack = new string[] { };
            }

            switch (status)
            {
                case PvStatus.OUT_OF_MEMORY:
                    return new OrcaMemoryException(message, messageStack);
                case PvStatus.IO_ERROR:
                    return new OrcaIOException(message, messageStack);
                case PvStatus.INVALID_ARGUMENT:
                    return new OrcaInvalidArgumentException(message, messageStack);
                case PvStatus.STOP_ITERATION:
                    return new OrcaStopIterationException(message, messageStack);
                case PvStatus.KEY_ERROR:
                    return new OrcaKeyException(message, messageStack);
                case PvStatus.INVALID_STATE:
                    return new OrcaInvalidStateException(message, messageStack);
                case PvStatus.RUNTIME_ERROR:
                    return new OrcaRuntimeException(message, messageStack);
                case PvStatus.ACTIVATION_ERROR:
                    return new OrcaActivationException(message, messageStack);
                case PvStatus.ACTIVATION_LIMIT_REACHED:
                    return new OrcaActivationLimitException(message, messageStack);
                case PvStatus.ACTIVATION_THROTTLED:
                    return new OrcaActivationThrottledException(message, messageStack);
                case PvStatus.ACTIVATION_REFUSED:
                    return new OrcaActivationRefusedException(message, messageStack);
                default:
                    return new OrcaException("Unmapped error code returned from Orca.", messageStack);
            }
        }

        /// <summary>
        /// Frees memory that was allocated for Orca
        /// </summary>
        public void Dispose()
        {
            if (_libraryPointer != IntPtr.Zero)
            {
                pv_orca_delete(_libraryPointer);
                _libraryPointer = IntPtr.Zero;

                // ensures finalizer doesn't trigger if already manually disposed
                GC.SuppressFinalize(this);
            }
        }

        ~Orca()
        {
            Dispose();
        }

        private static IntPtr GetSynthesizeParams(float? speechRate, long? randomState)
        {
            IntPtr synthesizeParams;
            PvStatus status = pv_orca_synthesize_params_init(out synthesizeParams);
            if (status != PvStatus.SUCCESS)
            {
                HandlePvStatus(status, "Orca init synthesize params failed");
            }

            if (speechRate != null)
            {
                status = pv_orca_synthesize_params_set_speech_rate(synthesizeParams, (float)speechRate);
                if (status != PvStatus.SUCCESS)
                {
                    HandlePvStatus(status, "Orca set speech rate failed");
                }
            }

            if (randomState != null)
            {
                status = pv_orca_synthesize_params_set_random_state(synthesizeParams, (long)randomState);
                if (status != PvStatus.SUCCESS)
                {
                    HandlePvStatus(status, "Orca set random state failed");
                }
            }

            return synthesizeParams;
        }

        private static OrcaWord[] GetWordAlignments(int numAlignments, IntPtr alignments)
        {
            OrcaWord[] words = new OrcaWord[numAlignments];

            for (int i = 0; i < numAlignments; i++)
            {
                IntPtr alignmentPtr = Marshal.ReadIntPtr(alignments, i * Marshal.SizeOf(typeof(IntPtr)));
                CWordAlignment word = (CWordAlignment)Marshal.PtrToStructure(alignmentPtr, typeof(CWordAlignment));

                OrcaPhoneme[] phonemes = new OrcaPhoneme[word.numPhonemes];
                for (int j = 0; j < word.numPhonemes; j++)
                {
                    IntPtr phonemePtr = Marshal.ReadIntPtr(word.phonemes, j * Marshal.SizeOf(typeof(IntPtr)));
                    CPhonemeAlignment phoneme = (CPhonemeAlignment)Marshal.PtrToStructure(phonemePtr, typeof(CPhonemeAlignment));
                    phonemes[j] = new OrcaPhoneme(
                        Utils.GetUtf8StringFromPtr(phoneme.phonemePtr),
                        phoneme.startSec,
                        phoneme.endSec);
                }
                words[i] = new OrcaWord(
                    Utils.GetUtf8StringFromPtr(word.wordPtr),
                    word.startSec,
                    word.endSec,
                    phonemes);
            }

            return words;
        }

        private static void HandlePvStatus(PvStatus status, string message)
        {
            string[] messageStack = GetMessageStack();
            throw PvStatusToException(status, message, messageStack);
        }

        private static string[] GetMessageStack()
        {
            int messageStackDepth;
            IntPtr messageStackRef;

            PvStatus status = pv_get_error_stack(out messageStackRef, out messageStackDepth);
            if (status != PvStatus.SUCCESS)
            {
                throw PvStatusToException(status, "Unable to get Orca error state");
            }

            int elementSize = Marshal.SizeOf(typeof(IntPtr));
            string[] messageStack = new string[messageStackDepth];

            for (int i = 0; i < messageStackDepth; i++)
            {
                messageStack[i] = Marshal.PtrToStringAnsi(Marshal.ReadIntPtr(messageStackRef, i * elementSize));
            }

            pv_free_error_stack(messageStackRef);

            return messageStack;
        }
    }
}