/*
    Copyright 2024-2025 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is
    located in the "LICENSE" file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the
    License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
    express or implied. See the License for the specific language governing permissions and
    limitations under the License.
*/

import { Orca, OrcaWorker } from '../';
import { OrcaError } from '../dist/types/orca_errors';
import { PvModel } from '@picovoice/web-utils';

// @ts-ignore
import orcaParamsMale from './orca_params_en_male';

// @ts-ignore
import orcaParamsFemale from './orca_params_en_female';

/* eslint camelcase: 0 */

import testData from '../cypress/fixtures/resources/.test/test_data.json';

const ACCESS_KEY = Cypress.env('ACCESS_KEY');
const DEVICE: string = Cypress.env('DEVICE');

const EXPECTED_MAX_CHARACTER_LIMIT = 2000;
const EXPECTED_SAMPLE_RATE = 22050;

const getAudioFileName = (model: string, synthesis_type: string): string => model.replace(".pv", `_${synthesis_type}.wav`);

const PCM_OUTLIER_THRESHOLD = 400;
const PCM_OUTLIER_COUNT_THRESHOLD = 0.05;

const validatePcm = (pcm: Int16Array, groundTruth: Int16Array) => {
  expect(pcm.length).gt(0);
  expect(pcm.length).eq(groundTruth.length);
  const diffPcm = pcm.map((a, i) => Math.abs(a - groundTruth[i]));
  const diffOutliers = diffPcm.filter(d => d > PCM_OUTLIER_THRESHOLD).length / diffPcm.length;
  expect(diffOutliers).lte(PCM_OUTLIER_COUNT_THRESHOLD);
};

const runInitTest = async (
  instance: typeof Orca | typeof OrcaWorker,
  params: {
    accessKey?: string;
    model?: PvModel;
    device?: string;
    expectFailure?: boolean;
  } = {},
) => {
  const {
    accessKey = ACCESS_KEY,
    model = { publicPath: `/test/orca_params_en_male.pv`, forceWrite: true },
    device = DEVICE,
    expectFailure = false,
  } = params;

  let orca: Orca | OrcaWorker | null = null;
  let isFailed = false;

  try {
    orca = await instance.create(accessKey, model, { device });
    expect(typeof orca.version).eq('string');
    expect(orca.version.length).gt(0);
    expect(orca.maxCharacterLimit).eq(EXPECTED_MAX_CHARACTER_LIMIT);
    expect(orca.sampleRate).eq(EXPECTED_SAMPLE_RATE);
    expect(orca.validCharacters.length).gt(0);
  } catch (e) {
    if (expectFailure) {
      isFailed = true;
    } else {
      expect(e).to.be.undefined;
    }
  }

  if (orca instanceof OrcaWorker) {
    orca.terminate();
  } else if (orca instanceof Orca) {
    await orca.release();
  }

  if (expectFailure) {
    expect(isFailed).to.be.true;
  } else {
    expect(isFailed).to.be.false;
  }
};

describe('Orca Binding', function() {
  for (const instance of [Orca, OrcaWorker]) {
    const instanceString = instance === Orca ? 'main' : 'worker';
    const testCaseString = `/test/orca_params_en_male.pv | ${instanceString}`;

    const publicPath = `/test/orca_params_en_male.pv`;

    it(`should be able to handle invalid public path (${testCaseString})`, async () => {
      await runInitTest(instance, {
        model: { publicPath: 'invalid', forceWrite: true },
        expectFailure: true,
      });
    });

    it(`should be able to handle invalid base64 (${testCaseString})`, async () => {
      await runInitTest(instance, {
        model: { base64: 'invalid', forceWrite: true },
        expectFailure: true,
      });
    });

    it(`should be able to handle invalid access key (${testCaseString})`, async () => {
      await runInitTest(instance, {
        accessKey: 'invalid',
        expectFailure: true,
      });
    });

    it(`should be able to handle invalid device (${testCaseString})`, async () => {
      await runInitTest(instance, {
        device: "cloud:9",
        expectFailure: true,
      });
    });

    it(`should be able to init with public path (${testCaseString})`, async () => {
      await runInitTest(instance, {
        model: { publicPath, forceWrite: true },
      });
    });

    it(`should be able to init with base64 (${testCaseString})`, async () => {
      await runInitTest(instance, {
        model: { base64: orcaParamsMale, forceWrite: true },
      });
    });

    it(`should be able to handle UTF-8 public path (${testCaseString})`, async () => {
      await runInitTest(instance, {
        model: { publicPath, forceWrite: true, customWritePath: '테스트' },
      });
    });

    it(`should return process and flush error message stack (${testCaseString})`, async () => {
      const orca = await Orca.create(
        ACCESS_KEY,
        { publicPath: publicPath, forceWrite: true },
        { device: DEVICE }
      );

      // @ts-ignore
      const objectAddress = orca._objectAddress;

      // @ts-ignore
      orca._objectAddress = 0;

      const errors: OrcaError[] = [];
      try {
        await orca.synthesize('test');
      } catch (e: any) {
        errors.push(e);
      }

      // @ts-ignore
      orca._objectAddress = objectAddress;
      await orca.release();

      expect(errors.length).to.be.gte(0);

      for (let i = 0; i < errors.length; i++) {
        expect((errors[i] as OrcaError).messageStack.length).to.be.gt(0);
        expect((errors[i] as OrcaError).messageStack.length).to.be.lte(8);
      }
    });

    it(`should return correct error message stack (${testCaseString})`, async () => {
      let messageStack = [];
      try {
        const orca = await instance.create('invalidAccessKey', {
          publicPath,
          forceWrite: true,
        },
        { device: DEVICE });
        expect(orca).to.be.undefined;
      } catch (e: any) {
        messageStack = e.messageStack;
      }

      expect(messageStack.length).to.be.gt(0);
      expect(messageStack.length).to.be.lte(8);

      try {
        const orca = await instance.create('invalidAccessKey', {
          publicPath,
          forceWrite: true,
        },
        { device: DEVICE });
        expect(orca).to.be.undefined;
      } catch (e: any) {
        expect(messageStack.length).to.be.eq(e.messageStack.length);
      }
    });
    it('List hardware devices', async () => {
      const hardwareDevices: string[] = await Orca.listAvailableDevices();
      expect(Array.isArray(hardwareDevices)).to.be.true;
      expect(hardwareDevices).length.to.be.greaterThan(0);
    });
  }
});

