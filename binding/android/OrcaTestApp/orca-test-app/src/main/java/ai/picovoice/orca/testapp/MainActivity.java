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

import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;

import ai.picovoice.orca.Orca;
import ai.picovoice.orca.OrcaException;
import ai.picovoice.orca.OrcaSynthesizeParams;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    public void startTest(View view) {
        Button testButton = findViewById(R.id.testButton);
        testButton.setBackground(ContextCompat.getDrawable(
                getApplicationContext(),
                R.drawable.button_disabled));
        runTest();

        testButton.setBackground(ContextCompat.getDrawable(
                getApplicationContext(),
                R.drawable.button_background));
    }

    public void runTest() {
        String accessKey = getApplicationContext().getString(R.string.pvTestingAccessKey);

        ArrayList<TestResult> results = new ArrayList<>();

        final String modelFile = "models/orca_params_female.pv";

        TestResult result = new TestResult();
        result.testName = "Test Init";
        Orca orca = null;
        try {
            orca = new Orca.Builder()
                    .setAccessKey(accessKey)
                    .setModelPath(modelFile)
                    .build(getApplicationContext());
            result.success = true;
        } catch (OrcaException e) {
            result.success = false;
            result.errorMessage = String.format("Failed to init Orca with '%s'", e);
        } finally {
            results.add(result);
        }

        result = new TestResult();
        result.testName = "Test Orca properties";
        try {
            int maxCharLimit = orca.getMaxCharacterLimit();
            int sampleRate = orca.getSampleRate();
            String[] symbols = orca.getValidCharacters();
            String version = orca.getVersion();

            if (maxCharLimit > 0 && sampleRate > 0 && symbols.length > 0 && version.length() > 0) {
                result.success = true;
            } else {
                result.success = false;
                result.errorMessage = "Orca properties returned invalid result.";
            }
        } catch (Exception e) {
            result.success = false;
            result.errorMessage = String.format("Failed to process with '%s'", e);
        } finally {
            results.add(result);
        }

        result = new TestResult();
        result.testName = "Test Synthesize";
        try {
            short[] pcm = orca.synthesize("Hello", new OrcaSynthesizeParams.Builder().build());
            if (pcm.length > 0) {
                result.success = true;
            } else {
                result.success = false;
                result.errorMessage = "Synthesize returned invalid result.";
            }
        } catch (Exception e) {
            result.success = false;
            result.errorMessage = String.format("Failed to synthesize with '%s'", e);
        } finally {
            results.add(result);
        }

        result = new TestResult();
        result.testName = "Test Synthesize To File";
        try {
            File outputFile = new File(getApplicationContext().getFilesDir(), "out.wav");

            orca.synthesizeToFile(
                    "Hello",
                    outputFile.getAbsolutePath(),
                    new OrcaSynthesizeParams.Builder().build());
            if (outputFile.exists()) {
                result.success = true;
            } else {
                result.success = false;
                result.errorMessage = "Synthesize to file returned invalid result.";
            }
        } catch (Exception e) {
            result.success = false;
            result.errorMessage = String.format("Failed to synthesize to file with '%s'", e);
        } finally {
            results.add(result);
        }

        result = new TestResult();
        result.testName = "Test Exception";
        try {
            new Orca.Builder().setAccessKey("").build(getApplicationContext());
            result.success = false;
            result.errorMessage = "Init should have throw an exception";
        } catch (OrcaException e) {
            result.success = true;
        } finally {
            results.add(result);
        }

        displayTestResults(results);
    }

    private void displayTestResults(ArrayList<TestResult> results) {
        ListView resultList = findViewById(R.id.resultList);

        int passed = 0;
        int failed = 0;

        ArrayList<HashMap<String, String>> list = new ArrayList<>();
        for (TestResult result : results) {
            HashMap<String, String> map = new HashMap<>();
            map.put("testName", result.testName);

            String message;
            if (result.success) {
                message = "Test Passed";
                passed += 1;
            } else {
                message = String.format("Test Failed: %s", result.errorMessage);
                failed += 1;
            }

            map.put("testMessage", message);
            list.add(map);
        }

        SimpleAdapter adapter = new SimpleAdapter(
                getApplicationContext(),
                list,
                R.layout.list_view,
                new String[]{"testName", "testMessage"},
                new int[]{R.id.testName, R.id.testMessage});

        resultList.setAdapter(adapter);

        TextView passedView = findViewById(R.id.testNumPassed);
        TextView failedView = findViewById(R.id.testNumFailed);

        passedView.setText(String.valueOf(passed));
        failedView.setText(String.valueOf(failed));

        TextView resultView = findViewById(R.id.testResult);
        if (passed == 0 || failed > 0) {
            resultView.setText("Failed");
        } else {
            resultView.setText("Passed");
        }
    }
}
