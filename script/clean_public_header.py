import os
import shutil
import sys

from argparse import ArgumentParser

PEER_MODULE_DIR = os.environ.get("PV_PEER_MODULE_DIR", os.path.join(os.path.dirname(__file__), '..', '..'))
sys.path.append(os.path.join(PEER_MODULE_DIR, 'zoo-dev/script'))

from util.clean_public_headers import read_and_basic_clean

def clean_orca_header(output_dir: str):
    input_path = os.path.join(os.path.dirname(__file__), "../include/orca/pv_orca.h")
    content = read_and_basic_clean(input_path)

    with open(os.path.join(output_dir, "pv_orca.h"), 'w') as f:
        f.write(content)


def main():
    parser = ArgumentParser()
    parser.add_argument(
        '--picovoice_header_path', '-p',
        required=True,
        type=str)
    parser.add_argument(
        '--output_dir', '-o',
        required=True,
        type=str)
    args = parser.parse_args()

    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)

    if not os.path.exists(args.picovoice_header_path):
        print(f"picovoice.h not found at {args.picovoice_header_path}")
        sys.exit(1)

    shutil.copy(args.picovoice_header_path, os.path.join(args.output_dir, "picovoice.h"))


    clean_orca_header(output_dir=args.output_dir)



if __name__ == "__main__":
    main()
