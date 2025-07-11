import logging
import os
import sys
from argparse import ArgumentParser

PEER_MODULE_DIR = os.environ.get("PV_PEER_MODULE_DIR", os.path.join(os.path.dirname(__file__), '..', '..'))
sys.path.append(os.path.join(PEER_MODULE_DIR, 'zoo-dev/script'))

from automation.artifact_copy import copy_artifacts, platform_archs
from util.build import Platforms


def main():
    parser = ArgumentParser()
    parser.add_argument(
        '--platform',
        required=True,
        choices=[x.value for x in Platforms],
        help='The target platform for compilation')
    args = parser.parse_args()

    args.platform = Platforms(args.platform)
    products = ["orca"]
    try:
        for arch in platform_archs(args.platform):
            copy_artifacts(
                platform_name=args.platform,
                root_dir=os.path.join(os.path.dirname(__file__), ".."),
                arch=arch,
                products=products,
                products_java=products,
                products_node=products,
                products_binaries=[],
                use_product_subdir=False)
    except Exception as e:
        logging.error(e)
        sys.exit(1)


if __name__ == '__main__':
    main()
