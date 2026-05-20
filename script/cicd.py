import glob
import json
import logging
import os
import platform
import shutil
import sys
from argparse import ArgumentParser
from multiprocessing.pool import ThreadPool
from .orca_symbols import *

PEER_MODULE_DIR = os.environ.get("PV_PEER_MODULE_DIR", os.path.join(os.path.dirname(__file__), '..', '..'))
sys.path.append(os.path.join(PEER_MODULE_DIR, 'zoo-dev/script'))

from automation.check_symbols import check_symbols
from automation.gitlab_runner import (
    ANDROID_ABIS,
    ANDROID_CLANG_ABIS,
    get_mem_usage_bytes,
    KeymakerEndpoints,
    OSX_ARCHS,
    PARALLEL_TEST_PLATFORMS,
    run_command,
    run_jni_test,
    run_unit_test,
    test_build,
    test_napi,
    run_windows_ucrt_test,
    update_environment,
)
from util.build import Platforms, TestTypes


def check_symbols_orca(
    build_dir: str,
    platform_name: Platforms
):
    check_symbols(
        build_dir=build_dir,
        platform_name=platform_name,
        expected_symbols=EXPECTED_ORCA_SYMBOLS,
        expected_symbols_jni=EXPECTED_ORCA_SYMBOLS_JNI,
        expected_symbols_node=EXPECTED_ORCA_SYMBOLS_NODE,
        expected_symbols_wasm=EXPECTED_ORCA_SYMBOLS_WASM,
        expected_symbols_android=EXPECTED_ORCA_SYMBOLS_ANDROID,
        expected_symbols_ios=EXPECTED_ORCA_SYMBOLS_IOS
    )


def test_jni_orca(platform_name, build_mode, access_key):
    current_dir = os.getcwd()
    root_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
    buildPath = os.path.join(root_dir, "build", build_mode, platform.machine())
    platform_dir = os.path.join(root_dir, "platform")
    shared_res_dir = os.path.join(os.path.abspath(PEER_MODULE_DIR), "zoo-dev", "res")

    if platform_name == Platforms.WINDOWS and platform.machine() == 'AMD64':
        shutil.copy(
            os.path.join(buildPath, '_deps', 'ypu-build', 'pv_ypu_impl_cuda_orca.dll'),
            os.path.join(platform_dir, 'java'))

    cmd = f'./gradlew test -DaccessKey="{access_key}" ' \
          f'-DbuildPath="{buildPath}" ' \
          f'-DsharedResDirectory="{shared_res_dir}" ' \
          f'-DmoduleResDirectory="{root_dir}/res" -Dplatform="{platform_name.value}"'
    if platform_name == Platforms.LINUX:
        cmd += ' -DlibExt="so"'
    elif platform_name == Platforms.MAC:
        cmd += ' -DlibExt="dylib"'
    elif platform_name == Platforms.WINDOWS:
        cmd = cmd[2:]
        cmd += ' -DlibExt="dll"'
    else:
        return 0

    os.chdir(platform_dir)
    res = run_jni_test(cmd)
    os.chdir(current_dir)
    return res


def test_napi_orca(platform_name, build_mode, access_key):
    root_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
    build_path = os.path.join(root_dir, "build", build_mode, platform.machine())
    node_test_dir = os.path.join(root_dir, 'platform', 'node')

    if platform_name == Platforms.WINDOWS and platform.machine() == 'AMD64':
        shutil.copy(
            os.path.join(build_path, '_deps', 'ypu-build', 'pv_ypu_impl_cuda_orca.dll'),
            node_test_dir)

    res = test_napi(
        platform_name=platform_name,
        access_key=access_key,
        node_test_dir=node_test_dir)

    return res


