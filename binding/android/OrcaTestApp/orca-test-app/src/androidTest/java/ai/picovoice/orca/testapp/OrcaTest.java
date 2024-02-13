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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.google.gson.JsonObject;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.experimental.runners.Enclosed;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.io.File;
import java.util.Arrays;
import java.util.Collection;
import java.util.stream.Collectors;

import ai.picovoice.orca.Orca;
import ai.picovoice.orca.OrcaException;
import ai.picovoice.orca.OrcaInvalidArgumentException;
import ai.picovoice.orca.OrcaSynthesizeParams;


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
            return Arrays.stream(getModelFiles())
                    .map(modelFile -> new Object[]{modelFile})
                    .collect(Collectors.toList());
        }

        String text;
        String textNoPunctuation;
        String textCustomPronunciation;

        float werThreshold;

        Orca orca;

        @Before
        public void Setup() throws Exception {
            super.Setup();

            final JsonObject testSentences = testJson.getAsJsonObject("test_sentences");
            text = testSentences.get("text").getAsString();
            textNoPunctuation = testSentences.get("text_no_punctuation").getAsString();
            textCustomPronunciation = testSentences.get("text_custom_pronunciation").getAsString();
            werThreshold = testJson.get("wer_threshold").getAsFloat();

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
        public void testMaxCharacterLimit() {
            assertTrue(orca.getMaxCharacterLimit() > 0);
        }

        @Test
        public void testVersion() {
            final String version = orca.getVersion();
            assertNotNull(version);
            assertNotEquals(version, "");
        }

        @Test
        public void testSampleRate() throws OrcaException {
            assertTrue(orca.getSampleRate() > 0);
        }

        @Test
        public void testValidCharacters() throws OrcaException {
            String[] characters = orca.getValidCharacters();
            assertTrue(characters.length > 0);
            assertTrue(Arrays.asList(characters).contains(","));
        }

        @Test
        public void testSynthesize() throws OrcaException {
            final short[] pcm = orca.synthesize(
                    text,
                    new OrcaSynthesizeParams.Builder().build());
            assertTrue(pcm.length > 0);
        }

        @Test
        public void testSynthesizeToFile() throws OrcaException {
            final File outputFile = new File(
                    appContext.getFilesDir(),
                    "text.wav");
            orca.synthesizeToFile(
                    outputFile.getAbsolutePath(),
                    text,
                    new OrcaSynthesizeParams.Builder().build());
            assertTrue(outputFile.exists());
            outputFile.delete();
        }

        @Test
        public void testSynthesizeCustomPronunciation() throws OrcaException {
            final short[] pcm = orca.synthesize(
                    textCustomPronunciation,
                    new OrcaSynthesizeParams.Builder().build());
            assertTrue(pcm.length > 0);
        }

        @Test
        public void testSynthesizeSpeechRate() throws OrcaException {
            final short[] pcmSlow = orca.synthesize(
                    textCustomPronunciation,
                    new OrcaSynthesizeParams.Builder()
                            .setSpeechRate(0.7f)
                            .build());
            assertTrue(pcmSlow.length > 0);

            final short[] pcmFast = orca.synthesize(
                    textCustomPronunciation,
                    new OrcaSynthesizeParams.Builder()
                            .setSpeechRate(1.3f)
                            .build());
            assertTrue(pcmFast.length > 0);
            assertTrue(pcmFast.length < pcmSlow.length);

            try {
                orca.synthesize(
                        textCustomPronunciation,
                        new OrcaSynthesizeParams.Builder()
                                .setSpeechRate(9999f)
                                .build());
                fail();
            } catch (OrcaInvalidArgumentException e) {
                assertNotNull(e);
            }
        }
    }
}
