# DW3 QM33 SDK #

This readme describes how to build and run provided code and examples from QM33 SDK on the development kit. For more information about functionalities please refer to Developer Manual from SDK package.

## Required tools

The following tools are required to build and run the examples:

### Windows

- **ARM Toolchain**:
  - Download **arm-none-eabi-gcc-10.3-2021.10-win32** from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads.
  - Install it inside: **C:\\GnuToolsArmForEmbedded\\gcc-arm-none-eabi-10.3-2021.10\\bin** to align with VS Code.
  - Add `C:\\GnuToolsArmForEmbedded\\gcc-arm-none-eabi-10.3-2021.10\\bin` to your system/user environment variables.

- **CMake**:
  - Download CMake from: https://cmake.org/download/. Make sure to use CMake version higher than 3.23.
  - Install CMake inside `<cmake_install_dir>`.
  - Add `<cmake_install_dir>/bin` to your system/user environment variables.

- **Python**:
  - Download Python from: https://www.python.org/downloads/. The required version is 3.10 or higher.
  - Install Python and make sure to select the option "Add Python to PATH" during the installation.

- **MinGW and make**:
  - Mingw can be downloaded from: https://sourceforge.net/projects/mingw/.
  - Install Mingw in the directory of your choice.
  - Using the MinGW Installation Manager, install `mingw32-base`.
  - Create a copy of `<install path>/bin/mingw32-make.exe` and name it `<install path>/bin/make.exe`.
  - Add `<install path>/bin` to your system/user environment variables.

- **SEGGER J-Link**:
  - Install J-Link Software and Documentation Pack from: https://www.segger.com/downloads/jlink/.

- **VS Code**:
  - Install VS Code from: https://code.visualstudio.com/.


### Linux

- **ARM Toolchain**

  - Create an install directory:

    ```bash
    mkdir /opt/gcc
    ```

  - Go to the install directory:

    ```bash
    cd /opt/gcc
    ```

  - Download arm-none-eabi gcc from: https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2

  - Extract the tarball:

    ```bash
    tar -xvf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
    ```

  - Remove the tarball:

    ```bash
    rm gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
    ```

  - Open **~/.bashrc** and add this line `export PATH="/opt/gcc/gcc-arm-none-eabi-10.3-2021.10/bin:${PATH}`

- **Make**
  - Install Make:

    ```bash
    sudo apt-get install build-essential
    ```

- **CMake**

  - Download CMake from https://github.com/Kitware/CMake/releases/. Make sure to use CMake version higher than 3.23.
  - Create a cmake directory in usr/bin:

    ```bash
    sudo mkdir /usr/bin/cmake
    ```

  - Execute:

    ```bash
    sudo cmake-<cmake_version>-Linux-x86_64.sh --skip-license --prefix=/usr/bin/cmake
    ```

  - open **~/.bashrc** and add this line `export PATH="/usr/bin/cmake/bin:${PATH}"`

- **Python**
  - Install Python:

    ```bash
    sudo apt-get install -y python3 python3-pip
    ```

- **SEGGER J-Link**:
  - Install J-Link Software and Documentation Pack from: https://www.segger.com/downloads/jlink/.

- **VS Code**
  - Install VS Code from: https://code.visualstudio.com/

### macOS

- **ARM Toolchain**

  - Create an install directory:

    ```bash
    sudo mkdir /opt/gcc
    ```

  - Go to the install directory:

    ```bash
    cd /opt/gcc
    ```

  - Download arm-none-eabi gcc from: https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2

  - Move the tarball to /opt/gcc and extract it:

    ```bash
    sudo tar -xvf gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2
    ```

  - Remove the tarball:

    ```bash
    sudo rm gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2
    ```

  - Open **~/.zshrc**

    ```bash
    open ~/.zshrc
    ```

    If the file does not exist, create it and open it

    ```bash
    touch ~/.zshrc
    open ~/.zshrc
    ```

    Add this line in the file `export PATH="/opt/gcc/gcc-arm-none-eabi-10.3-2021.10/bin:${PATH}"`
    and save it.

    You may need to restart your shell to have those changes applied.

