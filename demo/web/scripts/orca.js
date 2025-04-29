const ORCA_SAMPLE_RATE = 22050;

let orca = null;
let pcm = null;
let alignments = null;
let orcaStream = null;

function writeMessage(message) {
  console.log(message);
  document.getElementById("status").innerText = message;
}

window.onload = function () {
  const audioContext = new (window.AudioContext || window.webKitAudioContext)({
    sampleRate: ORCA_SAMPLE_RATE,
  });

  const originalAudioGain = audioContext.createGain();
  originalAudioGain.gain.value = 1;
  originalAudioGain.connect(audioContext.destination);

  function createBuffer(data) {
    const buffer = audioContext.createBuffer(1, data.length, ORCA_SAMPLE_RATE);
    const source = new Float32Array(data.length);
    for (let i = 0; i < data.length; i++) {
      source[i] = data[i] < 0 ? data[i] / 32768 : data[i] / 32767;
    }
    buffer.copyToChannel(source, 0);
    return buffer;
  }

  const chooseBtnsEl = document.getElementById("choose-btns");
  const chooseSingleBtnEl = document.getElementById("choose-single-btn");
  const chooseStreamBtnEl = document.getElementById("choose-stream-btn");
  const singleSynthesisEl = document.getElementById("single-synthesis");
  const streamSynthesisEl = document.getElementById("stream-synthesis");

  chooseSingleBtnEl.addEventListener("click", () => {
    chooseBtnsEl.style.display = "none";
    singleSynthesisEl.style.display = "block";
  });

  chooseStreamBtnEl.addEventListener("click", () => {
    chooseBtnsEl.style.display = "none";
    streamSynthesisEl.style.display = "block";
  });

  const textToSynthesizeEl = document.getElementById("text-to-synthesize");
  const textToSynthesizeNumCharsEl = document.getElementById(
    "text-to-synthesize-num-chars",
  );
  const textToSynthesizeErrorEl = document.getElementById(
    "text-to-synthesize-error",
  );
  const speechRateSliderEl = document.getElementById("speech-rate");
  const speechRateDisplayEl = document.getElementById("speech-rate-display");
  const synthesizeBtnEl = document.getElementById("synthesize-btn");
  const controlBtnEl = document.getElementById("control-btn");
  const downloadBtnEl = document.getElementById("download-btn");
  const alignmentsTableEl = document.getElementById("alignments-table");

  const streamTextToSynthesizeEl = document.getElementById(
    "stream-text-to-synthesize",
  );
  const streamTextDisplayEl = document.getElementById("stream-text-display");
  const streamSecondsDisplayEl = document.getElementById(
    "stream-seconds-display",
  );
  const streamTextToSynthesizeErrorEl = document.getElementById(
    "stream-text-to-synthesize-error",
  );
  const streamSpeechRateSliderEl =
    document.getElementById("stream-speech-rate");
  const streamSpeechRateDisplayEl = document.getElementById(
    "stream-speech-rate-display",
  );
  const streamOpenBtnEl = document.getElementById("stream-open-btn");
  const streamPlayBtnEl = document.getElementById("stream-play-btn");
  const streamCloseBtnEl = document.getElementById("stream-close-btn");

  function validateInput(input, validChars) {
    let textToValidate = input;
    if (orcaModel.publicPath.includes("ko")) {
      textToValidate = decomposeHangul(input);
    } else if (orcaModel.publicPath.includes("ja")) {
      textToValidate = filterValidCharsJa(input);
    }

    let nonAllowedCharacters = [];

    for (let i = 0; i < textToValidate.length; i++) {
      if (
        !validChars.includes(textToValidate[i]) &&
        !nonAllowedCharacters.includes(textToValidate[i])
      ) {
        nonAllowedCharacters.push(textToValidate[i]);
      }
    }

    if (nonAllowedCharacters.length > 0) {
      textToSynthesizeErrorEl.innerText = `Error: Characters ${JSON.stringify(nonAllowedCharacters)} are not allowed.`;
      streamTextToSynthesizeErrorEl.innerText = `Characters ${JSON.stringify(nonAllowedCharacters)} will be ignored.`;
      synthesizeBtnEl.disabled = true;
    } else {
      const text = "&nbsp;";
      textToSynthesizeErrorEl.innerHTML = text;
      streamTextToSynthesizeErrorEl.innerHTML = text;
      synthesizeBtnEl.disabled = false;
    }
  }

  // Single Synthesis
  let isPlaying = false;
  let originalAudioSource;

  textToSynthesizeEl.addEventListener("input", (e) => {
    textToSynthesizeNumCharsEl.innerText = e.target.value
      .trim()
      .length.toString();
  });

  function onSynthesizeParamChange() {
    if (orca !== null && isPlaying === false) {
      synthesizeBtnEl.disabled = false;
      controlBtnEl.disabled = true;
      downloadBtnEl.disabled = true;
      controlBtnEl.innerText = "Play";
    }
  }

  function setAlignmentsTable(alignments) {
    if (alignments === null) {
      alignmentsTableEl.style.display = "none";
      return;
    }

    alignmentsTableEl.style.display = "block";
    const rowCount = alignmentsTableEl.rows.length;
    for (let i = 1; i < rowCount; i++) {
      alignmentsTableEl.deleteRow(1);
    }

    alignments.forEach((a) => {
      const row = alignmentsTableEl.insertRow(-1);
      row.style.verticalAlign = "top";
      const word = row.insertCell(0);
      const start = row.insertCell(1);
      const end = row.insertCell(2);
      const phonemes = row.insertCell(3);

      word.innerHTML = `${a.word}`;
      start.innerHTML = `${a.startSec.toFixed(3)}`;
      end.innerHTML = `${a.endSec.toFixed(3)}`;
      const phonemesInnerHTML = a.phonemes
        .map((p) => {
          return `<tr>
              <td style="text-align: left">${p.phoneme}</td>
              <td style="text-align: left">[${p.startSec.toFixed(3)} - ${p.endSec.toFixed(3)}s]</td>
            </tr>`;
        })
        .join("");
      phonemes.innerHTML = `
            <table>
              <colgroup>
                <col span="1" style="width: 25%" />
                <col span="1" style="width: 75%" />
              </colgroup>
              ${phonemesInnerHTML}
            </table>
          `;
    });
  }

  async function synthesize() {
    const text = textToSynthesizeEl.value.trim();
    if (text === "") return;

    writeMessage("Synthesizing. Please wait...");
    try {
      textToSynthesizeEl.disabled = true;
      speechRateSliderEl.disabled = true;
      synthesizeBtnEl.disabled = true;
      controlBtnEl.disabled = true;
      downloadBtnEl.disabled = true;

      const result = await orca.synthesize(text, {
        speechRate: speechRateSliderEl.value,
      });

      pcm = result.pcm;
      setAlignmentsTable(result.alignments);
      writeMessage("Synthesizing complete!");

      controlBtnEl.disabled = false;
      downloadBtnEl.disabled = false;
    } catch (err) {
      writeMessage(err);
    } finally {
      textToSynthesizeEl.disabled = false;
      speechRateSliderEl.disabled = false;
    }
  }

  function onAudioStop() {
    isPlaying = false;
    controlBtnEl.innerText = "Play";
    textToSynthesizeEl.disabled = false;
    speechRateSliderEl.disabled = false;
    synthesizeBtnEl.disabled = false;
  }

  textToSynthesizeEl.addEventListener("input", (e) => {
    onSynthesizeParamChange();
    if (orca !== null) {
      validateInput(e.target.value, orca.validCharacters);
    }
  });

  speechRateSliderEl.addEventListener("change", () => {
    onSynthesizeParamChange();
    speechRateDisplayEl.innerText = speechRateSliderEl.value;
  });

  synthesizeBtnEl.addEventListener("click", async () => await synthesize());

  controlBtnEl.addEventListener("click", () => {
    if (pcm === null) return;

    if (!isPlaying) {
      originalAudioSource = audioContext.createBufferSource();
      originalAudioSource.addEventListener("ended", onAudioStop);
      originalAudioSource.buffer = createBuffer(pcm);
      originalAudioSource.connect(originalAudioGain);
      originalAudioSource.start();

      isPlaying = true;
      controlBtnEl.innerText = "Stop";
      textToSynthesizeEl.disabled = true;
      speechRateSliderEl.disabled = true;
      synthesizeBtnEl.disabled = true;
    } else {
      originalAudioSource.stop();
      onAudioStop();
    }
  });

  // Streaming Synthesis
  let isPlayingAudio = false;
  let isStreaming = false;
  const audioBuffer = [];
  let streamSource;

  function playStream() {
    if (isPlayingAudio) return;

    if (audioBuffer.length === 0) {
      if (!isStreaming) {
        streamPlayBtnEl.disabled = false;
        streamCloseBtnEl.disabled = false;
      }
      return;
    } else {
      streamPlayBtnEl.disabled = true;
      streamCloseBtnEl.disabled = true;
    }

    streamSource = audioContext.createBufferSource();

    streamSource.buffer = audioBuffer.shift();
    streamSource.connect(originalAudioGain);

    streamSource.onended = async () => {
      isPlayingAudio = false;
      playStream();
    };

    streamSource.start();
    isPlayingAudio = true;
  }

  async function streamOpen() {
    writeMessage("Opening stream. Please wait...");
    try {
      streamTextToSynthesizeEl.disabled = true;
      streamSpeechRateSliderEl.disabled = true;
      streamOpenBtnEl.disabled = true;

      orcaStream = await orca.streamOpen({
        speechRate: streamSpeechRateSliderEl.value,
      });

      streamTextToSynthesizeEl.disabled = false;
      streamCloseBtnEl.disabled = false;
      streamPlayBtnEl.disabled = false;

      writeMessage("Stream opened. Type in the input field!");
    } catch (err) {
      writeMessage(err);
    }
  }

  function splitText(text, language) {
    // TODO: Update once Orca supports passing in partial bytes
    if (language === "ko" || language === "ja") {
      return text.split("");
    } else {
      const ALPHA_NUMERIC = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 '
      const PUNCTUATION = '!"#$%&\'()*+,-./:;<=>?@[\]^_`{|}~ '
      const tokensRaw = [text[0]]
      for (let i = 1; i < text.length; i++) {
        let ch = text[i];
        let token = tokensRaw[tokensRaw.length - 1];
        if ((ALPHA_NUMERIC.includes(ch) && !ALPHA_NUMERIC.includes(token[token.length - 1])) || PUNCTUATION.includes(ch)) {
          tokensRaw.push(ch);
        } else {
          tokensRaw[tokensRaw.length - 1] += ch;
        }
      }
      return tokensRaw;
    }
  }

  function tokenizeText(text, language) {
    const CUSTOM_PRON_PATTERN = /\{(.*?\|.*?)}/g;
    const CUSTOM_PRON_PATTERN_NO_WHITESPACE = /\{(.*?\|.*?)}(?!\s)/g;

    text = text.replace(CUSTOM_PRON_PATTERN_NO_WHITESPACE, '{$1} ');
    let customPronunciations = text.match(CUSTOM_PRON_PATTERN) || [];
    customPronunciations = new Set(customPronunciations);

    const tokensRaw = splitText(text, language);

    let customPron = '';
    const tokensWithCustomPronunciations = [];

    tokensRaw.forEach((token, i) => {
      let inCustomPron = false;
      customPronunciations.forEach(pron => {
        const inCustomPronGlobal = customPron.length > 0;
        const currentMatch = !inCustomPronGlobal ? token.trim() : customPron + token;
        if (pron.startsWith(currentMatch)) {
          customPron += !inCustomPronGlobal ? token.trim() : token;
          inCustomPron = true;
        }
      });

      if (!inCustomPron) {
        if (customPron !== '') {
          tokensWithCustomPronunciations.push(i !== 0 ? ` ${customPron}` : customPron);
          customPron = '';
        }
        tokensWithCustomPronunciations.push(token);
      }
    });

    return tokensWithCustomPronunciations;
  }

  async function streamPlay() {
    writeMessage("Synthesizing and playing speech! Please listen for audio.");
    try {
      isStreaming = true;
      streamTextDisplayEl.innerText = "";
      streamSecondsDisplayEl.innerText = "0";

      const text = streamTextToSynthesizeEl.value;
      
      const languagePrefix = "orca_params_";
      const languageIdx = orcaModel.publicPath.indexOf(languagePrefix) + languagePrefix.length;
      const language = orcaModel.publicPath.substring(languageIdx, languageIdx + 2);
      const words = tokenizeText(text, language);
      let numIterations = 0;

      for (const word of words) {
        streamTextDisplayEl.innerText += word;
        const wordPcm = await orcaStream.synthesize(word);
        if (wordPcm !== null) {
          const curSecs = parseFloat(streamSecondsDisplayEl.innerText);
          const newSecs = wordPcm.length / orca.sampleRate;
          const time = curSecs + newSecs;
          streamSecondsDisplayEl.innerText = time.toFixed(3);
          audioBuffer.push(createBuffer(wordPcm));
          if (numIterations === 1 || !isPlayingAudio) {
            playStream();
          }
          numIterations++;
        }
        await new Promise((r) => setTimeout(r, 100));
      }

      const flushPcm = await orcaStream.flush();
      if (flushPcm !== null) {
        const curSecs = parseFloat(streamSecondsDisplayEl.innerText);
        const newSecs = flushPcm.length / orca.sampleRate;
        const time = curSecs + newSecs;
        streamSecondsDisplayEl.innerText = time.toFixed(3);
        audioBuffer.push(createBuffer(flushPcm));
        playStream();
      }
    } catch (err) {
      writeMessage(err);
    } finally {
      isStreaming = false;
    }
  }

  async function streamClose() {
    writeMessage("Closing stream. Please wait...");
    try {
      streamTextToSynthesizeEl.disabled = true;
      if (streamSource) {
        streamSource.stop();
      }

      await orcaStream.close();
      orcaStream = null;

      streamSpeechRateSliderEl.disabled = false;
      streamOpenBtnEl.disabled = false;
      streamPlayBtnEl.disabled = true;
      streamCloseBtnEl.disabled = true;
      streamTextToSynthesizeEl.value = "";
      writeMessage('Stream closed! Click "Open Stream" to begin.');
    } catch (err) {
      writeMessage(err);
    }
  }

  streamOpenBtnEl.addEventListener("click", async () => await streamOpen());
  streamPlayBtnEl.addEventListener("click", async () => await streamPlay());
  streamCloseBtnEl.addEventListener("click", async () => await streamClose());

  streamTextToSynthesizeEl.addEventListener("input", (e) => {
    if (orca !== null) {
      validateInput(e.target.value, orca.validCharacters);
    }
  });

  streamSpeechRateSliderEl.addEventListener("change", () => {
    streamSpeechRateDisplayEl.innerText = streamSpeechRateSliderEl.value;
  });
};

