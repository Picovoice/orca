import { Orca, OrcaWorker } from '../';
import testData from '../cypress/fixtures/.test/test_data.json';

const ACCESS_KEY = Cypress.env('ACCESS_KEY');
const NUM_TEST_ITERATIONS = Number(Cypress.env('NUM_TEST_ITERATIONS'));
const INIT_PERFORMANCE_THRESHOLD_SEC = Number(Cypress.env('INIT_PERFORMANCE_THRESHOLD_SEC'));
const PROC_PERFORMANCE_THRESHOLD_SEC = Number(Cypress.env('PROC_PERFORMANCE_THRESHOLD_SEC'));

async function testPerformance(
  instance: typeof Orca | typeof OrcaWorker,
  publicPath: string,
  text: string,
) {
  const initPerfResults: number[] = [];
  const procPerfResults: number[] = [];

  for (let j = 0; j < NUM_TEST_ITERATIONS; j++) {
    let isSynthesized = false;

    let start = Date.now();
    const orca = await instance.create(
      ACCESS_KEY,
      orcaTranscript => {
        if (orcaTranscript.speech) {
          isSynthesized = true;
        }
      },
      { publicPath: publicPath, forceWrite: true },
    );
    let end = Date.now();
    initPerfResults.push((end - start) / 1000);

    const waitUntil = (): Promise<void> =>
      new Promise(resolve => {
        setInterval(() => {
          if (isSynthesized) {
            resolve();
          }
        }, 100);
      });

    start = Date.now();
    await orca.synthesize(text);
    await waitUntil();
    end = Date.now();
    procPerfResults.push((end - start) / 1000);

    if (orca instanceof OrcaWorker) {
      orca.terminate();
    } else {
      await orca.release();
    }
  }

  const initAvgPerf = initPerfResults.reduce((a, b) => a + b) / NUM_TEST_ITERATIONS;
  const procAvgPerf = procPerfResults.reduce((a, b) => a + b) / NUM_TEST_ITERATIONS;

  // eslint-disable-next-line no-console
  console.log(`Average init performance: ${initAvgPerf} seconds`);
  // eslint-disable-next-line no-console
  console.log(`Average proc performance: ${procAvgPerf} seconds`);

  expect(initAvgPerf).to.be.lessThan(INIT_PERFORMANCE_THRESHOLD_SEC);
  expect(procAvgPerf).to.be.lessThan(PROC_PERFORMANCE_THRESHOLD_SEC);
}

describe('Orca binding performance test', () => {
  Cypress.config('defaultCommandTimeout', 120000);

  it(`should be lower than performance threshold`, async () => {
    await testPerformance(Orca, `/test/orca_params_male.pv`, testData.test_sentences.text);
  });

  for (const instance of [Orca, OrcaWorker]) {
    const instanceString = (instance === OrcaWorker) ? 'worker' : 'main';

    for (const modelFileSuffix of ['male', 'female']) {
      it(`should be lower than performance threshold [${modelFileSuffix}] (${instanceString})`, async () => {
        await testPerformance(instance, `/test/orca_params_${modelFileSuffix}.pv`, testData.test_sentences.text);
      });
    }
  }
});
