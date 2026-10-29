############
Introduction
############

.. _ccl-introduction:

Welcome to Crystal Class Library\ |reg| (CCL), a cross-platform framework for developing applications for desktop, mobile, embedded, and browser environments in C++. As of 2026, CCL supports the following target platforms:

- Windows
- macOS
- iOS/iPadOS
- Linux
- Android
- WebAssembly

You can think of CCL as a unified virtual operating system. Rather than relying directly on platform-specific APIs and UI elements, applications use an additional abstraction layer that helps make them portable across operating systems. For example, you can develop and debug most of a mobile app for iOS/iPadOS or Android on a PC using Visual Studio or VS Code. You can also release an application for multiple supported platforms with little additional effort.

CCL's coding style is inspired by Java. It uses camel case rather than snake case, as used by some popular C++ libraries. Code and documentation are written in American English. Many concepts in CCL will be familiar if you have worked with other application frameworks such as .NET or Qt. However, CCL also has some distinct concepts and conventions that you will need to learn.

======================
A short history lesson
======================

The first version of CCL was created by Matthias Juwan in 1999. The framework is distributed and maintained by CCL Software Licensing GmbH, with PreSonus contributing to its development since 2009.

CCL continues to evolve to meet new requirements. In 2012, parts of the framework were moved into the Core Library to enable code reuse and sharing with RTOS-based platforms. That same year, iOS became the first mobile platform supported by CCL, bringing features such as multi-touch input and hardware-accelerated graphical animations. With version 5.2, CCL can also target browser environments using WebAssembly.

For more information and licensing terms and conditions, please visit `ccl.dev <https://ccl.dev>`_.

===============
Basic structure
===============

The framework is divided into several libraries, some of which are linked statically into the application, while others are loaded dynamically.

Dynamic libraries
-----------------

- **ccltext**: Unicode strings, XML, localization, and related functionality.
- **cclsystem**: File system access, multithreading and synchronization, inter-process communication, plug-in loading, and other system services.
- **cclgui**: Graphical user interface functionality, including XML-based skins.
- **cclnet**: Network communication, including sockets, TCP, UDP, and HTTP.
- **cclsecurity**: Cryptographic algorithms and related security functionality.

Static libraries
----------------

- **cclbase**: Runtime type information (RTTI), collections, persistence, and other fundamental functionality. Linked into all other libraries except **ccltext**.
- **cclapp**: Application component classes, parameters, and related application-level functionality.

Why this separation?
--------------------

Larger applications and ecosystems need a certain level of modularity to remain maintainable over time. One way to achieve this is to move functionality into plug-ins rather than continually adding it to a single, monolithic application. These plug-ins also need access to the framework. By linking most of the framework dynamically, CCL avoids compiling the same framework code into multiple binaries.

There is another architectural reason for this separation. In a large codebase, the fundamentals shared by all applications need to remain consistent. You don't want different teams independently implementing slightly different versions of a class when a single, unified, multi-purpose class would suffice. Dynamic linking helps establish a clear boundary between the shared framework and application-specific code.

CCL deliberately maintains this boundary at the API level. As a result, you cannot always derive your own classes directly from framework classes using C++ inheritance. This is an architectural choice rather than an inherent limitation. Instead, CCL provides alternative ways to extend and customize framework functionality, often resulting in better-structured code. We'll explore these concepts in the following sections.

=================
Interface concept
=================

CCL makes extensive use of the interface concept, as found in `COM <https://en.wikipedia.org/wiki/Component_Object_Model>`_, `XPCOM <https://en.wikipedia.org/wiki/XPCOM>`_, `UNO <https://www.openoffice.org/udk/>`_, and similar component models. In CCL, interfaces are represented in C++ by abstract classes containing only pure virtual methods and no data members.

The fundamental interface :cref:`CCL::IUnknown` provides methods for reference counting (``retain`` and ``release``) and a query mechanism (``queryInterface``) for obtaining additional purpose-specific interfaces supported by an object.

Conceptually, an interface defines a contract that every implementing class must fulfill. The implementation details remain hidden from the caller, which interacts with the object exclusively through its interfaces and can rely on the contracts they define.

Interfaces also help maintain ABI (Application Binary Interface) compatibility between dynamically linked binaries. The ABI defines how compiled code interacts at the binary level, including object layouts, calling conventions, and method calls. Changes to a class's layout or other ABI-relevant details can make independently compiled modules incompatible, potentially causing crashes or other failures.

By exposing stable interfaces rather than implementation classes, CCL helps preserve binary compatibility across framework releases. Interface definitions are kept stable, and some interfaces are designated as frozen, meaning they must remain binary-compatible indefinitely. There are exceptions, however. If binary compatibility is broken, the framework ABI revision is incremented, and all dependent binaries must be recompiled.

Interface names are prefixed with ``I``, as in ``IUnknown``. Each interface is identified by a 16-byte globally unique identifier (`GUID <https://en.wikipedia.org/wiki/Globally_unique_identifier>`_).

==================
Allocating objects
==================

Framework classes that you can't access directly are either created with special functions in the :cref:`CCL::System` namespace exported by the framework DLLs, or by using the :cref:`CCL::ccl_new` template function.

In regular C++ you would create and release object like this:

.. code-block:: cpp

    Object* object = new Object;
    delete object;

