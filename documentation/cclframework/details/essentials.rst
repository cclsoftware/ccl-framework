##########
Essentials
##########

Overview of essential interfaces, classes, concepts, etc.

============
Fundamentals
============

.. list-table::
  :header-rows: 1
  :widths: 1 3

  * - Interface / Class
    - Notes

  * - :cref:`IUnknown<CCL::IUnknown>`
    - Basic interface to manage object lifetime and to obtain other interface pointers

  * - :cref:`Unknown<CCL::Unknown>`
    - Reference-counted base class for implementing interfaces.

      **Initial reference count is 1**

  * - :cref:`Object<CCL::Object>`
    - Object base class with RTTI, messages, storage

  * - :cref:`UID<CCL::UID>`, :cref:`UIDRef<CCL::UIDRef>`
    - 16-Byte Globally Unique Identifier.

      - Interface ID used with :cref:`queryInterface<CCL::IUnknown::queryInterface>`
      - Class ID used with :cref:`ccl_new<CCL::ccl_new>`

  * - :cref:`AutoPtr<CCL::AutoPtr>`
    - Smart pointer. Takes ownership on assignement of raw IUnknown/Object pointer

  * - :cref:`SharedPtr<CCL::SharedPtr>`
    - Smart pointer. Similar to AutoPtr, calls addRef and release balanced

  * - :cref:`UnknownPtr<CCL::UnknownPtr>`
    - Smart pointer + queryInterface

  * - :cref:`Variant<CCL::Variant>`
    - Data type used to store different types of data. A Variant can contain various types of data, such as:

      - Integers
      - Floating-point numbers
      - Strings
      - Object references
      - Boolean values (True/False)


--------------
Using IUnknown
--------------

.. list-table::
  :header-rows: 1
  :widths: 1 3

  * - Function / Interface
    - Notes  / Examples

  * - :cref:`ccl_new<CCL::ccl_new>`
    - Create instance via Plug-in Manager

      .. code-block:: cpp

        AutoPtr<IProgressNotify> dialog;
        dialog = ccl_new<IProgressNotify> (CCL::ClassID::ProgressDialog);

  * - :cref:`ccl_release<CCL::ccl_release>`
    - Instances  of 'real' plug-ins (dynamically loaded modules) must be released with ccl_release. This can be a little bit tricky, because ccl_release is not not supported by smart pointers.

  * - :cref:`isEqualUnknown<CCL::isEqualUnknown>`
    - Test if interface pointers have the same implementation class instance

      .. code-block:: cpp

        bool isSame (ISubject* subject, IUnknown* unknown)
        {
          return isEqualUnknown (subject, unknown);
        }

      Pointers must not be compared directly !

  * - Cast interfaces:

      :cref:`UnknownPtr<CCL::UnknownPtr>`
    - Example for casting one interface to another

      .. code-block:: cpp

        void addObserver (IUnknown* unknown, IObserver* observer)
        {
          UnknownPtr<ISubject> subject (unknown);
          if(subject)
            subject->addObserver (observer);
        }


.. _cclframework_define_iunknown_interface:

----------------------------------
Define interface based on IUnknown
----------------------------------

Only "public" data types are allowed as parameters and return types.
The idea is that the code has the same binary output with any compiler on a platform.

**Per value: fundamental data types:**

- int, int64
- float, double
- tbool, tresult
- **but no compiler dependent types** like 'bool'
- enum types are allowed if they specify an explicit underlying type, like `enum Mode: int`

**Per reference or pointer:**

- String, Variant
- struct (interface specific with no or inline methods)
- IUnknown and other interfaces but **no implementation classes**.


.. list-table::
  :header-rows: 1
  :widths: 2 3 4

  * - Macro
    - Notes
    - Example

  * - :cref:`DECLARE_IID`, :cref:`DEFINE_IID`
    - Declare and define identifier of interface
    - .. code-block:: cpp

        interface INewInterface: IUnknown
        {
          // ...
          DECLARE_IID (INewInterface)
        };

        DEFINE_IID (INewInterface, 0xc5d8ffd1, 0xd7cd, ...)

  * - :cref:`CCL_API`
    - Calling convention needed for every interface method
    - .. code-block:: cpp

        virtual tresult CCL_API interfaceMethod () = 0;


--------------
Object Details
--------------

