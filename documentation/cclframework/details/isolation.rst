###################
Framework Isolation
###################

Multiple versions of CCL can co-exist in the same process without clashes by using the framework isolation feature. This way, each framework consumer has access to its own (isolated) set of singleton instances like the built-in plug-in manager, etc. With framework isolation enabled, an application-defined postfix is appended to the names of shared framework libraries. The same postfix is applied to all exported functions.

==========================================
Building an isolated copy of the framework
==========================================

To use framework isolation, CCL needs to be built from source. See :ref:`ccl-building`.

In your top-level ``CMakeLists.txt`` file use ``find_package`` to import the required framework libraries.

Use ``ccl_add_app`` or a similar macro to define your targets.

Instead of using ``target_link_libraries`` to link framework libraries, use ``target_link_ccl_framework`` right after defining your target:

.. code-block:: cmake

    target_link_ccl_framework (my_application ISOLATION_POSTFIX custompostfix PRIVATE
	  cclapp cclbase ccltext cclsecurity cclsystem cclnet cclgui cclextras-webfs cclextras-analytics
    )

This does multiple things:

First, by providing an ``ISOLATION_POSTFIX``, new variables ```CCL_ISOLATION_POSTFIX`` and ``CCL_EXPORT_POSTFIX`` are set to the provided value. 
``CCL_ISOLATION_POSTFIX`` is used to modify the binary file names of relevant libraries. ``CCL_EXPORT_POSTFIX`` is appended to exported function names.

Isolated copies of framework libraries are created. For example, a target ``cclgui.custompostfix`` is created for cclgui. When building this target on Windows, a binary file ``cclgui.custompostfix.dll`` is created.

Finally, isolated copies of framework libraries are linked to the top-level target (``my_application`` in this example).

Make sure to follow these steps for any target in your project that is being used by the isolated top-level target.

You don't need to repeat the ``ISOLATION_POSTFIX`` in every call to ``target_link_ccl_framework``. The ``CCL_ISOLATION_POSTFIX`` and ``CCL_EXPORT_POSTFIX`` variables are valid within the CMake directory scope of the top-level target. ``target_link_ccl_framework`` will find the correct targets for the current isolation scope.

==============================================================
Sharing CMake files between isolated and non-isolated projects
==============================================================

When sharing CMake files between projects, possibly using isolated and non-isolated copies of the same targets in a single top-level CMake file, care must be taken to create isolated copies of all shared targets.

For example, when creating a plug-in or service library that should be used in isolated and non-isolated projects, avoid using CMake include guards in your CMake files.
Instead, use ``ccl_check_target_exists``:

.. code-block:: cmake

    ccl_check_target_exists (my_service target_exists)
    if (target_exists)
	    return ()
    endif ()

    # Add target
    ccl_add_plugin_library (my_service
	    VENDOR presonus
	    VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/../source/plugversion.h"
	    VERSION_PREFIX PLUG
    )
	
This macro takes the current value of ``CCL_ISOLATION_POSTFIX`` into account and sets the variable ``target_exists`` to ``ON`` if a target named ``my_service`` (in a non-isolated project) or ``my_service.${CCL_ISOLATION_POSTFIX}`` (in an isolated project) exists.

Note that ``ccl_add_plugin_library`` automatically appends ``CCL_ISOLATION_POSTFIX`` to the target name and defines a variable ``my_service_target``.

Use ``${my_service_target}`` instead of ``my_service`` to reference the correct copy of your target after the call to ``ccl_add_plugin_library``.

If you don't use ``ccl_add_plugin_library``, you may explicitly pass a ``POSTFIX`` to ``ccl_add_library``: 

.. code-block:: cmake
  
    ccl_add_library (my_library MODULE POSTFIX "${CCL_ISOLATION_POSTFIX}")
