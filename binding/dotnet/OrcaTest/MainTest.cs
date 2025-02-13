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

        [ClassInitialize]
        public static void ClassInitialize(TestContext _)
        {
            _accessKey = Environment.GetEnvironmentVariable("ACCESS_KEY");
        }

        [Serializable]
        private class TestDataJson
        {
            public TestsJson tests { get; set; }

            public string audio_data_folder { get; set; }
        }

        [Serializable]
        private class TestsJson
        {
            public SentenceTestsJson[] sentence_tests { get; set; }

            public AlignmentTestsJson[] alignment_tests { get; set; }

            public InvalidTestsJson[] invalid_tests { get; set; }
        }

        [Serializable]
        public class SentenceTestsJson
        {
            public string language { get; set; }

            public string[] models { get; set; }

            public long random_state { get; set; }

            public string text { get; set; }

            public string text_no_punctuation { get; set; }

            public string text_custom_pronunciation { get; set; }
        }

        [Serializable]
        public class AlignmentTestsJson
        {
            public string language { get; set; }

            public string model { get; set; }

            public long random_state { get; set; }

            public string text_alignment { get; set; }

            public TestWordAlignmentsJson[] alignments { get; set; }
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

        [Serializable]
        public class InvalidTestsJson
        {
            public string language { get; set; }

            public string[] models { get; set; }

            public string[] text_invalid { get; set; }
        }

        private static IEnumerable<object[]> SentenceTestParameters
        {
            get
            {
                TestDataJson testData = LoadJsonTestData();
                return testData.tests.sentence_tests
                    .SelectMany(x => x.models.Select(model => new object[] {
                        x.language,
                        model,
                        x.random_state,
                        x.text,
                        x.text_no_punctuation,
                        x.text_custom_pronunciation,
                    }));
            }
        }

        private static IEnumerable<object[]> AlignmentTestParameters
        {
            get
            {
                TestDataJson testData = LoadJsonTestData();
                return testData.tests.alignment_tests
                    .Select(x => new object[] {
                        x.language,
                        x.model,
                        x.random_state,
                        x.text_alignment,
                        x.alignments,
                    });
            }
        }

        private static IEnumerable<object[]> InvalidTestParameters
        {
            get
            {
                TestDataJson testData = LoadJsonTestData();
                return testData.tests.invalid_tests
                    .SelectMany(x => x.models.Select(model => new object[] {
                        x.language,
                        model,
                        x.text_invalid,
                    }));
            }
        }

        private static void ValidateAudio(short[] synthesizedPcm, short[] testPcm)
        {
            Assert.AreEqual(synthesizedPcm.Length, testPcm.Length);
            for (int i = 0; i < synthesizedPcm.Length; i++)
            {
                Assert.AreEqual(synthesizedPcm[i], testPcm[i], 8000);
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
        [DynamicData(nameof(SentenceTestParameters))]
        public void TestInit(
            string language,
            string model,
            long random_state,
            string text,
            string text_no_punctuation,
            string text_custom_pronunciation)
        {
            using (Orca orca = Orca.Create(_accessKey, GetModelPath(model)))
            {
                Assert.IsFalse(string.IsNullOrWhiteSpace(orca?.Version), "Orca did not return a valid version string.");
                Assert.IsTrue(orca.SampleRate > 0, "Orca did not return a valid sample rate.");
                Assert.IsTrue(orca.MaxCharacterLimit > 0, "Orca did not return a valid max character limit.");
                Assert.IsTrue(orca.ValidCharacters.Length > 0, "Orca did not return any valid characters.");
                Assert.IsTrue(orca.ValidCharacters.ToList().Contains(","), "Orca valid characters did not include ','.");
            }
        }

        [TestMethod]
        [DynamicData(nameof(SentenceTestParameters))]
        public void TestSynthesize(
            string language,
            string model,
            long random_state,
            string text,
            string text_no_punctuation,
            string text_custom_pronunciation)
        {
            using (Orca orca = Orca.Create(_accessKey, GetModelPath(model)))
            {
                OrcaAudio res = orca.Synthesize(
                    text,
                    randomState: random_state);
                List<short> expectedPcm = GetTestAudio(model);
                ValidateAudio(res.Pcm, expectedPcm.ToArray());
                ValidateAlignments(res.Words);
            }
        }

        [TestMethod]
        [DynamicData(nameof(SentenceTestParameters))]
        public void TestSynthesizeNoPunctuation(
            string language,
            string model,
            long random_state,
            string text,
            string text_no_punctuation,
            string text_custom_pronunciation)
        {
            using (Orca orca = Orca.Create(_accessKey, GetModelPath(model)))
            {
                OrcaAudio res = orca.Synthesize(
                    text_no_punctuation,
                    randomState: random_state);
                Assert.IsTrue(res.Pcm.Length > 0);
                ValidateAlignments(res.Words);
            }
        }

        [TestMethod]
        [DynamicData(nameof(SentenceTestParameters))]
        public void TestSynthesizeWithCustomPronunciation(
            string language,
            string model,
            long random_state,
            string text,
            string text_no_punctuation,
            string text_custom_pronunciation)
        {
            using (Orca orca = Orca.Create(_accessKey, GetModelPath(model)))
            {
                OrcaAudio res = orca.Synthesize(
                    text_custom_pronunciation,
                    randomState: random_state);
                Assert.IsTrue(res.Pcm.Length > 0);
                ValidateAlignments(res.Words);
            }
        }

        [TestMethod]
        [DynamicData(nameof(SentenceTestParameters))]
        public void TestSynthesizeToFile(
            string language,
            string model,
            long random_state,
            string text,
            string text_no_punctuation,
            string text_custom_pronunciation)
        {
            using (Orca orca = Orca.Create(_accessKey, GetModelPath(model)))
            {
                string outputPath = "orca-temp.wav";
                OrcaWord[] res = orca.SynthesizeToFile(
                    text,
                    outputPath,
                    randomState: random_state);
                List<short> expectedPcm = GetTestAudio(model);
                List<short> synthesizedPcm = GetPcmFromFile(outputPath);
                ValidateAudio(synthesizedPcm.ToArray(), expectedPcm.ToArray());
                ValidateAlignments(res);
            }
        }

        [TestMethod]
        [DynamicData(nameof(AlignmentTestParameters))]
        public void TestSynthesizeAlignments(
            string language,
            string model,
            long random_state,
            string text_alignment,
            TestWordAlignmentsJson[] alignments)
        {
            using (Orca orca = Orca.Create(_accessKey, GetModelPath(model)))
            {
                OrcaAudio res = orca.Synthesize(
                    text_alignment,
                    randomState: random_state);
                Assert.IsTrue(res.Pcm.Length > 0);
                ValidateAlignmentsExact(res.Words, alignments);
            }
        }

        [TestMethod]
        [DynamicData(nameof(SentenceTestParameters))]
        public void TestStreamingSynthesize(
            string language,
            string model,
            long random_state,
            string text,
            string text_no_punctuation,
            string text_custom_pronunciation)
        {
            List<short> fullPcm = new List<short>();
            using (Orca orca = Orca.Create(_accessKey, GetModelPath(model)))
            {
                using (Orca.OrcaStream orcaStream = orca.StreamOpen(randomState: random_state))
                {
                    foreach (char c in text)
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
            List<short> expectedPcm = GetTestAudio(model, "stream");
            ValidateAudio(fullPcm.ToArray(), expectedPcm.ToArray());
        }

        [TestMethod]
        [DynamicData(nameof(SentenceTestParameters))]
        public void TestSpeechRate(
            string language,
            string model,
            long random_state,
            string text,
            string text_no_punctuation,
            string text_custom_pronunciation)
        {
            using (Orca orca = Orca.Create(_accessKey, GetModelPath(model)))
            {
                OrcaAudio resSlow = orca.Synthesize(
                    text,
                    speechRate: 0.7f,
                    randomState: random_state);
                OrcaAudio resFast = orca.Synthesize(
                    text,
                    speechRate: 1.3f,
                    randomState: random_state);
                Assert.IsTrue(resSlow.Pcm.Length > 0);
                Assert.IsTrue(resFast.Pcm.Length > 0);
                Assert.IsTrue(resSlow.Pcm.Length > resFast.Pcm.Length);
            }
        }

        [TestMethod]
        [DynamicData(nameof(InvalidTestParameters))]
        public void TestInvalidInput(
            string language,
            string model,
            string[] text_invalid)
        {
            using (Orca orca = Orca.Create(_accessKey, GetModelPath(model)))
            {
                foreach (string invalidText in text_invalid)
                {
                    Assert.ThrowsException<OrcaInvalidArgumentException>(() => orca.Synthesize(
                        invalidText));
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
                        "test text",
                        randomState: 42);
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
                        "test text",
                        randomState: 42);
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

        private static string GetModelPath(string model)
        {
            return Path.Combine(ROOT_DIR, "lib/common", model);
        }

        private static List<short> GetTestAudio(string model, string synthesisType = "single")
        {
            TestDataJson testData = LoadJsonTestData();
            string testAudioPath = Path.Combine(
                ROOT_DIR,
                testData.audio_data_folder,
                model.Replace(".pv", $"_{synthesisType}.wav"));
            return GetPcmFromFile(testAudioPath);
        }

        private static TestDataJson LoadJsonTestData()
        {
            string content = File.ReadAllText(Path.Combine(ROOT_DIR, "resources/.test/test_data.json"));
            return JObject.Parse(content).ToObject<TestDataJson>();
        }
    }
}
