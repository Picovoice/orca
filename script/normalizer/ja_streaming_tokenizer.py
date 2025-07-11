import time
from argparse import ArgumentParser
from collections import namedtuple
from enum import Enum
from fugashi import GenericTagger

AHEAD_CHARS = 10


class CharType(Enum):
    HIRAGANA = 0
    KATAKANA = 1
    KANJI = 2
    OTHER = 3


JaToken = namedtuple('JaToken', ['text', 'reading'])


class JaStreamingTokenizer(object):

    def __init__(self, tagger, num_ahead_switches, num_ahead_chars, num_prev_tokens):
        self.tagger = tagger
        self.num_ahead_switches_context = num_ahead_switches
        self.num_ahead_chars_context = num_ahead_chars
        self.num_prev_tokens_context = num_prev_tokens
        self.text_buffer = list()
        self.prev_tokens = list()

    def tokenize(self, text):
        if text == '':
            return

        self.text_buffer.append(text)
        cur_char_type = get_char_type(self.text_buffer[0])
        char_switches = 0
        chars_without_switch = 0
        i = 0
        while (i < len(self.text_buffer) and
               char_switches < self.num_ahead_switches_context and
               chars_without_switch < self.num_ahead_chars_context):
            char_type = get_char_type(self.text_buffer[i])
            if char_type != cur_char_type:
                chars_without_switch = 0
                char_switches += 1
                cur_char_type = char_type
            else:
                chars_without_switch += 1
            i += 1

        if (char_switches < self.num_ahead_switches_context and
                chars_without_switch < self.num_ahead_chars_context):
            return list()

        tokenize_chunk = list()
        ret_tokens = list()
        if len(self.prev_tokens) > 0:
            tokenize_chunk.extend([x.text for x in self.prev_tokens[-self.num_prev_tokens_context:]])
            ret_tokens.extend(self.prev_tokens[:-self.num_prev_tokens_context])
            self.prev_tokens = list()

        tokenize_chunk.extend(self.text_buffer[:i])
        self.text_buffer = self.text_buffer[i:]
        tokenize_hyp = self.tagger("".join(tokenize_chunk))

        for x in tokenize_hyp:
            if x.is_unk:
                if x.char_type != 2 and x.char_type != 8:
                    reading = x.surface
                else:
                    reading = None
            else:
                if x.char_type == 2 or x.char_type == 8:
                    reading = x.feature[7]
                else:
                    reading = x.surface

            self.prev_tokens.append(JaToken(x.surface, reading))

        return ret_tokens

    def flush(self):
        ret_tokens = list()
        tokenize_chunk = list()
        if len(self.prev_tokens) > 0:
            tokenize_chunk.extend([x.text for x in self.prev_tokens])
            self.prev_tokens = list()

        tokenize_chunk.extend(self.text_buffer)
        self.text_buffer = list()
        tokenize_hyp = self.tagger("".join(tokenize_chunk))

        for x in tokenize_hyp:
            if x.is_unk:
                if x.char_type != 2 and x.char_type != 8:
                    reading = x.surface
                else:
                    reading = None
            else:
                if x.char_type == 2 or x.char_type == 8:
                    reading = x.feature[7]
                else:
                    reading = x.surface
            ret_tokens.append(JaToken(x.surface, reading))

        return ret_tokens


def get_char_type(c):
    code = ord(c[0])
    if 0x3040 <= code <= 0x309F:
        return CharType.HIRAGANA
    elif 0x30A0 <= code <= 0x30FF:
        return CharType.KATAKANA
    elif 0x4E00 <= code <= 0x9FFF:
        return CharType.KANJI
    else:
        return CharType.OTHER


def edit_distance_with_backtracking(arr1, arr2):
    len1, len2 = len(arr1), len(arr2)
    dp = [[0] * (len2 + 1) for _ in range(len1 + 1)]
    backtrack = [[None] * (len2 + 1) for _ in range(len1 + 1)]

    for i in range(len1 + 1):
        dp[i][0] = i
        backtrack[i][0] = 'D'
    for j in range(len2 + 1):
        dp[0][j] = j
        backtrack[0][j] = 'I'

    for i in range(1, len1 + 1):
        for j in range(1, len2 + 1):
            if arr1[i - 1] == arr2[j - 1]:
                dp[i][j] = dp[i - 1][j - 1]
                backtrack[i][j] = 'M'
            else:
                delete_cost = dp[i - 1][j] + 1
                insert_cost = dp[i][j - 1] + 1
                substitute_cost = dp[i - 1][j - 1] + 1

                dp[i][j], operation = min(
                    (delete_cost, 'D'),
                    (insert_cost, 'I'),
                    (substitute_cost, 'S')
                )
                backtrack[i][j] = operation

    i, j = len1, len2
    operations = []
    while i > 0 or j > 0:
        op = backtrack[i][j]
        if op == 'M':
            i -= 1
            j -= 1
        elif op == 'D':
            operations.append(f"Delete '{arr1[i - 1]}' from position {i - 1}")
            i -= 1
        elif op == 'I':
            operations.append(f"Insert '{arr2[j - 1]}' at position {i}")
            j -= 1
        elif op == 'S':
            operations.append(f"Substitute '{arr1[i - 1]}' with '{arr2[j - 1]}' at position {i - 1}")
            i -= 1
            j -= 1

    operations.reverse()

    return dp[len1][len2], operations


