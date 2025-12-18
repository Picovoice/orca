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

import argparse
import os
import sys
import unittest
import dataclasses

from parameterized import parameterized
from typing import List, Sequence

from _orca import Orca, OrcaError, OrcaInvalidArgumentError
from _util import default_library_path, default_model_path
from test_util import get_model_path, get_test_data, read_wav_file


test_data = get_test_data()

PCM_OUTLIER_THRESHOLD = 400
PCM_OUTLIER_COUNT_THRESHOLD = 0.05

class OrcaTestCase(unittest.TestCase):
    access_key: str
    device: str
    orcas: List[Orca]
    model_paths: List[str]

    def _test_equal_timestamp(self, timestamp: float, timestamp_truth: float) -> None:
        self.assertAlmostEqual(timestamp, timestamp_truth, places=2)

    def _test_phoneme_equal(self, phoneme: Orca.PhonemeAlignment, phoneme_truth: Orca.PhonemeAlignment) -> None:
        self.assertEqual(phoneme.phoneme, phoneme_truth.phoneme)
        self._test_equal_timestamp(phoneme.start_sec, phoneme_truth.start_sec)
        self._test_equal_timestamp(phoneme.end_sec, phoneme_truth.end_sec)

    def _test_word_equal(self, word: Orca.WordAlignment, word_truth: Orca.WordAlignment) -> None:
        self.assertEqual(word.word, word_truth.word)
        self._test_equal_timestamp(word.start_sec, word_truth.start_sec)
        self._test_equal_timestamp(word.end_sec, word_truth.end_sec)

        self.assertEqual(len(word.phonemes), len(word_truth.phonemes))
        for phoneme, phoneme_truth in zip(word.phonemes, word_truth.phonemes):
            self._test_phoneme_equal(phoneme, phoneme_truth)

    def _test_audio(self, pcm: Sequence[int], ground_truth: Sequence[int]) -> None:
        pcm = pcm[:len(ground_truth)]  # compensate for discrepancies due to wav header
        self.assertEqual(len(pcm), len(ground_truth))
        diff_pcm = [abs(a - b) for a, b in zip(pcm, ground_truth)]
        diff_outliers = sum(1 for d in diff_pcm if d > PCM_OUTLIER_THRESHOLD) / len(diff_pcm)
        self.assertLessEqual(diff_outliers, PCM_OUTLIER_COUNT_THRESHOLD)


    @staticmethod
    def _get_pcm(model: str, audio_data_folder: str, synthesis_type: str = "single") -> Sequence[int]:
        test_wav_folder = os.path.join(os.path.dirname(__file__), "../../", audio_data_folder)
        test_wav_path = \
            os.path.join(f"{test_wav_folder}", model.replace(".pv", f"_{synthesis_type}.wav"))
        return read_wav_file(test_wav_path)

    @classmethod
    def _orca_iter(cls, models):
        for model in models:
            orca = None
            try:
                orca = Orca(
                    access_key=cls.access_key,
                    model_path=get_model_path(model),
                    device=cls.device,
                    library_path=default_library_path('../..'))
                yield orca, model

            finally:
                if orca is not None:
                    orca.delete()

    @parameterized.expand([(t.language, t.models, t.random_state, t.text) for t in test_data.sentence_tests])
    def test_synthesize(
            self,
            language: str,
            models: List[str],
            random_state: int,
            text: str):

        for orca, model in OrcaTestCase._orca_iter(models):
            pcm, alignment = orca.synthesize(text, random_state=random_state)
            self.assertGreater(len(pcm), 0)

            ground_truth = self._get_pcm(
                model=model,
                audio_data_folder=test_data.audio_data_folder,
                synthesis_type="single")

            self._test_audio(pcm=pcm, ground_truth=ground_truth)

    @parameterized.expand([(t.language, t.models) for t in test_data.sentence_tests])
    def test_valid_characters(
            self,
            language: str,
            models: List[str]) -> None:
        for orca, model in OrcaTestCase._orca_iter(models):
            characters = orca.valid_characters
            self.assertGreaterEqual(len(characters), 0)
            self.assertTrue(all(isinstance(x, str) for x in characters))
            self.assertTrue("," in characters)

    @parameterized.expand([(t.language, t.models) for t in test_data.sentence_tests])
    def test_max_character_limit(
            self,
            language: str,
            models: List[str]) -> None:
        for orca, model in OrcaTestCase._orca_iter(models):
            self.assertGreaterEqual(orca.max_character_limit, 0)

    @parameterized.expand([(t.language, t.models) for t in test_data.sentence_tests])
    def test_sample_rate(
            self,
            language: str,
            models: List[str]) -> None:
        for orca, model in OrcaTestCase._orca_iter(models):
            self.assertGreater(orca.sample_rate, 0)

    @parameterized.expand([dataclasses.astuple(t) for t in test_data.alignment_tests])
    def test_synthesize_alignment_exact(
            self,
            language: str,
            model: str,
            random_state: int,
            text_alignment: str,
            expected_alignments: Sequence[Orca.WordAlignment]) -> None:
        for orca, model in OrcaTestCase._orca_iter([model]):
            pcm, alignments = orca.synthesize(text_alignment, random_state=random_state)
            self.assertGreater(len(pcm), 0)

            self.assertTrue(len(alignments) == len(expected_alignments))
            for word, word_truth in zip(alignments, expected_alignments):
                self._test_word_equal(word, word_truth)

    @parameterized.expand([dataclasses.astuple(t) for t in test_data.alignment_tests])
    def test_synthesize_alignment(
            self,
            language: str,
            model: str,
            random_state: int,
            text_alignment: str,
            expected_alignments: Sequence[Orca.WordAlignment]) -> None:
        for orca, model in OrcaTestCase._orca_iter([model]):
            pcm, alignments = orca.synthesize(text_alignment, random_state=random_state)
            self.assertGreater(len(pcm), 0)

            previous_word_end_sec = 0
            previous_phoneme_end_sec = 0
            for word in alignments:
                self.assertTrue(word.start_sec == previous_word_end_sec)
                self.assertTrue(word.end_sec > word.start_sec)
                previous_word_end_sec = word.end_sec

                for phoneme in word.phonemes:
                    self.assertTrue(phoneme.start_sec == previous_phoneme_end_sec)
                    self.assertTrue(phoneme.start_sec >= word.start_sec)
                    self.assertTrue(phoneme.end_sec <= word.end_sec)
                    self.assertTrue(phoneme.end_sec > phoneme.start_sec)
                    previous_phoneme_end_sec = phoneme.end_sec

    @parameterized.expand([(t.language, t.models, t.random_state, t.text) for t in test_data.sentence_tests])
    def test_streaming_synthesis(
            self,
            language: str,
            models: List[str],
            random_state: int,
            text: str):

        for orca, model in OrcaTestCase._orca_iter(models):
            stream = orca.stream_open(random_state=random_state)
            pcm = []
            for c in text:
                pcm_chunk = stream.synthesize(c)
                if pcm_chunk is not None:
                    pcm.extend(pcm_chunk)
            pcm_chunk = stream.flush()
            if pcm_chunk is not None:
                pcm.extend(pcm_chunk)
            stream.close()

            ground_truth = self._get_pcm(
                model=model,
                audio_data_folder=test_data.audio_data_folder,
                synthesis_type="stream")

            self._test_audio(pcm=pcm, ground_truth=ground_truth)

    @parameterized.expand([(t.language, t.models, t.random_state, t.text_custom_pronunciation) for t in test_data.sentence_tests])
    def test_synthesize_custom_pron(
            self,
            language: str,
            models: List[str],
            random_state: int,
            text_custom_pronunciation: str):

        for orca, model in OrcaTestCase._orca_iter(models):
            pcm, alignment = orca.synthesize(text_custom_pronunciation, random_state=random_state)
            self.assertGreater(len(pcm), 0)

    @parameterized.expand([(t.language, t.models, t.random_state, t.text) for t in test_data.sentence_tests])
    def test_synthesize(
            self,
            language: str,
            models: List[str],
            random_state: int,
            text: str):

        for orca, model in OrcaTestCase._orca_iter(models):
            pcm_fast, _ = orca.synthesize(text, speech_rate=1.3)
            pcm_slow, _ = orca.synthesize(text, speech_rate=0.7)

            self.assertLess(len(pcm_fast), len(pcm_slow))

            with self.assertRaises(OrcaError):
                _ = orca.synthesize(text, speech_rate=9999)

    @parameterized.expand([(t.language, t.models, t.random_state, t.text) for t in test_data.sentence_tests])
    def test_synthesize_to_file(
            self,
            language: str,
            models: List[str],
            random_state: int,
            text: str):

        for orca, model in OrcaTestCase._orca_iter(models):
            output_path = os.path.join(os.path.dirname(__file__), "output.wav")
            orca.synthesize_to_file(text, output_path=output_path)
            self.assertTrue(os.path.isfile(output_path))
            os.remove(output_path)

    @parameterized.expand([(t.language, t.models) for t in test_data.sentence_tests])
    def test_version(
            self,
            language: str,
            models: List[str]):

        for orca, model in OrcaTestCase._orca_iter(models):
            version = orca.version
            self.assertIsInstance(version, str)
            self.assertGreater(len(version), 0)

    @parameterized.expand([(t.language, t.models, t.text_invalid) for t in test_data.invalid_tests])
    def test_invalid_input(
            self,
            language: str,
            models: List[str],
            text_invalid: List[str]):

        for orca, model in OrcaTestCase._orca_iter(models):
            for sentence in text_invalid:
                with self.assertRaises(OrcaInvalidArgumentError):
                    orca.synthesize(sentence)

    def test_message_stack(self):
        relative_path = '../..'

        error = None
        try:
            orca = Orca(
                access_key='invalid',
                model_path=default_model_path(relative_path),
                device=self.device,
                library_path=default_library_path(relative_path))
            self.assertIsNone(orca)
        except OrcaError as e:
            error = e.message_stack

        self.assertIsNotNone(error)
        self.assertGreater(len(error), 0)

        try:
            orca = Orca(
                access_key='invalid',
                model_path=default_model_path(relative_path),
                device=self.device,
                library_path=default_library_path(relative_path))
            self.assertIsNone(orca)
        except OrcaError as e:
            self.assertEqual(len(error), len(e.message_stack))
            self.assertListEqual(list(error), list(e.message_stack))

    def test_process_message_stack(self):
        relative_path = '../..'

        orca = Orca(
            access_key=self.access_key,
            model_path=default_model_path(relative_path),
            device=self.device,
            library_path=default_library_path(relative_path))

        address = orca._handle
        orca._handle = None

        try:
            res = orca.synthesize("test text")
            self.assertEqual(len(res), 0)
        except OrcaError as e:
            self.assertGreater(len(e.message_stack), 0)
            self.assertLess(len(e.message_stack), 8)
        finally:
            orca._handle = address
            orca.delete()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--access-key', required=True)
    parser.add_argument('--device', required=True)
    args = parser.parse_args()

    OrcaTestCase.access_key = args.access_key
    OrcaTestCase.device = args.device
    unittest.main(argv=sys.argv[:1])
