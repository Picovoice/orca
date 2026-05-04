import os
import shutil
import struct
import subprocess
from argparse import ArgumentParser
import locale

from _util import (
    Languages,
    Speakers,
)

LANGUAGES_WITH_GENDERED_NOUNS = {
    Languages.GERMAN.value,
    Languages.FRENCH.value,
    Languages.SPANISH.value,
    Languages.ITALIAN.value,
    Languages.PORTUGUESE.value,
}

# TODO: add more language here
LANGUAGES_WITH_HETERONYMS = {
    Languages.ENGLISH.value,
    Languages.GERMAN.value,
}


def normalize_dict(input_path: str, output_path: str) -> None:
    d = dict()
    if os.stat(input_path).st_size > 0:
        with open(input_path) as f:
            for line in f.read().strip('\n').split('\n'):
                word, pro = line.split('\t')
                # TODO (Ted): Below line removing parenthesis has issue with Japanese base lexicon which
                # contain words with parenthesis. Will deal with it after Japanese Orca is properly updated.
                word = word.split('(')[0]
                if word in d:
                    # heteronym have "_1" or "_2" after them, so there should not be any duplicates
                    # makesure this pattern is align with the script in zoo-research, where the dictionary is generated
                    raise ValueError(f"Multiple pronunciations for `{word}`. Choose a different lexicon file")
                else:
                    d[word] = pro

    with open(output_path, 'w') as f:
        old_collate_locale = locale.getlocale(locale.LC_COLLATE)
        print(f"Current LC_COLLATE locale: {old_collate_locale}")
        locale.setlocale(locale.LC_COLLATE, "C")
        print(f"Set LC_COLLATE locale for sorting to: {locale.getlocale(locale.LC_COLLATE)}")

        words = set()
        for word, _ in list(d.items()):
            if word[-1].isnumeric() and word[-2] == "_":
                words.add(word[:-2])
            else:
                words.add(word)
        words = sorted(words, key=locale.strxfrm)

        for word in words:
            if word in d:
                pro = d[word]
                f.write(f'{word}\t{pro}\n')
            else:
                if (word+"_1") in d:
                    pro = d[word+"_1"]
                    f.write(f'{word}\t{pro}\n')
                if (word+"_2") in d:
                    pro = d[word+"_2"]
                    f.write(f'{word}\t{pro}\n')

        locale.setlocale(locale.LC_COLLATE, old_collate_locale)
        print(f"Set LC_COLLATE locale back to: {locale.getlocale(locale.LC_COLLATE)}")


def lexicon_from_dict(dict_path: str, lexicon_path: str) -> None:
    with open(dict_path) as f:
        old_collate_locale = locale.getlocale(locale.LC_COLLATE)
        print(f"Current LC_COLLATE locale: {old_collate_locale}")
        locale.setlocale(locale.LC_COLLATE, "C")
        print(f"Set LC_COLLATE locale for sorting to: {locale.getlocale(locale.LC_COLLATE)}")

        words = '\n'.join(sorted(set([x.split('\t')[0] for x in f.read().strip('\n ').split('\n')]),
                                 key=locale.strxfrm))

        locale.setlocale(locale.LC_COLLATE, old_collate_locale)
        print(f"Set LC_COLLATE locale back to: {locale.getlocale(locale.LC_COLLATE)}")

    with open(lexicon_path, 'w') as f:
        f.write(words)
        f.write('\n')


def c_from_bin(bin_path: str, c_path: str, speaker_or_language: str) -> None:
    indent = 8
    line_width = 120

    with open(bin_path, 'rb') as f:
        array = f.read()
        res = list()

        array = ['0x%s' % z.hex() for z in struct.unpack('%dc' % len(array), array)]

        row = ' ' * indent
        last_x = 0
        for x in array:
            if len(row) >= line_width:
                row = row.rsplit(', ', maxsplit=1)[0] + ','
                res.append(row)
                row = ' ' * indent + last_x

            if row != ' ' * indent:
                row += ', '
            row += x

            last_x = x

        if row != ' ' * indent:
            res.append(row)
        res.append('')

    variable = os.path.basename(c_path).rsplit('.', maxsplit=1)[0].upper()
    variable = variable.rsplit(speaker_or_language.upper(), 1)[0].rstrip("_")

    guard = os.path.basename(c_path).replace('.', '_').upper()

    with open(c_path, 'w') as f:
        f.write(f'#ifndef {guard}\n')
        f.write(f'#define {guard}\n\n')
        f.write('#include <stdint.h>\n\n')
        f.write(f'const uint8_t {variable}[] = {{\n')
        f.write('\n'.join(res))
        f.write('};\n\n')
        f.write(f'const int32_t {variable}_LENGTH = {len(array)};\n\n')
        f.write(f'#endif // {guard}\n')


