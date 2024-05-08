/*
    Copyright 2024 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is
    located in the "LICENSE" file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the
    License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
    express or implied. See the License for the specific language governing permissions and
    limitations under the License.
*/

package ai.picovoice.orca.testapp;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.experimental.runners.Enclosed;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.io.File;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.Objects;

import ai.picovoice.orca.Orca;
import ai.picovoice.orca.OrcaAudio;
import ai.picovoice.orca.OrcaException;
import ai.picovoice.orca.OrcaInvalidArgumentException;
import ai.picovoice.orca.OrcaSynthesizeParams;
import ai.picovoice.orca.OrcaWord;
import ai.picovoice.orca.OrcaPhoneme;

@RunWith(Enclosed.class)
public class OrcaTest {

    public static class StandardTests extends BaseTest {

        String[] modelFiles;

        @Before
        public void Setup() throws Exception {
            super.Setup();
            modelFiles = getModelFiles();
        }

        @Test
        public void testErrorStack() {
            String[] error = {};
            try {
                new Orca.Builder()
                        .setAccessKey("invalid")
                        .setModelPath(modelFiles[0])
                        .build(appContext);
            } catch (OrcaException e) {
                error = e.getMessageStack();
            }

            assertTrue(0 < error.length);
            assertTrue(error.length <= 8);

            try {
                new Orca.Builder()
                        .setAccessKey("invalid")
                        .setModelPath(modelFiles[0])
                        .build(appContext);
            } catch (OrcaException e) {
                for (int i = 0; i < error.length; i++) {
                    assertEquals(e.getMessageStack()[i], error[i]);
                }
            }
        }
    }

    @RunWith(Parameterized.class)
    public static class ModelTests extends BaseTest {

        @Parameterized.Parameter(value = 0)
        public String modelFile;

        @Parameterized.Parameters(name = "{0}")
        public static Collection<Object[]> initParameters() {
            List<Object[]> parameters = new ArrayList<>();
            for (String modelFile : getModelFiles()) {
                parameters.add(new Object[]{modelFile});
            }
            return parameters;
        }

        String text;
        String textNoPunctuation;
        String textCustomPronunciation;
        String textAlignment;
        static JsonArray textInvalid;

        long randomState;
        static JsonArray alignments;

        String modelFileUsed;
        String EXACT_ALIGNMENT_TEST_MODEL_IDENTIFIER = "female";

        Orca orca;

        @Before
        public void Setup() throws Exception {
            super.Setup();

            final JsonObject testSentences = testJson.getAsJsonObject("test_sentences");
            text = testSentences.get("text").getAsString();
            textNoPunctuation = testSentences.get("text_no_punctuation").getAsString();
            textCustomPronunciation = testSentences.get("text_custom_pronunciation").getAsString();
            textAlignment = testSentences.get("text_alignment").getAsString();
            textInvalid = testSentences.get("text_invalid").getAsJsonArray();

            randomState = testJson.get("random_state").getAsLong();
            alignments = testJson.getAsJsonArray("alignments");

            modelFileUsed = modelFile.contains("female") ? "female" : "male";

            orca = new Orca.Builder()
                    .setAccessKey(accessKey)
                    .setModelPath(modelFile)
                    .build(appContext);
        }

        @After
        public void TearDown() {
            if (orca != null) {
                orca.delete();
            }
        }

        @Test
        public void testVersion() {
            final String version = orca.getVersion();
            assertNotNull(version);
            assertNotEquals(version, "");
        }

        @Test
        public void testSampleRate() {
            assertTrue(orca.getSampleRate() > 0);
        }

        @Test
        public void testMaxCharacterLimit() {
            assertTrue(orca.getMaxCharacterLimit() > 0);
        }

        @Test
        public void testValidCharacters() {
            String[] characters = orca.getValidCharacters();
            assertTrue(characters.length > 0);
            assertTrue(Arrays.asList(characters).contains(","));
        }

