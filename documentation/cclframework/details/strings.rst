.. _using_strings:

#############
Using strings
#############

CCL provides classes for managing Unicode strings (:cref:`CCL::String`) and simple ASCII strings (:cref:`CCL::MutableCString`). Both support copy-on-write semantics and both can be passed safely between ABI boundaries as the memory is managed by the **ccltext** dynamic library.

===============
Unicode strings
===============

The :cref:`CCL::String` class should be used for all strings that are displayed to the user or can be entered by a user (e.g. via an EditBox control in the UI). Unicode strings use 16 bit per character in a subset of the UTF-16 text encoding. On macOS and iOS, the string class is a wrapper around a CFString object and on Windows, Android, and Linux it is a wide character buffer.

=========
C-Strings
=========

Besides mutable strings, there are some more specialized string classes like :cref:`CCL::CString` which is a light-weight wrapper for a C-string pointer, and Core::CStringBuffer which doesn't allocate additional heap memory and thus is more suitable for embedded systems.

:cref:`CCL::CString` and :cref:`CCL::MutableCString` can be used for internal IDs that are never presented to a user.
They have the advantage of a better performance and lower memory usage.

.. list-table::
  :widths: 2 5

  * - :cref:`CCL::CStringPtr`
    - An alias that should be used instead of const char*

  * - class :cref:`CCL::CString`
    - Lightweight wrapper to a plain CStringPtr that provides read-only string methods. Does not own the string's memory.

  * - class :cref:`CCL::MutableCString`
    - Allows read and write access to a C-String, manages the required memory.

.. _string_reference_types:

======================
String reference types
======================

.. list-table::

  * - :cref:`CCL::StringRef`
    - const reference to a :cref:`CCL::String`, to be used e.g. for read-only arguments in methods.

  * - :cref:`CCL::CStringRef`
    - const reference to a :cref:`CCL::CString`, to be used e.g. for read-only arguments in methods.

  * - :cref:`CCL::StringID`
    - same as :cref:`CCL::CStringRef`, additionally expresses "identifier" semantics.

.. _converting_strings:

================================================
Converting between Unicode strings and C-Strings
================================================

Since a C-String has only one byte per character (with 256 possible values), a TextEncoding is needed for converting between Unicode strings and C-strings. It's important to choose a reasonable encoding.

- The TextEncoding argument in methods of :cref:`CCL::MutableCString` usually defaults to ASCII. This is only sufficient if the strings are expected to be ASCII-only. 
- When C-strings appear in a certain protocol or file format, the encoding is typically defined in the corresponding specification.
- When arbitrary Unicode strings need to be losslessly encoded as C-strings, UTF8 is often a good choice.

Examples:

.. code-block:: cpp

	// create a C-String from a Unicode string
	String userEmail;
	queryEmail (userEmail); // user input
	MutableCString utf8UserEmail (userEmail, Text::kUTF8); // converts to C-String with UTF8 encoding
	
	// create a Unicode string from a C-String
	String email (Text::kUTF8, utf8UserEmail); // converts back to the exact same string above

.. code-block:: cpp

	MutableCString asciiIdentifier (unicodeIdentifier); // defaults to encoding Text::kASCII

================
String constants
================

The framework manages a global hash table of string constants of both :cref:`CCL::String` and :cref:`CCL::MutableCString`. Using them can be beneficial for often used strings, as it avoids creating the same string object many times and so saves memory. `System::GetConstantString` and `System::GetConstantCString` allocate a String object on the first call for a certain constant and return it on later calls. The macros `CCLSTR` and `CSTR` should be used as shortcuts.


.. list-table::
  :header-rows: 1

  * - Macro
    - Shortcut for
                                       
  * - ``CCLSTR``
    - ``System::GetConstantString (CStringPtr asciiString)``

  * - ``CSTR``
    - ``System::GetConstantCString (CStringPtr asciiString``

Examples:

.. code-block:: cpp

    fileType.setMimeType (CCLSTR ("application/octet-stream"));
 
    paramList.addString (CSTR ("documentName"));
 