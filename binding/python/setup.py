#
# Copyright 2024 Picovoice Inc.
#
# You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
# file accompanying this source.
#
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
# an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
# specific language governing permissions and limitations under the License.
#

import os
import shutil

import setuptools

INCLUDE_FILES = ('../../LICENSE', '__init__.py', '_factory.py', '_orca.py', '_util.py')
INCLUDE_LIBS = ('linux', 'mac', 'raspberry-pi', 'windows')
DEFAULT_MODEL_FILE = 'orca_params_female.pv'

os.system('git clean -dfx')

package_folder = os.path.join(os.path.dirname(__file__), 'pvorca')
os.mkdir(package_folder)
manifest_in = ""

for rel_path in INCLUDE_FILES:
    shutil.copy(os.path.join(os.path.dirname(__file__), rel_path), package_folder)
    manifest_in += "include pvorca/%s\n" % os.path.basename(rel_path)

os.mkdir(os.path.join(package_folder, 'lib'))
for platform in INCLUDE_LIBS:
    shutil.copytree(
        os.path.join(os.path.dirname(__file__), '../../lib', platform),
        os.path.join(package_folder, 'lib', platform))

os.makedirs(os.path.join(package_folder, 'lib', 'common'))
shutil.copy(
    os.path.join(os.path.dirname(__file__), '../../lib/common', DEFAULT_MODEL_FILE),
    os.path.join(package_folder, 'lib', 'common', DEFAULT_MODEL_FILE))

manifest_in += "recursive-include pvorca/lib *\n"

with open(os.path.join(os.path.dirname(__file__), 'MANIFEST.in'), 'w') as f:
    f.write(manifest_in)

with open(os.path.join(os.path.dirname(__file__), 'README.md'), 'r') as f:
    long_description = f.read()

setuptools.setup(
    name="pvorca",
    version="1.0.2",
    author="Picovoice",
    author_email="hello@picovoice.ai",
    description="Orca Streaming Text-to-Speech Engine",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/Picovoice/orca",
    packages=["pvorca"],
    include_package_data=True,
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: Apache Software License",
        "Operating System :: OS Independent",
        "Programming Language :: Python :: 3",
        "Topic :: Multimedia :: Sound/Audio :: Speech",
    ],
    python_requires='>=3.9',
    keywords="Streaming Text-to-Speech, TTS, Speech Synthesis, Voice Generation, Speech Engine",
)