        @Test
        public void testStreaming() throws Exception {
            Orca.OrcaStream orcaStream = orca.streamOpen(
                    new OrcaSynthesizeParams.Builder()
                            .setRandomState(randomState)
                            .build());

            short[] fullPcm = new short[0];
            for (char c : text.toCharArray()) {
                short[] pcm = orcaStream.synthesize(String.valueOf(c));
                if (pcm != null && pcm.length > 0) {
                    fullPcm = concatArrays(fullPcm, pcm);
                }
            }

            short[] flushedPcm = orcaStream.flush();
            if (flushedPcm != null && flushedPcm.length > 0) {
                fullPcm = concatArrays(fullPcm, flushedPcm);
            }

            orcaStream.close();
            short[] testFilePcm = readAudioFile(String.format(
                    "%s/wav/orca_params_%s_stream.wav", testResourcesPath, modelFileUsed));

            compareArrays(fullPcm, testFilePcm, 1);
        }

        @Test
        public void testSynthesize() throws Exception {
            final OrcaAudio pcm = orca.synthesize(
                    text,
                    new OrcaSynthesizeParams.Builder()
                            .setRandomState(randomState)
                            .build());

            short[] testFilePcm = readAudioFile(String.format(
                    "%s/wav/orca_params_%s_single.wav", testResourcesPath, modelFileUsed));

            compareArrays(pcm.getPcm(), testFilePcm, 1);
        }

        @Test
        public void testSynthesizeToFile() throws Exception {
            final File outputFile = new File(
                    appContext.getFilesDir(),
                    "text.wav");
            orca.synthesizeToFile(
                    text,
                    outputFile.getAbsolutePath(),
                    new OrcaSynthesizeParams.Builder()
                            .setRandomState(randomState)
                            .build());

            short[] outputFilePcm = readAudioFile(outputFile.getAbsolutePath());
            short[] testFilePcm = readAudioFile(String.format(
                    "%s/wav/orca_params_%s_single.wav", testResourcesPath, modelFileUsed));

            compareArrays(outputFilePcm, testFilePcm, 1);
            outputFile.delete();
        }

        @Test
        public void testSynthesizeNoPronunciation() throws OrcaException {
            final OrcaAudio result = orca.synthesize(
                    textNoPunctuation,
                    new OrcaSynthesizeParams.Builder()
                            .setRandomState(randomState)
                            .build());
            assertTrue(result.getPcm().length > 0);
        }

        @Test
        public void testSynthesizeCustomPronunciation() throws OrcaException {
            final OrcaAudio result = orca.synthesize(
                    textCustomPronunciation,
                    new OrcaSynthesizeParams.Builder()
                            .setRandomState(randomState)
                            .build());
            assertTrue(result.getPcm().length > 0);
        }

        @Test
        public void testSynthesizeAlignment() throws OrcaException {
            final OrcaAudio result = orca.synthesize(
                    textAlignment,
                    new OrcaSynthesizeParams.Builder()
                            .setRandomState(randomState)
                            .build());
            final OrcaWord[] synthesizeTestData = new OrcaWord[alignments.size()];
            for (int i = 0; i < alignments.size(); i++) {
                final JsonObject testData = alignments.get(i).getAsJsonObject();
                final String word = testData.get("word").getAsString();
                final float startSec = testData.get("start_sec").getAsFloat();
                final float endSec = testData.get("end_sec").getAsFloat();
                final JsonArray phonemesJson = testData.getAsJsonArray("phonemes");
                final OrcaPhoneme[] phonemes = new OrcaPhoneme[phonemesJson.size()];
                for (int j = 0; j < phonemesJson.size(); j++) {
                    final JsonObject phonemeJson = phonemesJson.get(j).getAsJsonObject();
                    phonemes[j] = new OrcaPhoneme(
                            phonemeJson.get("phoneme").getAsString(),
                            phonemeJson.get("start_sec").getAsFloat(),
                            phonemeJson.get("end_sec").getAsFloat());
                }
                synthesizeTestData[i] = new OrcaWord(
                        word,
                        startSec,
                        endSec,
                        phonemes);
            }
            validateMetadata(
                    result.getWordArray(),
                    synthesizeTestData,
                    Objects.equals(modelFileUsed, EXACT_ALIGNMENT_TEST_MODEL_IDENTIFIER));
        }

