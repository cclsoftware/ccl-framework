############
CCL Modeller
############

*This document refers to :ref:`tool version 1.2.0 <changelog_modeller>`*.

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

* **[action]**: one of [-list, -export, -scan, -update]
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

	$ cclmodeller -scan <sourcecode path> <classmodel file>


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