.. list-table::
  :header-rows: 1
  :widths: 1 3 4

  * - Macro / Function
    - Notes
    - Example

  * - :cref:`DECLARE_CLASS`, :cref:`DECLARE_CLASS_ABSTRACT`
    - Header: Declare RTTI for Object (:cref:`CCL::MetaClass`)
    - .. code-block:: cpp

        class MyClass: public CCL::Object
        {
          public:
            DECLARE_CLASS (MyClass, Object)
           // ...
        };

  * - :cref:`DEFINE_CLASS`, :cref:`DEFINE_CLASS_HIDDEN`, ...
    - CPP: Define Meta Class
    - .. code-block:: cpp

        DEFINE_CLASS_HIDDEN (MyClass, Object)

      Hidden: Class is not added to the class registry.

  * - :cref:`CLASS_INTERFACE`, :cref:`CLASS_INTERFACE2` :cref:`CLASS_INTERFACES`
    - Define Interfaces in an implementation class
    - .. code-block:: cpp

        class MyClass: public CCL::Object,
                       public CCL::IMyInterface
        {
          public:
            // ...
            CLASS_INTERFACE (IMyInterface, Object)
            // ...
        };

  * - :cref:`ccl_typeid<CCL::ccl_typeid>`
    - Template function to get the meta class of a class derived from Object
    - .. code-block:: cpp

        ccl_typeid<MyClass> ().getPersistentName ();


-------
Casting
-------

.. list-table::
  :header-rows: 1
  :widths: 1 3 4

  * - Case
    - Function
    - Example

  * - Object to Object
    - :cref:`ccl_cast<CCL::ccl_cast>` + :cref:`ccl_strict_cast<CCL::ccl_strict_cast>`
    - .. code-block:: cpp

        Url* asUrl (Object* obj)
        {
          return ccl_cast<Url> (obj);
        }

  * - Interface to Object
    - :cref:`unknown_cast<CCL::unknown_cast>`
    - .. code-block:: cpp

        Object* getUrlObject (IUrl* url)
        {
          return unknown_cast<Object> (url);
        }

      Only works if the implementation of interface is in the same module

  * - Object to interface
    - :cref:`ccl_as_unknown<CCL::ccl_as_unknown>`
    - .. code-block:: cpp

        bool activateObject (Object* object, bool state)
        {
          if(IActivatable* act = ccl_as_unknown<IActivatable> (object))
          {
            if(state)
              act->activate ();
            else
              act->deactivate ();
          }
        }


=========
Container
=========

.. list-table::
  :header-rows: 1

  * - Familiy
    - Class
    - Notes

  * - Template Container
    - :cref:`LinkedList<Core::LinkedList>`
    - Double-linked list container class

  * -
    - :cref:`Vector<Core::Vector>`
    - One-dimensional array that grows dynamically

  * - Object Container
    - :cref:`Container<CCL::Container>`, :cref:`Iterator<CCL::Iterator>`
    - Abstract container base class and iterator

  * -
    - :cref:`ObjectList<CCL::ObjectList>`
    - Double-linked object list

  * -
    - :cref:`ObjectArray<CCL::ObjectArray>`
    - Container class for array of objects

  * - Interface Container
    - :cref:`UnknownList<CCL::UnknownList>`, :cref:`IUnknownList<CCL::IUnknownList>`
    - List of IUnknown instances


**Template Container**

- \+ type-safe
- \+ can contain any data
- \+ reference counting can be added via smart pointers  ``Vector <AutoPtr <Unknown> > UnknownVector;``
- \- storage not supported
- \- no scripting access


**Object Container**

- \+ common abstract interfaces (Container, Iterator)
- \+ provide reference counting (but not in 'remove')
- \+ provide storage support
- \- content type always unspecific (``ccl_cast`` is needed)
- preferred for types derived from Object
- combine with iterate_as<> in range-for loop
- use when script access is needed


**Interface Container**

- use with interface at ABI boundaries (IUnknownList)


=======
Strings
=======

.. list-table::
  :header-rows: 1

  * - Classs
    - Related
    - Notes

  * - :cref:`String<CCL::String>`
    - :cref:`StringChars<CCL::StringChars>`

      :cref:`StringWriter<CCL::StringWriter>`

      :cref:`StringBuilder<CCL::StringBuilder>`
    - Unicode string and utility classes.

  * - :cref:`MutableCString<CCL::MutableCString>`
    - :cref:`CString<CCL::CString>`
    - C-String, 8-Bit. CString is read only


===================
Messages + Observer
===================

