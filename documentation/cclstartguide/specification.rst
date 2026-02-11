#######################
Technical Specification
#######################

================================
Development and Target Platforms
================================

You can target the following platforms and architectures with CCL on the given development platforms:

.. list-table::
    :widths: 50 50 50
	:header-rows: 1

    * - Development on Windows
	  - Development on macOS
	  - Development on Linux 
    * - Target Windows (x86, x86-64, Arm64 and Arm64EC) and Android.
	  - Target macOS (Intel and Arm), iOS/iPadOS, and Android.
	  - Target Linux and Android.

For Android, the following architectures are supported: x86, x86-64, Armv7, and Arm64.

Development for embedded Linux and RTOS platforms requires a custom environment and tools.

==================================
Minimum Target System Requirements
==================================

Applications based on CCL have the following minimum target system requirements:

* Windows 10 22H2
* Windows 11 22H2
* macOS 12.4 (Monterey)
* Ubuntu 22.04 LTS (or similar)
* Ubuntu 24.04 LTS (or similar)
* Android 8.0 (API level 26)
* iOS/iPadOS 15

Linux platforms require Wayland session and Vulkan 1.1 or OpenGL ES 2 graphics driver. X11 is not supported.

RTOS platforms (Core Library only):

* Zephyr
* Azure RTOS
* CMSIS RTOS
* Little Kernel
* CrossWorks Tasking Library (CTL)

===============================
Supported Programming Languages
===============================

* C++17
* TypeScript
* JavaScript

Platform implementation is using Objective-C on macOS/iOS, and Java on Android.

============================
Supported Compilers and IDEs
============================

.. list-table::
    :widths: 50 50 50
	:header-rows: 1

    * - Host Operating System
	  - Supported IDEs
	  - Supported Compilers
    * - Windows
      - Visual Studio or Visual Studio Code
	  - Microsoft C++ compiler
	* - macOS
	  - XCode, Visual Studio Code
	  - Clang
	* - Linux
	  - Various, including Visual Studio Code
	  - Clang, GCC

Build scripts are based on CMake (version 3.30 or later).
