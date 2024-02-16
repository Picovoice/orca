import { LeopardWorker } from '@picovoice/leopard-web';
import { Orca, OrcaWorker } from '../';
import { OrcaError } from '../dist/types/orca_errors';

// @ts-ignore
import orcaParamsMale from './orca_params_male';
// @ts-ignore
import orcaParamsFemale from './orca_params_female';
import { PvModel } from '@picovoice/web-utils';

import testData from '../cypress/fixtures/.test/test_data.json';

const ACCESS_KEY = Cypress.env('ACCESS_KEY');

const EXPECTED_MAX_CHARACTER_LIMIT = 200;
const EXPECTED_SAMPLE_RATE = 22050;
const EXPECTED_VALID_CHARACTERS = [
  '.', ':', ',', '"', '?', '!', 'a', 'b',
  'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
  's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', '\'', '{', '}', '|', ' ', '-',
];

const levenshteinDistance = (words1: string[], words2: string[]) => {
  const res = Array.from(Array(words1.length + 1), () => new Array(words2.length + 1));
  for (let i = 0; i <= words1.length; i++) {
    res[i][0] = i;
  }
  for (let j = 0; j <= words2.length; j++) {
    res[0][j] = j;
  }
  for (let i = 1; i <= words1.length; i++) {
    for (let j = 1; j <= words2.length; j++) {
      res[i][j] = Math.min(
        res[i - 1][j] + 1,
        res[i][j - 1] + 1,
        res[i - 1][j - 1] + (words1[i - 1].toUpperCase() === words2[j - 1].toUpperCase() ? 0 : 1),
      );
    }
  }
  return res[words1.length][words2.length];
};

const wordErrorRate = (reference: string, hypothesis: string, useCER = false): number => {
  const splitter = (useCER) ? '' : ' ';
  const ed = levenshteinDistance(reference.split(splitter), hypothesis.split(splitter));
  return ed / reference.length;
};

const delay = (time: number) => new Promise(resolve => setTimeout(resolve, time));

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

  let orca = null;
  let isFailed = false;

  try {
    orca = await instance.create(accessKey, () => {
    }, model);

    expect(typeof orca.version).to.eq('string');
    expect(orca.version.length).to.be.greaterThan(0);
    expect(orca.maxCharacterLimit).to.eq(EXPECTED_MAX_CHARACTER_LIMIT);
    expect(orca.sampleRate).to.eq(EXPECTED_SAMPLE_RATE);
    expect(orca.validCharacters.length).to.eq(EXPECTED_VALID_CHARACTERS.length);
    orca.validCharacters.forEach((symbol: string, i: number) => {
      expect(symbol).to.eq(EXPECTED_VALID_CHARACTERS[i]);
    });
  } catch (e) {
    if (expectFailure) {
      isFailed = true;
    } else {
      expect(e).to.be.undefined;
    }
  } finally {
    if (orca !== null) {
      if (orca instanceof OrcaWorker) {
        orca.terminate();
      } else {
        await orca.release();
      }
    }
  }

  if (expectFailure) {
    expect(isFailed).to.be.true;
  } else {
    expect(isFailed).to.be.false;
  }
};

