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

import android.annotation.SuppressLint;
import android.media.AudioAttributes;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.ToggleButton;

import androidx.appcompat.app.AppCompatActivity;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.regex.Pattern;

import ai.picovoice.orca.Orca;
import ai.picovoice.orca.OrcaActivationException;
import ai.picovoice.orca.OrcaActivationLimitException;
import ai.picovoice.orca.OrcaActivationRefusedException;
import ai.picovoice.orca.OrcaActivationThrottledException;
import ai.picovoice.orca.OrcaException;
import ai.picovoice.orca.OrcaInvalidArgumentException;
import ai.picovoice.orca.OrcaSynthesizeParams;


public class MainActivity extends AppCompatActivity {
    private static final String ACCESS_KEY = "${YOUR_ACCESS_KEY_HERE}";

    private static final String MODEL_FILE = "orca_params_female.pv";

    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final ExecutorService executor = Executors.newSingleThreadExecutor();

    private String synthesizedFilePath;
    private MediaPlayer synthesizedPlayer;

    private boolean isTextSynthesized = false;

    private Pattern validationRegex;

    private Orca orca;

    TextView errorText;
    TextView infoTextView;
    TextView numCharsTextView;
    EditText synthesizeEditText;
    ToggleButton synthesizeButton;
    ProgressBar synthesizeProgress;

