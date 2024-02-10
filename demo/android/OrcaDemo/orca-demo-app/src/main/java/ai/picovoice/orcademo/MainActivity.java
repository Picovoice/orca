/*
    Copyright 2024 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is
    located in the "LICENSE" file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the
    License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
    express or implied. See the License for the specific language governing permissions and
    limitations under the License.
*/

package ai.picovoice.orcademo;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.os.Bundle;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;
import android.widget.ToggleButton;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;

import java.util.ArrayList;
import java.util.List;

import ai.picovoice.android.voiceprocessor.VoiceProcessor;
import ai.picovoice.android.voiceprocessor.VoiceProcessorException;
import ai.picovoice.orca.Orca;
import ai.picovoice.orca.OrcaActivationException;
import ai.picovoice.orca.OrcaActivationLimitException;
import ai.picovoice.orca.OrcaActivationRefusedException;
import ai.picovoice.orca.OrcaActivationThrottledException;
import ai.picovoice.orca.OrcaException;
import ai.picovoice.orca.OrcaInvalidArgumentException;


public class MainActivity extends AppCompatActivity {
    private static final String ACCESS_KEY = "${YOUR_ACCESS_KEY_HERE}";

    @SuppressLint("SetTextI18n")
    private void setUIState(UIState state) {

        runOnUiThread(() -> {
            TextView errorText = findViewById(R.id.errorTextView);
            TextView recordingTextView = findViewById(R.id.recordingTextView);
            ToggleButton enrollButton = findViewById(R.id.enrollButton);
            ProgressBar enrollProgress = findViewById(R.id.enrollProgress);
            ToggleButton testButton = findViewById(R.id.testButton);

            switch (state) {
                case IDLE:
                    break;
                default:
                    break;
            }
        });
    }

    private void OrcaException(OrcaException e) {
        if (e instanceof OrcaInvalidArgumentException) {
            displayError(e.getMessage());
        } else if (e instanceof OrcaActivationException) {
            displayError("AccessKey activation error");
        } else if (e instanceof OrcaActivationLimitException) {
            displayError("AccessKey reached its device limit");
        } else if (e instanceof OrcaActivationRefusedException) {
            displayError("AccessKey refused");
        } else if (e instanceof OrcaActivationThrottledException) {
            displayError("AccessKey has been throttled");
        } else {
            displayError("Failed to initialize Orca " + e.getMessage());
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.orca_demo);

        Toolbar toolbar = findViewById(R.id.toolbar);
        toolbar.setTitleTextColor(Color.WHITE);
        setSupportActionBar(toolbar);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    private void displayError(String message) {
        setUIState(UIState.ERROR);

        TextView errorText = findViewById(R.id.errorTextView);
        errorText.setText(message);
        errorText.setVisibility(View.VISIBLE);

        ToggleButton enrollButton = findViewById(R.id.enrollButton);
        enrollButton.setEnabled(false);

        ToggleButton testButton = findViewById(R.id.testButton);
        testButton.setEnabled(false);
    }

    private enum UIState {
        IDLE,
        ENROLLING,
        INITIALIZING,
        TESTING,
        ERROR
    }
}
