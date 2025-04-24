#! /usr/bin/env node
//
// Copyright 2024 Picovoice Inc.
//
// You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
// file accompanying this source.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
// an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
//
'use strict';

const os = require('os');
const { program } = require('commander');
const { performance } = require('perf_hooks');
const { execSync } = require('child_process');
const tiktoken = require('tiktoken');

const { PvSpeaker } = require('@picovoice/pvspeaker-node');
const { Orca, OrcaActivationLimitReachedError } = require('@picovoice/orca-node');
const { getAvailableLanguages, getAvailableGenders, getModelPath } = require('./utils');

const availableLanguages = getAvailableLanguages()
const availableGenders = getAvailableGenders()

program
  .requiredOption(
    '-a, --access_key <string>',
    'AccessKey obtain from the Picovoice Console (https://console.picovoice.ai/)',
  )
  .requiredOption(
    '-t, --text_to_stream <string>',
    'Text to be streamed to Orca',
  )
  .option(
    '-l, --library_file_path <string>',
    'Absolute path to dynamic library',
  )
  .option(
    '-m, --model_file_path <string>',
    'Absolute path to Orca model',
  )
  .option(
    '--language <string>',
    `The language you would like to run the demo in. Available languages are ${availableLanguages.join(", ")}.`,
  )
  .option(
    '--gender <string>',
    `The gender of the synthesized voice. Available genders are ${availableGenders.join(", ")}.`,
  )
  .option(
    '--tokens_per_second <number>',
    'Number of tokens per second to be streamed to Orca, simulating an LLM response',
    '15',
  )
  .option(
    '--audio_wait_chunks <number>',
    'Number of PCM chunks to wait before starting to play audio',
  )
  .option(
    '--buffer_size_secs <number>',
    'The size in seconds of the internal buffer used by PvSpeaker to play audio',
    '20',
  )
  .option(
    '--audio_device_index <number>',
    'Index of input audio device',
    '-1',
  )
  .option(
    '--show_audio_devices',
    'Only list available audio output devices and exit',
  );

if (process.argv.length < 3) {
  program.help();
}
program.parse(process.argv);
 
function splitText(text) {
  // TODO: Remove once tiktoken supports windows-arm64
  if (os.platform() === 'win32' && os.arch() === 'arm64') {
    const ALPHA_NUMERIC = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 '
    const PUNCTUATION = '!"#$%&\'()*+,-./:;<=>?@[\]^_`{|}~ '
    const tokensRaw = [ text[0] ]
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
  } else {
    const textDecoder = new TextDecoder();
    const encoder = tiktoken.encoding_for_model('gpt-4');
    const tokensRaw = Array.from(encoder.encode(text), e => textDecoder.decode(encoder.decode([e])));
    encoder.free();
    return tokensRaw;
  }
}

