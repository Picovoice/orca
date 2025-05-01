#
#    Copyright 2024-2025 Picovoice Inc.
#
#    You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
#    file accompanying this source.
#
#    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
#    an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
#    specific language governing permissions and limitations under the License.
#

import os.path
import platform as pltf
import subprocess
import sys
import unittest

from test_util import get_model_paths, get_test_data

test_data = get_test_data()


class OrcaCTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._access_key = sys.argv[1]
        platform = sys.argv[2]
        if platform == "mac":
            if pltf.machine() == "x86_64":
                cls._arch = "x86_64"
            elif pltf.machine() == "arm64":
                cls._arch = "arm64"
        else:
            cls._arch = "" if len(sys.argv) != 4 else sys.argv[3]
        cls._platform = platform
        cls._root_dir = os.path.join(os.path.dirname(__file__), "../../..")

    @staticmethod
    def _get_lib_ext(platform: str) -> str:
        if platform == "windows":
            return "dll"
        elif platform == "mac":
            return "dylib"
        else:
            return "so"

    def _get_library_file(self) -> str:
        return os.path.join(
            self._root_dir,
            "lib",
            self._platform,
            self._arch,
            "libpv_orca." + self._get_lib_ext(self._platform)
        )

    def run_orca(self, model_path: str) -> None:
        output_path = os.path.join(os.path.dirname(__file__), "output.wav")
        args = [
            os.path.join(os.path.dirname(__file__), "../build/orca_demo"),
            "-a", self._access_key,
            "-l", self._get_library_file(),
            "-m", model_path,
            "-t", test_data.text,
            "-o", output_path,
        ]

        process = subprocess.Popen(args, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()

        poll_result = process.poll()
        if poll_result != 0:
            print(stdout.decode('utf-8'))
            print(stderr.decode('utf-8'))
            raise RuntimeError("Error running demo. See details above")

        self.assertEqual(poll_result, 0)
        self.assertEqual(stderr.decode('utf-8'), '')
        self.assertTrue("Saved audio" in stdout.decode('utf-8'))
        os.remove(output_path)

    def run_orca_streaming(self, model_path: str) -> None:
        output_path = os.path.join(os.path.dirname(__file__), "output.wav")
        args = [
            os.path.join(os.path.dirname(__file__), "../build/orca_demo_streaming"),
            "-a", self._access_key,
            "-l", self._get_library_file(),
            "-m", model_path,
            "-t", test_data.text,
            "-o", output_path,
        ]

        process = subprocess.Popen(args, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        stdout, stderr = process.communicate()

        self.assertEqual(process.poll(), 0)
        self.assertEqual(stderr.decode('utf-8'), '')
        self.assertTrue("Saved final audio" in stdout.decode('utf-8'))
        os.remove(output_path)

    def test_orca(self) -> None:
        for model_path in get_model_paths():
            self.run_orca(model_path=model_path)


if __name__ == '__main__':
    if len(sys.argv) < 3 or len(sys.argv) > 4:
        print("Usage: test_orca_c.py ${AccessKey} ${Platform} [${Arch}]")
        exit(1)
    unittest.main(argv=sys.argv[:1])