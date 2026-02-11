
.. _cmake-developer:

######################
Writing CMake Projects
######################

This guide is intended to help developers in writing CMake files for CCL-based applications.

Each application has a ``CMakeLists.txt`` file, which is located at ``<appname>/cmake/``.
This file describes the application's source files, dependencies and other settings.

A ``CMakePresets.json`` file is located next to the ``CMakeLists.txt`` file.

Usually, this file does only include a shared presets file, which is located at ``build/cmake/modules/shared/CMakePresets.json``.

A CMake presets file contains presets with default values for variables and options that are needed when building the application.

Among other settings, the shared presets file adds the directory ``build/cmake/modules/shared/`` to the ``CMAKE_MODULE_PATH``.

--------
CCL Demo
--------

This guide is based on the ``CMakeLists.txt`` file of CCL Demo (updated 02/27/2023).

The first line specifies the minimum required CMake version. Our shared presets file also specifies a minimum version. However, CMake requires to have this line in every top-level CMakeLists.txt file. Update this version number when you use a feature that is only available in later versions of CMake.

.. code:: cmake

	cmake_minimum_required (VERSION 3.30 FATAL_ERROR)
	
The next line specifies the name of the project. Think of it as the name of the Visual Studio solution or Xcode workspace.

.. code:: cmake

	project ("CCL Demo")

The variable ``PROJECT_VENDOR`` is used to determine the code signing identity and version information in application installers / packages.

.. code:: cmake

	set (PROJECT_VENDOR ccl)

Next, a shared cmake module named ``vendor`` is included. This cmake module is located at ``build/cmake/shared/vendor.cmake``. When including modules, CMake searches in directories specified in the variable ``CMAKE_MODULE_PATH``. The shared presets file already added ``build/cmake/shared/`` to this variable, so we can simply include ``vendor``:

.. code:: cmake

	include (vendor)

The vendor module does several things, e.g. it detects the target architecture and the host system, defines variables like ``REPOSITORY_ROOT`` or ``REPOSITORY_FRAMEWORKS_DIR`` which help in finding source files, and parses the ``vendor.h`` file to define ``VENDOR_NAME``, ``VENDOR_PACKAGE_DOMAIN``, etc.

The vendor module also adds all directories and subdirectories in ``build/cmake/`` to the ``CMAKE_MODULE_PATH``.

CCL Demo depends on CCL and needs Skin XML packages. We also want to include CCL Spy in the CCL Demo project. 

CMake provides a mechanism for finding dependencies with the :cmakelink:`find_package` command. 
When calling ``find_package (xy)``, CMake searches the ``CMAKE_MODULES_PATH`` for a file called ``Findxy.cmake``. 
See https://cmake.org/cmake/help/latest/guide/using-dependencies/ for details.

There are several ``Find*.cmake`` modules in ``build/shared/cmake`` to find and include targets for frameworks, libraries, and third-party dependencies. 

For example, ``build/cmake/modules/ccl/Findccl.cmake`` looks for the file ``public/cclversion.h`` in ``${CCL_FRAMEWORK_DIR}``, and includes ``${CCL_FRAMEWORK_DIR}/cmake/ccl-config.cmake``.
This file in turn defines all CCL targets, including include paths and transitive dependencies (corelib, pnglib, zlib, ...), which can then be used by the application.

.. code:: cmake

	find_package (ccl REQUIRED COMPONENTS cclapp cclbase ccltext cclsystem cclnet cclgui)
	find_package (cclspy)
	find_package (modelimporter3d)
	find_package (skins)

