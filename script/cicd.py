import glob
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
    KeymakerEndpoints,
    OSX_ARCHS,
    PARALLEL_TEST_PLATFORMS,
    run_command,
    run_jni_test,
    run_napi_test,
    run_unit_test,
    test_build,
    run_windows_ucrt_test,
)
from util.build import Platforms


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

    cmd = f'./gradlew test -DaccessKey="{access_key}" ' \
          f'-DbuildPath="{buildPath}" ' \
          f'-DresourceDirectory="{root_dir}/res" -Dplatform="{platform_name.value}"'
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


def test_napi_orca(platform_name, access_key):
    test_platforms = [Platforms.LINUX, Platforms.MAC, Platforms.WINDOWS]
    if platform_name not in test_platforms:
        return 0

    os.environ['ELECTRON_RUN_AS_NODE'] = '1'
    cmd = f'yarn && yarn test --access_key={access_key} --platform={platform_name.value}'
    cmd2 = f'yarn test-electron --access_key={access_key} --platform={platform_name.value}'

    node_test_dir = os.path.join(os.path.dirname(__file__), '..', 'platform', 'node')
    if platform_name == Platforms.WINDOWS and platform.machine() == "ARM64":
        res = run_napi_test(cmd, node_test_dir=node_test_dir)
    else:
        res = run_napi_test(cmd, node_test_dir=node_test_dir) or run_napi_test(cmd2, node_test_dir=node_test_dir)

    return res


