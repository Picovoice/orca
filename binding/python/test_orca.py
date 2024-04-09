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
from typing import List, Sequence

import editdistance
import pvleopard
from pvleopard import Leopard

from _orca import Orca, OrcaError, OrcaInvalidArgumentError
from _util import default_library_path, default_model_path
from test_util import get_model_paths, get_test_data

test_sentences, wer_threshold = get_test_data()

RANDOM_STATE = 42


class OrcaTestCase(unittest.TestCase):
    access_key: str
    orcas: List[Orca]

    @classmethod
    def setUpClass(cls):
        cls.orcas = [
            Orca(
                access_key=cls.access_key,
                model_path=model_path,
                library_path=default_library_path('../..'))
            for model_path in get_model_paths()]

    @classmethod
    def tearDownClass(cls):
        for orca in cls.orcas:
            orca.delete()

    def test_valid_characters(self) -> None:
        for orca in self.orcas:
            characters = orca.valid_characters
            self.assertGreaterEqual(len(characters), 0)
            self.assertTrue(all(isinstance(x, str) for x in characters))
            self.assertTrue("," in characters)

    def _test_word_error_rate(self, leopard: Leopard, pcm: Sequence[int], ground_truth: Sequence[str]) -> None:
        predicted, _ = leopard.process(pcm)

        wer = editdistance.eval(predicted.split(), ground_truth) / len(ground_truth)

        if wer > wer_threshold:
            print("Ground truth transcript: `%s`" % " ".join(ground_truth))
            print("Predicted transcript from synthesized audio: `%s`" % " ".join(predicted))
            print("=> WER: %.2f" % wer)
        self.assertTrue(wer <= wer_threshold)

    def _test_equal_timestamp(self, timestamp: float, timestamp_truth: float) -> None:
        self.assertAlmostEqual(timestamp, timestamp_truth, places=3)

    def _test_phoneme_equal(self, phoneme: Orca.PhonemeAlignment, phoneme_truth: Orca.PhonemeAlignment) -> None:
        self.assertEqual(phoneme.phoneme, phoneme_truth.phoneme)
        self._test_equal_timestamp(phoneme.start_sec, phoneme_truth.start_sec)
        self._test_equal_timestamp(phoneme.end_sec, phoneme_truth.end_sec)

    def _test_word_equal(self, word: Orca.WordAlignment, word_truth: Orca.WordAlignment) -> None:
        self.assertEqual(word.word, word_truth.word)
        self._test_equal_timestamp(word.start_sec, word_truth.start_sec)
        self._test_equal_timestamp(word.end_sec, word_truth.end_sec)

        for i, phoneme in enumerate(word.phonemes):
            phoneme_truth = word_truth.phonemes[i]
            self._test_phoneme_equal(phoneme, phoneme_truth)

    def test_max_character_limit(self) -> None:
        for orca in self.orcas:
            self.assertGreaterEqual(orca.max_character_limit, 0)

    def test_sample_rate(self) -> None:
        for orca in self.orcas:
            self.assertGreater(orca.sample_rate, 0)

    def test_synthesize(self) -> None:
        leopard = None
        try:
            leopard = pvleopard.create(access_key=self.access_key)
        except NotImplementedError as e:
            pass

        for orca in self.orcas:
            pcm, alignment = orca.synthesize(test_sentences.text, random_state=RANDOM_STATE)
            self.assertGreater(len(pcm), 0)

            if leopard is None:
                continue

            ground_truth = test_sentences.text_no_punctuation.split()
            self._test_word_error_rate(leopard=leopard, pcm=pcm, ground_truth=ground_truth)

    def test_synthesize_alignment(self) -> None:
        orca = self.orcas[0]
        pcm, alignments = orca.synthesize(test_sentences.text_alignment, random_state=RANDOM_STATE)
        self.assertGreater(len(pcm), 0)

        for i, word in enumerate(alignments):
            word_truth = test_sentences.alignments[i]
            self._test_word_equal(word, word_truth)

    def test_streaming_synthesis(self) -> None:
        leopard = None
        try:
            leopard = pvleopard.create(access_key=self.access_key)
        except NotImplementedError as e:
            pass

        for orca in self.orcas:
            stream = orca.open_stream(random_state=RANDOM_STATE)
            pcm = []
            for c in test_sentences.text:
                pcm_chunk = stream.synthesize(c)
                pcm.extend(pcm_chunk)
            pcm.extend(stream.flush())
            stream.close()

            if leopard is None:
                continue

            ground_truth = test_sentences.text_no_punctuation.split()
            self._test_word_error_rate(leopard=leopard, pcm=pcm, ground_truth=ground_truth)

    def test_synthesize_custom_pron(self) -> None:
        for orca in self.orcas:
            pcm, _ = orca.synthesize(test_sentences.text_custom_pronunciation)
            self.assertGreater(len(pcm), 0)

    def test_synthesize_speech_rate(self) -> None:
        for orca in self.orcas:
            pcm_fast, _ = orca.synthesize(test_sentences.text, speech_rate=1.3)
            pcm_slow, _ = orca.synthesize(test_sentences.text, speech_rate=0.7)

            self.assertLess(len(pcm_fast), len(pcm_slow))

            try:
                _ = orca.synthesize(test_sentences.text, speech_rate=9999)
            except OrcaError:
                pass
            else:
                self.fail("Expected OrcaError")

    def test_synthesize_to_file(self) -> None:
        for orca in self.orcas:
            output_path = os.path.join(os.path.dirname(__file__), "output.wav")
            orca.synthesize_to_file(test_sentences.text, output_path=output_path)
            self.assertTrue(os.path.isfile(output_path))
            os.remove(output_path)

    def test_version(self) -> None:
        for orca in self.orcas:
            version = orca.version
            self.assertIsInstance(version, str)
            self.assertGreater(len(version), 0)

    def test_invalid_input(self) -> None:
        for orca in self.orcas:
            for sentence in test_sentences.text_invalid:
                with self.assertRaises(OrcaInvalidArgumentError):
                    orca.synthesize(sentence)

    def test_message_stack(self):
        relative_path = '../..'

        error = None
        try:
            orca = Orca(
                access_key='invalid',
                model_path=default_model_path(relative_path),
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
            library_path=default_library_path(relative_path))

        address = orca._handle
        orca._handle = None

        try:
            res = orca.synthesize(test_sentences.text)
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
    args = parser.parse_args()

    OrcaTestCase.access_key = args.access_key
    unittest.main(argv=sys.argv[:1])
