/*
    Copyright 2024-2025 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is
    located in the "LICENSE" file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the
    License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
    express or implied. See the License for the specific language governing permissions and
    limitations under the License.
*/

package ai.picovoice.orca.testapp;

import static org.junit.Assert.assertEquals;

import android.content.Context;
import android.content.res.AssetManager;

import androidx.test.platform.app.InstrumentationRegistry;

import com.google.gson.Gson;
import com.google.gson.JsonObject;

import org.junit.BeforeClass;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import ai.picovoice.orca.OrcaPhoneme;
import ai.picovoice.orca.OrcaWord;

public class BaseTest {
    static Set<String> extractedFiles;

    static Context testContext;
    static Context appContext;
    static AssetManager assetManager;
    static String testResourcesPath;
    static JsonObject testJson;

    static String accessKey;
    static String device;

    private static final int PCM_OUTLIER_THRESHOLD = 400;
    private static final double PCM_OUTLIER_COUNT_THRESHOLD = 0.05;

    @BeforeClass
    public static void setup() throws Exception {
        extractedFiles = new HashSet<>();

        testContext = InstrumentationRegistry.getInstrumentation().getContext();
        appContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
        assetManager = testContext.getAssets();
        testResourcesPath = new File(
                appContext.getFilesDir(),
                "test_resources").getAbsolutePath();

        extractTestFile("test_resources/test_data.json");
        FileReader reader = new FileReader(
                new File(testResourcesPath, "test_data.json").getAbsolutePath()
        );
        testJson = new Gson().fromJson(reader, JsonObject.class);
        reader.close();

        accessKey = appContext.getString(R.string.pvTestingAccessKey);
        device = appContext.getString(R.string.pvTestingDevice);
    }

    public static String getTestDataString() throws IOException {
        Context testContext = InstrumentationRegistry.getInstrumentation().getContext();
        AssetManager assetManager = testContext.getAssets();

        InputStream is = new BufferedInputStream(assetManager.open("test_resources/test_data.json"), 256);
        ByteArrayOutputStream result = new ByteArrayOutputStream();

        byte[] buffer = new byte[256];
        int bytesRead;
        while ((bytesRead = is.read(buffer)) != -1) {
            result.write(buffer, 0, bytesRead);
        }

        return result.toString("UTF-8");
    }

    public static String getModelFilepath(String modelFilename) throws IOException {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        String resPath = new File(
                context.getFilesDir(),
                "test_resources").getAbsolutePath();
        String modelPath = String.format("model_files/%s", modelFilename);
        extractTestFile(String.format("test_resources/%s", modelPath));
        return new File(resPath, modelPath).getAbsolutePath();
    }

    public static String getAudioFilepath(String modelFilename, String synthesisType) throws IOException {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        String resPath = new File(
                context.getFilesDir(),
                "test_resources").getAbsolutePath();
        String audioFilename = modelFilename.replace(".pv", String.format("_%s.wav", synthesisType));
        extractTestFile(String.format("test_resources/wav/%s", audioFilename));
        return new File(resPath, String.format("wav/%s", audioFilename)).getAbsolutePath();
    }

    protected static void validatePcm(short[] synthesizedPcm, short[] groundTruth) {
        assertEquals(groundTruth.length, pcm.length);

        int outlierCount = 0;
        for (int i = 0; i < pcm.length; i++) {
            int diff = Math.abs(pcm[i] - groundTruth[i]);
            if (diff > PCM_OUTLIER_THRESHOLD) {
                outlierCount++;
            }
        }

        double diffOutliers = (double) outlierCount / pcm.length;
        assertTrue(diffOutliers <= PCM_OUTLIER_COUNT_THRESHOLD);
    }

    protected static short[] concatArrays(short[] existingArray, short[] arrayToAdd) {
        short[] result = new short[existingArray.length + arrayToAdd.length];

        System.arraycopy(existingArray, 0, result, 0, existingArray.length);
        System.arraycopy(arrayToAdd, 0, result, existingArray.length, arrayToAdd.length);

        return result;
    }

    protected static short[] readAudioFile(String audioFile) throws Exception {
        FileInputStream audioInputStream = new FileInputStream(audioFile);
        ByteArrayOutputStream audioByteBuffer = new ByteArrayOutputStream();
        byte[] buffer = new byte[1024];
        for (int length; (length = audioInputStream.read(buffer)) != -1; ) {
            audioByteBuffer.write(buffer, 0, length);
        }
        byte[] rawData = audioByteBuffer.toByteArray();

        short[] pcm = new short[rawData.length / 2];
        ByteBuffer pcmBuff = ByteBuffer.wrap(rawData).order(ByteOrder.LITTLE_ENDIAN);
        pcmBuff.asShortBuffer().get(pcm);
        pcm = Arrays.copyOfRange(pcm, 22, pcm.length);

        return pcm;
    }

    protected void validateMetadata(
            OrcaWord[] words,
            OrcaWord[] expectedWords,
            boolean isExpectExact
    ) {
        assertEquals(words.length, expectedWords.length);
        for (int i = 0; i < words.length; i++) {
            assertEquals(words[i].getWord(), expectedWords[i].getWord());
            if (isExpectExact) {
                assertEquals(words[i].getStartSec(), expectedWords[i].getStartSec(), 0.1);
                assertEquals(words[i].getEndSec(), expectedWords[i].getEndSec(), 0.1);
            }
            OrcaPhoneme[] phonemes = words[i].getPhonemeArray();
            OrcaPhoneme[] expectedPhonemes = expectedWords[i].getPhonemeArray();
            assertEquals(phonemes.length, expectedPhonemes.length);
            for (int j = 0; j < phonemes.length; j++) {
                assertEquals(phonemes[j].getPhoneme(), expectedPhonemes[j].getPhoneme());
                if (isExpectExact) {
                    assertEquals(phonemes[j].getStartSec(), expectedPhonemes[j].getStartSec(), 0.1);
                    assertEquals(phonemes[j].getEndSec(), expectedPhonemes[j].getEndSec(), 0.1);
                }
            }
        }
    }

    private static void extractAssetsRecursively(String path) throws IOException {
        String[] dirList = assetManager.list(path);
        if (dirList != null && dirList.length > 0) {
            File outputFile = new File(appContext.getFilesDir(), path);
            if (!outputFile.exists()) {
                if (!outputFile.mkdirs()) {
                    throw new IOException(
                            String.format(
                                    "Failed to create directory %s",
                                    outputFile.getAbsolutePath())
                    );
                }
            }

            for (String filename : dirList) {
                String filepath = path + "/" + filename;
                extractAssetsRecursively(filepath);
            }
        } else {
            extractTestFile(path);
        }
    }

    private static void extractTestFile(String filepath) throws IOException {
        File absPath = new File(
                appContext.getFilesDir(),
                filepath);

        if (extractedFiles.contains(filepath)) {
            return;
        }

        if (!absPath.exists()) {
            if (absPath.getParentFile() != null) {
                absPath.getParentFile().mkdirs();
            }
            absPath.createNewFile();
        }

        InputStream is = new BufferedInputStream(
                assetManager.open(filepath),
                256);
        OutputStream os = new BufferedOutputStream(
                new FileOutputStream(absPath),
                256);

        int r;
        while ((r = is.read()) != -1) {
            os.write(r);
        }
        os.flush();

        is.close();
        os.close();

        extractedFiles.add(filepath);
    }
}
