/*
    Copyright 2024-2025 Picovoice Inc.

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
import android.text.method.ScrollingMovementMethod;
import android.view.View;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.ToggleButton;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.SwitchCompat;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.regex.Matcher;
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

    private static final String language = BuildConfig.FLAVOR.substring(0, 2);
    private static final String gender = BuildConfig.FLAVOR.substring(2);
    private static final int STREAMING_NUM_AUDIO_WAIT_CHUNKS = 1;

    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final ExecutorService executorStreamingSynthesis = Executors.newSingleThreadExecutor();
    private final ExecutorService executorStreamingAudio = Executors.newSingleThreadExecutor();

    private String synthesizedFilePath;
    private MediaPlayer synthesizedPlayer;

    private String previousText = "";

    private Pattern validationRegex;

    private Orca orca;

    private Orca.OrcaStream orcaStream = null;

    TextView errorText;
    TextView infoTextView;
    TextView streamTextView;
    TextView streamSecsTextView;
    TextView numCharsTextView;
    EditText synthesizeEditText;
    SwitchCompat streamSwitch;
    ToggleButton synthesizeButton;
    ProgressBar synthesizeProgress;

    @SuppressLint("DefaultLocale")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.orca_demo);
        errorText = findViewById(R.id.errorTextView);
        infoTextView = findViewById(R.id.infoTextView);
        streamTextView = findViewById(R.id.streamTextView);
        streamSecsTextView = findViewById(R.id.streamSecsTextView);
        numCharsTextView = findViewById(R.id.numCharsTextView);
        synthesizeEditText = findViewById(R.id.synthesizeEditText);
        streamSwitch = findViewById(R.id.streamSwitch);
        synthesizeButton = findViewById(R.id.synthesizeButton);
        synthesizeProgress = findViewById(R.id.synthesizeProgress);
        streamTextView.setMovementMethod(new ScrollingMovementMethod());

        try {
            String modelName = "orca_params_" + language + "_" + gender.toLowerCase() + ".pv";

            orca = new Orca.Builder()
                    .setAccessKey(ACCESS_KEY)
                    .setModelPath("models/" + modelName)
                    .build(getApplicationContext());
            StringBuilder validCharsEscaped = new StringBuilder();
            for (String ch : orca.getValidCharacters()) {
                switch (ch) {
                    case "\\": case "]": case "^": case "-": case "[":
                        validCharsEscaped.append("\\").append(ch);
                        break;
                    default:
                        validCharsEscaped.append(ch);
                }
            }
            validationRegex = Pattern.compile(String.format("[^%s ]", validCharsEscaped));
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
                    streamSwitch.setEnabled(true);
                    synthesizeButton.setEnabled(true);
                    synthesizeButton.setChecked(false);
                    synthesizeEditText.setEnabled(true);
                    synthesizeEditText.setVisibility(View.VISIBLE);
                    synthesizeProgress.setVisibility(View.INVISIBLE);
                    streamTextView.setVisibility(View.INVISIBLE);
                    break;
                case PLAYBACK:
                    infoTextView.setVisibility(View.VISIBLE);
                    synthesizeButton.setVisibility(View.VISIBLE);
                    streamSwitch.setEnabled(false);
                    synthesizeButton.setEnabled(true);
                    synthesizeEditText.setEnabled(false);
                    synthesizeProgress.setVisibility(View.INVISIBLE);
                    streamTextView.setVisibility(View.INVISIBLE);
                    streamSecsTextView.setVisibility(View.INVISIBLE);
                    break;
                case STREAMING_PLAYBACK:
                    infoTextView.setText("Streaming...");
                    infoTextView.setVisibility(View.VISIBLE);
                    synthesizeButton.setVisibility(View.VISIBLE);
                    streamSwitch.setEnabled(false);
                    synthesizeButton.setEnabled(false);
                    synthesizeEditText.setEnabled(false);
                    synthesizeProgress.setVisibility(View.INVISIBLE);
                    streamTextView.setVisibility(View.VISIBLE);
                    streamSecsTextView.setVisibility(View.VISIBLE);
                    synthesizeEditText.setVisibility(View.INVISIBLE);
                    break;
                case BUSY:
                    infoTextView.setVisibility(View.VISIBLE);
                    errorText.setVisibility(View.INVISIBLE);
                    synthesizeButton.setVisibility(View.INVISIBLE);
                    streamSwitch.setEnabled(false);
                    synthesizeButton.setEnabled(false);
                    synthesizeEditText.setEnabled(false);
                    synthesizeProgress.setVisibility(View.VISIBLE);
                    streamTextView.setVisibility(View.INVISIBLE);
                    streamSecsTextView.setVisibility(View.INVISIBLE);
                    break;
                case ERROR:
                    infoTextView.setVisibility(View.VISIBLE);
                    errorText.setVisibility(View.INVISIBLE);
                    streamSwitch.setEnabled(false);
                    synthesizeButton.setEnabled(false);
                    synthesizeEditText.setEnabled(true);
                    synthesizeProgress.setVisibility(View.INVISIBLE);
                    streamTextView.setVisibility(View.INVISIBLE);
                    streamSecsTextView.setVisibility(View.INVISIBLE);
                    break;
                case FATAL_ERROR:
                    infoTextView.setVisibility(View.INVISIBLE);
                    errorText.setVisibility(View.VISIBLE);
                    streamSwitch.setEnabled(false);
                    synthesizeButton.setEnabled(false);
                    synthesizeEditText.setEnabled(false);
                    synthesizeProgress.setVisibility(View.INVISIBLE);
                    streamTextView.setVisibility(View.INVISIBLE);
                    streamSecsTextView.setVisibility(View.INVISIBLE);
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
            if (text.length() > orca.getMaxCharacterLimit()) {
                runOnUiThread(() -> {
                    setUIState(UIState.ERROR);
                    infoTextView.setText("Too many characters");
                });
            } else {
                Set<Character> invalidChars = new HashSet<>();

                if (BuildConfig.FLAVOR.startsWith("ko")) {
                    text = ValidateTextHelper.filterValidCharsKo(text);
                } else if (BuildConfig.FLAVOR.startsWith("ja")) {
                    text = ValidateTextHelper.filterValidCharsJa(text);
                }

                Matcher m = validationRegex.matcher(text);
                while (m.find()) {
                    invalidChars.add(text.charAt(m.start()));
                }

                if (invalidChars.size() == 0) {
                    runOnUiThread(() -> {
                        setUIState(UIState.EDIT);
                        synthesizeButton.setEnabled(true);
                    });
                } else {
                    StringBuilder sb = new StringBuilder();
                    for (Character c : invalidChars) {
                        if (sb.length() > 0) {
                            sb.append(", ");
                        }
                        sb.append(c);
                    }
                    if (orcaStream == null) {
                        runOnUiThread(() -> {
                            setUIState(UIState.ERROR);
                            infoTextView.setText(String.format(
                                    "Invalid characters in text: [%s]",
                                    sb
                            ));
                        });
                    } else {
                        runOnUiThread(() -> {
                            infoTextView.setVisibility(View.VISIBLE);
                            infoTextView.setText(String.format(
                                    "Invalid characters in text will be ignored: [%s]",
                                    sb
                            ));
                        });
                    }
                }
            }
        } else {
            runOnUiThread(() -> synthesizeButton.setEnabled(false));
            infoTextView.setText("");
        }
    }

    private void runSynthesis(final String text) {
        runOnUiThread(() -> {
            setUIState(UIState.BUSY);
            infoTextView.setText("Synthesizing...");
            errorText.setText("");
        });
        executor.submit(() -> {
            try {
                orca.synthesizeToFile(
                        text,
                        synthesizedFilePath,
                        new OrcaSynthesizeParams.Builder().build());
            } catch (OrcaException e) {
                mainHandler.post(() -> onOrcaException(e));
            }

            synthesizedPlayer.reset();
            synthesizedPlayer.setAudioAttributes(
                    new AudioAttributes.Builder()
                            .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                            .setUsage(AudioAttributes.USAGE_MEDIA)
                            .build()
            );
            synthesizedPlayer.setLooping(false);
            synthesizedPlayer.setOnCompletionListener(mediaPlayer -> {
                mainHandler.post(() -> synthesizeButton.setChecked(false));
                stopPlayback();
            });
            try {
                synthesizedPlayer.setDataSource(synthesizedFilePath);
                synthesizedPlayer.prepare();
                synthesizedPlayer.setVolume(1f, 1f);
                previousText = text;
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

    public void onStreamSwitchClick(View view) {
        if (orca == null) {
            displayError("Orca is not initialized");
            streamSwitch.setChecked(false);
            return;
        }

        try {
            if (orcaStream == null) {
                orcaStream = orca.streamOpen(new OrcaSynthesizeParams.Builder().build());
                runOnUiThread(() -> {
                    synthesizeEditText.setText("");
                    streamSecsTextView.setText("");
                    streamSecsTextView.setVisibility(View.VISIBLE);
                });
            } else {
                orcaStream.close();
                orcaStream = null;
                runOnUiThread(() -> {
                    synthesizeEditText.setText("");
                    streamSecsTextView.setVisibility(View.INVISIBLE);
                });
            }
        } catch (OrcaException e) {
            onOrcaException(e);
        }
    }

    public void onSynthesizeClick(View view) {
        if (orca == null) {
            displayError("Orca is not initialized");
            synthesizeButton.setChecked(false);
            return;
        }

        String text = synthesizeEditText.getText().toString();
        if (orcaStream == null) {
            if (synthesizeButton.isChecked()) {
                if (!previousText.equals(text)) {
                    runSynthesis(text);
                } else {
                    startPlayback();
                }
            } else {
                stopPlayback();
            }
        } else {
            runStreamSynthesis(text);
        }
    }

    private void runStreamSynthesis(final String text) {
        setUIState(UIState.STREAMING_PLAYBACK);

        AtomicBoolean isStreamingText = new AtomicBoolean(false);
        ArrayList<String> textStream = new ArrayList<>();

        AtomicBoolean isQueueingStreamingPcm = new AtomicBoolean(false);
        ConcurrentLinkedQueue<short[]> pcmQueue = new ConcurrentLinkedQueue<>();
        CountDownLatch streamingSynthesisLatch = new CountDownLatch(1);
        CountDownLatch streamingAudioLatch = new CountDownLatch(1);

        executor.submit(() -> {
            mainHandler.post(() -> {
                streamTextView.setText("");
            });

            isStreamingText.set(true);

            String[] words = text.split(" ");
            for (String word : words) {
                word += " ";
                String finalWord = word;
                mainHandler.post(() -> {
                    textStream.add(finalWord);
                    streamTextView.append(finalWord);
                });
                streamingSynthesisLatch.countDown();
                try {
                    Thread.sleep(100);
                } catch (InterruptedException ignored) {
                }
            }

            isStreamingText.set(false);
        });

        executorStreamingSynthesis.submit(() -> {
            try {
                mainHandler.post(() -> {
                    streamSecsTextView.setText("Seconds of audio synthesized: 0.000s");
                    synthesizeButton.setEnabled(false);
                });

                int numIterations = 0;
                boolean isPcmPlayStarted = false;
                float secs = 0;
                isQueueingStreamingPcm.set(true);

                streamingSynthesisLatch.await();
                while (isStreamingText.get() || !textStream.isEmpty()) {
                    if (!textStream.isEmpty()) {
                        String word = textStream.remove(0);
                        try {
                            short[] pcm = orcaStream.synthesize(word);
                            if (pcm != null && pcm.length > 0) {
                                pcmQueue.add(pcm);
                                secs += (float) pcm.length / orca.getSampleRate();
                                float finalSecs = secs;
                                mainHandler.post(() -> streamSecsTextView.setText(String.format("Seconds of audio synthesized: %.3fs", finalSecs)));
                                if (numIterations == STREAMING_NUM_AUDIO_WAIT_CHUNKS) {
                                    streamingAudioLatch.countDown();
                                    isPcmPlayStarted = true;
                                }
                                numIterations++;
                            }
                        } catch (OrcaException e) {
                            mainHandler.post(() -> onOrcaException(e));
                        }
                    }
                }

                try {
                    short[] flushedPcm = orcaStream.flush();
                    if (flushedPcm != null && flushedPcm.length > 0) {
                        pcmQueue.add(flushedPcm);
                        secs += (float) flushedPcm.length / orca.getSampleRate();
                        float finalSecs = secs;
                        mainHandler.post(() -> streamSecsTextView.setText(String.format("Seconds of audio synthesized: %.3fs", finalSecs)));
                    }

                    if (!isPcmPlayStarted) {
                        streamingAudioLatch.countDown();
                    }
                } catch (OrcaException e) {
                    mainHandler.post(() -> onOrcaException(e));
                }

                isQueueingStreamingPcm.set(false);
            } catch (Exception e) {
                mainHandler.post(() -> displayError(e.toString()));
            }
        });

        executorStreamingAudio.submit(() -> {
            try {
                AudioTrack audioTrack = new AudioTrack(
                        AudioManager.STREAM_MUSIC,
                        orca.getSampleRate(),
                        AudioFormat.CHANNEL_OUT_MONO,
                        AudioFormat.ENCODING_PCM_16BIT,
                        AudioTrack.getMinBufferSize(
                                orca.getSampleRate(),
                                AudioFormat.CHANNEL_OUT_MONO,
                                AudioFormat.ENCODING_PCM_16BIT),
                        AudioTrack.MODE_STREAM);

                audioTrack.play();

                streamingAudioLatch.await();
                while (isQueueingStreamingPcm.get() || !pcmQueue.isEmpty()) {
                    if (!pcmQueue.isEmpty()) {
                        short[] pcm = pcmQueue.poll();
                        if (pcm != null && pcm.length > 0) {
                            audioTrack.write(pcm, 0, pcm.length);
                        }
                    }
                }

                audioTrack.stop();
                audioTrack.release();

                mainHandler.post(() -> {
                    setUIState(UIState.EDIT);
                    streamTextView.setText("");
                });
            } catch (Exception e) {
                mainHandler.post(() -> displayError(e.toString()));
            }
        });
    }

    private enum UIState {
        EDIT,
        PLAYBACK,
        STREAMING_PLAYBACK,
        BUSY,
        ERROR,
        FATAL_ERROR
    }
}
