.. _ccl-building:

############
Building CCL
############

CMake 3.30 or later is required to build CCL. If you are new to CMake, read the official `User Interaction Guide <https://cmake.org/cmake/help/latest/guide/user-interaction>`_ first.

If your project is based on CMake, you can use the ``find_package`` command to import CCL. If your project is not based on CMake, you might want to build the framework libraries first and use the prebuilt libraries in your project.

The ``ccl-framework`` repository contains a ``ccl-config.cmake`` file, which can be used to find CCL using the ``find_package`` command. Please refer to :ref:`corelib-build` for a detailed guide on how to use this file.

===============================
Building CCL-based Applications
===============================

CCL-based applications usually come with a ``CMakePresets.json`` file. Presets in this files define CMake options for different target platforms.

In order to build a CCL-based application, navigate to the application's ``cmake`` folder and run 

.. code-block:: bash

    cmake --list-presets
	
This command shows a list of presets available on your host system.

For example, if targeting Microsoft Windows, run 

.. code-block:: bash

    cd <application-directory>/cmake
    cmake --preset win64
	
This generates a Visual Studio solution. Locate and open the generated solution in Visual Studio or simply run

.. code-block:: bash

    cmake --open build/x64

If you don't want to build using an IDE, run the following command in the generated directory (build/x64 in this example). 

.. code-block:: bash

    cmake --build . --config Release

See `Invoking the Buildsystem <https://cmake.org/cmake/help/latest/guide/user-interaction/#invoking-the-buildsystem>`_ for additional details.

=============================
Building Android Applications
=============================

CCL-based Android applications are built using Android Studio.

To generate an Android Studio solution for an application, use the ``android`` presets for CMake, e.g.

.. code-block:: bash

    cd <application-directory>/cmake
    cmake --preset android

In order to use a custom upload signing key for your Android application, you will need to provide code-signing details in a ``signing.properties`` file. This should have the following structure::

    key.store=<path to key store file>
    key.alias=<alias of key to use>
    key.store.password=<key store password>
    key.alias.password=<key password>

Please place the ``signing.properties`` file in the same directory as the top level ``CMakeLists.txt`` or provide the path to it in the ``CCL_SIGNING_PROPERTIES`` CMake variable. The ``CCL_SIGNING_PROPERTIES`` variable takes precedence if set.

==========================
Cross-compilation on Linux
==========================

Cross-compiling requires a compiler toolchain and a sysroot matching the target platform.

For example, in order to build for Raspberry Pi / Debian Bookwork ARM64, install the following packages:

``crossbuild-essential-arm64 gcc-aarch64-linux-gnu g++-aarch64-linux-gnu binutils-multiarch ubuntu-dev-tools`` 

Create a sysroot for the target platform:

.. code-block:: bash
    
	mk-sbuild --arch=arm64 bookworm --debootstrap-mirror=http://deb.debian.org/debian --name=rpizero-bookworm --skip-proposed --skip-updates --skip-security
	
.. note::
	
	In case of errors stating that ``qemu-debootstrap`` is not available, replace ``qemu-debootstrap`` with ``debootstrap`` in ``/usr/bin/mk-sbuild``
	

Use ``sbuild-apt`` to install or update dependencies:

.. code-block:: bash
    
	sudo sbuild-apt rpizero-bookworm-arm64 apt-get install <packages>
	
Build your project using a toolchain file:

.. code-block:: bash
    
	export CMAKE_TOOLCHAIN_FILE=${WORKSPACE}/build/cmake/toolchains/aarch64-linux-rpi-toolchain.cmake
    cmake --preset linux-arm64-release -DCMAKE_SYSTEM_NAME=Linux
	cd build/arm64/Release
    cmake --build .

See `Cross Compiling for Linux <https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html#cross-compiling-for-linux>`_ for additional details.
