/*
    Copyright 2025 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is
    located in the "LICENSE" file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the
    License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
    express or implied. See the License for the specific language governing permissions and
    limitations under the License.
*/

package ai.picovoice.orca.testapp;

import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import ai.picovoice.orca.Orca;
import ai.picovoice.orca.OrcaAudio;
import ai.picovoice.orca.OrcaException;
import ai.picovoice.orca.OrcaInvalidArgumentException;
import ai.picovoice.orca.OrcaSynthesizeParams;

@RunWith(Parameterized.class)
public class SentenceTests extends BaseTest {

    @Parameterized.Parameter(value = 0)
    public String language;

    @Parameterized.Parameter(value = 1)
    public String modelFilename;

    @Parameterized.Parameter(value = 2)
    public int randomState;

    @Parameterized.Parameter(value = 3)
    public String text;

    @Parameterized.Parameter(value = 4)
    public String textNoPunctuation;

    @Parameterized.Parameter(value = 5)
    public String textCustomPronunciation;

    @Parameterized.Parameters(name = "{0} {1}")
    public static Collection<Object[]> initParameters() throws IOException {
        String testDataJsonString = getTestDataString();

        JsonParser parser = new JsonParser();
        JsonObject testDataJson = parser.parse(testDataJsonString).getAsJsonObject();

        final JsonArray testCases = testDataJson.getAsJsonObject("tests").get("sentence_tests").getAsJsonArray();

        List<Object[]> parameters = new ArrayList<>();
        for (JsonElement testCaseElem : testCases) {
            JsonObject testCase = testCaseElem.getAsJsonObject();

            String language = testCase.get("language").getAsString();
            int random_state = testCase.get("random_state").getAsInt();
            String text = testCase.get("text").getAsString();
            String text_no_punctuation = testCase.get("text_no_punctuation").getAsString();
            String text_custom_pronunciation = testCase.get("text_custom_pronunciation").getAsString();

            for (JsonElement modelJson : testCase.get("models").getAsJsonArray()) {
                String model = modelJson.getAsString();
                parameters.add(new Object[]{language, model, random_state, text, text_no_punctuation, text_custom_pronunciation});
            }
        }

        return parameters;
    }

    Orca orca;

    @Before
    public void Setup() throws Exception {
        orca = new Orca.Builder()
                .setAccessKey(accessKey)
                .setDevice(device)
                .setModelPath(getModelFilepath(modelFilename))
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
        short[] testFilePcm = readAudioFile(getAudioFilepath(modelFilename, "stream"));

        validatePcm(fullPcm, testFilePcm);
    }

    @Test
    public void testSynthesize() throws Exception {
        final OrcaAudio pcm = orca.synthesize(
                text,
                new OrcaSynthesizeParams.Builder()
                        .setRandomState(randomState)
                        .build());

        short[] testFilePcm = readAudioFile(getAudioFilepath(modelFilename, "single"));

        validatePcm(pcm.getPcm(), testFilePcm);
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
        short[] testFilePcm = readAudioFile(getAudioFilepath(modelFilename, "single"));

        validatePcm(outputFilePcm, testFilePcm);
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
