#
#    Copyright 2024 Picovoice Inc.
#
#    You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
#    file accompanying this source.
#
#    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
#    an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
#    specific language governing permissions and limitations under the License.
#

import argparse
import os
import sys
import unittest
from time import perf_counter

from _orca import Orca
from _util import default_library_path
from test_util import get_model_paths, get_test_data

test_data = get_test_data()


class OrcaPerformanceTestCase(unittest.TestCase):
    access_key: str
    num_test_iterations: int
    proc_performance_threshold_rtf: float

    def test_performance_proc(self) -> None:
        for model_path in get_model_paths():
            orca = Orca(
                access_key=self.access_key,
                library_path=default_library_path('../..'),
                model_path=model_path)

            num_audio_seconds = 0
            num_proc_seconds = 0
            for i in range(self.num_test_iterations):
                start = perf_counter()
                pcm, _ = orca.synthesize(test_data.text)
                if i > 0:
                    num_audio_seconds += len(pcm) / orca.sample_rate
                    num_proc_seconds += perf_counter() - start

            orca.delete()

            real_time_factor = num_audio_seconds / num_proc_seconds
            print("Average proc performance[model=%s]: RTF = %s " % (os.path.basename(model_path), real_time_factor))
            self.assertGreater(real_time_factor, self.proc_performance_threshold_rtf)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--access-key', required=True)
    parser.add_argument('--num-test-iterations', type=int, required=True)
    parser.add_argument('--proc-performance-threshold-rtf', type=float, required=True)
    args = parser.parse_args()

    OrcaPerformanceTestCase.access_key = args.access_key
    OrcaPerformanceTestCase.num_test_iterations = args.num_test_iterations
    OrcaPerformanceTestCase.proc_performance_threshold_rtf = args.proc_performance_threshold_rtf

    unittest.main(argv=sys.argv[:1])