const runProcTest = async (
  instance: typeof Orca | typeof OrcaWorker,
  text: string,
  speechRate: number,
  params: {
    accessKey?: string;
    model?: PvModel;
    isTestWER?: boolean;
    expectFailure?: boolean;
  } = {},
) => {
  const {
    accessKey = ACCESS_KEY,
    model = { publicPath: '/test/orca_params_male.pv', forceWrite: true },
    isTestWER = true,
    expectFailure = false,
  } = params;

  let orca = null;
  let isFailed = false;

  const checkWER = async (pcm: Int16Array) => {
    const leopard = await LeopardWorker.create(
      accessKey,
      { publicPath: '/test/leopard_params.pv', forceWrite: true },
    );

    const { transcript } = await leopard.process(pcm);
    const wer = wordErrorRate(transcript, testData.test_sentences.text_no_punctuation);
    expect(wer).lt(testData.wer_threshold);
    leopard.terminate();
  };

  const setOrcaSpeech = () => new Promise<Int16Array | null>(async (resolve, reject) => {
    orca = await instance.create(
      accessKey,
      orcaSpeech => {
        resolve(orcaSpeech.speech);
      },
      model,
      {
        synthesizeErrorCallback: () => {
          isFailed = true;
          reject(null);
        },
      },
    );

    try {
      await orca.synthesize(text, speechRate);
    } catch (e) {
      isFailed = true;
    }
  });

  try {
    const speech = await setOrcaSpeech();
    if (isTestWER) {
      await checkWER(speech);
    }

    if (!isTestWER && !expectFailure) {
      expect(speech.length).gt(0);
    }
  } catch (e) {
    if (expectFailure) {
      isFailed = true;
    }
  } finally {
    if (orca !== null) {
      if (orca instanceof OrcaWorker) {
        orca.terminate();
      } else if (orca instanceof Orca) {
        await orca.release();
      }
    }
  }

  if (expectFailure) {
    expect(isFailed).to.be.true;
  } else {
    expect(isFailed).to.be.false;
  }
};

