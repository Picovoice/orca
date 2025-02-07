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
using System.IO;
using System.Linq;
using System.Reflection;

using Microsoft.VisualStudio.TestTools.UnitTesting;

using Newtonsoft.Json.Linq;

using Pv;

namespace OrcaTest
{
    [TestClass]
    public class MainTest
    {
        private static readonly string ROOT_DIR = Path.Combine(
            AppContext.BaseDirectory,
            "../../../../../..");

        private static string _accessKey;

        private static TestJson _testJson;

        [ClassInitialize]
        public static void ClassInitialize(TestContext _)
        {
            _accessKey = Environment.GetEnvironmentVariable("ACCESS_KEY");
            _testJson = LoadJsonTestData();
        }

        [Serializable]
        private class TestJson
        {
            public TestSentencesJson test_sentences { get; set; }

            public long random_state { get; set; }

            public TestWordAlignmentsJson[] alignments { get; set; }

            public string audio_data_folder { get; set; }

            public string exact_alignment_test_model_identifier { get; set; }
        }

        [Serializable]
        public class TestSentencesJson
        {
            public string text { get; set; }

            public string text_no_punctuation { get; set; }

            public string text_custom_pronunciation { get; set; }

            public string text_alignment { get; set; }

            public string[] text_invalid { get; set; }
        }

        [Serializable]
        public class TestWordAlignmentsJson
        {
            public string word { get; set; }

            public float start_sec { get; set; }

            public float end_sec { get; set; }

            public TestPhonemeAlignmentsJson[] phonemes { get; set; }
        }

        [Serializable]
        public class TestPhonemeAlignmentsJson
        {
            public string phoneme { get; set; }

            public float start_sec { get; set; }

            public float end_sec { get; set; }
        }

        private static IEnumerable<object[]> ModelTestParameters
        {
            get
            {
                string[] modelPaths = GetModelPaths();
                object[][] modelTestParameters = new object[modelPaths.Length][];
                for (int i = 0; i < modelPaths.Length; i++)
                {
                    modelTestParameters[i] = new object[] { modelPaths[i] };
                }
                return modelTestParameters;
            }
        }

        private static void ValidateAudio(short[] synthesizedPcm, short[] testPcm)
        {
            Assert.AreEqual(synthesizedPcm.Length, testPcm.Length);
            for (int i = 0; i < synthesizedPcm.Length; i++)
            {
                Assert.AreEqual(synthesizedPcm[i], testPcm[i], 5000);
            }
        }

        public void ValidatePhonemes(OrcaPhoneme[] phonemes)
        {
            Assert.IsTrue(phonemes.Length >= 0);
            for (int i = 0; i < phonemes.Length; i++)
            {
                Assert.IsTrue(phonemes[i].Phoneme.Length > 0);
                Assert.IsTrue(phonemes[i].StartSec >= 0);
                Assert.IsTrue(phonemes[i].StartSec <= phonemes[i].EndSec);

                if (i < phonemes.Length - 1)
                {
                    Assert.IsTrue(phonemes[i].EndSec <= phonemes[i + 1].StartSec);
                }
            }
        }

        public void ValidateAlignments(OrcaWord[] alignments)
        {
            Assert.IsTrue(alignments.Length > 0);
            for (int i = 0; i < alignments.Length; i++)
            {
                Assert.IsTrue(alignments[i].Word.Length > 0);
                Assert.IsTrue(alignments[i].StartSec >= 0);
                Assert.IsTrue(alignments[i].StartSec <= alignments[i].EndSec);

                if (i < alignments.Length - 1)
                {
                    Assert.IsTrue(alignments[i].EndSec <= alignments[i + 1].StartSec);
                }

                ValidatePhonemes(alignments[i].Phonemes);
            }
        }

        public void ValidateAlignmentsExact(OrcaWord[] alignments, TestWordAlignmentsJson[] expectedAlignments)
        {
            Assert.AreEqual(alignments.Length, expectedAlignments.Length);
            for (int i = 0; i < alignments.Length; i++)
            {
                Assert.AreEqual(alignments[i].Word, expectedAlignments[i].word);
                Assert.AreEqual(alignments[i].StartSec, expectedAlignments[i].start_sec, 0.01);
                Assert.AreEqual(alignments[i].EndSec, expectedAlignments[i].end_sec, 0.01);

                TestPhonemeAlignmentsJson[] expectedPhonemes = expectedAlignments[i].phonemes;
                for (int j = 0; j < alignments[i].Phonemes.Length; j++)
                {
                    Assert.AreEqual(alignments[i].Phonemes[j].Phoneme, expectedPhonemes[j].phoneme);
                    Assert.AreEqual(alignments[i].Phonemes[j].StartSec, expectedPhonemes[j].start_sec, 0.01);
                    Assert.AreEqual(alignments[i].Phonemes[j].EndSec, expectedPhonemes[j].end_sec, 0.01);
                }
            }
        }