describe('Sentence Tests', function() {
  for (const testCase of testData.tests.sentence_tests) {
    for (const model of testCase.models) {
      for (const instance of [Orca, OrcaWorker]) {
        const instanceString = instance === Orca ? 'main' : 'worker';
        const testCaseString = `${testCase.language} | ${model} | ${instanceString}`;

        const publicPath = `/test/${model}`;

        it(`should be able to process text streaming (${testCaseString})`, () => {
          try {
            cy.getFramesFromFile(`${testData.audio_data_folder}${getAudioFileName(model, "stream")}`).then(
              async (rawPcm: Int16Array) => {
                const orca = await instance.create(
                  ACCESS_KEY,
                  { publicPath, forceWrite: true },
                  { device: DEVICE }
                );

                try {
                  const orcaStream = await orca.streamOpen({ randomState: testCase.random_state });

                  const streamPcm: number[] = [];
                  for (const c of testCase.text.split('')) {
                    const pcm = await orcaStream.synthesize(c);
                    if (pcm !== null) {
                      streamPcm.push(...pcm);
                    }
                  }

                  const endPcm = await orcaStream.flush();
                  if (endPcm !== null) {
                    streamPcm.push(...endPcm);
                  }

                  validatePcm(new Int16Array(streamPcm), rawPcm);
                  await orcaStream.close();
                } catch (e) {
                  expect(e).to.be.undefined;
                }

                if (orca instanceof OrcaWorker) {
                  orca.terminate();
                } else if (orca instanceof Orca) {
                  await orca.release();
                }
              },
            );
          } catch (e) {
            expect(e).to.be.undefined;
          }
        });

        it(`should be able to process - no punctuation (${testCaseString})`, async () => {
          try {
            const orca = await instance.create(
              ACCESS_KEY,
              { publicPath, forceWrite: true },
              { device: DEVICE }
            );

            const { pcm } = await orca.synthesize(testCase.text_no_punctuation);
            expect(pcm.length).gt(0);

            if (orca instanceof OrcaWorker) {
              orca.terminate();
            } else if (orca instanceof Orca) {
              await orca.release();
            }
          } catch (e) {
            expect(e).to.be.undefined;
          }
        });

        it(`should be able to process text (${testCaseString})`, () => {
          try {
            cy.getFramesFromFile(`${testData.audio_data_folder}${getAudioFileName(model, "single")}`).then(
              async rawPcm => {
                const orca = await instance.create(
                  ACCESS_KEY,
                  { publicPath, forceWrite: true },
                  { device: DEVICE }
                );

                const { pcm } = await orca.synthesize(
                  testCase.text,
                  { speechRate: 1, randomState: testCase.random_state },
                );
                validatePcm(pcm, rawPcm);

                if (orca instanceof OrcaWorker) {
                  orca.terminate();
                } else if (orca instanceof Orca) {
                  await orca.release();
                }
              },
            );
          } catch (e) {
            expect(e).to.be.undefined;
          }
        });

        it(`should be able to process custom punctuation (${testCaseString})`, async () => {
          try {
            const orca = await instance.create(
              ACCESS_KEY,
              { publicPath, forceWrite: true },
              { device: DEVICE }
            );

            const { pcm } = await orca.synthesize(testCase.text_custom_pronunciation);
            expect(pcm.length).gt(0);

            if (orca instanceof OrcaWorker) {
              orca.terminate();
            } else if (orca instanceof Orca) {
              await orca.release();
            }
          } catch (e) {
            expect(e).to.be.undefined;
          }
        });

        it(`should be able to handle different speech rates (${testCaseString})`, async () => {
          try {
            const orca = await instance.create(
              ACCESS_KEY,
              { publicPath, forceWrite: true },
              { device: DEVICE }
            );

            const { pcm: pcmSlow } = await orca.synthesize(testCase.text, { speechRate: 0.7 });
            const { pcm: pcmFast } = await orca.synthesize(testCase.text, { speechRate: 1.3 });
            expect(pcmSlow.length).gt(pcmFast.length);

            if (orca instanceof OrcaWorker) {
              orca.terminate();
            } else if (orca instanceof Orca) {
              await orca.release();
            }
          } catch (e) {
            expect(e).to.be.undefined;
          }
        });

        /* eslint-disable-next-line cypress/no-async-tests */
        it(`should be able to handle max num characters (${testCaseString})`, async () => {
          // test takes a while specifically in some languages
          // set timeout decently high and only run in worker
          Cypress.config('defaultCommandTimeout', 300000);

          if (instanceString !== "worker") {
            return;
          }

          try {
            const orca = await instance.create(
              ACCESS_KEY,
              { publicPath, forceWrite: true },
              { device: DEVICE }
            );

            let maxNumChars = orca.maxCharacterLimit;
            if (model === "orca_params_ko_female.pv") {
              maxNumChars /= 2;
              return;
            }

            const { pcm } = await orca.synthesize('a'.repeat(maxNumChars));
            expect(pcm.length).gt(0);

            if (orca instanceof OrcaWorker) {
              orca.terminate();
            } else if (orca instanceof Orca) {
              await orca.release();
            }
          } catch (e) {
            expect(e).to.be.undefined;
          }
        });
      }
    }
  }
});