async function startOrca(accessKey) {
  writeMessage("Orca is loading. Please wait...");
  try {
    document.getElementById("start-orca").disabled = true;

    orca = await OrcaWeb.OrcaWorker.create(accessKey, orcaModel);

    document.getElementById("choose-btns").style.display = "block";
    const maxCharacterLimit = orca.maxCharacterLimit.toString();
    document.getElementById("max-char-limit").innerText = maxCharacterLimit;
    document.getElementById("text-to-synthesize").maxLength = maxCharacterLimit;
    document.getElementById("stream-text-to-synthesize").maxLength =
      maxCharacterLimit;
    writeMessage("Orca worker ready!");
  } catch (err) {
    writeMessage(err);
  }
}

function downloadDumpAudio() {
  let blob = new Blob([pcm]);
  let a = document.createElement("a");
  a.download = "orca_speech_audio.pcm";
  a.href = window.URL.createObjectURL(blob);
  a.click();
}

function decomposeHangul(input) {
  const HANGUL_UNICODE_BASE = 0xAC00;
  const HANGUL_DECOMPOSED_ARRAY = [
    // Initial consonants
    "ㄱ", "ㄲ", "ㄴ", "ㄷ", "ㄸ", "ㄹ", "ㅁ", "ㅂ", "ㅃ", "ㅅ", "ㅆ", "ㅇ", "ㅈ", "ㅉ", "ㅊ", "ㅋ", "ㅌ", "ㅍ", "ㅎ",
    // Medial vowels
    "ㅏ", "ㅐ", "ㅑ", "ㅒ", "ㅓ", "ㅔ", "ㅕ", "ㅖ", "ㅗ", "ㅘ", "ㅙ", "ㅚ", "ㅛ", "ㅜ", "ㅝ", "ㅞ", "ㅟ", "ㅠ", "ㅡ", "ㅢ", "ㅣ",
    // Final consonants
    "", "ㄱ", "ㄲ", "ㄳ", "ㄴ", "ㄵ", "ㄶ", "ㄷ", "ㄹ", "ㄺ", "ㄻ", "ㄼ", "ㄽ", "ㄾ", "ㄿ", "ㅀ", "ㅁ", "ㅂ", "ㅄ", "ㅅ", "ㅆ", "ㅇ", "ㅈ", "ㅊ", "ㅋ", "ㅌ", "ㅍ", "ㅎ"
  ];

  let decomposed = "";

  for (const char of input) {
    const codePoint = char.codePointAt(0);

    if (codePoint < HANGUL_UNICODE_BASE) {
      decomposed += String.fromCodePoint(codePoint);
      continue;
    }

    let curr = codePoint - HANGUL_UNICODE_BASE;
    const initial = Math.floor(curr / 588);

    curr %= 588;
    const medial = Math.floor(curr / 28) + 19;

    curr %= 28;
    const finalConsonant = curr + 19 + 21;

    if (initial > 18) {
      decomposed += String.fromCodePoint(codePoint);
      continue;
    }

    decomposed += HANGUL_DECOMPOSED_ARRAY[initial];
    decomposed += HANGUL_DECOMPOSED_ARRAY[medial];
    decomposed += HANGUL_DECOMPOSED_ARRAY[finalConsonant];
  }

  return decomposed;
}

function filterValidCharsJa(input) {
  let invalidChars = "";

  for (const char of input) {
    const codePoint = char.codePointAt(0);

    const isJapanese =
        (codePoint >= 0x3001 && codePoint <= 0x301F) || // punctuation
        (codePoint >= 0x3040 && codePoint <= 0x309F) || // hiragana
        (codePoint >= 0x30A0 && codePoint <= 0x30FF) || // katakana
        (codePoint >= 0x4E00 && codePoint <= 0x9FFF);   // kanji

    if (!isJapanese) {
      invalidChars += String.fromCodePoint(codePoint);
    }
  }

  return invalidChars;
}
