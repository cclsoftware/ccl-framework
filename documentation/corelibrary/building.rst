
.. _corelib-build:

#########################
Building the Core Library
#########################

CMake 3.30 or later is required to build the Core Library. If you are new to CMake, read the official `User Interaction Guide <https://cmake.org/cmake/help/latest/guide/user-interaction>`_ first.

If your project is based on CMake, you can use the ``find_package`` command to import the Core Library as a static library target. If your project is not based on CMake, you might want to build the Core Library first and use the prebuilt library.

=========================================
Using the Core Library in a CMake project
=========================================

If you have installed the CCL SDK locally, simply import the Core Library (``find_package (corelib REQUIRED)``) and link the Core Library to your application (``target_link_libraries (MyApplication PRIVATE corelib)``).

To build from source, add the CCL framework repository to your project (e.g. using git submodule). Add ``<framework>/build/cmake/modules/shared`` to your ``CMAKE_MODULES_PATH`` and call ``include (vendor)`` in your ``CMakeLists.txt``. Then use `find_package` and `target_link_libraries` as shown above.

==================================================
Using the Core Library in other build environments
==================================================

Build the Core Library using CMake:

.. code-block:: bash
    :caption: Windows
    
	cd <framework>/core/cmake
	mkdir build
	cd build
	cmake -DREPOSITORY_ROOT=<framework> -DCMAKE_MODULE_PATH=<framework>/build/cmake/modules/shared ..    
	cmake --build . --config Release

.. code-block:: bash
    :caption: macOS
    
	cd <framework>/core/cmake
	mkdir build
	cd build
	cmake -GXcode -DREPOSITORY_ROOT=<framework> -DCMAKE_MODULE_PATH=<framework>/build/cmake/modules/shared -DCMAKE_OSX_DEPLOYMENT_TARGET="12.4" -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" ..    
	cmake --build . --config Release

.. code-block:: bash
    :caption: Linux
    
	cd <framework>/core/cmake
	mkdir build
	cd build
	cmake -GNinja -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_BUILD_TYPE=Release -DREPOSITORY_ROOT=<framework> -DCMAKE_MODULE_PATH=<framework>/build/cmake/modules/shared ..    
	cmake --build . 