In CCL, you'll often notice calls like this:

.. code-block:: cpp

    IObject* object = ccl_new<IObject> (ClassID::Object);
    object->release ();

In the second code snippet, the class is identified by a `GUID <http://en.wikipedia.org/wiki/Globally_unique_identifier>`_ and the requested interface is passed as template argument.

To deal with reference counting, we use smart pointer classes like :cref:`CCL::AutoPtr` and :cref:`CCL::SharedPtr`.

=========
Scripting
=========

CCL::IUnknown is a fundamental piece when writing native C++ code with CCL. However, to support dynamically typed scripting languages, the :cref:`CCL::IObject` interface comes into play. It exposes type information which can be used by a scripting engine to bridge calls between the native and the scripting world at runtime. CCL includes the `Mozilla SpiderMonkey JavaScript engine <https://spidermonkey.dev/>`_ as a service for applications.

================
Type information
================

All classes derived from :cref:`CCL::Object` provide runtime type information (RTTI) via :cref:`CCL::MetaClass` and thus dynamic type casting. Instead of the language feature dynamic_cast<>, we use our own :cref:`CCL::ccl_cast` and :cref:`CCL::unknown_cast` templates. Why? Because we have to deal with interfaces and we need our own meta classes anyway for object serialization and scripting support.

:cref:`CCL::ccl_cast` can be used when you already have pointer to a class derived from :cref:`CCL::Object`. To cast from an interface pointer (e.g. :cref:`CCL::IUnknown`), :cref:`CCL::unknown_cast` has to be used. In both cases, the class you are casting to has to implemented in the same module as the calling code, otherwise the cast fails.

.. code-block:: cpp

    // casting from an Object pointer to a more concrete class
    Container* container = ...;
    auto* array1 = ccl_cast<ObjectArray> (container);

    // casting from an interface pointer to a concrete class
    IUnknown* unknown = ...;
    auto* array2 = unknown_cast<ObjectArray> (unknown);

==================
Memory allocations
==================

All memory allocations made by CCL are redirected to our own memory management functions (``Core::core_malloc``, ``Core::core_free``, etc) and end up in the **ccltext** library. This way we can override the default memory allocator of the C runtime library with more efficient implementations and also add heap debugging features.

We use our own uppercase ``NEW`` macro instead of the lowercase ``new`` language operator to be able to trace back memory leaks to the source file and line number of allocation in debug builds. This is of course stripped from release builds.

=======
Strings
=======

Like other application frameworks, CCL also provides string classes. We differentiate between Unicode strings for user data and simple ASCII strings used as internal identifiers, represented by the two classes :cref:`CCL::String` and :cref:`CCL::MutableCString`. Both support copy-on-write semantics and both can be passed safely between ABI boundaries as the memory is managed by the **ccltext** dynamic library.

See also :ref:`using_strings` for details about using the string classes.
 
====================
File system and URLs
====================

Files on disk, in a network or from other sources are addressed using the :cref:`CCL::Url` class with interface :cref:`CCL::IUrl` (see also :cref:`CCL::UrlRef`).
An URL consists of a protocol, hostname, path and optional parameters.

On Windows, a file system location like ``C:\Users\Me\Documents`` is represented as ``file:///C:/Users/Me/Documents``. You can access the file system singleton via

::

     CCL::System::GetFileSystem (see CCL::INativeFileSystem).

See :ref:`file-system-urls` for more details about the:cref:`CCL::Url` class and its string representations.
See also :ref:`virtual-file-system` for details about the supported protocols for files from other sources.

=======
Threads
=======

The easiest way to create a background thread is by deriving from the :cref:`CCL::Threading::UserThread` helper class. The most commonly used synchronization object probably is :cref:`CCL::Threading::CriticalSection` in conjunction with :cref:`CCL::Threading::ScopedLock`.


=======
Signals
=======

CCL implements the `observer design pattern <http://en.wikipedia.org/wiki/Observer_pattern>`_ for notifications between objects. Dependencies and signaling is handled by the global :cref:`CCL::ISignalHandler` singleton accessed via  :cref:`CCL::System::GetSignalHandler`. Usually you don't have to call the signal handler directly because the :cref:`CCL::Object` class already implements methods for this purpose.

::

    object.signal (Message (kChanged));

=====================
Plug-ins and services
=====================

CCL provides a unified plug-in architecture. Plug-in DLLs (or bundles on macOS) export a single C-style function named 'CCLGetClassFactory' which returns a pointer to the DLL's factory object based on the :cref:`CCL::IClassFactory` interface. All plug-in and public framework classes are managed by a central :cref:`CCL::IPlugInManager` instance which you can access via  :cref:`CCL::System::GetPlugInManager`. Contrary to built-in framework classes, objects created from plug-in DLLs via :cref:`CCL::ccl_new` have to be destroyed via :cref:`CCL::ccl_release` to ensure that the binary image is unloaded from memory when the last class instance is released.

The plug-in manager also transparently loads DLLs based on the low-level Core plug-in API, and classes implemented in JavaScript.

Services are extensions to the framework like scripting support, managed by the :cref:`CCL::IServiceManager` singleton. A service is started when the host application starts up and remains active until the host shuts down.

.. |reg|    unicode:: U+000AE .. REGISTERED SIGN