        [TestMethod]
        [DynamicData(nameof(ModelTestParameters))]
        public void TestInit(string modelPath)
        {
            using (Orca orca = Orca.Create(_accessKey, modelPath))
            {
                Assert.IsFalse(string.IsNullOrWhiteSpace(orca?.Version), "Orca did not return a valid version string.");
                Assert.IsTrue(orca.SampleRate > 0, "Orca did not return a valid sample rate.");
                Assert.IsTrue(orca.MaxCharacterLimit > 0, "Orca did not return a valid max character limit.");
                Assert.IsTrue(orca.ValidCharacters.Length > 0, "Orca did not return any valid characters.");
                Assert.IsTrue(orca.ValidCharacters.ToList().Contains(","), "Orca valid characters did not include ','.");
            }
        }

        [TestMethod]
        [DynamicData(nameof(ModelTestParameters))]
        public void TestSynthesize(string modelPath)
        {
            using (Orca orca = Orca.Create(_accessKey, modelPath))
            {
                OrcaAudio res = orca.Synthesize(
                    _testJson.test_sentences.text,
                    randomState: _testJson.random_state);
                List<short> expectedPcm = GetTestAudio(modelPath);
                ValidateAudio(res.Pcm, expectedPcm.ToArray());
                ValidateAlignments(res.Words);
            }
        }

        [TestMethod]
        [DynamicData(nameof(ModelTestParameters))]
        public void TestSynthesizeNoPunctuation(string modelPath)
        {
            using (Orca orca = Orca.Create(_accessKey, modelPath))
            {
                OrcaAudio res = orca.Synthesize(
                    _testJson.test_sentences.text_no_punctuation,
                    randomState: _testJson.random_state);
                Assert.IsTrue(res.Pcm.Length > 0);
                ValidateAlignments(res.Words);
            }
        }

        [TestMethod]
        [DynamicData(nameof(ModelTestParameters))]
        public void TestSynthesizeWithCustomPronunciation(string modelPath)
        {
            using (Orca orca = Orca.Create(_accessKey, modelPath))
            {
                OrcaAudio res = orca.Synthesize(
                    _testJson.test_sentences.text_custom_pronunciation,
                    randomState: _testJson.random_state);
                Assert.IsTrue(res.Pcm.Length > 0);
                ValidateAlignments(res.Words);
            }
        }

        [TestMethod]
        [DynamicData(nameof(ModelTestParameters))]
        public void TestSynthesizeToFile(string modelPath)
        {
            using (Orca orca = Orca.Create(_accessKey, modelPath))
            {
                string outputPath = "orca-temp.wav";
                OrcaWord[] res = orca.SynthesizeToFile(
                    _testJson.test_sentences.text,
                    outputPath,
                    randomState: _testJson.random_state);
                List<short> expectedPcm = GetTestAudio(modelPath);
                List<short> synthesizedPcm = GetPcmFromFile(outputPath);
                ValidateAudio(synthesizedPcm.ToArray(), expectedPcm.ToArray());
                ValidateAlignments(res);
            }
        }

        [TestMethod]
        [DynamicData(nameof(ModelTestParameters))]
        public void TestSynthesizeAlignments(string modelPath)
        {
            if (Path.GetFileNameWithoutExtension(modelPath) != _testJson.exact_alignment_test_model_identifier)
            {
                return;
            }

            using (Orca orca = Orca.Create(_accessKey, modelPath))
            {
                OrcaAudio res = orca.Synthesize(
                    _testJson.test_sentences.text_alignment,
                    randomState: _testJson.random_state);
                Assert.IsTrue(res.Pcm.Length > 0);
                ValidateAlignmentsExact(res.Words, _testJson.alignments);
            }
        }

        [TestMethod]
        [DynamicData(nameof(ModelTestParameters))]
        public void TestStreamingSynthesize(string modelPath)
        {
            List<short> fullPcm = new List<short>();
            using (Orca orca = Orca.Create(_accessKey, modelPath))
            {
                using (Orca.OrcaStream orcaStream = orca.StreamOpen(randomState: _testJson.random_state))
                {
                    foreach (char c in _testJson.test_sentences.text)
                    {
                        short[] streamPcm = orcaStream.Synthesize(c.ToString());
                        if (streamPcm != null)
                        {
                            fullPcm.AddRange(streamPcm);
                        }
                    }
                    short[] flushPcm = orcaStream.Flush();
                    if (flushPcm != null)
                    {
                        fullPcm.AddRange(flushPcm);
                    }
                }
            }
            Assert.IsTrue(fullPcm.Count > 0);
            List<short> expectedPcm = GetTestAudio(modelPath, "stream");
            ValidateAudio(fullPcm.ToArray(), expectedPcm.ToArray());
        }

