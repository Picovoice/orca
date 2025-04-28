/*
    Copyright 2025 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
    file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
    an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
    specific language governing permissions and limitations under the License.
*/

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Threading;
using System.Timers;

using Pv;

using Tiktoken;

namespace OrcaDemo
{
    public class StreamingDemo
    {
        private static readonly List<string> languages = ModelUtils.GetAvailableLanguages();
        private static readonly List<string> genders = ModelUtils.GetAvailableGenders();

        public static void RunDemo(
            string accessKey,
            string language,
            string gender,
            string text,
            string modelPath,
            int tokensPerSecond,
            int? audioWaitChunks,
            int bufferSizeSecs,
            int audioDeviceIndex)
        {
            if (string.IsNullOrEmpty(modelPath))
            {
                modelPath = ModelUtils.GetModelPath(language, gender);
            }

            Orca orca = Orca.Create(accessKey, modelPath);

            PvSpeaker speaker = null;
            try
            {
                speaker = new PvSpeaker(
                    orca.SampleRate,
                    16,
                    bufferSizeSecs,
                    audioDeviceIndex);
                speaker.Start();
            }
            catch (Exception)
            {
                Console.WriteLine("\nWarning: Failed to initialize PvSpeaker. Orca will still generate PCM data, but it will not be played.");
            }

            Func<short[], int> playAudioCallback = (pcm) =>
            {
                try
                {
                    if (speaker != null)
                    {
                        Span<byte> bytes = MemoryMarshal.AsBytes(pcm.AsSpan());
                        return speaker.Write(bytes.ToArray());
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.ToString());
                }
                return pcm.Length;
            };

            Action<short[]> flushAudioCallback = (pcm) =>
            {
                try
                {
                    if (speaker != null)
                    {
                        Span<byte> bytes = MemoryMarshal.AsBytes(pcm.AsSpan());
                        speaker.Flush(bytes.ToArray());
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.ToString());
                }
            };

            OrcaThread orcaThread = new OrcaThread(
                orca,
                playAudioCallback,
                flushAudioCallback,
                tokensPerSecond,
                audioWaitChunks);

            orcaThread.Start();
            try
            {
                Console.WriteLine($"Orca version: {orca.Version}\n");

                IEnumerable<string> tokens = TokenizeText(text, language);

                Console.WriteLine("Simulated text stream:");
                long timeStartTextStream = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
                foreach (string token in tokens)
                {
                    Console.Write(token);
                    orcaThread.Synthesize(token);
                    Thread.Sleep(1000 / tokensPerSecond);
                }
                double textStreamDurationSeconds = (DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() - timeStartTextStream) / (double)1000;

                orcaThread.Flush();
                double firstAudioAvailableSeconds = (orcaThread.FirstAudioAvailableMilliseconds - timeStartTextStream) / (double)1000;
                Console.WriteLine($"\n\nTime to finish text stream:  {textStreamDurationSeconds:F2} seconds");
                Console.WriteLine($"Time to receive first audio: {firstAudioAvailableSeconds:F2} seconds after text stream started\n");

                if (speaker != null)
                {
                    Console.WriteLine("Waiting for audio to finish ...");
                    orcaThread.FlushAudio();
                    speaker.Dispose();
                }
            }
            catch (OperationCanceledException)
            {
                Console.WriteLine("\nStopped...");
                if (speaker != null)
                {
                    speaker.Stop();
                }
            }
            catch (OrcaActivationLimitException)
            {
                Console.WriteLine("\nAccessKey has reached its processing limit");
            }
            finally
            {
                orcaThread.Delete();
            }
        }

        private static readonly string CustomPronPattern = @"\{(.*?\|.*?)\}";
        private static readonly string CustomPronPatternNoWhitespace = @"\{(.*?\|.*?)\}(?!\s)";

        static IEnumerable<string> TokenizeText(string text, string language)
        {
            text = Regex.Replace(text, CustomPronPatternNoWhitespace, @"{\1} ");

            HashSet<string> customPronunciations = new HashSet<string>();
            foreach (Match m in Regex.Matches(text, CustomPronPattern))
            {
                customPronunciations.Add("{" + m.Groups[1].Value + "}");
            };

            List<string> tokensRaw = new List<string>();

            if (language == "ko" || language == "ja")
            {
                tokensRaw = text.Split(' ').ToList();
            }
            else
            {
                Encoder encoder = ModelToEncoder.For("gpt-4");
                IReadOnlyCollection<int> encodedTokens = encoder.Encode(text);

                foreach (int t in encodedTokens)
                {
                    tokensRaw.Add(encoder.Decode(new[] { t }));
                }
            }

            string customPron = string.Empty;
            List<string> tokensWithCustomPronunciations = new List<string>();

            for (int i = 0; i < tokensRaw.Count; i++)
            {
                string token = tokensRaw[i];
                bool inCustomPron = false;

                foreach (string pron in customPronunciations)
                {
                    bool inCustomPronGlobal = customPron.Length > 0;
                    string currentMatch = !inCustomPronGlobal ? token.Trim() : customPron + token;

                    if (pron.StartsWith(currentMatch))
                    {
                        customPron += !inCustomPronGlobal ? token.Trim() : token;
                        inCustomPron = true;
                    }
                }

                if (!inCustomPron)
                {
                    if (customPron.Length > 0)
                    {
                        tokensWithCustomPronunciations.Add(i != 0 ? $" {customPron}" : customPron);
                        customPron = string.Empty;
                    }
                    tokensWithCustomPronunciations.Add(token);
                }
            }

            if (customPron.Length > 0)
            {
                tokensWithCustomPronunciations.Add(customPron);
            }

            return tokensWithCustomPronunciations;
        }

