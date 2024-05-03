import os
import shutil

import setuptools

INCLUDE_FILES = [
    "../../LICENSE",
    "orca_demo.py",
    "orca_demo_streaming.py"]

os.system("git clean -dfx")

package_folder = os.path.join(os.path.dirname(__file__), "pvorcademo")
os.mkdir(package_folder)
manifest_in = ""

for rel_path in INCLUDE_FILES:
    shutil.copy(os.path.join(os.path.dirname(__file__), rel_path), package_folder)
    manifest_in += "include pvorcademo/%s\n" % os.path.basename(rel_path)

with open(os.path.join(os.path.dirname(__file__), "MANIFEST.in"), "w") as f:
    f.write(manifest_in)

with open(os.path.join(os.path.dirname(__file__), "README.md"), "r") as f:
    long_description = f.read()

setuptools.setup(
    name="pvorcademo",
    version="0.1.3",
    author="Picovoice",
    author_email="hello@picovoice.ai",
    description="Orca Text-to-Speech Engine demos",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/Picovoice/orca",
    packages=["pvorcademo"],
    install_requires=["numpy>=1.24.0", "sounddevice==0.4.6", "tiktoken==0.6.0"],
    include_package_data=True,
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: Apache Software License",
        "Operating System :: OS Independent",
        "Programming Language :: Python :: 3",
        "Topic :: Multimedia :: Sound/Audio :: Speech"
    ],
    entry_points=dict(
        console_scripts=[
            "orca_demo=pvorcademo.orca_demo:main",
            "orca_demo_streaming=pvorcademo.orca_demo_streaming:main",
        ],
    ),
    python_requires=">=3.8",
    keywords="Text-to-Speech, TTS, Speech Synthesis, Voice Generation, Speech Engine",
)