def run_mem_usage_orca(build_mode, access_key):
    root_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
    build_path = os.path.join(root_dir, "build", build_mode, platform.machine())

    module_res_dir = os.path.join(root_dir, "res")
    shared_res_dir = os.path.join(os.path.abspath(PEER_MODULE_DIR), "zoo-dev", "res")

    input_text_file = os.path.join(shared_res_dir, "wav", "very_long_sample.txt")
    with open(input_text_file, "r") as f:
        input_text = f.read()

    model_file = os.path.join(module_res_dir, "param", "orca_params_en_male.pv")
    output_file = os.path.join(module_res_dir, "output.wav")

    command = f"{build_path}/pv_orca_app -a {access_key} -m {model_file} -y cpu:1 -t '{input_text}' -o {output_file}"
    mem_usage = get_mem_usage_bytes(command)

    os.remove(output_file)

    if mem_usage == 0:
        return 1

    extract_mem_dir = os.path.join(os.path.dirname(__file__), "..", "res", "performance")
    process_memory_file = os.path.join(extract_mem_dir, "process_memory.json")

    process_memory_dict = {'pv_orca_app_bytes': mem_usage}
    with open(process_memory_file, "w") as f:
        json.dump(process_memory_dict, f, indent=4)

    return 0


def test_unittest_orca(
        platform_name,
        build_mode,
        test_types=[],
        disable_mocks=False):
    peer_dir = os.path.join(os.path.abspath(PEER_MODULE_DIR) ,'zoo-dev')
    root_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')

    # unittest project build
    if platform_name == Platforms.ANDROID:
        os.chdir(peer_dir)
        for arch, clang_arch in zip(ANDROID_ABIS, ANDROID_CLANG_ABIS):
            dst_path = os.path.join(peer_dir, 'android', 'app', 'src', 'main', 'jniLibs', arch)
            if not os.path.exists(dst_path):
                os.mkdir(dst_path)

            build_dir = os.path.join(root_dir, 'build', build_mode, arch)

            shutil.copy(os.path.join(build_dir, 'libpv_orca_test_collections.so'), dst_path)
            if not disable_mocks:
                shutil.copy(os.path.join(build_dir, 'libpv_orca_mock_tree_shared.so'), dst_path)
                shutil.copy(os.path.join(build_dir, 'libpv_orca_real_pv_orca_mock_tree.so'), dst_path)

            dependencies_build_dirs = [
                os.path.join(build_dir, '_deps', 'zoo-dev-build', 'src'),
                os.path.join(build_dir, '_deps', 'ypu-build'),
                os.path.join(build_dir, '_deps', 'model-build'),
                os.path.join(build_dir, '_deps', 'hippo-build'),
                os.path.join(build_dir, '_deps', 'normalizer-build'),
            ]
            for dep_build_dir in dependencies_build_dirs:
                for module in os.listdir(dep_build_dir):
                    if module == 'test' or module == 'mock':
                        continue
                    for src_path in glob.glob(os.path.join(dep_build_dir, module, "*_real_pv_orca_mock_tree.so")):
                        shutil.copy(src_path, dst_path)
                    for src_path in glob.glob(os.path.join(dep_build_dir, module, "*_real.so")):
                        shutil.copy(src_path, dst_path)

                for src_path in glob.glob(os.path.join(dep_build_dir, "*_real_pv_orca_mock_tree.so")):
                    shutil.copy(src_path, dst_path)
                for src_path in glob.glob(os.path.join(dep_build_dir, "*_real.so")):
                    shutil.copy(src_path, dst_path)

        setup_command_1 = ("grep -q '^testCollectionsLib=' local.properties && "
                           "sed -i 's/^testCollectionsLib=.*/testCollectionsLib=pv_orca_test_collections/' local.properties || "
                           "echo 'testCollectionsLib=pv_orca_test_collections' >> local.properties")
        setup_command_2 = ("grep -q '^gatekeeperRealLib=' local.properties && "
                           "sed -i 's/^gatekeeperRealLib=.*/gatekeeperRealLib=pv_gatekeeper_device_based_real_pv_orca_mock_tree/' local.properties || "
                           "echo 'gatekeeperRealLib=pv_gatekeeper_device_based_real_pv_orca_mock_tree' >> local.properties")
        res = run_command(f"cd android && {setup_command_1} && {setup_command_2}")
        if res != 0:
            return res

        res = run_command(f"cd android && ./gradlew assembleDebug")
        if res != 0:
            return res

        res = run_command(f"cd android && ./gradlew assembleAndroidTest")
        if res != 0:
            return res

    elif platform_name == Platforms.IOS:
        os.chdir(peer_dir)
        res = run_command(f"cp -r {root_dir}/build/{build_mode}/xcframeworks/PvTest.xcframework ios/test")
        if res != 0:
            return res

        if TestTypes.PERFORMANCE not in test_types:
            destination = 'platform=iOS Simulator,name=iPhone 11 Pro,OS=17.5'
        else:
            destination = 'platform=iOS,name=Picollm iPhone 13'
        res = run_command(
            f"cd ios/test && xcrun xcodebuild build-for-testing "
            f"-configuration Debug -project test.xcodeproj "
            f"-sdk iphoneos -scheme PvTestRunner "
            f"-destination '{destination}' CODE_SIGNING_ALLOWED=NO -allowProvisioningUpdates")
        if res != 0:
            return res

    # unittests
    try:
        os.makedirs(f"dump_{build_mode}", exist_ok=True)
    except OSError as e:
        print(f"Error: {e.strerror}")

    if platform_name == Platforms.ANDROID:
        # testing on Pixel_4_API_30
        run_command("adb root")
        run_command("adb shell setenforce 0")
        results = []
        env_path = os.path.join(peer_dir, 'android', 'app', 'src', 'androidTest', 'assets', 'res', 'environment.json')
        update_environment(env_path)
        if TestTypes.UNIT in test_types:
            res_unit_test = run_unit_test(
                f"cd android && "
                f"./gradlew connectedAndroidTest "
                f"-i "
                f"-Pandroid.testInstrumentationRunnerArguments.class=ai.picovoice.zoo.ZooDevUnitTestRunner",
                platform_name)
            results.append(res_unit_test)

        if TestTypes.INTEGRATION in test_types:
            res_integration_test = run_unit_test(
                f"cd android && "
                f"./gradlew connectedAndroidTest "
                f"-i "
                f"-Pandroid.testInstrumentationRunnerArguments.class=ai.picovoice.zoo.ZooDevIntegrationTestRunner",
                platform_name)
            results.append(res_integration_test)

        if TestTypes.PERFORMANCE in test_types:
            res_performance_test = run_unit_test(
                f"cd android && "
                f"adb logcat -c && "
                f"./gradlew connectedAndroidTest "                
                f"-Pandroid.testInstrumentationRunnerArguments.class=ai.picovoice.zoo.ZooDevPerformanceTestRunner; "
                "adb logcat -d -s PICOVOICE",
                platform_name,
                extract_mem_dir=os.path.join(os.path.dirname(__file__), "..", "res", "performance"),
                extract_perf_dir=os.path.join(os.path.dirname(__file__), "..", "res", "performance"))
            results.append(res_performance_test)

        res = 1 if any(results) else 0

    elif platform_name == Platforms.IOS:
        # testing on iPhone 11 Pro
        results = []
        env_path = os.path.join(peer_dir, 'ios', 'test', 'res', 'environment.json')
        update_environment(env_path)
        if TestTypes.UNIT in test_types:
            res_unit_test = run_unit_test(
                f"cd ios/test && xcrun xcodebuild test "
                f"-scheme PvTestRunner "
                f"-destination 'platform=iOS Simulator,name=iPhone 11 Pro,OS=17.5' "
                f"-only-testing PvTestRunnerUITests/PvUnitTests", platform_name)
            results.append(res_unit_test)

        if TestTypes.INTEGRATION in test_types:
            res_integration_test = run_unit_test(
                f"cd ios/test && xcrun xcodebuild test "
                f"-scheme PvTestRunner "
                f"-destination 'platform=iOS Simulator,name=iPhone 11 Pro,OS=17.5' "
                f"-only-testing PvTestRunnerUITests/PvIntegrationTests", platform_name)
            results.append(res_integration_test)

        if TestTypes.PERFORMANCE in test_types:
            res_performance_test = run_unit_test(
                f"cd ios/test && xcrun xcodebuild test "
                f"-scheme PvTestRunner "
                f"-destination 'platform=iOS,name=Picollm iPhone 13' "
                f"-only-testing PvTestRunnerUITests/PvPerformanceTests -allowProvisioningUpdates",
                platform_name,
                extract_mem_dir=os.path.join(os.path.dirname(__file__), "..", "res", "performance"),
                extract_perf_dir=os.path.join(os.path.dirname(__file__), "..", "res", "performance"))
            results.append(res_performance_test)

        res = 1 if any(results) else 0

    elif platform_name == Platforms.WASM:
        logging.info("Installing unit test dependencies")
        install_dir = os.path.join(peer_dir, 'wasm', 'wasi-zoo', 'unit_test', 'browser')
        res = run_command(f"cd {install_dir} && yarn && cd {root_dir}")
        if res != 0:
            return res

        res = run_command(f'cd {install_dir} && echo "window.PV_TEST_COLLECTIONS = \'pv_orca_test_collections\';" > env.js')
        if res != 0:
            return res

        res = 0
        selenium_script = os.path.join(peer_dir, 'script', 'wasm', 'selenium_runner.py')
        for arg in ["", "--no_pthread"]:
            if res == 0:
                if arg != "--no_pthread":
                    logging.info(f"running unit test in browser (pthread) ...")
                else:
                    logging.info(f"running unit test in browser ...")
                results = []

                env_path = os.path.join(peer_dir, 'wasm', 'wasi-zoo', 'unit_test', 'wasm', 'res', 'environment.json')
                update_environment(env_path)

                if TestTypes.UNIT in test_types:
                    res_unit_test = run_command(
                        f"python3 {selenium_script} {arg} "
                        f"--build_mode {build_mode} --root_path {peer_dir} --test_type unit")
                    results.append(res_unit_test)

                if TestTypes.INTEGRATION in test_types:
                    res_integration_test = run_command(
                        f"python3 {selenium_script} {arg} "
                        f"--build_mode {build_mode} --root_path {peer_dir} --test_type integration")
                    results.append(res_integration_test)

                if TestTypes.PERFORMANCE in test_types and arg == "":
                    res_performance_test = run_command(
                        f"python3 {selenium_script} {arg} "
                        f"--build_mode {build_mode} --root_path {peer_dir} --test_type performance",
                        extract_mem_dir=None,
                        extract_perf_dir=os.path.join(os.path.dirname(__file__), "..", "res", "performance"))
                    results.append(res_performance_test)

                res = 1 if any(results) else 0

    elif platform_name == Platforms.WASM_LINUX:
        results = []
        if TestTypes.UNIT in test_types:
            res_unit_test = run_unit_test(
                f"{root_dir}/build/{build_mode}/{platform.machine()}/"
                f"pv_orca_test_app res", platform_name)
            results.append(res_unit_test)

        if TestTypes.INTEGRATION in test_types:
            res_integration_test = run_unit_test(
                f"{root_dir}/build/{build_mode}/{platform.machine()}/"
                f"pv_orca_test_app res -i", platform_name)
            results.append(res_integration_test)

        if TestTypes.PERFORMANCE in test_types:
            res_performance_test = run_unit_test(
                f"{root_dir}/build/{build_mode}/{platform.machine()}/"
                f"pv_orca_test_app res -i",
                platform_name,
                extract_mem_dir=os.path.join(os.path.dirname(__file__), "..", "res", "performance"),
                extract_perf_dir=os.path.join(os.path.dirname(__file__), "..", "res", "performance"))
            results.append(res_performance_test)

        res = 1 if any(results) else 0

    elif platform_name == Platforms.WINDOWS:
        results = []

        if platform.machine() == 'AMD64':
            shutil.copy(
                os.path.join(root_dir, 'build', build_mode, platform.machine(), '_deps', 'ypu-build', 'pv_ypu_impl_cuda_orca.dll'),
                os.path.join(root_dir, 'build', build_mode, platform.machine()))

        if TestTypes.UNIT in test_types:
            res_unit_test = run_unit_test(
                f"{root_dir}/build/{build_mode}/{platform.machine()}/"
                f"pv_orca_test_app.exe res -d dump_{build_mode}", platform_name)
            results.append(res_unit_test)

        if TestTypes.INTEGRATION in test_types:
            res_integration_test = run_unit_test(
                f"{root_dir}/build/{build_mode}/{platform.machine()}/"
                f"pv_orca_test_app.exe res -i -d dump_{build_mode}", platform_name)
            results.append(res_integration_test)

        if TestTypes.PERFORMANCE in test_types:
            res_performance_test = run_unit_test(
                f"{root_dir}/build/{build_mode}/{platform.machine()}/"
                f"pv_orca_test_app.exe res -p -d dump_{build_mode}",
                platform_name,
                extract_mem_dir=os.path.join(os.path.dirname(__file__), "..", "res", "performance"),
                extract_perf_dir=os.path.join(os.path.dirname(__file__), "..", "res", "performance"))
            results.append(res_performance_test)

        absolute_peer_module_dir = os.path.join(os.path.dirname(__file__), '..', PEER_MODULE_DIR)
        build_dir = os.path.join(root_dir, 'build', build_mode)
        res_ucrt_test = run_windows_ucrt_test(
            root_dir,
            os.path.join(root_dir, build_dir, platform.machine().lower()),
            build_mode,
            ["orca"],
            peer_module_dir=absolute_peer_module_dir)
        results.append(res_ucrt_test)

        res = 1 if any(results) else 0

    elif platform_name == Platforms.MAC:
        for arch in OSX_ARCHS:
            results = []

            if TestTypes.UNIT in test_types:
                res_unit_test = run_unit_test(
                    f"{root_dir}/build/{build_mode}/{arch}/"
                    f"pv_orca_test_app res -d dump_{build_mode}", platform_name)
                results.append(res_unit_test)

            if TestTypes.INTEGRATION in test_types:
                res_integration_test = run_unit_test(
                    f"{root_dir}/build/{build_mode}/{arch}/"
                    f"pv_orca_test_app res -i -d dump_{build_mode}", platform_name)
                results.append(res_integration_test)

            if TestTypes.PERFORMANCE in test_types and arch == 'arm64':
                res_performance_test = run_unit_test(
                    f"{root_dir}/build/{build_mode}/{arch}/"
                    f"pv_orca_test_app res -p -d dump_{build_mode}",
                    platform_name,
                    extract_mem_dir=os.path.join(os.path.dirname(__file__), "..", "res", "performance"),
                    extract_perf_dir=os.path.join(os.path.dirname(__file__), "..", "res", "performance"))
                results.append(res_performance_test)

            res = 1 if any(results) else 0
            if res != 0:
                break

    elif platform_name == Platforms.MCU:
        return 0
    else:
        build_dir = f"build/{build_mode}/{platform.machine()}"
        results = []

        if TestTypes.UNIT in test_types:
            res_unit_test = run_unit_test(
                f"{root_dir}/{build_dir}/pv_orca_test_app res -d dump_{build_mode}",
                platform_name)
            results.append(res_unit_test)

        if TestTypes.INTEGRATION in test_types:
            res_integration_test = run_unit_test(
                f"{root_dir}/{build_dir}/pv_orca_test_app res -i -d dump_{build_mode}",
                platform_name)
            results.append(res_integration_test)

        if TestTypes.PERFORMANCE in test_types:
            res_performance_test = run_unit_test(
                f"{root_dir}/{build_dir}/pv_orca_test_app res -p -d dump_{build_mode}",
                platform_name,
                extract_mem_dir=os.path.join(os.path.dirname(__file__), "..", "res", "performance"),
                extract_perf_dir=os.path.join(os.path.dirname(__file__), "..", "res", "performance"))
            results.append(res_performance_test)

        res = 1 if any(results) else 0

    try:
        shutil.rmtree(f"./dump_{build_mode}")
    except OSError as e:
        print(f"Error: {e.strerror}")

    return res