def test_unittest_orca(
        platform_name,
        build_mode):
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
            shutil.copy(os.path.join(build_dir, 'libpv_orca_mock_tree_shared.so'), dst_path)
            shutil.copy(os.path.join(build_dir, 'libpv_orca_real_pv_mock_mock_tree.so'), dst_path)
            shutil.copy(os.path.join(build_dir, 'libpv_orca_real_pv_orca_mock_tree.so'), dst_path)
            shutil.copy(os.path.join(build_dir, 'libpv_normalizer_real_pv_orca_mock_tree.so'), dst_path)

            zoo_dev_build_dir = os.path.join(build_dir, '_deps', 'zoo-dev-build', 'src')
            for module in os.listdir(zoo_dev_build_dir):
                if module == 'test' or module == 'mock':
                    continue
                for src_path in glob.glob(os.path.join(zoo_dev_build_dir, module, "*_real_pv_orca_mock_tree.so")):
                    shutil.copy(src_path, dst_path)
                for src_path in glob.glob(os.path.join(zoo_dev_build_dir, module, "*_real.so")):
                    shutil.copy(src_path, dst_path)

        setup_command = ("grep -q '^testCollectionsLib=' local.properties && "
                         "sed -i 's/^testCollectionsLib=.*/testCollectionsLib=pv_orca_test_collections/' local.properties || "
                         "echo 'testCollectionsLib=pv_orca_test_collections' >> local.properties")
        res = run_command(f"cd android && {setup_command}")
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

        res = run_command(
            f"cd ios/test && xcrun xcodebuild build-for-testing "
            f"-configuration Debug -project test.xcodeproj "
            f"-sdk iphoneos -scheme PvTestRunner "
            f"-destination 'platform=iOS Simulator,name=iPhone 11 Pro,OS=17.5' CODE_SIGNING_ALLOWED=NO")
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
        res_unit_test = run_unit_test(
            f"cd android && "
            f"./gradlew connectedAndroidTest "
            f"-i "
            f"-Pandroid.testInstrumentationRunnerArguments.class=ai.picovoice.zoo.ZooDevUnitTestRunner",
            platform_name)

        res = 1 if any([res_unit_test]) else 0

    elif platform_name == Platforms.IOS:
        # testing on iPhone 11 Pro
        res_unit_test = run_unit_test(
            f"cd ios/test && xcrun xcodebuild test "
            f"-scheme PvTestRunner "
            f"-destination 'platform=iOS Simulator,name=iPhone 11 Pro,OS=17.5' "
            f"-only-testing PvTestRunnerUITests/PvUnitTests", platform_name)

        res_integration_test = run_unit_test(
            f"cd ios/test && xcrun xcodebuild test "
            f"-scheme PvTestRunner "
            f"-destination 'platform=iOS Simulator,name=iPhone 11 Pro,OS=17.5' "
            f"-only-testing PvTestRunnerUITests/PvIntegrationTests", platform_name)

        res = 1 if any([res_unit_test, res_integration_test]) else 0

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
        for arg in ["", "--no_simd"]:
            if res == 0:
                if arg != "--no_simd":
                    logging.info(f"running unit test in browser (simd) ...")
                else:
                    logging.info(f"running unit test in browser ...")
                res_unit_test = run_command(
                    f"python3 {selenium_script} {arg} "
                    f"--build_mode {build_mode} --root_path {peer_dir} --test_type unit")

                res_integration_test = run_command(
                    f"python3 {selenium_script} {arg} "
                    f"--build_mode {build_mode} --root_path {peer_dir} --test_type integration")

                res = 1 if any([res_unit_test, res_integration_test]) else 0

    elif platform_name == Platforms.WASM_LINUX:
        res_unit_test = run_unit_test(
            f"{root_dir}/build/{build_mode}/{platform.machine()}/"
            f"pv_orca_test_app res", platform_name)

        res_integration_test = run_unit_test(
            f"{root_dir}/build/{build_mode}/{platform.machine()}/"
            f"pv_orca_test_app res integration", platform_name)

        res = 1 if any([res_unit_test, res_integration_test]) else 0

    elif platform_name == Platforms.WINDOWS:
        res_unit_test = run_unit_test(
            f"{root_dir}/build/{build_mode}/{platform.machine()}/"
            f"pv_orca_test_app.exe res dump_{build_mode}", platform_name)

        res_integration_test = run_unit_test(
            f"{root_dir}/build/{build_mode}/{platform.machine()}/"
            f"pv_orca_test_app.exe res integration dump_{build_mode}", platform_name)

        build_dir = os.path.join(root_dir, 'build', build_mode)
        res_ucrt_test = run_windows_ucrt_test(
            root_dir,
            os.path.join(root_dir, build_dir, platform.machine().lower()),
            build_mode,
            ["orca"])

        res = 1 if any([res_unit_test, res_integration_test, res_ucrt_test]) else 0

    elif platform_name == Platforms.MAC:
        for arch in OSX_ARCHS:
            res_unit_test = run_unit_test(
                f"{root_dir}/build/{build_mode}/{arch}/"
                f"pv_orca_test_app res dump_{build_mode}", platform_name)

            res_integration_test = run_unit_test(
                f"{root_dir}/build/{build_mode}/{arch}/"
                f"pv_orca_test_app res integration dump_{build_mode}", platform_name)

            res = 1 if any([res_unit_test, res_integration_test]) else 0
            if res != 0:
                break

    elif platform_name == Platforms.MCU:
        return 0
    else:
        build_dir = f"build/{build_mode}/{platform.machine()}"
        res_unit_test = run_unit_test(
            f"{root_dir}/{build_dir}/pv_orca_test_app res dump_{build_mode}",
            platform_name)

        res_integration_test = run_unit_test(
            f"{root_dir}/{build_dir}/pv_orca_test_app res integration dump_{build_mode}",
            platform_name)

        res = 1 if any([res_unit_test, res_integration_test]) else 0

    try:
        shutil.rmtree(f"./dump_{build_mode}")
    except OSError as e:
        print(f"Error: {e.strerror}")

    return res


def main():
    parser = ArgumentParser()
    parser.add_argument('--build', action='store_true')
    parser.add_argument('--test', action='store_true')
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
                enable_asan=args.enable_asan)
            if res != 0:
                sys.exit(res)

        if args.test:
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
                    access_key=args.access_key)
                if res != 0:
                    sys.exit(res)

            logging.info(f"Running unit and integration tests ...")
            if args.platform in PARALLEL_TEST_PLATFORMS:
                input_arguments_list = list()
                for build in args.builds:
                    input_arguments_list.append((args.platform, build))
                with ThreadPool(min(os.cpu_count(), len(args.builds))) as pool:
                    res = pool.starmap(test_unittest_orca, input_arguments_list)
                res_sum = sum(res)
                if res_sum != 0:
                    sys.exit(res_sum)
            else:
                for build in args.builds:
                    res = test_unittest_orca(
                        args.platform,
                        build)
                    if res != 0:
                        sys.exit(res)

    except Exception as e:
        logging.error(e)
        sys.exit(1)


if __name__ == '__main__':
    main()
