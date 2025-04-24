# Copyright 2025 Picovoice Inc.
#
# You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
# file accompanying this source.
#
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
# an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
# specific language governing permissions and limitations under the License.
#

import json
import os

from xml.dom import minidom


def update_ios_demo(models):

    scheme_dir = os.path.join(
        os.path.dirname(__file__),
        "../../demo/ios/OrcaDemo/OrcaDemo.xcodeproj/xcshareddata/xcschemes")
    base_scheme = os.path.join(
        os.path.dirname(__file__),
        scheme_dir,
        "enFemaleDemo.xcscheme")

    for model in models:
        language, gender = model.removeprefix("orca_params_").removesuffix(".pv").split("_")
        if language == 'en' and gender == 'female':
            continue

        model_camel = language + gender.capitalize()
        model_snake = language + "_" + gender
        model_scheme = os.path.join(scheme_dir, f"{model_camel}Demo.xcscheme")
        if not os.path.exists(model_scheme):
            print(f"Creating iOS demo scheme for `{model_camel}`")
            base_scheme_content = minidom.parse(base_scheme)
            pre_build_action = base_scheme_content.getElementsByTagName('ActionContent')[0]
            env_var = base_scheme_content.getElementsByTagName('EnvironmentVariable')[0]
            pre_build_action.setAttribute(
                'scriptText',
                pre_build_action.attributes['scriptText'].value.replace(" en_female", f" {model_snake}"))
            env_var.setAttribute('value', model_snake)
            with open(os.path.join(scheme_dir, f"{model_camel}Demo.xcscheme"), 'w') as f:
                f.write(base_scheme_content.toxml())
        else:
            print(f"iOS scheme for `{model_camel}` already exists")


def main():
    with open(os.path.join(os.path.dirname(__file__), "../.test/test_data.json"), encoding='utf-8') as f:
        json_content = json.loads(f.read())

    models = list()
    for sentence_test in json_content['tests']['sentence_tests']:
        for model in sentence_test['models']:
            models.append(model)

    update_ios_demo(models=models)


if __name__ == '__main__':
    main()