Now that we set up some variables and found all dependencies, we can add a target for our application using ```ccl_add_app``.
:cmake:`ccl_add_app` creates an application target and applies some basic properties. This includes the C++ standard and default include directories. This macro also includes a platform-specific cmake module, e.g. ``ccldemo.win.cmake`` or ``ccldemo.mac.cmake``, which might set platform-specific properties or add platform-specific source files to the target. The GUI flag tells cmake to use a WinMain entry point on Windows and to create an application bundle on Mac.
We also need some information about the application. Using the ``VERSION_FILE`` parameter, we tell CMake the path to a header file containing version information for the application.
We also pass ``ccl`` as our vendor ID. This ID is used to define CMake-variables and C preprocessor definitions like ``VENDOR_NAME``.
The variable ``ccldemo_NAME`` has been defined by :cmake:`ccl_add_app` using information from the version header file.

.. code:: cmake

	ccl_add_app (ccldemo GUI
		VENDOR ccl
		VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/appversion.h"
		VERSION_PREFIX APP
	)
	ccl_set_product_name (ccldemo ${ccldemo_NAME})
	ccl_set_startup_project (ccldemo)

All of these macros are defined in ``core/cmake/coremacros.cmake``.

:cmake:`ccl_set_startup_project` sets ccldemo to be the startup project in a Visual Studio solution.

We also set the product name of our application using :cmake:`ccl_set_product_name`. Depending on the platform, this may set the name of the generated project and the name of the resulting binary.

As stated before, CCL Demo needs skin packages. 

We already included the ``skins`` cmake module using the :cmakelink:`find_package` command. The skins module defines macros that help with using skin packages.

.. code:: cmake

	use_skin_packages (ccldemo neutralbase neutraldesign)
	add_skin_package (ccldemo ${CMAKE_CURRENT_LIST_DIR}/../skin)

Next, we define variables that contain paths to CCL Demo's source files. We will use the variables later to add the source files to the target and to specify how these source files should be grouped in the IDE (filters/folders).

.. code:: cmake

	list (APPEND ccldemo_resources
		${CMAKE_CURRENT_LIST_DIR}/../resource/cclgui.config
		${CMAKE_CURRENT_LIST_DIR}/../resource/commands.xml
		${CMAKE_CURRENT_LIST_DIR}/../resource/embedded
		${CMAKE_CURRENT_LIST_DIR}/../resource/svg
		${CMAKE_CURRENT_LIST_DIR}/../resource/menubar.xml
	)
	 
	list (APPEND ccldemo_source_files
		${CMAKE_CURRENT_LIST_DIR}/../source/demoapp.h
		${CMAKE_CURRENT_LIST_DIR}/../source/demoitem.h
		${CMAKE_CURRENT_LIST_DIR}/../source/demoapp.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/coreviewdemo.h
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/compositiondemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/controlsdemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/dialogsdemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/embeddeddemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/focusdemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/graphicsdemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/graphicsdemo3d.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/layoutdemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/networkdemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/spritedemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/svgdemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/systemdemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/treeviewdemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/viewsdemo.cpp
		${CMAKE_CURRENT_LIST_DIR}/../source/demos/workspacedemo.cpp
	)
	 
	list (APPEND ccldemo_ccl_sources
		${CCL_DIR}/main/cclmain.cpp
	)

The groups/filters/folders for our IDE are set using the :cmakelink:`source_group` command.

The first :cmakelink:`source_group` call (with the ``TREE`` option) works for list of source files where all files share the same prefix. Source files under this prefix will be collected into groups/folders which match the file system folders.

The second call (without the ``TREE`` option) moves all files in the list to the given group/folder ``source\ccl``. No subfolders are created.

.. code:: cmake

	source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/../source PREFIX "source" FILES ${ccldemo_source_files})
	source_group ("source\\ccl" FILES ${ccldemo_ccl_sources})

All source files are addeded to another variable:

.. code:: cmake

	list (APPEND ccldemo_sources
		${ccldemo_source_files}
		${ccldemo_ccl_sources}
	)

And finally, resources and source files are added to the target and linked against dependencies:

.. code:: cmake

	ccl_add_resources (ccldemo ${ccldemo_resources})
	target_sources (ccldemo PRIVATE ${ccldemo_sources})
	target_link_libraries (ccldemo PRIVATE ${CCL_LIBRARIES} corelib)

	ccl_add_dependencies (ccldemo cclspy modelimporter3d)

---------
Debugging
---------

The :cmakelink:`message` command is useful when debugging CMake scripts:

.. code::
	
	message (WARNING "this should not happen!")
	message (FATAL_ERROR "oops!")

Call cmake with ``--log-level=<ERROR|WARNING|NOTICE|STATUS|VERBOSE|DEBUG|TRACE>`` to control verbosity.

---------
Profiling
---------

In case the generation step takes unexpectedly long, you can profile cmake using the ``--profiling`` and ``--profiling-format`` flags.

For example:

.. code:: bash
	
	cmake --preset win64 --profiling-output=perf.json --profiling-format=google-trace

To analyze the resulting ``perf.json``, open https://ui.perfetto.dev/ in your web browser and load the ``perf.json`` file.

