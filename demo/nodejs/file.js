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

const { program } = require('commander');
const fs = require('fs');
const { WaveFile } = require('wavefile');

const { Orca, OrcaActivationLimitReachedError } = require('@picovoice/orca-node');

program
  .requiredOption(
    '-a, --access_key <string>',
    'AccessKey obtain from the Picovoice Console (https://console.picovoice.ai/)',
  )
  .requiredOption(
    '-t, --text <string>',
    'Text to be synthesized',
  )
  .requiredOption(
    '-o, --output_path <string>',
    'Absolute path to .wav file where the generated audio will be stored',
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
    '-v, --verbose',
    'Verbose mode, prints metadata',
  );

if (process.argv.length < 3) {
  program.help();
}
program.parse(process.argv);

function fileDemo() {
  let accessKey = program['access_key'];
  let text = program['text'];
  let outputPath = program['output_path'];
  let libraryFilePath = program['library_file_path'];
  let modelFilePath = program['model_file_path'];
  let verbose = program['verbose'];

  let engineInstance = new Orca(
    accessKey,
    {
      'modelPath': modelFilePath,
      'libraryPath': libraryFilePath,
    },
  );

  const loadPcm = (audioFile) => {
    const waveBuffer = fs.readFileSync(audioFile);
    const waveAudioFile = new WaveFile(waveBuffer);
    return waveAudioFile.getSamples(false, Int16Array);
  };

  try {
    const startTime = performance.now();
    const alignments = engineInstance.synthesizeToFile(text, outputPath);
    const endTime = performance.now();
    const processingTime = ((endTime - startTime) / 1000).toFixed(2);
    const pcm = loadPcm(outputPath);
    const lengthSec = pcm.length / engineInstance.sampleRate;

    console.log(`Orca took ${processingTime} seconds to synthesize ${lengthSec} seconds of speech which is ~${lengthSec / processingTime} times faster than real-time.`);
    console.log(`Audio written to ${outputPath}.`);
    if (verbose) {
      console.table(
        alignments?.flatMap(alignment =>
          alignment.phonemes.map((phoneme, i) => {
            const row = {};
            if (i === 0) {
              row['Word'] = alignment.word;
              row['Word Start time (s)'] = alignment.startSec.toFixed(2);
              row['Word End time (s)'] = alignment.endSec.toFixed(2);
            }
            row['Phoneme'] = phoneme.phoneme;
            row['Phoneme Start time (s)'] = phoneme.startSec.toFixed(2);
            row['Phoneme End time (s)'] = phoneme.endSec.toFixed(2);
            return row;
          }),
        ),
      );
    }
  } catch (err) {
    if (err instanceof OrcaActivationLimitReachedError) {
      console.error(`AccessKey '${accessKey}' has reached it's processing limit.`);
    } else {
      console.error(err);
    }
  }

  engineInstance.release();
}

fileDemo();
