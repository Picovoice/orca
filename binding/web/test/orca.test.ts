import { Orca, OrcaWorker } from '../';
import { OrcaError } from '../dist/types/orca_errors';
import { PvModel } from '@picovoice/web-utils';

// @ts-ignore
import orcaParamsMale from './orca_params_male';

// @ts-ignore
import orcaParamsFemale from './orca_params_female';

/* eslint camelcase: 0 */

import testData from '../cypress/fixtures/resources/.test/test_data.json';

const ACCESS_KEY = Cypress.env('ACCESS_KEY');

const EXPECTED_MAX_CHARACTER_LIMIT = 2000;
const EXPECTED_SAMPLE_RATE = 22050;
const EXPECTED_VALID_CHARACTERS = [
  '.', ':', ',', '"', '?', '!', 'a',
  'b', 'c', 'd', 'e', 'f', 'g', 'h',
  'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', 'A', 'B', 'C',
  'D', 'E', 'F', 'G', 'H', 'I', 'J',
  'K', 'L', 'M', 'N', 'O', 'P', 'Q',
  'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', '\'', '{', '}', '|', ' ',
  '-', '/', '1', '2', '3', '4', '5',
  '6', '7', '8', '9', '0', '@', '%',
  '&', '\n', '_', '(', ')', '°', 'º',
  '²', '³',
];

const EXACT_ALIGNMENT_TEST_MODEL_IDENTIFIER = 'female';

const compareArrays = (arr1: Int16Array, arr2: Int16Array, step: number) => {
  expect(arr1.length).eq(arr2.length);
  for (let i = 0; i < arr1.length - step; i += step) {
    expect(arr1[i]).closeTo(arr2[i], 1);
  }
};