def main():
    parser = ArgumentParser()
    parser.add_argument('--build', action='store_true')
    parser.add_argument('--test', action='store_true')
    parser.add_argument('--perf', action='store_true')
    parser.add_argument('--mem', action='store_true')
    parser.add_argument(
        '--platform',
        '-p',
        required=True,
        choices=[x.value for x in Platforms],
        help='The target platform for compilation')
    parser.add_argument('--builds', '-b', nargs='+', default=['debug', 'release'])
    parser.add_argument('--keymaker_endpoint', '-k', required=True, choices=[x.value for x in KeymakerEndpoints])
    parser.add_argument('--access_key', '-a', required=True)
    parser.add_argument(
        '--repo_folder',
        '-r',
        default=os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
    parser.add_argument(
        '--build_folder',
        '-f',
        default=os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'build')))
    parser.add_argument('--toolchain_path', default=None)
    parser.add_argument('--toolchain_prefix', default=None)

    parser.add_argument('--builder_name', default=None)
    parser.add_argument('--remote_runner_ip', default=None)
    parser.add_argument('--remote_runner_user', default=None)
    parser.add_argument('--remote_runner_pwd', default=None)

    parser.add_argument('--disable_mocks', action='store_true')
    parser.add_argument('--enable_asan', action='store_true')
    args = parser.parse_args()

    args.platform = Platforms(args.platform)
    args.keymaker_endpoint = KeymakerEndpoints(args.keymaker_endpoint)

    try:
        if args.build:
            logging.info(f"Building ...")
            res = test_build(
                args.platform,
                ' '.join(args.builds),
                args.keymaker_endpoint,
                repo_folder=args.repo_folder,
                build_folder=args.build_folder,
                toolchain_path=args.toolchain_path,
                toolchain_prefix=args.toolchain_prefix,
                disable_mocks=args.disable_mocks,
                enable_asan=args.enable_asan)
            if res != 0:
                sys.exit(res)

        test_types = []
        if args.test:
            test_types.append(TestTypes.UNIT)
            test_types.append(TestTypes.INTEGRATION)

        if args.perf:
            test_types.append(TestTypes.PERFORMANCE)

        if args.mem:
            for build in args.builds:
                if build == 'release':
                    logging.info(f"Running memory usage...")
                    res = run_mem_usage_orca(build, args.access_key)
                    if res != 0:
                        sys.exit(res)

                    logging.info(f"Running memory usage completed successfully")

        if len(test_types) > 0:
            logging.info(f"Testing ...")
            for build in args.builds:
                if build == 'release':
                    logging.info(f"Running check symbol strings and visibility...")
                    try:
                        check_symbols_orca(os.path.join(args.build_folder, "release"), args.platform)
                        logging.info(f"Running check symbol strings and visibility completed successfully")
                    except ValueError as e:
                        logging.error(f"Check symbols failed with: {e}")
                        sys.exit(1)

            if not args.enable_asan:
                logging.info(f"Running JNI and NAPI  tests ...")
                for build in args.builds:
                    res = test_jni_orca(
                        platform_name=args.platform,
                        build_mode=build,
                        access_key=args.access_key)
                    if res != 0:
                        sys.exit(res)

                    res = test_napi_orca(
                        platform_name=args.platform,
                        build_mode=build,
                        access_key=args.access_key)
                    if res != 0:
                        sys.exit(res)

            logging.info(f"Running {' and '.join(map(lambda x: x.value, test_types))} tests ...")
            for build in args.builds:
                res = test_unittest_orca(
                    args.platform,
                    build,
                    test_types,
                    args.disable_mocks)
                if res != 0:
                    sys.exit(res)

    except Exception as e:
        logging.error(e)
        sys.exit(1)


if __name__ == '__main__':
    main()
