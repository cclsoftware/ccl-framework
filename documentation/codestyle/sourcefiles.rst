############
Source Files
############

Source file names are all lowercase letters. A source file begins with a unified header containing the project name, copyright, filename, and a description:

.. Not the exact format, long lines shortened to fit PDF format.

.. code-block:: cpp

  //************************************************************************
  //
  // Project Name
  // Copyright (c) 2023 CCL Software Licensing GmbH
  //
  // Filename    : module/folder/filename.cpp
  // Description : File Description
  //
  //************************************************************************

Add complete and correct headers only, refrain from adding them otherwise.

========
Includes
========

Include guards in header files are composed from the file name in lowercase letters. Adding a module name to avoid name clashes is optional:

.. code-block:: cpp

  #ifndef _filename_h
  #define _filename_h

  namespace CCL {

  // ...

  } // namespace CCL

  #endif // _filename_h

In general, included files are using paths relative to the root of your working copy:

.. code-block:: cpp

  #include "module/folder/file.h"


=================
Header file rules
=================

* Use forward declarations wherever possible instead of includes.
* Do not use *using namespace Core/CCL* .
* Avoid redundant include files, respect transitive includes.
  