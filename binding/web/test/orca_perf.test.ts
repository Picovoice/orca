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
import testData from '../cypress/fixtures/resources/.test/test_data.json';

const ACCESS_KEY = Cypress.env('ACCESS_KEY');
const NUM_TEST_ITERATIONS = Number(Cypress.env('NUM_TEST_ITERATIONS'));
const PROC_PERFORMANCE_THRESHOLD_SEC = Number(Cypress.env('PROC_PERFORMANCE_THRESHOLD_SEC'));

async function testPerformance(
  instance: typeof Orca | typeof OrcaWorker,
  publicPath: string,
  text: string,
) {
  const procPerfResults: number[] = [];

  for (let j = 0; j < NUM_TEST_ITERATIONS; j++) {
    const orca = await instance.create(
      ACCESS_KEY,
      { publicPath: publicPath, forceWrite: true },
    );

    let start = Date.now();
    await orca.synthesize(text);
    let end = Date.now();
    procPerfResults.push((end - start) / 1000);

    if (orca instanceof OrcaWorker) {
      orca.terminate();
    } else {
      await orca.release();
    }
  }

  const procAvgPerf = procPerfResults.reduce((a, b) => a + b) / NUM_TEST_ITERATIONS;

  // eslint-disable-next-line no-console
  console.log(`Average proc performance: ${procAvgPerf} seconds`);

  expect(procAvgPerf).to.be.lessThan(PROC_PERFORMANCE_THRESHOLD_SEC);
}

describe('Orca binding performance test', () => {
  Cypress.config('defaultCommandTimeout', 120000);

  for (const instance of [Orca, OrcaWorker]) {
    const instanceString = (instance === OrcaWorker) ? 'worker' : 'main';

    const testCase = testData.tests.sentence_tests[0];

    for (const model of testCase.models) {
      it(`should be lower than performance threshold [${model}] (${instanceString})`, async () => {
        await testPerformance(instance, `/test/${model}`, testCase.text);
      });
    }
  }
});
