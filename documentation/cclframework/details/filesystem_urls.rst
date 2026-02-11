.. _file-system-urls:

####################
File system and URLs
####################

Files on disk, in a network or from other sources are addressed using the :cref:`CCL::Url` class with interface :cref:`CCL::IUrl` (see also :cref:`CCL::UrlRef`).
An URL consists of a protocol, hostname, path and optional parameters.

On Windows, a file system location like ``C:\Users\Me\Documents`` is represented as ``file:///C:/Users/Me/Documents``. You can access the file system singleton via

::

     CCL::System::GetFileSystem (see CCL::INativeFileSystem).

The file data can be read/written with the :cref:`CCL::IStream` interface. The file is closed when the stream object is released.

See also :ref:`virtual-file-system` for details about the supported protocols for files from other sources.

Example:
========

::

    Url pathToFile ("file:///C:/Users/Me/Documents/data.bin");
    AutoPtr<IStream> stream = System::GetFileSystem ().openStream (pathToFile, IStream::kOpenMode);

.. _url_strings:

================================
String representations of an URL
================================

:cref:`CCL::IUrl` has methods for creating different string representation of an URL. Some helper classes simplifiy their use.
For displaying an URL to the user, :cref:`CCL::UrlFullString` should be used.

.. list-table::
  :header-rows: 1
  :widths: 2 2 4

  * - Class
    - Shortcut for method
    - Description

  * - :cref:`CCL::UrlFullString`
    - :cref:`IUrl::getUrl<CCL::IUrl::getUrl>`
    - Full url with protocol and optionally parameters

  * - :cref:`CCL::UrlDisplayString`
    - :cref:`IUrl::toDisplayString<CCL::IUrl::toDisplayString>`
    - Beautified string for display respecting the platform conventions

  * - :cref:`CCL::NativePath`
    - :cref:`IUrl::fromNativePath<CCL::IUrl::fromNativePath>`
    - Native path string in UTF-16 enconding

  * - :cref:`CCL::POSIXPath`
    - :cref:`IUrl::toPOSIXPath<CCL::IUrl::toPOSIXPath>`
    - POSIX-style path ('/' as separator) in UTF-8 encoding

Example (Windows):

.. list-table::

  * - :cref:`CCL::UrlFullString`
    - ``file:///C:/Users/CCL/Documents/Notes.txt``

  * - :cref:`CCL::UrlDisplayString`
    - ``C:\Users\CCL\Documents\Notes.txt``

  * - :cref:`CCL::NativePath`
    - ``C:\Users\CCL\Documents\Notes.txt``

  * - :cref:`CCL::POSIXPath`
    - ``/C:/Users/CCL/Documents/Notes.txt``