function tokenizeText(text) {
  const CUSTOM_PRON_PATTERN = /\{(.*?\|.*?)}/g;
  const CUSTOM_PRON_PATTERN_NO_WHITESPACE = /\{(.*?\|.*?)}(?!\s)/g;

  text = text.replace(CUSTOM_PRON_PATTERN_NO_WHITESPACE, '{$1} ');
  let customPronunciations = text.match(CUSTOM_PRON_PATTERN) || [];
  customPronunciations = new Set(customPronunciations);

  const tokensRaw = splitText(text);

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

function linuxMachine() {
  const machine = os.arch();
  let archInfo = '';
  if (machine === 'x64') {
    return machine;
  } else if (['arm64', 'arm'].includes(machine)) {
    if (os.arch() === 'x64') {
      archInfo = `-${machine}`;
    }
  } else {
    throw new Error(`Unsupported CPU architecture: ${machine}`);
  }

  let cpuInfo = '';
  let cpuPart = '';
  try {
    cpuInfo = execSync('cat /proc/cpuinfo').toString();
    const cpuPartList = cpuInfo.split('\n').filter(line => line.includes('CPU part'));
    cpuPart = cpuPartList[0].split(' ').pop().toLowerCase();
  } catch (e) {
    throw new Error(`Failed to identify the CPU with ${e.message}\nCPU info: ${cpuInfo}`);
  }

  switch (cpuPart) {
    case '0xd03':
      return 'cortex-a53' + archInfo;
    case '0xd08':
      return 'cortex-a72' + archInfo;
    case '0xd0b':
      return 'cortex-a76' + archInfo;
    default:
      throw new Error(`Unsupported CPU: ${cpuPart}`);
  }
}

function sleepSecs(ms) {
  return new Promise(resolve => setTimeout(resolve, ms * 1000));
}

async function streamingDemo() {
  let accessKey = program['access_key'];
  let language = program['language'];
  let gender = program['gender'];
  let libraryFilePath = program['library_file_path'];
  let modelFilePath = program['model_file_path'];
  let text = program['text_to_stream'];
  let tokensPerSeconds = program['tokens_per_second'];
  let audioWaitChunks = program['audio_wait_chunks'];
  let bufferSizeSecs = Number(program['buffer_size_secs']);
  let deviceIndex = Number(program['audio_device_index']);
  let showAudioDevices = program['show_audio_devices'];

  if (showAudioDevices) {
    const devices = PvSpeaker.getAvailableDevices();
    for (let i = 0; i < devices.length; i++) {
      console.log(`index: ${i}, device name: ${devices[i]}`);
    }
    return;
  }

  if (audioWaitChunks === undefined || audioWaitChunks === null) {
    audioWaitChunks = 0;
    if (os.platform() === 'linux') {
      const machine = linuxMachine();
      if (machine.includes('cortex')) {
        audioWaitChunks = 1;
      }
    }
  }

  if (!modelFilePath) {
    if (!availableLanguages.includes(language)) {
      throw new Error(
          `Given argument --language '${language}' is not an available language. ` +
          `Available languages are ${availableLanguages.join(", ")}.`)
    }

    if (!availableGenders.includes(gender)) {
      throw new Error(
          `Given argument --gender '${gender}' is not an available gender. ` +
          `Available genders are ${availableGenders.join(", ")}.`)
    }

    modelFilePath = getModelPath(language, gender);
  }

  try {
    const orca = new Orca(
      accessKey,
      {
        'modelPath': modelFilePath,
        'libraryPath': libraryFilePath,
      },
    );
    console.log(`\nOrca version: ${orca.version}`);
    const stream = orca.streamOpen();

    let speaker = null;

    try {
      const bitsPerSample = 16;
      speaker = new PvSpeaker(orca.sampleRate, bitsPerSample, { bufferSizeSecs, deviceIndex });
      speaker.start();
    } catch (e) {
      console.error('\nNote: External package \'@picovoice/pvspeaker-node\' failed to initialize.' +
        ' Orca will generate the pcm, but it will not be played to your speakers.');
    }

    let pcmBuffer = [];
    let numAudioChunks = 0;
    let isStartedPlaying = false;

    process.stdout.write('\nSimulated text stream: ');

    let timeFirstAudioAvailable = null;
    const tokens = tokenizeText(text);

    const startTime = performance.now();
    for (const token of tokens) {
      process.stdout.write(token);
      const pcm = stream.synthesize(token);
      if (pcm !== null) {
        if (timeFirstAudioAvailable === null) {
          timeFirstAudioAvailable = ((performance.now() - startTime) / 1000).toFixed(2);
        }
        pcmBuffer.push(...pcm);
        numAudioChunks++;
      }

      if (pcmBuffer.length > 0 && speaker !== null && (isStartedPlaying || numAudioChunks >= audioWaitChunks)) {
        const arrayBuffer = new Int16Array(pcmBuffer).buffer;
        const written = speaker.write(arrayBuffer);
        pcmBuffer = pcmBuffer.slice(written);
        isStartedPlaying = true;
      }

      await sleepSecs(1 / tokensPerSeconds);
    }

    const flushedPcm = stream.flush();
    if (flushedPcm !== null) {
      if (timeFirstAudioAvailable === null) {
        timeFirstAudioAvailable = ((performance.now() - startTime) / 1000).toFixed(2);
      }
      pcmBuffer.push(...flushedPcm);
    }
    const elapsedTime = ((performance.now() - startTime) / 1000).toFixed(2);

    console.log(`\n\nTime to finish text stream: ${elapsedTime} seconds`);
    console.log(`Time to receive first audio: ${timeFirstAudioAvailable} seconds after text stream started`);
    console.log('\nWaiting for audio to finish...');

    if (speaker !== null) {
      const arrayBuffer = new Int16Array(pcmBuffer).buffer;
      speaker.flush(arrayBuffer);
      speaker.stop();
      speaker.release();
    }
    stream.close();
    orca.release();
  } catch (err) {
    if (err instanceof OrcaActivationLimitReachedError) {
      console.error(`AccessKey '${accessKey}' has reached it's processing limit.`);
    } else {
      console.error(err);
    }
  }
}

void streamingDemo();