- **Make**
  - Make should be preinstalled, check it with

    ```bash
    make --version
    ```

    If you did not get the version, follow these steps to install Make:

    ```bash
    xcode-select --install
    ```

    A popup should appear asking if you to confirm you want to install it.

    To verify Make installation, check it with

    ```bash
    make --version
    ```

- **CMake**

  - Download CMake .dmg file from https://github.com/Kitware/CMake/releases/. Make sure to use CMake version higher than 3.23.
  - Double click the .dmg file, it will mount. Then, drag it to your Applications folder.

  - Open **~/.bashrc**

    ```bash
    open ~/.zshrc
    ```

    And add this line `export PATH="/Applications/CMake.app/Contents/bin:$PATH"`

    You may need to restart your shell to have those changes applied.

- **Python**
  - Download Python from: https://www.python.org/downloads/. The required version is 3.10 or higher.
  - Install Python pkg.

- **SEGGER J-Link**:
  - Install J-Link Software and Documentation Pack from: https://www.segger.com/downloads/jlink/.

- **VS Code**
  - Install VS Code from: https://code.visualstudio.com/


## Setup environment

1. Create a virtual environment:

   ```bash
   python -m venv .venv
   ```

2. Activate virtual environment:

   - On Linux & macOS:

     ```bash
     source .venv/bin/activate
     ```

   - On Windows:

     ```bash
     .\.venv\Scripts\Activate.ps1
     ```

2. Install the requirements:

   ```bash
   pip install -r requirements.txt
   ```

## Visual Studio Code

We provide support to build, flash and debug firmware directly from VS Code. Below you can find brief description of required setup and possible functionalities.

### Package location

> **Warning:** Paths containing spaces will lead to errors during the execution of tools and scripts provided within the package,
such as Visual Studio Code tasks. To ensure proper functionality, always place this package in a directory path without spaces.

### Open workspace

To make use of all VS Code features please remember to open project as a workspace:

1. Click `File` in the top menu.
2. Pick `Open Workspace from File...`.
3. Find and open `DW3_QM33_SDK.code-workspace`.

### Extensions

For best experience, please install all recommended extensions.

1. Go to Extensions tab `Ctr+Shift+X`
2. Click **Filter extensions** button (next to search field).
3. Pick **Recommended**.
4. Click on cloud icon just below to install all recommended extensions.

List of currently recommended extensions:

- ms-vscode.cpptools (C language support)
- marus25.cortex-debug (debugging firmware on target)
- rioj7.command-variable (automated tasks for building and debugging)
- ms-python.python (Python language support)

### Tasks

> **Warning:** Make sure VS Code uses Python interpreter from virtual environment. Execute ``Ctrl+Shift+P``, write ``Python: Select interpreter``, and choose path to your local env, e.g.  `./.venv/bin/python`.

> **Warning:** On Windows, the recommended and working Terminal Default Profile is PowerShell. Git Bash and other may not be working properly because of differences in path resolution. To select the proper terminal, execute ``Ctrl+Shift+P``, write ``Terminal: Select Default Profile``, and choose **PowerShell**.

VS Code built-in tasks and launch configuration can be utilized to build, flash and debug the firmware.
You can find short description of available tasks below:

- Build:
  - **Build the firmware** - build the firmware (compile only changed files).
  - **Build the clean firmware** - rebuild the firmware (delete output files and compile everything again).
  - **Flash the target** - flash devkit with currently built firmware and reset the target.
  - **Build & flash the target** - `Build the firmware` + `Flash the target`.

- Launch:
  - **Debug the firmware** - runs GDB sessions with target.
  - **Build & debug the firmware** - builds firmware if needed and runs GDB sessions with target.

#### Running tasks

The tasks utilize the **Command Variable** extension and allows to set build/board configuration which will be used in all later executed tasks.

1. Open build dialog `Ctrl+Shift+B`.
2. Pick **Choose a configuration** and VS Code will prompt about possible configurations.
3. Now you can run tasks (**Build the firmware**, **Build the clean firmware**, **Build & flash target**, **Flash target**) as well as launch firmware `Ctrl+Shift+D` (**Debug the firmware** and **Build & debug the firmware**), without specifying configuration again. Setting will be saved to file and will remain after VS Code restart.
4. If you want to check current configuration, choose **Check configuration** and it will be displayed in a separate terminal tab.