        [TestMethod]
        [DynamicData(nameof(ModelTestParameters))]
        public void TestSpeechRate(string modelPath)
        {
            using (Orca orca = Orca.Create(_accessKey, modelPath))
            {
                OrcaAudio resSlow = orca.Synthesize(
                    _testJson.test_sentences.text,
                    speechRate: 0.7f,
                    randomState: _testJson.random_state);
                OrcaAudio resFast = orca.Synthesize(
                    _testJson.test_sentences.text,
                    speechRate: 1.3f,
                    randomState: _testJson.random_state);
                Assert.IsTrue(resSlow.Pcm.Length > 0);
                Assert.IsTrue(resFast.Pcm.Length > 0);
                Assert.IsTrue(resSlow.Pcm.Length > resFast.Pcm.Length);
            }
        }

        [TestMethod]
        [DynamicData(nameof(ModelTestParameters))]
        public void TestInvalidInput(string modelPath)
        {
            using (Orca orca = Orca.Create(_accessKey, modelPath))
            {
                foreach (string invalidText in _testJson.test_sentences.text_invalid)
                {
                    Assert.ThrowsException<OrcaInvalidArgumentException>(() => orca.Synthesize(
                        invalidText,
                        randomState: _testJson.random_state));
                }
            }
        }

        [TestMethod]
        public void TestMessageStack()
        {
            Orca o;
            string[] messageList = { };

            try
            {
                o = Orca.Create("invalid");
                Assert.IsNull(o);
                o.Dispose();
            }
            catch (OrcaException e)
            {
                messageList = e.MessageStack;
            }

            Assert.IsTrue(0 < messageList.Length);
            Assert.IsTrue(messageList.Length < 8);

            try
            {
                o = Orca.Create("invalid");
                Assert.IsNull(o);
                o.Dispose();
            }
            catch (OrcaException e)
            {
                for (int i = 0; i < messageList.Length; i++)
                {
                    Assert.AreEqual(messageList[i], e.MessageStack[i]);
                }
            }
        }

        [TestMethod]
        public void TestSynthesizeMessageStack()
        {
            using (Orca o = Orca.Create(_accessKey))
            {
                var obj = typeof(Orca).GetField("_libraryPointer", BindingFlags.NonPublic | BindingFlags.Instance);
                IntPtr address = (IntPtr)obj.GetValue(o);
                obj.SetValue(o, IntPtr.Zero);

                try
                {
                    OrcaAudio res = o.Synthesize(
                        _testJson.test_sentences.text,
                        randomState: _testJson.random_state);
                    Assert.IsTrue(res.Pcm.Length == -1);
                }
                catch (OrcaException e)
                {
                    Assert.IsTrue(0 < e.MessageStack.Length);
                    Assert.IsTrue(e.MessageStack.Length < 8);
                }

                try
                {
                    OrcaAudio res = o.Synthesize(
                        _testJson.test_sentences.text,
                        randomState: _testJson.random_state);
                    Assert.IsTrue(res.Pcm.Length == -1);
                }
                catch (OrcaException e)
                {
                    Assert.IsTrue(0 < e.MessageStack.Length);
                    Assert.IsTrue(e.MessageStack.Length < 8);
                }

                obj.SetValue(o, address);
            }
        }

        private static List<short> GetPcmFromFile(string audioFilePath)
        {
            List<short> data = new List<short>();
            using (BinaryReader reader = new BinaryReader(File.Open(audioFilePath, FileMode.Open)))
            {
                reader.ReadBytes(44);

                while (reader.BaseStream.Position != reader.BaseStream.Length)
                {
                    data.Add(reader.ReadInt16());
                }
            }

            return data;
        }

        private static string[] GetModelPaths()
        {
            return Directory.GetFiles(
                Path.Combine(ROOT_DIR, "lib/common"),
                "*.pv");
        }

        private static List<short> GetTestAudio(string modelPath, string synthesisType = "single")
        {
            string modelName = Path.GetFileName(modelPath);
            string testAudioPath = Path.Combine(
                ROOT_DIR,
                _testJson.audio_data_folder,
                modelName.Replace(".pv", $"_{synthesisType}.wav"));
            return GetPcmFromFile(testAudioPath);
        }

        private static TestJson LoadJsonTestData()
        {
            string content = File.ReadAllText(Path.Combine(ROOT_DIR, "resources/.test/test_data.json"));
            return JObject.Parse(content).ToObject<TestJson>();
        }
    }
}