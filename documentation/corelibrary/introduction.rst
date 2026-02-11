############
Introduction
############

The Core Library is a self-contained part of the CCL Cross-platform Framework. It can be used for small cross-platform software projects, or for application development on RTOS-based embedded platforms with limited memory and processing resources. It follows the same design principles as its bigger sibling to make it easy for developers to switch between firmware and desktop/mobile app development.

The library has a dedicated low-level abstraction so that adding support for new platforms is straightforward. As of 2026, the following operating systems are supported out of the box:

- Windows (based on Win32)
- macOS and iOS/iPadOS (based on Cocoa)
- Linux (desktop and embedded)
- Android (via JNI, requires all of CCL)
- Zephyr
- Azure RTOS
- Little Kernel
- CMSIS RTOS
- CrossWorks Tasking Library (CTL)

The first version of CCL was created by Matthias Juwan in 1999. The framework is distributed and maintained by CCL Software Licensing GmbH. PreSonus is a contributor since 2009. For more information and licensing terms and conditions, please visit `ccl.dev <https://ccl.dev>`_.

=============================
Relation between Core and CCL
=============================

Most classes in the :cref:`Core` namespace are either imported into the :cref:`CCL` namespace or serve as base classes for their equivalents in CCL. Any changes made to :cref:`Core` classes need to be checked for compatibility with CCL. The naming scheme and folder structure might seem at little awkward at first, but it illustrates the module relationship between the framework libraries, and it is used to avoid name clashes at class and filename level.

Here's a quick, non-exhaustive overview of the folder and class relationships:

- Files from "core/public" can be referenced from files in "ccl/public"
- Files in "core/gui" correspond to the **cclgui** shared library
- Files in "core/system" correspond to the **cclsystem** shared library
- Files in "core/network" correspond to the **cclnet** shared library
- Files in "core/text" correspond to the **ccltext** shared library


Core::Portable
==============

Contains classes for standalone use (without all of CCL) or use in embedded systems. Implementations are platform-agnostic. Uses simplified CCL equivalent types to fit firmware development needs.


Core::Platform
==============

Contains abstractions for APIs that vary between platforms:

.. list-table::
   :widths: 30 70
   :header-rows: 1

   * - Path
     - Content
   * - *shared*
     - Common APIs.
   * - *shared/[platform]*
     - Platform specific classes wrapping native implementations.
   * - *shared/[multi-platform]*
     - Additional wrappers leveraging standard multi-platform libraries or interfaces.

API subsets and their implementation are defined in *platform/corefeatures.h*.


Linking
=======

The Core Library is linked statically to **ccltext**, **cclsystem** and **cclnet**, but it is typically not linked to individual applications. A desktop or mobile application can use classes from "core/public" directly. All other classes are considered hidden implementation details of the framework and application code needs to be based on "official" CCL APIs.

Examples:

- :cref:`Core::Threads` namespace is imported into :cref:`CCL::Threading` namespace
- :cref:`Core::Sockets` namespace is imported into :cref:`CCL::Net` namespace
- :cref:`Core::Text` namespace is imported into :cref:`CCL::Text` namespace
- :cref:`CCL::Threading::NativeThread` is derived from :cref:`Core::Threads::Thread`
- :cref:`CCL::Net::Socket` wraps a :cref:`Core::Sockets::Socket` instance
- :cref:`CCL::CString` is derived from :cref:`Core::CStringTraits`
- :cref:`CCL::MutableCString` is derived from :cref:`Core::MutableCStringTraits`
- :cref:`Core::Vector` and :cref:`Core::LinkedList` are imported directly into the :cref:`CCL` namespace
- etc.
