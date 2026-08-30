#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-QORVO-2

import os
import sys
import subprocess
import argparse
import shutil

class CustomFlagsAction(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        if getattr(namespace, 'build', None) != 'Custom' and values != "":
            parser.error(f"{self.dest} parameter can only be set if --build is 'Custom' or if flags are an empty string")
        setattr(namespace, self.dest, values)

class CreateTargetCommon:
    """Class to handle the creation of a target using common settings."""

    def __init__(self, project_base, target_os, board, example=""):
        """
        Initialize the CreateTargetCommon instance.

        Args:
            project_base (str): The base directory of the project.
            project (str): The project name.
            target_os (str): The target operating system.
            board (str): The target board.
            example (str): The example name.
        """
        self.project_base = project_base
        self.target_os = target_os
        self.board = board
        self.example = example
        self.libs_path = "Libs"

        self.parser = argparse.ArgumentParser(description='Build specified target')
        self.parser.add_argument('-no-force', action='store_true', help='Do not remove old build folder')
        self.parser.add_argument('-build', choices=['Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel', 'Custom'],
                            default='Debug', help="Specify the build type. "
                                                "Choices are 'Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel', 'Custom' (default: Debug). "
                                                "If 'Custom' is selected, no value will be assigned to CMAKE_BUILD_TYPE, allowing customization with -custom-flags option.")
        self.parser.add_argument('-custom-flags', type=str, action=CustomFlagsAction, help="Custom flags for the build, only available with -build 'Custom'. Example: '-O2 -g -DDEBUG'")

    def __convert_path(self, path):
        """
        Converts the path separators to forward slashes if the operating system is Windows.

        Args:
            path (str): The input path.

        Returns:
            str: The converted path.
        """
        if os.name == 'nt':
            return path.replace("\\", "/")
        else:
            return path

    def parse_arguments(self):
        """
        Parse command-line arguments for building specified target.

        Returns:
            argparse.Namespace: Parsed arguments.
        """
        # Make arguments case-insensitive
        args = vars(self.parser.parse_args())
        return argparse.Namespace(**args)

    def run(self):
        """
        Run the target creation process.

        Warning: Only FreeRTOS target OS is supported.
        """
        args = self.parse_arguments()

        # Set environment variables based on command line options

        if self.example != "":
            self.custom_cmake = f'_{self.example}'
        else:
            print("Error: Example parameter is required")
            sys.exit()

        # Variables based on class attributes and default values
        target_script = f"project{self.custom_cmake}.cmake" if self.custom_cmake else ""
        build_path = os.path.join(self.project_base, 'BuildOutput', self.example, self.target_os, self.board, args.build)
        source_path = os.path.join(self.project_base, 'Projects',  self.target_os, self.example, self.board)

        print(f"Build Output is {build_path}")
        print(f"Target script is {target_script}")
        print(f"Build type is {args.build}")
        if args.custom_flags:
            print(f"Custom flags are {args.custom_flags}")

        if os.path.exists(build_path):
            if args.no_force:
                print("Directory exists and no-force was specified, exiting...")
                sys.exit()

            print(f"Removing existing build output {build_path}")
            shutil.rmtree(build_path, ignore_errors=True)

        # Build for FreeRTOS
        if self.target_os == "FreeRTOS":
            generator = "MinGW Makefiles" if os.name == 'nt' else "Unix Makefiles"
            cmake_command = [
            'cmake', '-S', source_path,
            '-B', build_path,
            '-G', generator,
            '-DCMAKE_TOOLCHAIN_FILE=' + self.__convert_path(os.path.join(self.project_base, 'Projects', 'Common', 'cmakefiles', 'arm-none-eabi-gcc.cmake')),
            '-DMY_TARGET_SCRIPT=' + self.__convert_path(os.path.join(source_path, target_script)),
            '-DPROJECT_BASE=' + self.__convert_path(self.project_base),
            '-DCOMMON_PATH=' + self.__convert_path(os.path.join(self.project_base, 'Projects', 'Common', 'cmakefiles')),
            '-DPROJECT_COMMON=' + self.__convert_path(os.path.join(self.project_base, 'Projects', self.target_os, self.example, f"Common")),
            f'-DLIBS_PATH={self.__convert_path(self.libs_path)}'
            ]

            if args.build != 'Custom':
                cmake_command.append(f'-DCMAKE_BUILD_TYPE={args.build}')
            if args.custom_flags:
                cmake_command.append(f'-DCMAKE_C_FLAGS={args.custom_flags}')

            subprocess.run(cmake_command, check=True)
