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
const si = require('systeminformation');
const { program } = require('commander');
const { performance } = require('perf_hooks');
const { execSync } = require('child_process');
const tiktoken = require('tiktoken');
const convert = require('pcm-convert');

const { Orca, OrcaActivationLimitReachedError } = require('@picovoice/orca-node');

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
    'Absolute path to orca model',
  )
  .option(
    '--tokens_per_second <number>',
    'Number of tokens per second to be streamed to Orca, simulating an LLM response',
    '15',
  )
  .option(
    '--audio_wait_chunks <number>',
    'Number of PCM chunks to wait before starting to play audio',
  );

if (process.argv.length < 2) {
  program.help();
}
program.parse(process.argv);

function tokenizeText(text) {
  const CUSTOM_PRON_PATTERN = /\{(.*?\|.*?)}/g;
  const CUSTOM_PRON_PATTERN_NO_WHITESPACE = /\{(.*?\|.*?)}(?!\s)/g;

  text = text.replace(CUSTOM_PRON_PATTERN_NO_WHITESPACE, '{$1} ');
  let customPronunciations = text.match(CUSTOM_PRON_PATTERN) || [];
  customPronunciations = new Set(customPronunciations);

  const encoder = tiktoken.encoding_for_model('gpt-4');
  const tokensRaw = Array.from(encoder.encode(text), e => encoder.decode([e]));
  encoder.free();

  let customPron = '';
  const tokensWithCustomPronunciations = [];
  const textDecoder = new TextDecoder();

  tokensRaw.forEach((t, i) => {
    const token = textDecoder.decode(t);
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
    case '0xd07':
      return 'cortex-a57' + archInfo;
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
  let libraryFilePath = program['library_file_path'];
  let modelFilePath = program['model_file_path'];
  let text = program['text_to_stream'];
  let tokensPerSeconds = program['tokens_per_second'];
  let audioWaitChunks = program['audio_wait_chunks'];

  if (audioWaitChunks === undefined || audioWaitChunks === null) {
    audioWaitChunks = 0;
    if (os.platform() === 'linux') {
      const machine = linuxMachine();
      if (machine.includes('cortex')) {
        audioWaitChunks = 1;
      }
    }
  }

  try {
    let engineInstance = new Orca(
      accessKey,
      {
        'modelPath': modelFilePath,
        'libraryPath': libraryFilePath,
      },
    );
    console.log(`\nOrca version: ${engineInstance.version}`);
    const stream = engineInstance.streamOpen();

    let speaker = null;

    const initSpeaker = () => {
      const Speaker = require('speaker');
      speaker = new Speaker({
        channels: 1,
        bitDepth: 8,
        sampleRate: engineInstance.sampleRate,
      });
    };

    await si.audio((devices) => {
      if (devices.length > 0) {
        console.log(`Playing from device: ${devices[0].name}`);
        initSpeaker();
      } else {
        console.log('No sound card(s) detected. Orca will generate the pcm, but no audio will be played.');
      }
    });

    const pcmBuffer = [];

    function playStream() {
      if (pcmBuffer.length === 0) return;

      const pcmInt16 = pcmBuffer.shift();

      // for some reason, "speaker" does not accept Int16Array
      const pcmUint8 = convert(pcmInt16, 'int16', 'uint8');
      try {
        speaker?.write(pcmUint8);
      } catch (e) {
        console.log(`Unable to play audio: ${e}`);
      }

      playStream();
    }

    process.stdout.write('\nSimulated text stream: ');

    const startTime = performance.now();
    let timeFirstAudioAvailable = null;
    const tokens = tokenizeText(text);

    for (const token of tokens) {
      process.stdout.write(token);
      const pcm = stream.synthesize(token);
      if (pcm !== null) {
        if (timeFirstAudioAvailable === null) {
          timeFirstAudioAvailable = ((performance.now() - startTime) / 1000).toFixed(2);
        }
        pcmBuffer.push(pcm);
        if (pcmBuffer.length >= audioWaitChunks) {
          playStream();
        }
      }
      await sleepSecs(1 / tokensPerSeconds);
    }
    const elapsedTime = ((performance.now() - startTime) / 1000).toFixed(2);

    const flushedPcm = stream.flush();
    if (flushedPcm !== null) {
      if (timeFirstAudioAvailable === null) {
        timeFirstAudioAvailable = ((performance.now() - startTime) / 1000).toFixed(2);
      }
      pcmBuffer.push(flushedPcm);
      playStream();
    }

    console.log(`\n\nTime to finish text stream: ${elapsedTime} seconds`);
    console.log(`Time to receive first audio: ${timeFirstAudioAvailable} seconds after text stream started`);
    console.log('\nWaiting for audio to finish...');

    try {
      speaker?.end();
    } catch (e) {
      console.log(`Unable to close speaker: ${e}`);
    }
    stream.close();
    engineInstance?.release();
  } catch (err) {
    if (err instanceof OrcaActivationLimitReachedError) {
      console.error(`AccessKey '${accessKey}' has reached it's processing limit.`);
    } else {
      console.error(err);
    }
  }
}

void streamingDemo();