def main():
    parser = ArgumentParser()
    parser.add_argument(
        '--language',
        '-l',
        default=Languages.ENGLISH.value,
        choices=[lang.value for lang in Languages])
    parser.add_argument(
        '--speakers',
        '-s',
        nargs="+",
        default=[speaker.value for speaker in Speakers],
        choices=[speaker.value for speaker in Speakers])
    parser.add_argument('--phonebook_path', '-p', required=True)
    parser.add_argument(
        '--heteronym_path', '-t',
        default="res/heteronym/",
        help="Path to the folder with heteronym tree file in. Default: res/heteronym/")
    parser.add_argument(
        '--dictionary_folder',
        default="res/dictionary/",
        help="Path to the folder with speaker dictionary file in. Default: res/dictionary/")
    parser.add_argument(
        '--noun_gender_dict_folder',
        default="../normalizer/res/noun_gender_dict/",
        help="Path to the folder with speaker noun_gender_dict file in. Default: ../normalizer/res/noun_gender_dict/")
    parser.add_argument(
        '--zoodev_path',
        '-z',
        default=os.path.abspath(os.path.expanduser("~/work/gitlab/zoo-dev")))
    parser.add_argument(
        '--orca_path',
        default=os.path.abspath(os.path.expanduser("~/work/gitlab/orca")))
    parser.add_argument(
        '--language_info_dir',
        '-i',
        default=os.path.abspath(os.path.expanduser("~/work/gitlab/orca/res/core/")))
    parser.add_argument(
        '--working_dir',
        '-w',
        default=os.path.abspath(os.path.expanduser("~/work/orca_dict")))
    parser.add_argument('--save_working_dir', '-sw', action="store_true")
    parser.add_argument('--create_noun_gender_dict', '-ng', action="store_true")
    parser.add_argument('--output_dir', '-o', default="src/dict/")
    args = parser.parse_args()

    language = args.language
    speakers = args.speakers
    phonebook_path = args.phonebook_path
    heteronym_path = args.heteronym_path
    working_dir = args.working_dir
    save_working_dir = args.save_working_dir
    language_info_dir = args.language_info_dir
    zoodev_path = args.zoodev_path
    orca_path = args.orca_path
    output_dir = args.output_dir
    dictionary_folder = args.dictionary_folder
    noun_gender_dict_folder = args.noun_gender_dict_folder

    lexicon_app = os.path.join(zoodev_path, "build/release/x86_64/src/lm/pv_lexicon")
    dict_app = os.path.join(zoodev_path, "build/release/x86_64/src/lm/pv_dict")
    tree_app = os.path.join(zoodev_path, "build/release/x86_64/src/lm/pv_heteronym_tree_serializer")
    noun_gender_dict_app = os.path.join(zoodev_path, "build/release/x86_64/src/lm/pv_noun_gender_dict")

    os.makedirs(working_dir, exist_ok=True)

    for speaker in speakers:
        try:
            raw_dict_path = os.path.join(dictionary_folder, f"dictionary_{speaker}.txt")
            if not os.path.isfile(raw_dict_path):
                raise FileNotFoundError(f"Dictionary file not found: {raw_dict_path}")

            print(f"{speaker}: Normalizing dict")
            norm_dict_path = os.path.join(working_dir, f"{speaker}-lexicon-norm.txt")
            normalize_dict(input_path=raw_dict_path, output_path=norm_dict_path)

            print(f"{speaker}: Creating lexicon from dict")
            lexicon_path = os.path.join(working_dir, f"{speaker}-lexicon.txt")
            lexicon_from_dict(dict_path=norm_dict_path, lexicon_path=lexicon_path)

            print(f"{speaker}: Building binary dict")
            phonemes_path = os.path.join(phonebook_path, f"{language}-phonemes.txt")
            bin_dict_path = os.path.join(working_dir, f"{speaker}-dict.bin")
            language_info_path = os.path.join(language_info_dir, f"pv_language_info_{language}.json")
            cmd = [dict_app, phonemes_path, norm_dict_path, lexicon_path, bin_dict_path, language_info_path]
            print(f"{speaker}:", " ".join(cmd))
            subprocess.run(cmd, stderr=subprocess.STDOUT)

            print(f"{speaker}: Building C array from dict")
            c_dict_path = os.path.join(orca_path, output_dir, f"pv_dict_{speaker}.c")
            c_from_bin(bin_path=bin_dict_path, c_path=c_dict_path, speaker_or_language=speaker)

            print(f"{speaker}: Building binary lexicon")
            bin_lexicon_path = os.path.join(working_dir, f"{speaker}-lexicon.bin")
            cmd = [lexicon_app, '-t', lexicon_path, '-b', bin_lexicon_path, '-i', language_info_path]
            print(f"{speaker}:", " ".join(cmd))
            subprocess.run(cmd, stderr=subprocess.STDOUT)

            print(f"{speaker}: Building C array from lexicon")
            c_lexicon_path = os.path.join(orca_path, output_dir, f"pv_lexicon_{speaker}.c")
            c_from_bin(bin_path=bin_lexicon_path, c_path=c_lexicon_path, speaker_or_language=speaker)

            print(f"{speaker}: Building binary heteronym tree")
            tree_path =  os.path.join(heteronym_path, "empty-trees.txt")
            if language in LANGUAGES_WITH_HETERONYMS:
                tree_path = os.path.join(heteronym_path, f"{language}-trees.txt")
            bin_tree_path = os.path.join(working_dir, f"{speaker}-trees.bin")
            cmd = [tree_app, tree_path, bin_tree_path, lexicon_path, language_info_path]
            print(f"{speaker}:", " ".join(cmd))
            subprocess.run(cmd, stderr=subprocess.STDOUT)

            print(f"{speaker}: Building C array from heteronym tree")
            c_tree_path = os.path.join(orca_path, output_dir, f"pv_heteronym_tree_{speaker}.c")
            c_from_bin(bin_path=bin_tree_path, c_path=c_tree_path, speaker_or_language=speaker)


        finally:
            if not save_working_dir:
                shutil.rmtree(working_dir)

    os.makedirs(working_dir, exist_ok=True)
    try:
        raw_noun_gender_dict_path = os.path.join(noun_gender_dict_folder, "empty.txt")
        if language in LANGUAGES_WITH_GENDERED_NOUNS:
            raw_noun_gender_dict_path = os.path.join(
                noun_gender_dict_folder,
                f"noun_gender_dict_{language}.txt",
            )

        if not os.path.isfile(raw_noun_gender_dict_path):
            raise FileNotFoundError(f"Noun gender dict file not found: {raw_noun_gender_dict_path}")

        print(f"{language}: Building binary noun gender dict")
        bin_noun_gender_dict_path = os.path.join(working_dir, f"{language}-nouns-gendered.bin")
        cmd = [noun_gender_dict_app, raw_noun_gender_dict_path, bin_noun_gender_dict_path]
        print(f"{language}:", " ".join(cmd))
        subprocess.run(cmd, stderr=subprocess.STDOUT)

        print(f"{language}: Building C array from noun gender dict")
        c_noun_gender_dict_path = os.path.join(
            orca_path,
            output_dir,
            f"pv_noun_gender_dict_{language}.c",
        )
        c_from_bin(
            bin_path=bin_noun_gender_dict_path,
            c_path=c_noun_gender_dict_path,
            speaker_or_language=language,
        )
    finally:
        if not save_working_dir:
            shutil.rmtree(working_dir)


if __name__ == '__main__':
    main()
