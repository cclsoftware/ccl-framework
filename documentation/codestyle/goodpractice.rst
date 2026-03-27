#############
Good Practice
#############

=========
Variables
=========

* Declare variables on first use, do not place them on top of a function body like in C.


=========
Functions
=========

* Put output arguments before input arguments:

  .. code-block:: cpp

    // Right
    void print (String& result, int v);

    // Wrong
    void print (int v, String& result);

* Try to keep function argument count to 5 or fewer; consider structs or flags instead of multiple booleans.

.. code-block:: cpp

    // Right
    struct PrintOptions
    {
      bool bold;
      bool italic;
      bool underline;
      int fontSize;
      String fontName;
    };

    void printText (StringRef text, const PrintOptions& options);

    // Wrong
    void printText (StringRef text, bool bold, bool italic,
      bool underline, int fontSize, StringRef fontName);

Naming
------

* Follow the *Get Rule*: If function names start with "get", a returned object is not owned by the caller. Use "create", "copy", or any other more specific verb to indicate ownership.

.. code-block:: cpp

    // Right
    SomeClass* getObject ()
    {
        return someObject;
    }

    // Wrong
    SomeClass* getObject () // Use createObject () instead
    {
        return new SomeClass ();
    }

    SomeClass* getObject () // Use copyObject () instead
    {
        return new SomeClass (someObject);
    }


=====
Const
=====

* Omit const for local variables by default, unless it makes the code easier to understand.

  .. code-block:: cpp

    // Right
    int temporaryVariable = getValue ();
    useValue (temporaryVariable);

    for(int value : values)
    {
      ...
    }


    // Wrong
    const int temporaryVariable = getValue ();
    useValue (temporaryVariable);

    for(const int value : values)
    {
      ...
    }


* Omit const for local pointers.

  .. code-block:: cpp

    // Right
    SomeClass* object = Factory::createObject ();
    const SomeClass* immutableObject = Factory::getImmutableObject ();

    for(SomeClass* item : myVector)
    {
      ...
    }

    // Wrong
    SomeClass* const object = Factory::createObject ();
    const SomeClass* const immutableObject = Factory::getImmutableObject ();

    for(SomeClass* const item : myVector)
    {
      ...
    }

* Use const for function declarations by default.

  .. code-block:: cpp

    // Right
    int getValue () const;
    void setValue (const Object&);

    // Wrong
    int getValue ();
    void setValue (Object&);


==================
Control Statements
==================

* Case 'default' must not be the first or only case in a switch statement:

  .. code-block:: cpp

    // Right
    switch(a)
    {
    case kCase1 :
      // ...
      break;
    case default :
      // ...
    }

    // Wrong
    switch (a)
    {
      default :
        // ...
        break;
      case kCase1 :
        // ...
    }

    // Wrong
    switch (a)
    {
      default :
        // ...
        break;
    }

* No empty 'default' cases in switch statements:

  .. code-block:: cpp

    // Right
    switch(a)
    {
    case kCase1 :
      // ...
      break;
    }

    // Wrong
    switch (a)
    {
    case kCase1 :
      // ...
      break;
    default:
      break;
    }

* In conditional statements, put the constant term on the right side of the comparison (i.e. avoid the *Yoda Condition*):

  .. code-block:: cpp

    // Right
    if(x >= 0)

    // Wrong
    if(0 <= x)


========
Pointers
========

* Check for null pointers but don't get too crazy: If you have just created the object a few lines above you are safe.
* Consider using a reference over a pointer:
  
  * Use ``Type&`` instead of ``Type*`` if null is not expected anyway.
  * Return const references from get functions instead of pointers.

* Use ``nullptr`` over ``0`` or ``NULL`` for unitialized state.

 
=======
Spacing
=======

* Do not align assignments:

  .. code-block:: cpp

    // Right
    int shortName = 1;
    int longerVariableName = 2;

    // Wrong
    int shortName           = 1;
    int longerVariableName  = 2;


=========
Operators
=========

* Use postfix increment operator unless prefix increment is absolutely necessary:

  .. code-block:: cpp

    // Right
    for(int i = 0; i < 10; i++)

    // Wrong
    for(int i = 0; i < 10; ++i)


=====
Casts
=====

* Prefer C++ style casts for non-primitive types:

  .. code-block:: cpp

    // Right
    SomeEnum value = static_cast<SomeEnum> (intVariable)

    // Wrong
    SomeEnum value = (SomeEnum) intVariable;

* Prefer C++ style casts for pointer types:

  .. code-block:: cpp

    // Right
    char* value = reinterpret_cast<char*> (data)

    // Wrong
    char* value = (char*) data;

* Use function style cast for primitive types:

  .. code-block:: cpp

    // Right
    double value = double(expr);

    // Wrong
    double value = static_cast<double> (expr);

