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
using System.IO;
using System.Linq;
using System.Text;

using Pv;

using Spectre.Console;

namespace OrcaDemo
{
    public class FileDemo
    {
        private static readonly List<string> languages = ModelUtils.GetAvailableLanguages();
        private static readonly List<string> genders = ModelUtils.GetAvailableGenders();

        public static void RunDemo(
            string accessKey,
            string language,
            string gender,
            string outputPath,
            string text,
            bool verbose)
        {
            string modelPath = ModelUtils.GetModelPath(language, gender);

            using (Orca orca = Orca.Create(accessKey, modelPath))
            {
                Console.WriteLine($"Orca version: {orca.Version}\n");

                long startTime = Stopwatch.GetTimestamp();
                OrcaWord[] alignments = orca.SynthesizeToFile(text, outputPath);
                long endTime = Stopwatch.GetTimestamp();
                double processingTime = (endTime - startTime) / (double)Stopwatch.Frequency;
                double lengthSec = GetPcmLengthSec(outputPath);

                Console.WriteLine($"Orca took {processingTime:F2} seconds to synthesize {lengthSec:F2} seconds of " +
                    $"speech which is ~{(lengthSec / processingTime):F2} times faster than real-time.");
                Console.WriteLine($"Audio written to {outputPath}");

                if (verbose)
                {
                    List<string[]> rows = alignments
                        ?.SelectMany(alignment => alignment.Phonemes.Select((phoneme, i) =>
                        {
                            string[] row = new string[6];
                            if (i == 0)
                            {
                                row[0] = alignment.Word;
                                row[1] = alignment.StartSec.ToString("F2");
                                row[2] = alignment.EndSec.ToString("F2");
                            }
                            else
                            {
                                row[0] = "";
                                row[1] = "";
                                row[2] = "";
                            }
                            row[3] = phoneme.Phoneme;
                            row[4] = phoneme.StartSec.ToString("F2");
                            row[5] = phoneme.EndSec.ToString("F2");
                            return row;
                        }))
                        .ToList();

                    if (rows != null)
                    {
                        Table table = new Table();
                        table.AddColumn("Word");
                        table.AddColumn("Word Start Time (s)");
                        table.AddColumn("Word End Time (s)");
                        table.AddColumn("Phoneme");
                        table.AddColumn("Phoneme Start Time (s)");
                        table.AddColumn("Phoneme End time (s)");
                        foreach (string[] row in rows)
                        {
                            table.AddRow(row);
                        }

                        AnsiConsole.Write(table);
                    }
                }
            }
        }

        private static double GetPcmLengthSec(string audioFilePath)
        {
            using (BinaryReader reader = new BinaryReader(File.Open(audioFilePath, FileMode.Open)))
            {
                reader.ReadBytes(24);
                int sampleRate = reader.ReadInt32();
                reader.ReadBytes(6);
                ushort bitDepth = reader.ReadUInt16();
                reader.ReadBytes(4);
                int dataSize = reader.ReadInt32();
                return dataSize / (double)(sampleRate * bitDepth / 8);
            }
        }
        private static void WriteWavHeader(
            BinaryWriter writer,
            ushort channelCount,
            ushort bitDepth,
            int sampleRate,
            int totalSampleCount)
        {
            if (writer == null)
            {
                return;
            }

            _ = writer.Seek(0, SeekOrigin.Begin);
            writer.Write(Encoding.ASCII.GetBytes("RIFF"));
            writer.Write((bitDepth / 8 * totalSampleCount) + 36);
            writer.Write(Encoding.ASCII.GetBytes("WAVE"));
            writer.Write(Encoding.ASCII.GetBytes("fmt "));
            writer.Write(16);
            writer.Write((ushort)1);
            writer.Write(channelCount);
            writer.Write(sampleRate);
            writer.Write(sampleRate * channelCount * bitDepth / 8);
            writer.Write((ushort)(channelCount * bitDepth / 8));
            writer.Write(bitDepth);
            writer.Write(Encoding.ASCII.GetBytes("data"));
            writer.Write(bitDepth / 8 * totalSampleCount);
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
            string outputPath = null;
            bool verbose = false;

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
                else if (args[argIndex] == "--text")
                {
                    if (++argIndex < args.Length)
                    {
                        text = args[argIndex++];
                    }
                }
                else if (args[argIndex] == "--output_path")
                {
                    if (++argIndex < args.Length)
                    {
                        outputPath = args[argIndex++];
                    }
                }
                else if (args[argIndex] == "--verbose")
                {
                    verbose = true;
                    argIndex++;
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
            if (string.IsNullOrEmpty(outputPath))
            {
                throw new ArgumentNullException("output_path");
            }
            if (!outputPath.EndsWith(".wav"))
            {
                throw new ArgumentException($"Output path must have WAV file extension (.wav)");
            }
            if (string.IsNullOrEmpty(text))
            {
                throw new ArgumentNullException("text");
            }

            RunDemo(
                accessKey,
                language,
                gender,
                outputPath,
                text,
                verbose);
        }

        private static void OnUnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            Console.WriteLine(e.ExceptionObject.ToString());
            Environment.Exit(1);
        }

        private static readonly string HELP_STR = "Available options: \n " +
            $"\t--access_key (required): AccessKey obtained from Picovoice Console (https://console.picovoice.ai/)\n" +
            $"\t--language (required): The language you would like to run the demo in. " +
                $"Available languages are {string.Join(", ", languages)}\n" +
            $"\t--gender (required): The gender of the synthesized voice. " +
                $"Available genders are {string.Join(", ", genders)}\n" +
            $"\t--text (required): Text to be synthesized\n" +
            $"\t--output_path (required): Absolute path to .wav file where the generated audio will be stored\n";
    }
}