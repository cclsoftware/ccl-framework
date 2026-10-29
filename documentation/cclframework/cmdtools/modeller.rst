############
CCL Modeller
############

*This document refers to :ref:`tool version 1.3.0 <changelog_modeller>`*.

============
Introduction
============

The *CCL Modeller Tool* is a command line based utility tool that performs different tasks related to CCL type libraries and classmodel files.

=====
Usage
=====

.. code-block:: rst

	$ cclmodeller [action] <arg1> <arg2> [-v]


**Arguments:**

* **[action]**: one of [-list, -export, -scan, -staticexport, -update]
* **arg1**, **arg2**: action specific positional arguments
* **[-v]**: print debug logs

Run the tool without arguments to get usage instructions.



Actions
=======

list
----

Print a list of all registered type libraries. Usage:

.. code-block:: rst

	$ cclmodeller -list


export
------

Export type library *type library name* to *classmodel file*. Usage:

.. code-block:: rst

	$ cclmodeller -export <type library name> <classmodel file>


Output *classmodel file* argument is optional. If not specified, the type library classmodel is exported to working directory. Use **list** action to get a list of known type libraries, use **scan** action to add documentation from source code.


scan
----

Add source code documentation scanned from *sourcecode path* to *classmodel file*. Usage:

.. code-block:: rst

	$ cclmodeller -scan <sourcecode path>[;<sourcecode path>...] <classmodel file>


*sourcecode path* can be a folder, a single source file, or a semicolon separated list of folders and files (quote the argument in Unix shells), which are scanned as one source tree (e.g. ``#define`` constants in one folder are resolved for documentation in another). Duplicate paths are ignored with a warning.


staticexport
------------

Build a classmodel by statically scanning *sourcecode path* for CCL class declaration macros (``DEFINE_CLASS`` and friends), without compiling or running the code. Usage:

.. code-block:: rst

	$ cclmodeller -staticexport <sourcecode path>[;<sourcecode path>...] <classmodel file> [typelib=<type library>]


Output *classmodel file* argument is optional. If not specified, the classmodel is exported to the working directory, named after the last path component of *sourcecode path* (the first one, if several are given; without extension for a file). Unlike **export**, this action does not require the code to be compiled and loaded into a running process - use it when the type library cannot be loaded into the modeller (e.g. cross-compiled targets, or code not yet integrated into a plugin). Use **scan** action afterwards to add documentation from source code.

If the ``typelib`` option is given, the action scans *sourcecode path* for a type library instead, building the classmodel from the entries of *type library* (e.g. ``SKIN_TYPELIB``).

*sourcecode path* can be a folder, a single source file, or a semicolon separated list of folders and files (quote the argument in Unix shells). All paths are scanned as one source tree, so classes, parent classes and symbols (e.g. string id constants) can be spread across them (duplicate paths are ignored with a warning). Example:

.. code-block:: rst

	$ cclmodeller -staticexport "ccl/gui;ccl/base" gui.classModel typelib=SKIN_TYPELIB


update
------

Merge *prototype classmodel file* into *classmodel file*. Usage:

.. code-block:: rst

	$ cclmodeller -update <classmodel file> <prototype classmodel file>


Input *classmodel file* is updated in-place.

.. _changelog_modeller:


===============
Version history
===============

Changelog v1.3.0
================

* add classmodel 'staticexport' action
* 'scan' and 'staticexport' actions accept source files as well as folders, and a semicolon separated list of them


Changelog v1.2.1
================

* enums: scan docs from generated 'inline constexpr' constants


Changelog v1.2.0
================

* omit 'doc' option


Changelog v1.1.0
================

* add classmodel 'export' action
* add type library names 'list' action
* make output file arg for 'doc' action optional
* omit 'preparedoc' option


Changelog v1.0.1
================

* support escaped periods in docscan


Changelog v1.0.0
================

* initial version