describe('Alignment Tests', function() {
  for (const testCase of testData.tests.alignment_tests) {
    for (const instance of [Orca, OrcaWorker]) {
      const instanceString = instance === Orca ? 'main' : 'worker';
      const testCaseString = `${testCase.language} | ${testCase.model} | ${instanceString}`;

      const publicPath = `/test/${testCase.model}`;

      it(`should be able to process alignment exact (${testCaseString})`, async () => {
        try {
          const orca = await instance.create(
            ACCESS_KEY,
            { publicPath, forceWrite: true },
            { device: DEVICE }
          );

          const {
            pcm,
            alignments,
          } = await orca.synthesize(testCase.text_alignment, { randomState: testCase.random_state });
          expect(pcm.length).gt(0);
          expect(alignments.length).eq(testCase.alignments.length);

          alignments.forEach((w, i) => {
            const { word, start_sec, end_sec, phonemes } = testCase.alignments[i];
            expect(w.word).eq(word);
            expect(w.startSec).closeTo(start_sec, 0.01);
            expect(w.endSec).closeTo(end_sec, 0.01);
            w.phonemes.forEach((p, j) => {
              expect(p.phoneme).eq(phonemes[j].phoneme);
              expect(p.startSec).closeTo(phonemes[j].start_sec, 0.01);
              expect(p.endSec).closeTo(phonemes[j].end_sec, 0.01);
            });
          });

          if (orca instanceof OrcaWorker) {
            orca.terminate();
          } else if (orca instanceof Orca) {
            await orca.release();
          }
        } catch (e) {
          expect(e).to.be.undefined;
        }
      });
    }
  }
});

describe('Invalid Tests', function() {
  for (const testCase of testData.tests.invalid_tests) {
    for (const model of testCase.models) {
      for (const instance of [Orca, OrcaWorker]) {
        const instanceString = instance === Orca ? 'main' : 'worker';
        const testCaseString = `${testCase.language} | ${model} | ${instanceString}`;

        const publicPath = `/test/${model}`;

        it(`should handle invalid input (${testCaseString})`, async () => {
          const orca = await instance.create(
            ACCESS_KEY,
            { publicPath, forceWrite: true },
            { device: DEVICE }
          );

          for (const failureCase of testCase.text_invalid) {
            try {
              await orca.synthesize(failureCase);
            } catch (e) {
              expect(e).not.to.be.undefined;
            }
          }

          if (orca instanceof OrcaWorker) {
            orca.terminate();
          } else if (orca instanceof Orca) {
            await orca.release();
          }
        });
      }
    }
  }
});
