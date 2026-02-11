############
Introduction
############

.. _ccl-introduction:

Welcome to Crystal Class Library\ |reg| (CCL), a cross-platform framework for desktop, mobile, and embedded application development in C++. As of 2026, the following target platforms are supported:

- Windows (based on Win32)
- macOS and iOS/iPadOS (based on Cocoa)
- Linux (with support for Gnome and KDE)
- Android

You can think of CCL as a unified virtual operating system. Instead of using platform-specific APIs and UI elements, applications are based on an additional abstract layer to make them portable between OS platforms. For example, you can develop and debug most of a mobile app for iOS/iPadOS on a PC using Visual Studio or VS Code, or release an application for all supported platforms simultaneously with little overhead.

The code esthetics of CCL is inspired by Java. It is using camel case instead of snake case like some popular C++ libraries. Code and documentation are written in American English. Many concepts used in CCL will be familiar if you have worked with other application frameworks like .NET, MFC, Qt, etc. before. However, there are some differences you need to learn and be aware of. 

======================
A short history lesson
======================

The first version of CCL was created by Matthias Juwan in 1999. The framework is distributed and maintained by CCL Software Licensing GmbH. PreSonus is a contributor since 2009.

The code is evolving continuously to adapt to new requirements. In 2012, parts of the framework were moved into the Core Library to allow reuse and sharing of code with RTOS-based platforms. By adding iOS as first mobile platform the same year, the framework was extended with features like multi-touch input and hardware-accelerated graphical animations. 

For more information and licensing terms and conditions, please visit `ccl.dev <https://ccl.dev>`_.

===============
Basic structure
===============

The framework is divided into several libraries, some are linked statically to the application and some dynamically.

Dynamic libraries
=================

- **ccltext**: everything related to Unicode strings, XML, localization support, etc.
- **cclsystem**: basics like file system access, multithreading and synchronization, IPC, plug-in loading, etc.
- **cclgui**: everything needed for a graphical user interface, XML-based skins, etc.
- **cclnet**: support for sockets, TCP, UDP, HTTP, etc.
- **cclsecurity**: crypthographic cipher algorithms, etc.

Static libraries
================

- **cclbase**: provides RTTI, collections, persistence, etc. (linked into all other libs, except **ccltext**)
- **cclapp**: application component classes and parameters, etc.

The reason for this separation is simple. A large-scale desktop or mobile application needs a certain level of modularity to stay maintainable for a longer period of time and certain features are split into plug-ins. The plug-ins also want to make use of the framework and instead of compiling the same code into all binaries multiple times, the majority of the framework itself is linked and loaded dynamically.

Another architectural reason is that in a large code base you don't want everyone in the team to mess with the fundamentals shared by all applications and e.g. create several versions of a class which might easily fit into a single, unified multi-purpose class. Dynamic linking sets a hard limit between the foundation and the application code.

That said, you'll notice that you sometimes cannot derive your own classes directly from framework classes via C++ inheritance. The good thing is, in general you don't have to and CCL provides alternative concepts which often result in better structured code. We'll follow up on that below.

=================
Interface concept
=================

CCL heavily uses the interface concept as found in `COM <https://en.wikipedia.org/wiki/Component_Object_Model>`_, `XPCOM <https://en.wikipedia.org/wiki/XPCOM>`_, `UNO <https://www.openoffice.org/udk/>`_, etc. At the C++ language level, interfaces are abstract classes with pure virtual methods only without any data members. The basic interface :cref:`CCL::IUnknown` provides methods for reference counting (retain/release) and a query mechanism to obtain additional purpose-specific interfaces of an object (queryInterface). On the semantic level, an interface can be seen like a contract that is obligatory to all classes implementing it. The implementation details of a class are hidden and they don't matter to the caller who can see the interface only - the caller can simply rely on the contract.

Interfaces also help with ABI-compatibility between multiple dynamically linked binaries. The ABI (Application Binary Interface) is the memory layout of your class at runtime. An application would simply crash if calls between modules aren't compatible on this lower level if e.g. members of a class are added or removed. Interfaces provide the required stability. They remain stable between framework releases, some of them are even marked frozen for eternity. Like with any other rule, there are exceptions here as well. If binary compatibility is lost, the framework revision is incremented and all related binaries have to be recompiled. This doesn't happen very often, though.

Interface names are prefixed with 'I' like 'IUnknown'. They are identified with 16 byte globally unique identifiers
(`GUID <http://en.wikipedia.org/wiki/Globally_unique_identifier>`_).

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