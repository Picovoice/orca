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
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;


public class BaseTest {

    @Rule
    public ReportHelper reportHelper = Factory.getReportHelper();

    Context testContext;
    Context appContext;
    AssetManager assetManager;
    String testResourcesPath;
    JsonObject testJson;
    String leopardModelPath;
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

    protected static float getWordErrorRate(
            String transcript,
            String expectedTranscript,
            boolean useCER) {
        String splitter = (useCER) ? "" : " ";
        return (float) levenshteinDistance(
                transcript.split(splitter),
                expectedTranscript.split(splitter)) / transcript.length();
    }

    private static int levenshteinDistance(String[] words1, String[] words2) {
        int[][] res = new int[words1.length + 1][words2.length + 1];
        for (int i = 0; i <= words1.length; i++) {
            res[i][0] = i;
        }
        for (int j = 0; j <= words2.length; j++) {
            res[0][j] = j;
        }
        for (int i = 1; i <= words1.length; i++) {
            for (int j = 1; j <= words2.length; j++) {
                res[i][j] = Math.min(
                        Math.min(
                                res[i - 1][j] + 1,
                                res[i][j - 1] + 1),
                        res[i - 1][j - 1] + (words1[i - 1].equalsIgnoreCase(words2[j - 1]) ? 0 : 1)
                );
            }
        }
        return res[words1.length][words2.length];
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