.. list-table::
  :header-rows: 1
  :widths: 1 2 4

  * - Interface / Class
    - Related
    - Notes

  * - :cref:`Message<CCL::Message>`
    - :cref:`IMessage<CCL::IMessage>`

      :cref:`MessageRef<CCL::MessageRef>`
    - Has Identifier + Parameter. Most messages are specific to a class, but there a some common messages:

      - :cref:`kChanged<CCL::kChanged>`
      - :cref:`kPropertyChanged<CCL::kPropertyChanged>`
      - :cref:`kDestroyed<CCL::kDestroyed>` **(see remark below)**

  * - :cref:`ISubject<CCL::ISubject>`
    - Implemented in :cref:`Object<CCL::Object>`
    - A subject notifies observers about changes. Observers register with a subject

  * - :cref:`IObserver<CCL::IObserver>`
    - Implemented in :cref:`Object<CCL::Object>`
    - Message receiver. Will be notified if one of its subjects sends a message.

  * - :cref:`ISignalHandler<CCL::ISignalHandler>`
    - :cref:`System::GetSignalHandler<CCL::System::GetSignalHandler>`
    - Central registry of ISubject/IObserver relations. Also implements deferred messages.

  * - :cref:`SignalSink<CCL::SignalSink>`
    - :cref:`SignalSource<CCL::SignalSource>`
    - Process-wide signals. Categories and Message are defined in namespace ``Signals``

- Message instances can be created direcly (no ccl_new needed)
- Object implements both ISubject (full) and IObserver (empty)
- **kDestroyed** is usually called inside of a destructor.
  isEqualUnknown or other methods that modify the reference count of an object must not used when this message is received
- Message can be delivered **deferred**. In this case the receiver must call **cancelSignals** in its destructor

  Example:

  .. code-block:: cpp

    DEFINE_STRINGID_MEMBER_ (MyClass, kOpenWindow, "openWindow")

    //////////////////////////////////////////////////////////////////////////////////

    MyClass::~MyClass
    {
      cancelSignals ();
    }

    //////////////////////////////////////////////////////////////////////////////////

    void MyClass::openWindow (bool deferred)
    {
      if(deferred)
        (NEW Message (kOpenWindow))->post (this, -1);
      else
      {
        // ...
      }
    }

    //////////////////////////////////////////////////////////////////////////////////

    void CCL_API MyClass::notify (ISubject* subject, MessageRef msg)
    {
      if(msg == kOpenWindow)
      {
        openWindow (false);
        return;
      }

      SuperClass::notify (subject, msg);
    }


===========
File System
===========

.. list-table::
  :header-rows: 1

  * - Interface / Class
    - Notes

  * - :cref:`FileType<CCL::FileType>`
    - Extension, MIME type, Description

  * - :cref:`Url<CCL::Url>`, :cref:`IUrl<CCL::IUrl>`
    - Uniform Resource Locator

  * - :cref:`IFileSystem<CCL::IFileSystem>`, :cref:`INativeFileSystem<CCL::INativeFileSystem>`
    - Singleton :cref:`System::GetFileSystem<CCL::System::GetFileSystem>`

  * - :cref:`IStream<CCL::IStream>`
    - Access to open file. See also :cref:`Streamer<CCL::Streamer>`

  * - :cref:`File<CCL::File>`
    - Helper class


==================
Object Persistence
==================

.. list-table::
  :header-rows: 1
  :widths: 1 3

  * - Class / Method
    - Notes

  * - :cref:`Object::save<CCL::Object::save>`

      :cref:`Object::load<CCL::Object::load>`
    - Overwrite in derived class to store values of members in Attributes. See also :cref:`DEFINE_CLASS_PERSISTENT`

  * - :cref:`Attributes<CCL::Attributes>`
    - Nested map of string id and value

  * - :cref:`Archive<CCL::Archive>`
    - An archive reads/writes Attributes from/to streams. Archive is a base class.

      Most often used archive type is XML :cref:`XmlArchive<CCL::XmlArchive>`
  * - :cref:`Storage<CCL::Storage>`
    - Storage combines Archive and Attributes.

  * - :cref:`ArchiveHandler<CCL::ArchiveHandler>`
    - Helper class to load/save objects from/to a structured storage (Zip-File). An Archive in this case reads/writes a sub-stream (file) of the structured file.


=============
Miscellaneous
=============

