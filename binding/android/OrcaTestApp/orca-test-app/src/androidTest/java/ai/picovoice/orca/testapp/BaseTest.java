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

import android.content.Context;
import android.content.res.AssetManager;

import androidx.test.platform.app.InstrumentationRegistry;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import com.microsoft.appcenter.espresso.Factory;
import com.microsoft.appcenter.espresso.ReportHelper;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;

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

import static org.junit.Assert.assertEquals;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;

import ai.picovoice.orca.OrcaWord;
import ai.picovoice.orca.OrcaPhoneme;

public class BaseTest {

    @Rule
    public ReportHelper reportHelper = Factory.getReportHelper();

    Context testContext;
    Context appContext;
    AssetManager assetManager;
    String testResourcesPath;
    JsonObject testJson;
    String accessKey;

    @Before
    public void Setup() throws Exception {
        testContext = InstrumentationRegistry.getInstrumentation().getContext();
        appContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
        assetManager = testContext.getAssets();
        extractAssetsRecursively("test_resources");
        testResourcesPath = new File(
                appContext.getFilesDir(),
                "test_resources").getAbsolutePath();

        FileReader reader = new FileReader(
                new File(testResourcesPath, "test_data.json").getAbsolutePath()
        );
        testJson = new Gson().fromJson(reader, JsonObject.class);
        reader.close();

        accessKey = appContext.getString(R.string.pvTestingAccessKey);
    }

    @After
    public void TearDown() {
        // DO NOT REMOVE - this is required for AppCenter testing
        reportHelper.label("Stopping App");
    }

    public static String[] getModelFiles() {
        Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        String resPath = new File(
                context.getFilesDir(),
                "test_resources").getAbsolutePath();
        return new String[]{
                new File(
                        resPath,
                        "model_files/orca_params_female.pv").getAbsolutePath(),
                new File(
                        resPath,
                        "model_files/orca_params_male.pv").getAbsolutePath()
        };
    }

    protected static boolean compareArrays(short[] arr1, short[] arr2, int step) {
        for (int i = 0; i < arr1.length - step; i += step) {
            if (!(Math.abs(arr1[i] - arr2[i]) <= 500)) {
                return false;
            }
        }
        return true;
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

    private void extractAssetsRecursively(String path) throws IOException {
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

    private void extractTestFile(String filepath) throws IOException {

        InputStream is = new BufferedInputStream(
                assetManager.open(filepath),
                256);
        File absPath = new File(
                appContext.getFilesDir(),
                filepath);
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
    }
}