const runInitTest = async (
  instance: typeof Orca | typeof OrcaWorker,
  params: {
    accessKey?: string;
    model?: PvModel;
    expectFailure?: boolean;
  } = {},
) => {
  const {
    accessKey = ACCESS_KEY,
    model = { publicPath: `/test/orca_params_male.pv`, forceWrite: true },
    expectFailure = false,
  } = params;

  let orca: Orca | OrcaWorker | null = null;
  let isFailed = false;

  try {
    orca = await instance.create(accessKey, model);
    expect(typeof orca.version).eq('string');
    expect(orca.version.length).gt(0);
    expect(orca.maxCharacterLimit).eq(EXPECTED_MAX_CHARACTER_LIMIT);
    expect(orca.sampleRate).eq(EXPECTED_SAMPLE_RATE);
    expect(orca.validCharacters.length).eq(EXPECTED_VALID_CHARACTERS.length);
    orca.validCharacters.forEach((symbol: string, i: number) => {
      expect(symbol).eq(EXPECTED_VALID_CHARACTERS[i]);
    });
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

    it(`should be able to handle invalid public path (${instanceString})`, async () => {
      await runInitTest(instance, {
        model: { publicPath: 'invalid', forceWrite: true },
        expectFailure: true,
      });
    });

    it(`should be able to handle invalid base64 (${instanceString})`, async () => {
      await runInitTest(instance, {
        model: { base64: 'invalid', forceWrite: true },
        expectFailure: true,
      });
    });

    it(`should be able to handle invalid access key (${instanceString})`, async () => {
      await runInitTest(instance, {
        accessKey: 'invalid',
        expectFailure: true,
      });
    });

    for (const modelFileSuffix of ['male', 'female']) {
      const publicPath = modelFileSuffix === 'male' ? `/test/orca_params_male.pv` : `/test/orca_params_female.pv`;
      const base64Path = modelFileSuffix === 'male' ? orcaParamsMale : orcaParamsFemale;

      it(`should be able to init with public path [${modelFileSuffix}] (${instanceString})`, async () => {
        await runInitTest(instance, {
          model: { publicPath, forceWrite: true },
        });
      });

      it(`should be able to init with base64 [${modelFileSuffix}] (${instanceString})`, async () => {
        await runInitTest(instance, {
          model: { base64: base64Path, forceWrite: true },
        });
      });

      it(`should be able to handle UTF-8 public path [${modelFileSuffix}] (${instanceString})`, async () => {
        await runInitTest(instance, {
          model: { publicPath, forceWrite: true, customWritePath: '테스트' },
        });
      });

      it(`should be able to process text streaming [${modelFileSuffix}] (${instanceString})`, () => {
        try {
          cy.getFramesFromFile(`${testData.audio_data_folder}orca_params_${modelFileSuffix}_stream.wav`).then(
            async (rawPcm: Int16Array) => {
              const orca = await instance.create(
                ACCESS_KEY,
                { publicPath, forceWrite: true },
              );

              try {
                const orcaStream = await orca.streamOpen({ randomState: testData.random_state });

                const streamPcm: number[] = [];
                for (const c of testData.test_sentences.text.split('')) {
                  const pcm = await orcaStream.synthesize(c);
                  if (pcm !== null) {
                    streamPcm.push(...pcm);
                  }
                }

                const endPcm = await orcaStream.flush();
                if (endPcm !== null) {
                  streamPcm.push(...endPcm);
                }

                compareArrays(new Int16Array(streamPcm), rawPcm, 500);
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

      if (modelFileSuffix === EXACT_ALIGNMENT_TEST_MODEL_IDENTIFIER) {
        it(`should be able to process alignment exact [${modelFileSuffix}] (${instanceString})`, async () => {
          try {
            const orca = await instance.create(
              ACCESS_KEY,
              { publicPath, forceWrite: true },
            );

            const {
              pcm,
              alignments,
            } = await orca.synthesize(testData.test_sentences.text_alignment, { randomState: testData.random_state });
            expect(pcm.length).gt(0);
            expect(alignments.length).eq(testData.alignments.length);

            alignments.forEach((w, i) => {
              const { word, start_sec, end_sec, phonemes } = testData.alignments[i];
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
      } else {
        it(`should be able to process alignment [${modelFileSuffix}] (${instanceString})`, async () => {
          try {
            const orca = await instance.create(
              ACCESS_KEY,
              { publicPath, forceWrite: true },
            );

            const {
              pcm,
              alignments,
            } = await orca.synthesize(testData.test_sentences.text_alignment, { randomState: testData.random_state });
            expect(pcm.length).gt(0);
            expect(alignments.length).eq(testData.alignments.length);

            let prevWordEndSec = 0;
            let prevPhonemeEndSec = 0;
            alignments.forEach(w => {
              expect(w.startSec).closeTo(prevWordEndSec, 0.001);
              expect(w.endSec).gt(w.startSec);
              prevWordEndSec = w.endSec;
              w.phonemes.forEach(p => {
                expect(p.startSec).closeTo(prevPhonemeEndSec, 0.001);
                expect(p.endSec).gt(p.startSec);
                prevPhonemeEndSec = p.endSec;
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

      it(`should be able to process text [${modelFileSuffix}] (${instanceString})`, () => {
        try {
          cy.getFramesFromFile(`${testData.audio_data_folder}orca_params_${modelFileSuffix}_single.wav`).then(
            async rawPcm => {
              const orca = await instance.create(
                ACCESS_KEY,
                { publicPath, forceWrite: true },
              );

              const { pcm } = await orca.synthesize(
                testData.test_sentences.text,
                { speechRate: 1, randomState: testData.random_state },
              );
              compareArrays(pcm, rawPcm, 500);

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

      it(`should be able to process - no punctuation [${modelFileSuffix}] (${instanceString})`, async () => {
        try {
          const orca = await instance.create(
            ACCESS_KEY,
            { publicPath, forceWrite: true },
          );

          const { pcm } = await orca.synthesize(testData.test_sentences.text_no_punctuation);
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

      it(`should be able to process custom punctuation [${modelFileSuffix}] (${instanceString})`, async () => {
        try {
          const orca = await instance.create(
            ACCESS_KEY,
            { publicPath, forceWrite: true },
          );

          const { pcm } = await orca.synthesize(testData.test_sentences.text_custom_pronunciation);
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

      it(`should be able to handle different speech rates [${modelFileSuffix}] (${instanceString})`, async () => {
        try {
          const orca = await instance.create(
            ACCESS_KEY,
            { publicPath, forceWrite: true },
          );

          const { pcm: pcmSlow } = await orca.synthesize(testData.test_sentences.text, { speechRate: 0.7 });
          const { pcm: pcmFast } = await orca.synthesize(testData.test_sentences.text, { speechRate: 1.3 });
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

      it(`should be able to handle max num characters [${modelFileSuffix}] (${instanceString})`, async () => {
        try {
          const orca = await instance.create(
            ACCESS_KEY,
            { publicPath, forceWrite: true },
          );

          const maxNumChars = orca.maxCharacterLimit;
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

      it(`should handle invalid input [${modelFileSuffix}] (${instanceString})`, async () => {
        const orca = await instance.create(
          ACCESS_KEY,
          { publicPath, forceWrite: true },
        );

        for (const failureCase of testData.test_sentences.text_invalid) {
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

      it(`should return process and flush error message stack [${modelFileSuffix}] (${instanceString})`, async () => {
        const orca = await Orca.create(
          ACCESS_KEY,
          { publicPath: publicPath, forceWrite: true },
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

      it(`should return correct error message stack [${modelFileSuffix}] (${instanceString})`, async () => {
        let messageStack = [];
        try {
          const orca = await instance.create('invalidAccessKey', {
            publicPath,
            forceWrite: true,
          });
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
          });
          expect(orca).to.be.undefined;
        } catch (e: any) {
          expect(messageStack.length).to.be.eq(e.messageStack.length);
        }
      });
    }
  }
});