        public static void ShowAudioDevices()
        {
            string[] devices = PvSpeaker.GetAvailableDevices();
            for (int i = 0; i < devices.Length; i++)
            {
                Console.WriteLine($"index: {i}, device name: {devices[i]}");
            }
        }

        public static void Main(string[] args)
        {
            AppDomain.CurrentDomain.UnhandledException += OnUnhandledException;
            if (args.Length == 0)
            {
                Console.WriteLine(HELP_STR);
                return;
            }

            string accessKey = null;
            string language = null;
            string gender = null;
            string text = null;
            string modelPath = null;
            int tokensPerSecond = 15;
            int? audioWaitChunks = null;
            int bufferSizeSecs = 20;
            int audioDeviceIndex = -1;

            int argIndex = 0;
            while (argIndex < args.Length)
            {
                if (args[argIndex] == "--access_key")
                {
                    if (++argIndex < args.Length)
                    {
                        accessKey = args[argIndex++];
                    }
                }
                else if (args[argIndex] == "--language")
                {
                    if (++argIndex < args.Length)
                    {
                        language = args[argIndex++];
                    }
                }
                else if (args[argIndex] == "--gender")
                {
                    if (++argIndex < args.Length)
                    {
                        gender = args[argIndex++];
                    }
                }
                else if (args[argIndex] == "--text_to_stream")
                {
                    if (++argIndex < args.Length)
                    {
                        text = args[argIndex++];
                    }
                }
                else if (args[argIndex] == "--model_path")
                {
                    if (++argIndex < args.Length)
                    {
                        modelPath = args[argIndex++];
                    }
                }
                else if (args[argIndex] == "--tokens_per_second")
                {
                    if (++argIndex < args.Length && int.TryParse(args[argIndex], out int tps))
                    {
                        tokensPerSecond = tps;
                        argIndex++;
                    }
                }
                else if (args[argIndex] == "--audio_wait_chunks")
                {
                    if (++argIndex < args.Length && int.TryParse(args[argIndex], out int awc))
                    {
                        audioWaitChunks = awc;
                        argIndex++;
                    }
                }
                else if (args[argIndex] == "--buffer_size_secs")
                {
                    if (++argIndex < args.Length && int.TryParse(args[argIndex], out int bss))
                    {
                        bufferSizeSecs = bss;
                        argIndex++;
                    }
                }
                else if (args[argIndex] == "--audio_device_index")
                {
                    if (++argIndex < args.Length && int.TryParse(args[argIndex], out int deviceIdx))
                    {
                        audioDeviceIndex = deviceIdx;
                        argIndex++;
                    }
                }
                else if (args[argIndex] == "--show_audio_devices")
                {
                    ShowAudioDevices();
                    return;
                }
                else if (args[argIndex] == "-h" || args[argIndex] == "--help")
                {
                    Console.WriteLine(HELP_STR);
                    return;
                }
                else
                {
                    argIndex++;
                }
            }

            if (string.IsNullOrEmpty(accessKey))
            {
                throw new ArgumentNullException("access_key");
            }
            if (string.IsNullOrEmpty(modelPath))
            {
                if (string.IsNullOrEmpty(language))
                {
                    throw new ArgumentNullException("language");
                }
                if (!languages.Contains(language))
                {
                    throw new ArgumentException($"Given argument '{language}' is not an available language. " +
                                                $"Available languages are '{string.Join(", ", languages)}'.");
                }
                if (string.IsNullOrEmpty(gender))
                {
                    throw new ArgumentNullException("gender");
                }
                if (!genders.Contains(gender))
                {
                    throw new ArgumentException($"Given argument '{gender}' is not an available gender. " +
                                                $"Available genders are '{string.Join(", ", genders)}'.");
                }
            }
            if (string.IsNullOrEmpty(text))
            {
                throw new ArgumentNullException("text_to_stream");
            }

            RunDemo(
                accessKey,
                language,
                gender,
                text,
                modelPath,
                tokensPerSecond,
                audioWaitChunks,
                bufferSizeSecs,
                audioDeviceIndex);
        }

        private static void OnUnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            Console.WriteLine(e.ExceptionObject.ToString());
            Environment.Exit(1);
        }

        private static readonly string HELP_STR = "Available options: \n " +
            "\t--access_key (required): AccessKey obtained from Picovoice Console (https://console.picovoice.ai/)\n" +
            "\t--text_to_stream (required): Text to be streamed to Orca\n" +
            "\t--language: The language you would like to run the demo in. " +
                $"Available languages are {string.Join(", ", languages)}\n" +
            "\t--gender: The gender of the synthesized voice. " +
                $"Available genders are {string.Join(", ", genders)}\n" +
            "\t--model_path: Absolute path to Orca voice model (`.pv`).\n" +
            "\t--tokens_per_second: Number of tokens per second to be streamed to Orca, simulating an LLM response\n" +
            "\t--audio_wait_chunks: Number of PCM chunks to wait before starting to play audio. Default: system-dependent\n" +
            "\t--buffer_size_secs: The size in seconds of the internal buffer used by pvspeaker to play audio\n" +
            "\t--audio_device_index: Index of input audio device.\n" +
            "\t--show_audio_devices: Print available recording devices.\n";
    }
}