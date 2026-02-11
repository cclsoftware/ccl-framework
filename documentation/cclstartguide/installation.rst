############
Installation
############

CCL can be installed locally using the CCL SDK installers for macOS, Windows, or Linux.

Alternatively, the framework can be built from source. Build instructions can be found in the CCL and Core Library reference documentation.

The latest installers and source releases are available on Github at https://github.com/cclsoftware/ccl-framework.

=======================
Installation on Windows
=======================

CMake presets shipped with CCL use Visual Studio.

Install Visual Studio, including the "Desktop development with C++" workload.

Install the latest version of CMake from https://cmake.org/download/.

=====================
Installation on macOS
=====================

A development system runnning macOS can build targets for macOS and iOS/iPadOS.

Install Xcode from the App Store or Apple's developer website. Install CMake, e.g. using the Homebrew package manager (https://brew.sh).

Install the CCL SDK to an arbritrary folder, like the "Documents" folder:

.. code-block:: bash

	cd $HOME/Documents
	tar -xzf $HOME/Downloads/CCL\ SDK-<version>-macOS.tar.gz

Before running CMake from the command line, the following environment variables need to be set:

.. code-block:: bash

	CCLSDKROOT=$HOME/Documents/CCL\ SDK-<version>-macOS
	export CMAKE_PREFIX_PATH=$CCLSDKROOT/Frameworks/cmake/ccl

=====================
Installation on Linux
=====================

CCL is being developed and tested on Ubuntu 24.04. It is compatible with Ubuntu 22.04 as well, although some features may be disabled due to missing dependencies. In general, it should be possible to build CCL on any Linux distribution with reasonably up-to-date packages and Wayland support.

CMake presets shipped with CCL use ``ninja`` and ``clang``. Building with a different build system (e.g. ``make``) or a different compiler (e.g. ``gcc``) is possible, but requires some customization or additional command line arguments when running ``cmake``.

To install the CCL SDK, download and run the shell installer:

.. code-block:: bash

	chmod +x CCL\ SDK-<version>-Linux.sh
	sudo ./CCL\ SDK-<version>-Linux.sh

The following dependencies are required to build CCL and CCL-based applications:

Tools and data:
``build-essential`` ``clang`` ``cmake`` ``ninja-build`` ``lld`` ``git`` ``python-is-python3`` ``gettext`` ``wayland-protocols``

Build time dependencies:
``libwayland-dev`` ``libsdbus-c++-dev`` ``libsdbus-c++-bin`` ``libxkbcommon-dev`` ``libx11-dev`` ``libavahi-compat-libdnssd-dev`` ``libfontconfig-dev`` ``uuid-dev`` ``libssl-dev``

Runtime dependencies:
``libwayland-client0`` ``libwayland-cursor0`` ``libwayland-egl1`` ``libwayland-server0`` ``libsdbus-c++1`` ``libxkbcommon0`` ``xkb-data`` ``libfontconfig1`` ``libuuid1`` ``libssl3t64``

Installation of the following packages is recommended:

.. list-table::
    :widths: 25 25 50
	:header-rows: 1
   
    * - Packages
	  - CMake Options
	  - Remarks
    * - ``ccache``
	  - ``VENDOR_USE_CCACHE``
      - Compiler cache, speeds up recompilation
	* - ``libunistring-dev``
	  - ``CCL_ENABLE_LIBUNISTRING``
	  - Full Unicode support (falls back to simple implementation with reduced Unicode support)
	* - ``libgles-dev``
	  - ``CCL_ENABLE_OPENGLES2``
	  - OpenGL ES 2 graphics backend
	* - ``vulkan-validationlayers-dev`` ``vulkan-utility-libraries-dev``
	  - ``CCL_ENABLE_VULKAN``
	  - Vulkan graphics backend (skip vulkan-validationlayers-dev on Ubuntu 22.04)
	* - ``spirv-cross`` ``glslc``
	  - ``CCL_ENABLE_OPENGLES2`` ``CCL_ENABLE_VULKAN``
	  - Vulkan graphics backend and 3D graphics in general
	* - ``xdg-desktop-portal-dev``
	  - ``CCL_BUILD_XDGPORTAL_INTEGRATION``
	  - Desktop integration (file dialogs, printing, etc.)
	* - none
	  - ``CCL_BUILD_SECRETSERVICE_INTEGRATION``
	  - Secret Service integration (credential store)

Installation of the following packages is optional:

.. list-table::
    :widths: 25 25 50
	:header-rows: 1
   
    * - Packages
	  - CMake Options
	  - Remarks	
	* - ``libharfbuzz-dev``
	  - ``SKIA_USE_SYSTEM_HARFBUZZ``
	  - Dynamically link to system harfbuzz libraries (falls back to static linkage)
	* - ``libicu-dev``
	  - ``SKIA_USE_SYSTEM_ICU``
	  - Dynamically link to system ICU libraries (falls back to static linkage)
	* - ``libfreetype-dev``
	  - ``SKIA_USE_SYSTEM_FREETYPE``
	  - Dynamically link to system freetype libraries (falls back to static linkage)
	* - ``libjpeg-dev``
	  - ``SKIA_USE_SYSTEM_JPEG``
	  - Dynamically link to system jpeg libraries (falls back to static linkage)
	   
====================================
Installation for Android development
====================================

Android apps can be developed with Android Studio 2025.4.1 or later on Windows, macOS, or Linux.

In addition to Android Studio, CMake and Ninja need to be installed on the host system. On Windows, please also install Git for Windows to provide a Unix shell.

In Android Studio, install the following components using the SDK manager:

* Android 16.0 SDK platform
* Android Build Tools 36.1.0
* Android NDK 29.0.14206865
* CMake 4.1.2 (must match the CMake version defined in vendor.android.cmake)

IDE heap size must be configured to at least 4096 MB in Android Studio's memory settings.
