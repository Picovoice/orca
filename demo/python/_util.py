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

import platform
import subprocess


RASPBERRY_PI_MACHINES = {
    "cortex-a53",
    "cortex-a72",
    "cortex-a76",
    "cortex-a53-aarch64",
    "cortex-a72-aarch64",
    "cortex-a76-aarch64",
}
JETSON_MACHINES = {"cortex-a57-aarch64"}


def _is_64bit():
    return "64bit" in platform.architecture()[0]


def linux_machine() -> str:
    machine = platform.machine()
    if machine == "x86_64":
        return machine
    elif machine in ["aarch64", "armv7l"]:
        arch_info = ("-" + machine) if _is_64bit() else ""
    else:
        raise NotImplementedError("Unsupported CPU architecture: `%s`" % machine)

    cpu_info = ""
    try:
        cpu_info = subprocess.check_output(["cat", "/proc/cpuinfo"]).decode("utf-8")
        cpu_part_list = [x for x in cpu_info.split("\n") if "CPU part" in x]
        cpu_part = cpu_part_list[0].split(" ")[-1].lower()
    except Exception as e:
        raise RuntimeError("Failed to identify the CPU with `%s`\nCPU info: `%s`" % (e, cpu_info))

    if "0xd03" == cpu_part:
        return "cortex-a53" + arch_info
    elif "0xd07" == cpu_part:
        return "cortex-a57" + arch_info
    elif "0xd08" == cpu_part:
        return "cortex-a72" + arch_info
    elif "0xd0b" == cpu_part:
        return "cortex-a76" + arch_info
    else:
        raise NotImplementedError("Unsupported CPU: `%s`." % cpu_part)


__all__ = [
    "JETSON_MACHINES",
    "linux_machine",
    "RASPBERRY_PI_MACHINES",
]
