using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;

using Pv;

namespace OrcaDemo
{
    public class OrcaThread
    {
        public class OrcaInput
        {
            public string Text { get; set; }
            public bool Flush { get; set; }
        }

        private readonly Orca _orca;
        private readonly Orca.OrcaStream _orcaStream;
        private readonly Func<short[], int> _playAudioCallback;
        private readonly Action<short[]> _flushAudioCallback;
        private readonly int _numTokensPerSecond;
        private readonly ConcurrentQueue<OrcaInput> _queue = new ConcurrentQueue<OrcaInput>();
        private readonly Queue<short[]> _pcmBuffer = new Queue<short[]>();
        private readonly int _waitChunks;
        private Thread _thread;
        private int _numPcmChunksProcessed = 0;

        public long FirstAudioAvailableMilliseconds { get; private set; }

        public OrcaThread(
            Orca orca,
            Func<short[], int> playAudioCallback,
            Action<short[]> flushAudioCallback,
            int numTokensPerSecond,
            int? audioWaitChunks = null)
        {
            _orca = orca;
            _orcaStream = _orca.StreamOpen();
            _playAudioCallback = playAudioCallback;
            _flushAudioCallback = flushAudioCallback;
            _numTokensPerSecond = numTokensPerSecond;

            _waitChunks = audioWaitChunks ?? GetFirstAudioWaitChunks();
        }

        private static int GetFirstAudioWaitChunks()
        {
            int waitChunks = 0;
            if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            {
                string machine = LinuxMachine();
                if (machine.Contains("cortex"))
                {
                    waitChunks = 1;
                }
            }
            return waitChunks;
        }

        private static string LinuxMachine()
        {
            string archInfo = "";
            if (RuntimeInformation.ProcessArchitecture == Architecture.X64)
            {
                return "x86_64";
            }

            else if (RuntimeInformation.ProcessArchitecture == Architecture.Arm64)
            {
                archInfo = "-aarch64";
            }

            string cpuInfo = File.ReadAllText("/proc/cpuinfo");
            string[] cpuPartList = cpuInfo.Split('\n').Where(x => x.Contains("CPU part")).ToArray();
            if (cpuPartList.Length == 0)
            {
                throw new PlatformNotSupportedException($"Unsupported CPU.\n{cpuInfo}");
            }

            string cpuPart = cpuPartList[0].Split(' ').Last().ToLower();
            switch (cpuPart)
            {
                case "0xd03": return "cortex-a53" + archInfo;
                case "0xd08": return "cortex-a72" + archInfo;
                case "0xd0b": return "cortex-a76" + archInfo;
                default:
                    throw new PlatformNotSupportedException($"This device (CPU part = {cpuPart}) is not supported by Picovoice.");
            }
        }

        private void Run()
        {
            while (true)
            {
                if (_queue.TryDequeue(out OrcaInput orcaInput))
                {
                    if (orcaInput == null) break;

                    try
                    {
                        short[] pcm;
                        if (!orcaInput.Flush)
                        {
                            pcm = _orcaStream.Synthesize(orcaInput.Text);
                        }
                        else
                        {
                            pcm = _orcaStream.Flush();
                        }

                        if (pcm != null)
                        {
                            if (_numPcmChunksProcessed == 0)
                            {
                                FirstAudioAvailableMilliseconds = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
                            }
                            _numPcmChunksProcessed++;
                            _pcmBuffer.Enqueue(pcm);
                        }

                        if (_numPcmChunksProcessed > _waitChunks)
                        {
                            if (_pcmBuffer.Count > 0)
                            {
                                short[] currentPcm = _pcmBuffer.Dequeue();
                                int written = _playAudioCallback(currentPcm);
                                if (written < currentPcm.Length)
                                {
                                    _pcmBuffer.Enqueue(currentPcm.Skip(written).ToArray());
                                }
                            }
                        }
                    }
                    catch (OrcaInvalidArgumentException ex)
                    {
                        throw new ArgumentException($"Orca could not synthesize text input `{orcaInput.Text}`: `{ex.Message}`");
                    }
                }
            }
        }

        private void CloseThread()
        {
            _queue.Enqueue(null);
            _thread.Join();
        }

        public void Start()
        {
            _thread = new Thread(Run);
            _thread.Start();
        }

        public void Synthesize(string text)
        {
            _queue.Enqueue(new OrcaInput { Text = text, Flush = false });
        }

        public void Flush()
        {
            _queue.Enqueue(new OrcaInput { Text = "", Flush = true });
            CloseThread();
        }

        public void FlushAudio()
        {
            List<short> remainingPcm = new List<short>();
            while (_pcmBuffer.Count > 0)
            {
                remainingPcm.AddRange(_pcmBuffer.Dequeue());
            }
            Thread flushThread = new Thread(() => _flushAudioCallback(remainingPcm.ToArray()));
            flushThread.Start();
            flushThread.Join();
        }

        public void Delete()
        {
            CloseThread();
            _orcaStream.Dispose();
            _orca.Dispose();
        }
    }
}