.. list-table::
  :header-rows: 1
  :widths: 1 3

  * - Interface / Class
    - Notes

  * - :cref:`Singleton<CCL::Singleton>`
    - Singleton mix-in, destroyed automatically on exit

      .. code-block::

        class MyClass: public CCL::Object,
                       public CCL::Singleton<MyClass>
        {
          void myMethod ();
        };

        // ...

        MyClass::instance ().myMethod ();

        // ...


      Variants:

      - :cref:`ExternalSingleton<CCL::ExternalSingleton>`
      - :cref:`SharedSingleton<CCL::SharedSingleton>`
      - :cref:`UnmanagedSingleton<CCL::UnmanagedSingleton>`
      - :cref:`ComponentSingleton<CCL::ComponentSingleton>`

  * - :cref:`NumericLimits<CCL::NumericLimits>`
    - Namespace containing constants like :cref:`kMaxInt<CCL::NumericLimits::kMaxInt>`

  * - Primitives
    - Various utility functions, examples:

      - :cref:`ccl_min`
      - :cref:`ccl_max`
      - :cref:`ccl_abs`
      - :cref:`ccl_bound`

  * - :cref:`CCL_KERNEL_INIT`, :cref:`CCL_KERNEL_TERM`
    - Initializers are called on application/module startup. The main purpose is to register something somewhere.

      Example:

      .. code-block::

        CCL_KERNEL_INIT (AudioEffect)
        {
          // register native .fxpreset handler
          (NEW AudioFXPresetHandler)->registerSelf ();
          return true;
        }

  * - :cref:`IProgressNotify<CCL::IProgressNotify>`
    - Used to report the progress of an operation


======================
Parameter + Controller
======================

.. list-table::
  :header-rows: 1
  :widths: 1 3

  * - Class / Method
    - Notes

  * - :cref:`IParameter<CCL::IParameter>`
    - GUI data object representing a simple value that is displayed and modified by standard controls.
      Usually a IParameter is owned by a controller that handles changes.

  * - :cref:`IController<CCL::IController>`
    - Implements the back-end logic for user interaction.
      It provides parameters and other objects used by GUI widgets.
      It can also be in the role of a model, or it can represent another data model

  * - :cref:`Component<CCL::Component>`
    - Standard implementation of a Controller. Ready to use or base class for user defined components.
      Components are nested and can also create specific views.

  * - :cref:`ParamContainer<CCL::ParamContainer>`
    - Manages parameters. Has methods to create and add standard and user defined parameter objects.
      Is a member 'paramList' of :cref:`Component<CCL::Component>`. But can also be used by itself.


===============
Frequently Used
===============

.. list-table:: Most Frequently used system singletons
  :header-rows: 1
  :widths: 2 2 3

  * - Interface
    - Function
    - Notes

  * - :cref:`INativeFileSystem<CCL::INativeFileSystem>`
    - :cref:`System::GetFileSystem<CCL::System::GetFileSystem>`
    - Get file system singleton

  * - :cref:`ISystemInformation<CCL::ISystemInformation>`
    - :cref:`System::GetSystem<CCL::System::GetSystem>`
    - Get system information singleton

  * - :cref:`ISignalHandler<CCL::ISignalHandler>`
    - :cref:`System::GetSignalHandler<CCL::System::GetSignalHandler>`
    - Get signal handler singleton

  * - :cref:`IUserInterface<CCL::IUserInterface>`
    - :cref:`System::GetGUI<CCL::System::GetGUI>`
    - Get GUI Management singleton

  * - :cref:`IDesktop<CCL::IDesktop>`
    - :cref:`System::GetDesktop<CCL::System::GetDesktop>`
    - Get Desktop singleton

  * - :cref:`IWindowManager<CCL::IWindowManager>`
    - :cref:`System::GetWindowManager<CCL::System::GetWindowManager>`
    - Get Window Manager singleton

  * - :cref:`IPlugInManager<CCL::IPlugInManager>`
    - :cref:`System::GetPlugInManager<CCL::System::GetPlugInManager>`
    - Get plug-in manager singleton. Manages classes from all process modules ("Plug-ins").


.. list-table:: Most Frequently used components created with ccl_new
  :header-rows: 1

  * - Class ID
    - Interfaces
    - Notes

  * - ``CCL::ClassID::ProgressDialog``
    - :cref:`IProgressNotify<CCL::IProgressNotify>`, :cref:`IProgressDialog<CCL::IProgressDialog>`, :cref:`IProgressDetails<CCL::IProgressDetails>`
    - Creates a progress dialog

  * - ``CCL::ClassID::FileSelector``
    - :cref:`IFileSelector<CCL::IFileSelector>`
    - Creates a platform specific file selector dialog. Can be used to select files for open or save in the platform file system.

  * - ``CCL::ClassID::FolderSelector``
    - :cref:`IFolderSelector<CCL::IFolderSelector>`
    - Creates a platform specific folder selector dialog. Can be used to let the user select a folder in the platform file system.

  * - ``CCL::ClassID::PopupSelector``
    - :cref:`IPopupSelector<CCL::IPopupSelector>`
    - Create a popup window that allows to select one out of multiple items.