    @SuppressLint("DefaultLocale")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.orca_demo);
        errorText = findViewById(R.id.errorTextView);
        infoTextView = findViewById(R.id.infoTextView);
        numCharsTextView = findViewById(R.id.numCharsTextView);
        synthesizeEditText = findViewById(R.id.synthesizeEditText);
        synthesizeButton = findViewById(R.id.synthesizeButton);
        synthesizeProgress = findViewById(R.id.synthesizeProgress);

        try {
            orca = new Orca.Builder()
                    .setAccessKey(ACCESS_KEY)
                    .setModelPath(MODEL_FILE)
                    .build(getApplicationContext());
            validationRegex = Pattern.compile(String.format(
                    "[%s ]+",
                    String.join("", orca.getValidCharacters())));
            numCharsTextView.setText(String.format("0/%d", orca.getMaxCharacterLimit()));
        } catch (OrcaException e) {
            onOrcaException(e);
        }

        synthesizedFilePath = getApplicationContext()
                .getFileStreamPath("synthesized.wav")
                .getAbsolutePath();
        synthesizedPlayer = new MediaPlayer();

        synthesizeEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void afterTextChanged(Editable s) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                isTextSynthesized = false;
                runOnUiThread(() ->
                        numCharsTextView.setText(String.format(
                                "%d/%d",
                                s.toString().length(),
                                orca.getMaxCharacterLimit()))
                );
                validateText(s.toString());
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        orca.delete();
        if (synthesizedPlayer != null) {
            synthesizedPlayer.release();
        }
    }


    @SuppressLint("SetTextI18n")
    private void setUIState(UIState state) {
        runOnUiThread(() -> {
            switch (state) {
                case EDIT:
                    infoTextView.setVisibility(View.INVISIBLE);
                    synthesizeButton.setVisibility(View.VISIBLE);
                    synthesizeButton.setEnabled(true);
                    synthesizeEditText.setEnabled(true);
                    synthesizeProgress.setVisibility(View.INVISIBLE);
                    break;
                case PLAYBACK:
                    infoTextView.setVisibility(View.VISIBLE);
                    synthesizeButton.setVisibility(View.VISIBLE);
                    synthesizeButton.setEnabled(true);
                    synthesizeEditText.setEnabled(false);
                    synthesizeProgress.setVisibility(View.INVISIBLE);
                    break;
                case BUSY:
                    infoTextView.setVisibility(View.VISIBLE);
                    synthesizeButton.setVisibility(View.INVISIBLE);
                    synthesizeButton.setEnabled(false);
                    synthesizeEditText.setEnabled(false);
                    synthesizeProgress.setVisibility(View.VISIBLE);
                    break;
                case ERROR:
                    infoTextView.setVisibility(View.VISIBLE);
                    errorText.setVisibility(View.INVISIBLE);
                    synthesizeButton.setEnabled(false);
                    synthesizeEditText.setEnabled(true);
                    synthesizeProgress.setVisibility(View.INVISIBLE);
                    break;
                case FATAL_ERROR:
                    infoTextView.setVisibility(View.INVISIBLE);
                    errorText.setVisibility(View.VISIBLE);
                    synthesizeButton.setEnabled(false);
                    synthesizeEditText.setEnabled(false);
                    synthesizeProgress.setVisibility(View.INVISIBLE);
                    break;
                default:
                    break;
            }
        });
    }

    private void onOrcaException(OrcaException e) {
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

    private void displayError(String message) {
        setUIState(UIState.FATAL_ERROR);
        errorText.setText(message);
    }

    private void validateText(String text) {
        if (text.length() > 0) {
            if (text.length() >= orca.getMaxCharacterLimit()) {
                runOnUiThread(() -> {
                    setUIState(UIState.ERROR);
                    infoTextView.setText("Too many characters");
                });
            } else {
                if (validationRegex.matcher(text).matches()) {
                    runOnUiThread(() -> {
                        setUIState(UIState.EDIT);
                        synthesizeButton.setEnabled(true);
                    });
                } else {
                    runOnUiThread(() -> {
                        setUIState(UIState.ERROR);
                        infoTextView.setText("Invalid characters in text");
                    });
                }
            }
        } else {
            runOnUiThread(() -> synthesizeButton.setEnabled(false));
        }
    }

    private void runSynthesis() {
        runOnUiThread(() -> {
            setUIState(UIState.BUSY);
            infoTextView.setText("Synthesizing...");
        });
        executor.submit(() -> {
            String text = synthesizeEditText.getText().toString();
            try {
                orca.synthesizeToFile(
                        synthesizedFilePath,
                        text,
                        new OrcaSynthesizeParams.Builder().build());
            } catch (OrcaException e) {
                mainHandler.post(() -> onOrcaException(e));
            }

            synthesizedPlayer.reset();
            synthesizedPlayer.setAudioAttributes(
                    new AudioAttributes.Builder()
                            .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
                            .setUsage(AudioAttributes.USAGE_MEDIA)
                            .build()
            );
            synthesizedPlayer.setLooping(false);
            synthesizedPlayer.setOnCompletionListener(mediaPlayer -> {
                runOnUiThread(() -> synthesizeButton.setChecked(false));
                stopPlayback();
            });
            try {
                synthesizedPlayer.setDataSource(synthesizedFilePath);
                synthesizedPlayer.prepare();
                synthesizedPlayer.setVolume(1f, 1f);
                isTextSynthesized = true;
                mainHandler.post(this::startPlayback);
            } catch (Exception e) {
                mainHandler.post(() -> displayError(e.toString()));
            }
        });
    }

    private void startPlayback() {
        try {
            synthesizedPlayer.start();
            updateCurrentTime(0);
            runOnUiThread(() -> {
                setUIState(UIState.PLAYBACK);
                infoTextView.setText(
                        String.format("0:00 / %s", formatTime(synthesizedPlayer.getDuration()))
                );
            });
        } catch (Exception e) {
            displayError(e.toString());
        }
    }

    private void updateCurrentTime(int delayMillis) {
        mainHandler.postDelayed(() -> {
            if (synthesizedPlayer != null && synthesizedPlayer.isPlaying()) {
                infoTextView.setText(
                        String.format("%s / %s",
                                formatTime(synthesizedPlayer.getCurrentPosition()),
                                formatTime(synthesizedPlayer.getDuration())));
                updateCurrentTime(500);
            }
        }, delayMillis);
    }

    @SuppressLint("DefaultLocale")
    private String formatTime(int milliseconds) {
        int seconds = (milliseconds / 1000) % 60;
        int minutes = (milliseconds / (1000 * 60)) % 60;
        return String.format("%d:%02d", minutes, seconds);
    }

    private void stopPlayback() {
        if (synthesizedPlayer.isPlaying()) {
            synthesizedPlayer.pause();
        }
        synthesizedPlayer.seekTo(0);

        runOnUiThread(() -> setUIState(UIState.EDIT));
    }

    public void onSynthesizeClick(View view) {
        if (orca == null) {
            displayError("Orca is not initialized");
            synthesizeButton.setChecked(false);
            return;
        }

        if (synthesizeButton.isChecked()) {
            if (!isTextSynthesized) {
                runSynthesis();
            } else {
                startPlayback();
            }
        } else {
            stopPlayback();
        }
    }

    private enum UIState {
        EDIT,
        PLAYBACK,
        BUSY,
        ERROR,
        FATAL_ERROR
    }
}
