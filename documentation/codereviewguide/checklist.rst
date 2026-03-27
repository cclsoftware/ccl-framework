################
Review Checklist
################

Checklist version: 1.0.0, 2025-05-23.

=======
General
=======

* Adhere to :ref:`CCL Coding Conventions<ccl_coding_conventions>`.
* Use forward declarations where applicable.
* Omit redundant namespace qualifiers, such as when using the ``using`` directive or already inside a namespace.
* Avoid micro-optimizations or premature optimizations that increase code complexity.
* Ensure state transitions are balanced. Examples: use ``setActive(true/false)``, pair ``initialize()`` with ``terminate()``, match ``startup()`` with ``shutdown()``.


=====
Types
=====

* Avoid mixing string types such as :cref:`MutableCString<CCL::MutableCString>` and :cref:`String<CCL::String>` to reduce overhead.
* Beware of implicit text encoding conversions. For example, converting a :cref:`String<CCL::String>` to a :cref:`MutableCString<CCL::MutableCString>` can downgrade Unicode to ASCII. See :ref:`Converting between Unicode strings and C-Strings<converting_strings>`.
* Use :cref:`UrlDisplayString<CCL::UrlDisplayString>` or :cref:`UrlFullString<CCL::UrlFullString>` to convert URLs to strings and preserve their representation. Vice versa, use methods such as :cref:`Url::fromDisplayString()<CCL::Url::fromDisplayString>`. Do not break file system URLs apart using :cref:`Url::getPath()<CCL::Url::getPath>`. See :ref:`String representations of an URL<url_strings>`.
* Use predefined reference types whenever possible. Prefer :cref:`StringRef<CCL::StringRef>` over ``const String&``. Other reference types: :cref:`CStringRef<CCL::CStringRef>`, :cref:`StringID<CCL::StringID>`, :cref:`UIDRef<CCL::UIDRef>`, :cref:`UrlRef<CCL::UrlRef>`. See :ref:`String reference types`<string_reference_types>.
* Always use :cref:`XSTRING<XSTRING>` or :cref:`XSTR<XSTR>` for user-facing strings, even if the application is not localized.
* Double-check data types in public interfaces; see :ref:`IUnknown interface reference<cclframework_define_iunknown_interface>`.

======
Memory
======

* Use :cref:`NEW<NEW>` macro instead of ``new`` operator for memory leak detection.
* Ensure memory allocations and deallocations are balanced. Use :cref:`NEW<NEW>` and ``delete``, ``retain()`` and ``release()``. Consider smart pointers and encapsulated classes such as :cref:`Buffer<CCL::Buffer>`.

=========
Scripting
=========

* When extending scriptable classes, maintain parity between native and script code.
* Use ``DEFINE_METHOD_*`` macros for all methods in :cref:`invokeMethod()<CCL::IObject::invokeMethod>`.
* Think about object lifetime when combining native code (using reference counting) and script code (using garbage collection).

===========
Application
===========

* Use human-readable title case for command categories and names, even for hidden commands. Command name and category should match the English display category and title. Example: *File - Save As*