def parse_test_data(tagger, token_file):
    test_data = list()
    with open(token_file, 'r') as f:
        unk_ref_tokens = 0
        total_ref_tokens = 0
        for line in f.read().split('\n'):
            if len(line) == 0:
                continue
            for sentence in line.split('。'):
                sentence = sentence.strip()
                if len(sentence) == 0:
                    continue
                tokens = sentence.split(' ') + ['。']
                ref_tokens = tagger("".join(tokens))
                total_ref_tokens += len(ref_tokens)
                ja_ref_tokens = list()
                for x in ref_tokens:
                    if x.is_unk:
                        if x.char_type != 2 and x.char_type != 8:
                            reading = x.surface
                        else:
                            reading = None
                            unk_ref_tokens += 1
                    else:
                        if x.char_type == 2 or x.char_type == 8:
                            reading = x.feature[7]
                        else:
                            reading = x.surface
                    ja_ref_tokens.append(JaToken(x.surface, reading))

                test_data.append((tokens, ja_ref_tokens))
    print(f"REF_OOV_RATE={unk_ref_tokens / float(total_ref_tokens):.5f}")
    return test_data


def main():
    parser = ArgumentParser()
    parser.add_argument('--token_file', '-t', required=True)
    parser.add_argument('--dict_dir', '-d', required=True)
    parser.add_argument('--verbose', '-v', action="store_true")
    args = parser.parse_args()

    tagger = GenericTagger(f"-r {args.dict_dir}/mecabrc -d {args.dict_dir}")
    test_data = parse_test_data(tagger, args.token_file)
    for ahead_switches_context in range(1, 2):
        for behind_context in range(2, 3):
            edit_distance_total = 0
            first_token_total = 0
            time_taken_total = 0
            unknown_total = 0
            ref_total = 0
            hyp_total = 0
            tokens_total = 0
            tokenizer = JaStreamingTokenizer(
                tagger=tagger,
                num_ahead_chars=AHEAD_CHARS,
                num_ahead_switches=ahead_switches_context,
                num_prev_tokens=behind_context)
            for tokens, ref in test_data:
                hyp = list()
                start = time.time()
                first_token_idx = -1
                for token_idx in range(len(tokens)):
                    res = tokenizer.tokenize(tokens[token_idx])
                    if len(res) > 0 and first_token_idx == -1:
                        first_token_idx = token_idx
                    hyp.extend(res)
                res = tokenizer.flush()
                hyp.extend(res)

                time_taken = time.time() - start

                edit_distance, ops = edit_distance_with_backtracking([x.text for x in ref], [x.text for x in hyp])
                if args.verbose and edit_distance > 0:
                    print([x.reading for x in ref])
                    print([x.reading for x in hyp])
                    print("Edit Distance:", edit_distance)
                    print("Operations:")
                    for op in ops:
                        print("\t" + op)
                edit_distance_total += edit_distance
                time_taken_total += time_taken
                first_token_total += first_token_idx
                hyp_total += len(hyp)
                ref_total += len(ref)
                tokens_total += len(tokens)
                for t in hyp:
                    if t.reading is None:
                        unknown_total += 1
                    elif get_char_type(t.reading) is CharType.KANJI:
                        unknown_total += 1

            print(f"AHEAD_CHAR {AHEAD_CHARS}, AHEAD_SWITCH {ahead_switches_context}, BACK {behind_context}: \n"
                  f"TOKEN_ERROR_RATE={edit_distance_total / float(ref_total):.4f} \n"
                  f"AVG_FIRST_TOKEN={first_token_total / float(len(test_data)):.4f} \n"
                  f"TIME_PER_TOKEN={time_taken_total / float(tokens_total):.7f}\n"
                  f"OOV_RATE={unknown_total / float(hyp_total):.5f} \n")


if __name__ == '__main__':
    main()
