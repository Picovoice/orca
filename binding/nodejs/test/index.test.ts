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

import * as fs from 'fs';
import { WaveFile } from 'wavefile';

import { getSystemLibraryPath } from '../src/platforms';
import { Orca, OrcaAlignment, OrcaInvalidArgumentError, OrcaPhoneme, OrcaSynthesizeParams } from '../src';
import { getAudioFile, getTestData, getModelPath } from './test_utils';

const ACCESS_KEY = process.argv
  .filter(x => x.startsWith('--access_key='))[0]
  .split('--access_key=')[1];

// const EXACT_ALIGNMENT_TEST_MODEL = getModelPathFemale();

// const models = [
//   {
//     modelFilePath: getModelPathFemale(),
//     wavPathSingle: 'orca_params_female_single.wav',
//     wavPathStream: 'orca_params_female_stream.wav',
//   },
//   {
//     modelFilePath: getModelPathMale(),
//     wavPathSingle: 'orca_params_male_single.wav',
//     wavPathStream: 'orca_params_male_stream.wav',
//   },
// ];

const testData = getTestData()

// const testDataText = getTestData().test_sentences.text;
// const testDataNoPunctuation = getTestData().test_sentences.text_no_punctuation;
// const testDataCustomPronunciation = getTestData().test_sentences.text_custom_pronunciation;
// const testDataAlignment = getTestData().test_sentences.text_alignment;
// const testDataInvalid = getTestData().test_sentences.text_invalid;
// const testDataRandomState = getTestData().random_state;
// const testDataAlignments = getTestData().alignments;

const getAudioFileName = (model: string, synthesis_type: string): string => {
  return model.replace(".pv", `_${synthesis_type}.wav`)
}

const loadPcm = (audioFile: string): any => {
  const waveFilePath = getAudioFile(audioFile);
  const waveBuffer = fs.readFileSync(waveFilePath);
  const waveAudioFile = new WaveFile(waveBuffer);

  return waveAudioFile.getSamples(false, Int16Array);
};

const validatePcm = (pcm: Int16Array, groundTruth: Int16Array) => {
  expect(pcm.length).toBeGreaterThan(0);
  expect(pcm.length).toEqual(groundTruth.length);
  for (let i = 0; i < pcm.length; i++) {
    expect(Math.abs(pcm[i] - groundTruth[i])).toBeLessThanOrEqual(8000);
  }
};

const validatePhonemes = (phonemes: OrcaPhoneme[]) => {
  expect(phonemes.length).toBeGreaterThanOrEqual(0);
  for (let i = 0; i < phonemes.length; i++) {
    expect(phonemes[i].phoneme.length).toBeGreaterThan(0);
    expect(phonemes[i].startSec).toBeGreaterThanOrEqual(0);
    expect(phonemes[i].startSec).toBeLessThanOrEqual(phonemes[i].endSec);
    if (i < phonemes.length - 1) {
      expect(phonemes[i].endSec).toBeLessThanOrEqual(phonemes[i + 1].startSec);
    }
  }
};

const validateAlignments = (alignments: OrcaAlignment[]) => {
  expect(alignments.length).toBeGreaterThanOrEqual(0);
  for (let i = 0; i < alignments.length; i++) {
    expect(alignments[i].word.length).toBeGreaterThan(0);
    expect(alignments[i].startSec).toBeGreaterThanOrEqual(0);
    expect(alignments[i].startSec).toBeLessThanOrEqual(alignments[i].endSec);
    if (i < alignments.length - 1) {
      expect(alignments[i].endSec).toBeLessThanOrEqual(alignments[i + 1].startSec);
    }
    validatePhonemes(alignments[i].phonemes);
  }
};

const validateAlignmentsExact = (alignments: OrcaAlignment[], expectedAlignments: any[]) => {
  alignments.forEach((alignment, i) => {
    expect(alignment.word).toEqual(expectedAlignments[i].word);
    expect(alignment.startSec).toBeCloseTo(expectedAlignments[i].start_sec, 2);
    expect(alignment.endSec).toBeCloseTo(expectedAlignments[i].end_sec, 2);
    const expectedPhonemes = expectedAlignments[i].phonemes;
    alignment.phonemes.forEach((phoneme, j) => {
      expect(phoneme.phoneme).toEqual(expectedPhonemes[j].phoneme);
      expect(phoneme.startSec).toBeCloseTo(expectedPhonemes[j].start_sec, 2);
      expect(phoneme.endSec).toBeCloseTo(expectedPhonemes[j].end_sec, 2);
    });
  });
};

describe('properties', () => {
  describe.each<any>(testData.tests.sentence_tests)('$language', ({language, models}) => {
    describe.each<any>(models)('%s', (model) => {
      it('version', () => {
        const orcaEngine = new Orca(ACCESS_KEY, { modelPath: getModelPath(model) });
        expect(typeof orcaEngine.version).toEqual('string');
        expect(orcaEngine.version.length).toBeGreaterThan(0);
        orcaEngine.release();
      });

      it('sample rate', () => {
        const orcaEngine = new Orca(ACCESS_KEY, { modelPath: getModelPath(model) });
        expect(orcaEngine.sampleRate).toBeGreaterThan(0);
        orcaEngine.release();
      });

      it('valid characters', () => {
        const orcaEngine = new Orca(ACCESS_KEY, { modelPath: getModelPath(model) });
        expect(orcaEngine.validCharacters.length).toBeGreaterThan(0);
        expect(orcaEngine.validCharacters.every(x => typeof x === 'string')).toBeTruthy();
        expect(orcaEngine.validCharacters.includes(',')).toBeTruthy();
        orcaEngine.release();
      });

      it('max character limit', () => {
        const orcaEngine = new Orca(ACCESS_KEY, { modelPath: getModelPath(model) });
        expect(orcaEngine.maxCharacterLimit).toBeGreaterThan(0);
        orcaEngine.release();
      });
    })
  })
});

