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

import { Orca } from '../src';
import { performance } from 'perf_hooks';

import { getTestData } from './test_utils';

const ACCESS_KEY =
  process.argv
    .filter(x => x.startsWith('--access_key='))[0]
    ?.split('--access_key=')[1] ?? '';
const NUM_TEST_ITERATIONS = Number(
  process.argv
    .filter(x => x.startsWith('--num_test_iterations='))[0]
    ?.split('--num_test_iterations=')[1] ?? 0,
);
const PROC_PERFORMANCE_THRESHOLD_SEC = Number(
  process.argv
    .filter(x => x.startsWith('--proc_performance_threshold_sec='))[0]
    ?.split('--proc_performance_threshold_sec=')[1] ?? 0,
);

describe('Performance', () => {
  test('synthesize', () => {
    let orcaEngine = new Orca(ACCESS_KEY);

    let perfResults: number[] = [];
    for (let i = 0; i < NUM_TEST_ITERATIONS; i++) {
      const before = performance.now();
      orcaEngine.synthesize(getTestData().tests.sentence_tests[0].text);
      let synthTime = performance.now() - before;

      if (i > 0) {
        perfResults.push(synthTime);
      }
    }
    orcaEngine.release();

    let avgPerfMs =
      perfResults.reduce((acc, a) => acc + a, 0) / NUM_TEST_ITERATIONS;
    let avgPerfSec = Number((avgPerfMs / 1000).toFixed(3));
    // eslint-disable-next-line no-console
    console.log('Average proc performance: ' + avgPerfSec);
    expect(avgPerfSec).toBeLessThanOrEqual(PROC_PERFORMANCE_THRESHOLD_SEC);
  });
});