describe('Orca Binding', function() {
  it(`should return process and flush error message stack`, async () => {
    // @ts-ignore
    let errors: [OrcaError] = [];

    const runProcess = () => new Promise<void>(async resolve => {
      const orca = await Orca.create(
        ACCESS_KEY,
        () => {
        },
        { publicPath: '/test/orca_params_male.pv', forceWrite: true },
        {
          synthesizeErrorCallback: (e: OrcaError) => {
            errors.push(e);
            resolve();
          },
        },
      );

      // @ts-ignore
      const objectAddress = orca._objectAddress;

      // @ts-ignore
      orca._objectAddress = 0;
      await orca.synthesize('test', 1.0);

      await delay(1000);

      // @ts-ignore
      orca._objectAddress = objectAddress;
      await orca.release();
    });

    await runProcess();
    expect(errors.length).to.be.gte(0);

    for (let i = 0; i < errors.length; i++) {
      expect((errors[i] as OrcaError).messageStack.length).to.be.gt(0);
      expect((errors[i] as OrcaError).messageStack.length).to.be.lte(8);
    }
  });

  for (const instance of [Orca, OrcaWorker]) {
    const instanceString = instance === OrcaWorker ? 'worker' : 'main';

    it(`should be able to handle invalid public path (${instanceString})`, () => {
      cy.wrap(null).then(async () => {
        await runInitTest(instance, {
          model: { publicPath: 'invalid', forceWrite: true },
          expectFailure: true,
        });
      });
    });

    it(`should be able to handle invalid base64 (${instanceString})`, () => {
      cy.wrap(null).then(async () => {
        await runInitTest(instance, {
          model: { base64: 'invalid', forceWrite: true },
          expectFailure: true,
        });
      });
    });

    it(`should be able to handle invalid access key (${instanceString})`, () => {
      cy.wrap(null).then(async () => {
        await runInitTest(instance, {
          accessKey: 'invalid',
          expectFailure: true,
        });
      });
    });

    for (const modelFileSuffix of ['male', 'female']) {
      const publicPath = modelFileSuffix === 'male' ? `/test/orca_params_male.pv` : `/test/orca_params_female.pv`;
      const base64Path = modelFileSuffix === 'male' ? orcaParamsMale : orcaParamsFemale;

      it(`should return correct error message stack [${modelFileSuffix}] (${instanceString})`, async () => {
        let messageStack = [];
        try {
          const orca = await instance.create('invalidAccessKey', () => {
          }, {
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
          const orca = await instance.create('invalidAccessKey', () => {
          }, {
            publicPath,
            forceWrite: true,
          });
          expect(orca).to.be.undefined;
        } catch (e: any) {
          expect(messageStack.length).to.be.eq(e.messageStack.length);
        }
      });

      it(`should be able to init with public path [${modelFileSuffix}] (${instanceString})`, () => {
        cy.wrap(null).then(async () => {
          await runInitTest(instance, {
            model: {
              publicPath,
              forceWrite: true,
            },
          });
        });
      });

      it(`should be able to init with base64 [${modelFileSuffix}] (${instanceString})`, () => {
        cy.wrap(null).then(async () => {
          await runInitTest(instance, {
            model: { base64: base64Path, forceWrite: true },
          });
        });
      });

      it(`should be able to handle UTF-8 public path [${modelFileSuffix}] (${instanceString})`, () => {
        cy.wrap(null).then(async () => {
          await runInitTest(instance, {
            model: {
              publicPath,
              forceWrite: true,
              customWritePath: '테스트',
            },
          });
        });
      });

      it(`should be able to handle different speech rates [${modelFileSuffix}] (${instanceString})`, () => {
        cy.wrap(null).then(async () => {
          try {
            let orca: any = null;

            const setOrcaSpeech = (customSpeechRate: number) => new Promise<Int16Array | null>(async (resolve, reject) => {
              orca = await instance.create(
                ACCESS_KEY,
                orcaSpeech => resolve(orcaSpeech.speech),
                { publicPath, forceWrite: true },
                { synthesizeErrorCallback: () => reject(null) },
              );

              await orca.synthesize(testData.test_sentences.text, customSpeechRate);
            });

            const speechSlow = await setOrcaSpeech(0.7);
            const speechFast = await setOrcaSpeech(1.3);
            expect(speechSlow.length).gt(speechFast.length);

            if (orca instanceof OrcaWorker) {
              orca.terminate();
            } else if (orca instanceof Orca) {
              await orca.release();
            }
          } catch (e) {
            expect(e).to.be.undefined;
          }
        });
      });

      it(`should be able to handle max num characters [${modelFileSuffix}] (${instanceString})`, () => {
        cy.wrap(null).then(async () => {
          try {
            let orca: any = null;

            const setOrcaSpeech = () => new Promise<Int16Array | null>(async resolve => {
              orca = await instance.create(
                ACCESS_KEY,
                orcaSpeech => {
                  resolve(orcaSpeech.speech);
                },
                {
                  publicPath,
                  forceWrite: true,
                },
              );
              const maxNumChars = orca.maxCharacterLimit;
              await orca.synthesize('a'.repeat(maxNumChars));
            });

            const speech = await setOrcaSpeech();
            expect(speech.length).gt(0);

            if (orca instanceof OrcaWorker) {
              orca.terminate();
            } else if (orca instanceof Orca) {
              await orca.release();
            }
          } catch (e) {
            expect(e).to.be.undefined;
          }
        });
      });

      it(`should be able to process - punctuation [${modelFileSuffix}] (${instanceString})`, async () => {
        try {
          await runProcTest(
            instance,
            testData.test_sentences.text,
            1.0,
            {
              model: {
                publicPath,
                forceWrite: true,
              },
            },
          );
        } catch (e) {
          expect(e).to.be.undefined;
        }
      });

      it(`should be able to process - no punctuation [${modelFileSuffix}] (${instanceString})`, async () => {
        try {
          await runProcTest(
            instance,
            testData.test_sentences.text_no_punctuation,
            1.0,
            {
              model: {
                publicPath,
                forceWrite: true,
              },
            },
          );
        } catch (e) {
          expect(e).to.be.undefined;
        }
      });

      it(`should be able to process - custom punctuation [${modelFileSuffix}] (${instanceString})`, async () => {
        try {
          await runProcTest(
            instance,
            testData.test_sentences.text_custom_pronunciation,
            1.0,
            {
              model: {
                publicPath,
                forceWrite: true,
              },
              isTestWER: false,
            },
          );
        } catch (e) {
          expect(e).to.be.undefined;
        }
      });

      for (const failureCase of testData.test_sentences.text_invalid) {
        it(`should handle invalid text (${failureCase}) [${modelFileSuffix}] (${instanceString})`, async () => {
          try {
            await runProcTest(
              instance,
              failureCase,
              1.0,
              {
                model: {
                  publicPath,
                  forceWrite: true,
                },
                isTestWER: false,
                expectFailure: true,
              },
            );
          } catch (e) {
            expect(e).to.be.undefined;
          }
        });
      }
    }
  }
});
