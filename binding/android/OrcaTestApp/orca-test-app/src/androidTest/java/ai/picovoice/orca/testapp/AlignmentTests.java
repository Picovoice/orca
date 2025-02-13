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
import java.util.Objects;

import ai.picovoice.orca.Orca;
import ai.picovoice.orca.OrcaAudio;
import ai.picovoice.orca.OrcaException;
import ai.picovoice.orca.OrcaInvalidArgumentException;
import ai.picovoice.orca.OrcaSynthesizeParams;
import ai.picovoice.orca.OrcaWord;
import ai.picovoice.orca.OrcaPhoneme;

@RunWith(Parameterized.class)
public class AlignmentTests extends BaseTest {

    @Parameterized.Parameter(value = 0)
    public String language;

    @Parameterized.Parameter(value = 1)
    public String modelFilename;

    @Parameterized.Parameter(value = 2)
    public int randomState;

    @Parameterized.Parameter(value = 3)
    public String textAlignment;

    @Parameterized.Parameter(value = 4)
    public JsonArray alignments;

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
            String model = testCase.get("model").getAsString();
            int random_state = testCase.get("random_state").getAsInt();
            String text_alignment = testCase.get("text_alignment").getAsString();
            JsonArray alignments = testCase.get("alignments").getAsJsonArray();

            parameters.add(new Object[]{language, model, random_state, text_alignment, alignments});
        }

        return parameters;
    }

    Orca orca;

    @Before
    public void Setup() throws Exception {
        super.Setup();

        orca = new Orca.Builder()
                .setAccessKey(accessKey)
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
                true);
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
                true);
    }
}
