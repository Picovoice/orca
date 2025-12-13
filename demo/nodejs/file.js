#! /usr/bin/env node
//
// Copyright 2024-2025 Picovoice Inc.
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
  .option(
    '-a, --access_key <string>',
    'AccessKey obtain from the Picovoice Console (https://console.picovoice.ai/)',
  )
  .option(
    '-t, --text <string>',
    'Text to be synthesized',
  )
  .option(
    '-o, --output_path <string>',
    'Absolute path to .wav file where the generated audio will be stored',
  )
  .option(
    '-m, --model_file_path <string>',
    'Absolute path to Orca model',
  )
  .option(
    "-y, --device <string>",
    "Device to run inference on (`best`, `cpu:{num_threads}` or `gpu:{gpu_index}`). Default: selects best device"
  )
  .option(
    '-l, --library_file_path <string>',
    'Absolute path to dynamic library',
  )
  .option(
    '-v, --verbose',
    'Verbose mode, prints metadata',
  )
  .option(
    "-z, --show_inference_devices",
    "Print devices that are available to run Orca inference.",
    false
  );

if (process.argv.length < 4) {
  program.help();
}
program.parse(process.argv);

const showInferenceDevices = program["show_inference_devices"];
if (showInferenceDevices) {
  console.log(Orca.listAvailableDevices().join('\n'));
  process.exit();
}

function fileDemo() {
  let accessKey = program['access_key'];
  let text = program['text'];
  let outputPath = program['output_path'];
  let libraryFilePath = program['library_file_path'];
  let modelFilePath = program['model_file_path'];
  let device = program["device"];
  let verbose = program['verbose'];

  if (accessKey === undefined || text === undefined || outputPath === undefined || modelFilePath === undefined) {
    console.error(
      "`--access_key` `output_path`, `model_file_path`, and `--text` are required arguments"
    );
    return;
  }
  let orca = new Orca(
    accessKey,
    {
      modelPath: modelFilePath,
      device: device,
      libraryPath: libraryFilePath,
    },
  );

  const loadPcm = (audioFile) => {
    const waveBuffer = fs.readFileSync(audioFile);
    const waveAudioFile = new WaveFile(waveBuffer);
    return waveAudioFile.getSamples(false, Int16Array);
  };

  try {
    const startTime = performance.now();
    const alignments = orca.synthesizeToFile(text, outputPath);
    const endTime = performance.now();
    const processingTime = ((endTime - startTime) / 1000).toFixed(2);
    const pcm = loadPcm(outputPath);
    const lengthSec = (pcm.length / orca.sampleRate).toFixed(2);

    console.log(`Orca took ${processingTime} seconds to synthesize ${lengthSec} seconds of speech which is ~${(lengthSec / processingTime).toFixed(2)} times faster than real-time.`);
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

  orca.release();
}

fileDemo();