        @Test
        public void testSynthesizeToFileAlignment() throws OrcaException {
            final File outputFile = new File(
                    appContext.getFilesDir(),
                    "text.wav");
            OrcaWord[] result = orca.synthesizeToFile(
                    textAlignment,
                    outputFile.getAbsolutePath(),
                    new OrcaSynthesizeParams.Builder()
                            .setRandomState(randomState)
                            .build());
            outputFile.delete();

            final OrcaWord[] synthesizeTestData = new OrcaWord[alignments.size()];
            for (int i = 0; i < alignments.size(); i++) {
                final JsonObject testData = alignments.get(i).getAsJsonObject();
                final String word = testData.get("word").getAsString();
                final float startSec = testData.get("start_sec").getAsFloat();
                final float endSec = testData.get("end_sec").getAsFloat();
                final JsonArray phonemesJson = testData.getAsJsonArray("phonemes");
                final OrcaPhoneme[] phonemes = new OrcaPhoneme[phonemesJson.size()];
                for (int j = 0; j < phonemesJson.size(); j++) {
                    final JsonObject phonemeJson = phonemesJson.get(j).getAsJsonObject();
                    phonemes[j] = new OrcaPhoneme(
                            phonemeJson.get("phoneme").getAsString(),
                            phonemeJson.get("start_sec").getAsFloat(),
                            phonemeJson.get("end_sec").getAsFloat());
                }
                synthesizeTestData[i] = new OrcaWord(
                        word,
                        startSec,
                        endSec,
                        phonemes);
            }
            validateMetadata(
                    result,
                    synthesizeTestData,
                    Objects.equals(modelFileUsed, EXACT_ALIGNMENT_TEST_MODEL_IDENTIFIER));
        }

        @Test
        public void testSynthesizeSpeechRate() throws OrcaException {
            final OrcaAudio slow = orca.synthesize(
                    textCustomPronunciation,
                    new OrcaSynthesizeParams.Builder()
                            .setSpeechRate(0.7f)
                            .setRandomState(randomState)
                            .build());
            assertTrue(slow.getPcm().length > 0);

            final OrcaAudio fast = orca.synthesize(
                    textCustomPronunciation,
                    new OrcaSynthesizeParams.Builder()
                            .setSpeechRate(1.3f)
                            .setRandomState(randomState)
                            .build());
            assertTrue(slow.getPcm().length > 0);
            assertTrue(fast.getPcm().length < slow.getPcm().length);

            try {
                orca.synthesize(
                        textCustomPronunciation,
                        new OrcaSynthesizeParams.Builder()
                                .setSpeechRate(9999f)
                                .setRandomState(randomState)
                                .build());
                fail();
            } catch (OrcaInvalidArgumentException e) {
                assertNotNull(e);
            }
        }

        @Test
        public void testSynthesizeRandomState() throws OrcaException {
            final OrcaAudio randomState1 = orca.synthesize(
                    text,
                    new OrcaSynthesizeParams.Builder()
                            .setRandomState(1)
                            .build());
            assertTrue(randomState1.getPcm().length > 0);
            assertTrue(randomState1.getWordArray().length > 0);

            final OrcaAudio randomState2 = orca.synthesize(
                    text,
                    new OrcaSynthesizeParams.Builder()
                            .setRandomState(2)
                            .build());
            assertTrue(randomState2.getPcm().length > 0);
            assertTrue(randomState2.getWordArray().length > 0);

            assertNotEquals(randomState1, randomState2);
            assertNotEquals(randomState1.getWordArray(), randomState2.getWordArray());

            final OrcaAudio randomStateNull = orca.synthesize(
                    text,
                    new OrcaSynthesizeParams.Builder()
                            .build());
            assertTrue(randomStateNull.getPcm().length > 0);
            assertTrue(randomStateNull.getWordArray().length > 0);

            final OrcaAudio randomStateMaxValue = orca.synthesize(
                    text,
                    new OrcaSynthesizeParams.Builder()
                            .setRandomState(Long.MAX_VALUE)
                            .build());
            assertTrue(randomStateMaxValue.getPcm().length > 0);
            assertTrue(randomStateMaxValue.getWordArray().length > 0);
        }
    }
}