## Building from command line

If you use other IDE/editor than VS Code you can also build and flash directly from your terminal.

### 1. Create a target

`CreateTarget.py` automates the process of generating a target for a project using common settings. It calls CMake with the appropriate options to configure the build environment and generate the target.

> **Warning:** Make sure your Python virtual environment is activated before running this script.

#### Usage

The project to be built needs to be created. Navigate to the project folder `<project_dir>/Projects/FreeRTOS/<example><board_name>` and execute:

- On Linux:
    ```bash
    ./CreateTarget.py [-no-force] [-build] [-custom-flags]
    ```
- On Windows:
    ```bash
    python ./CreateTarget.py [-no-force] [-build] [-custom-flags]
    ```

##### Options

- `-no-force`: Optional flag to prevent the removal of the old build folder.
- `-build {Debug,Release,RelWithDebInfo,MinSizeRel,Custom}`: Optional flag to specify the build type. Choices are `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`, `Custom` (default: `Debug`). If `Custom` is selected, no value will be assigned to `CMAKE_BUILD_TYPE`, allowing customization with `-custom-flags`.
- `-custom-flags CUSTOM_FLAGS`: Optional flag for custom build flags, only available with `-build Custom`.

#### Example:

- On Linux:
    ```shell
    ./CreateTarget.py -build Release
    ```
- On Windows:
    ```shell
    python ./CreateTarget__dev.py -build Custom -custom-flags='-O2 -g -DDEBUG'
    ```

#### Help Output

```
./CreateTarget.py --help
usage: CreateTarget.py [-h] [-no-force] [-build {Debug,Release,RelWithDebInfo,MinSizeRel,Custom}] [-custom-flags CUSTOM_FLAGS]

Build specified target

options:
  -h, --help            show this help message and exit
  -no-force             Do not remove old build folder
  -build {Debug,Release,RelWithDebInfo,MinSizeRel,Custom}
                        Specify the build type. Choices are 'Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel', 'Custom' (default: Debug). If 'Custom' is selected, no value will
                        be assigned to CMAKE_BUILD_TYPE, allowing customization with -custom-flags option.
  -custom-flags CUSTOM_FLAGS
                        Custom flags for the build, only available with -build 'Custom'. Example: '-O2 -g -DDEBUG'
```

### 2. Build the firmware

This step compiles the firmware, producing output files in three formats: `.hex`, `.bin`, and `.elf`.

#### Usage

Navigate to the project folder `<project_dir>/BuildOutput/<example>/FreeRTOS/<board_name>/<build_type>` and execute:

```bash
make -j
```

#### Examples

- compile CLI example for DWM30001CDK and Debug build:

  ```bash
  cd BuildOutput/CLI/FreeRTOS/DWM3001CDK/Debug
  make -j
  ```

- compile UCI example for DWM30001CDK and Release build:

  ```bash
  cd BuildOutput/UCI/FreeRTOS/DWM3001CDK/Release
  make -j
  ```

> Note: to speed up building process we use ``-j`` flag so all CPU cores will be utilized to build the firmware. You can specify to use e.g. only 4 cores ``make -j4``.

### 3. Flash the firmware

There is wide variety of tools which can be used to flash the target. Below is an example demonstrating how to use the JLink command line tool.

1. Create script.jlink and paste configuration given below.

   ```txt
   si 1
   speed 4000
   device <cpu_type>
   loadfile <firmware_path>
   r
   g
   exit
   ```

   Replace **<cpu_type>** with CPU that you use:
     - **nrf52840_xxaa** for QM33120WDK1 and Murata Type2AB EVB,
     - **nrf52833_xxaa** for DWM3001CDK.

   Replace **<firmware_path>** with path to build firmware, e.g.:
   - ``BuildOutput/CLI/FreeRTOS/nRF52840DK/Debug/nRF52840DK-CLI-FreeRTOS.hex`` to flash CLI example for QM33120WDK1 and Debug build.

2. Launch JLink:

   - On Linux:

       ```bash
       JLinkExe -CommanderScript script.jlink
       ```

   - On Windows:

     ```bash
     JLink.exe -CommanderScript script.jlink
     ```

3. Upon successful completion of the script, the device is ready for use.