================
String Constants
================

* When declaring string constants in namespaces, use ``const CStringPtr``. Details:

  * ``CStringPtr`` is a pointer to constant data, ``const CStringPtr`` is a const pointer to const data
  * ``static`` is not required here, as const variables have internal linkage by default in a C++ namespace scope
    
  .. code-block:: cpp
  
    namespace MyStringConstants
    {
        const CStringPtr kSomeConstant = "someConstant";
    }
    
* When declaring string constants in a class definition, use ``static const CStringPtr`` or ``DECLARE_STRINGID_MEMBER``:

  * Use ``static const CStringPtr`` for simple constants
  * Use ``DECLARE_STRINGID_MEMBER`` for use of ``CString`` class methods
  
  .. code-block:: cpp
  
    class MyClass
    {
    public:
        MyClass ();
    
        DECLARE_STRINGID_MEMBER (kStringConstant)
        
    protected:
        static const CStringPtr kAnotherStringConstant;
    };
    

====
auto
====
    
* Do use the ``auto`` keyword to simplify code for the reader, particularly in context of lambdas and templates.

* Don't use the ``auto`` keyword to make code simpler to write.

* When assigning return values from API methods, use an explicit type to make the code easy to understand:
    
    * It is ok to use ``auto`` for temporary variables that are only used to pass values to other methods.

  .. code-block:: cpp

    // Right 
    ISpecificInterface* instance = someObject->getSomething ();
    
    // Wrong
    auto* instance = someObject->getSomething ();
    
* You may use the ``auto`` keyword to avoid redundancy:

  .. code-block:: cpp

    // Right
    auto* implementation = unknown_cast<MyImplementationClass> (instance);
    
    // Also right
    MyImplementationClass* implementation = unknown_cast<MyImplementationClass> (instance);
  
* Prefer ``auto*`` and ``auto&`` over ``auto`` when assigning pointer or reference types.

    * Note that pointers are auto-deducted, but references are not. Using ``auto`` instead of ``auto&`` may lead to unwantend copies.

  .. code-block:: cpp

    // Right
    auto* implementation = unknown_cast<MyImplementationClass> (instance);
    
    // Wrong
    auto implementation = unknown_cast<MyImplementationClass> (instance);
    
* Do use the ``auto`` keyword when defining lambdas:
  
  .. code-block:: cpp
  
    // Right
    auto someLambdaFunction = [&] ()
    {
    ...
    }

* Do use the ``auto`` keyword to simplify templates.

  .. code-block:: cpp
 
    // Right
    template <typename T>
    void templateMethod (const T& item)
    {
        auto internalData = item.getData ();
        ...
    }
 
    // Wrong
    template <typename T>
    void templateMethod (const T& item)
    {
        typedef decltype(T::data) InternalType;
        InternalType internalData = item.getData ();
        ...
    }
    
* Prefer typedef declarations over using ``auto`` in non-template classes
    
  .. code-block:: cpp
 
    // Right
    class DataManager
    {
        ...
        typedef LinkedList<InternalType> DataList
    };
    ...
    DataManager::DataList& list = instance.getItems ();
    
    // Wrong
    auto& list = instance.getItems ();
    
    // Wrong
    LinkedList<DataManager::InternalType>& list = instance.getItems ();


======
Macros
======

* Only add a semicolon after a macro if the macro requires it; avoid unnecessary semicolons.

  .. code-block:: cpp

    // Right
    ASSERT (x == 0)
    
    // Wrong
    ASSERT (x == 0);


=======
Classes
=======

* Ensure constructors init class state.
* Do not perform work in constructors, use post-construction methods like ``initialize()`` or ``load()`` instead.

.. code-block:: cpp

    // Right
    class MyClass
    {
      public:
        MyClass ()
        {
          // initialize members only
        }

        void initialize ()
        {
          // perform setup or resource loading here
        }
    };

    // Wrong
    class MyClass
    {
      public:
        MyClass ()
        {
          // performs setup or resource loading here
        }
    };


=====
Other
=====

* Use int as general purpose type in for-loops
  
  * Assumed to be 32 bit on all supported platforms.
  * All container classes use this type.
  * Do not use int32, unsigned int, short, unless you have a very good reason.

* Do not pollute the global namespace, use namespaces per framework, product, etc.

* Use C++ constant definitions instead of macros for constant values - unless you have a very good reason. Motivation: Macros are evaluated by the preprocessor and therefore do not follow namespaces.

* Use C++ templates instead of macros.

* Avoid anonymous enums because they would show up unnamend in Doxygen documentation.

* Use assignment operator to initialize simple types:

  .. code-block:: cpp

    // Right
    int x = 3;

    // Wrong
    int x (3);