describe('sentences', () => {
  describe.each<any>(testData.tests.sentence_tests)('$language', ({language, models, random_state, text, text_no_punctuation, text_custom_pronunciation}) => {
    describe.each<any>(models)('%s', (model) => {
      it('synthesize', () => {
        const orcaEngine = new Orca(ACCESS_KEY, { modelPath: getModelPath(model) });
        const { pcm, alignments } = orcaEngine.synthesize(
          text,
          { speechRate: 1, randomState: random_state },
        );
        const groundTruth = loadPcm(getAudioFileName(model, "single"));
        validatePcm(pcm, groundTruth);
        validateAlignments(alignments);
        orcaEngine.release();
      });

      it('synthesize with no punctuation', () => {
        const orcaEngine = new Orca(ACCESS_KEY, { modelPath: getModelPath(model) });

        try {
          const { pcm, alignments } = orcaEngine.synthesize(text_no_punctuation);
          expect(pcm.length).toBeGreaterThan(0);
          validateAlignments(alignments);
        } catch (err) {
          expect(err).toBeUndefined();
        }

        orcaEngine.release();
      });

      it('synthesize with custom punctuation', () => {
        const orcaEngine = new Orca(ACCESS_KEY, { modelPath: getModelPath(model) });

        try {
          const { pcm, alignments } = orcaEngine.synthesize(text_custom_pronunciation);
          expect(pcm.length).toBeGreaterThan(0);
          validateAlignments(alignments);
        } catch (err) {
          expect(err).toBeUndefined();
        }

        orcaEngine.release();
      });

      it('synthesize to file', () => {
        const orcaEngine = new Orca(ACCESS_KEY, { modelPath: getModelPath(model) });
        const filePath = './orca-temp.wav';
        const alignments = orcaEngine.synthesizeToFile(text, filePath);
        validateAlignments(alignments);
        expect(fs.existsSync(filePath)).toBeTruthy();
        orcaEngine.release();
      });

      it('streaming synthesis', () => {
        const orcaEngine = new Orca(ACCESS_KEY, { modelPath: getModelPath(model) });
        const orcaStream = orcaEngine.streamOpen({ randomState: random_state });
        const fullPcm: number[] = [];
        for (const char of text) {
          const streamPcm = orcaStream.synthesize(char);
          if (streamPcm !== null) {
            fullPcm.push(...streamPcm);
          }
        }
        const flushedPcm = orcaStream.flush();
        if (flushedPcm !== null) {
          fullPcm.push(...flushedPcm);
        }
        orcaStream.close();
        const pcm = new Int16Array(fullPcm);
        const groundTruth = loadPcm(getAudioFileName(model, "stream"));
        validatePcm(pcm, groundTruth);
        orcaEngine.release();
      });

      it('speech rate', () => {
        const orcaEngine = new Orca(ACCESS_KEY, { modelPath: getModelPath(model) });
        const { pcm: pcmSlow } = orcaEngine.synthesize(text, { speechRate: 0.7 });
        const { pcm: pcmFast } = orcaEngine.synthesize(text, { speechRate: 1.3 });
        expect(pcmSlow.length).toBeGreaterThan(0);
        expect(pcmFast.length).toBeGreaterThan(0);
        expect(pcmSlow.length).toBeGreaterThan(pcmFast.length);
        orcaEngine.release();
      });
    })
  })
});

describe('alignments', () => {
  describe.each<any>(testData.tests.alignment_tests)('$language $model', ({language, model, random_state, text_alignment, alignments: expectedAlignments}) => {
    it('synthesize alignment', () => {
      const orcaEngine = new Orca(ACCESS_KEY, { modelPath: getModelPath(model) });
      const { pcm, alignments } = orcaEngine.synthesize(text_alignment, { randomState: random_state });
      expect(pcm.length).toBeGreaterThan(0);
      validateAlignmentsExact(alignments, expectedAlignments);
      orcaEngine.release();
    });
  })
});

describe('invalids', () => {
  describe.each<any>(testData.tests.invalid_tests)('$language', ({language, models, text_invalid}) => {
    describe.each<any>(models)('%s', (model) => {
      it('invalid input', () => {
        text_invalid.forEach((sentence: string) => {
          const orcaEngine = new Orca(ACCESS_KEY, { modelPath: getModelPath(model) });
          try {
            orcaEngine.synthesize(sentence);
          } catch (e) {
            expect(e).toBeDefined();
          }
        });
      });
    })
  })
});


describe('Defaults', () => {
  test('Empty AccessKey', () => {
    expect(() => {
      new Orca('');
    }).toThrow(OrcaInvalidArgumentError);
  });
});

describe('manual paths', () => {
  test('manual library path', () => {
    const libraryPath = getSystemLibraryPath();

    let orcaEngine = new Orca(
      ACCESS_KEY,
      { libraryPath },
    );

    let { pcm, alignments } = orcaEngine.synthesize(
      "test text",
      { randomState: 42 },
    );

    expect(pcm.length).toBeGreaterThan(0);
    expect(alignments.length).toBeGreaterThan(0);

    orcaEngine.release();
  });
});

describe('error message stack', () => {
  test('message stack cleared after read', () => {
    let error: string[] = [];
    try {
      new Orca('invalid');
    } catch (e: any) {
      error = e.messageStack;
    }

    expect(error.length).toBeGreaterThan(0);
    expect(error.length).toBeLessThanOrEqual(8);

    try {
      new Orca('invalid');
    } catch (e: any) {
      for (let i = 0; i < error.length; i++) {
        expect(error[i]).toEqual(e.messageStack[i]);
      }
    }
  });
});
