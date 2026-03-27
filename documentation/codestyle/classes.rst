#######
Classes
#######

============
Declarations
============

Class **declarations** begin with asterisk separators and an optional comment in Doxygen style. Use Doxygen syntax as well to document other class aspects:

.. Not the exact format, long lines shortened to fit PDF format.

.. code-block:: cpp

  //************************************************************************
  // MyClass
  /** Specify here what your class is doing. */
  //************************************************************************

  class MyClass: public BaseClass
  {
  public:
      MyClass ();

      /** Long comment. */
      void testMethodOne ();
      void testMethodTwo (); ///< short comment
  };


Methods
=======

* Group methods in the class declaration:

  * Build functional groups.
  * Place new methods before overwritten methods.
  * Comment the block of overwritten methods with the immediate base class or interface name. This makes your new class easier to understand for your co-workers and it simplifies refactoring.

  .. code-block:: cpp

    class MyClass: public Object
    {
    public:

      // Client handling.
      void addClient (Client* client);
      void removeClient (Client* client);
      void clearClients ();

      // Object
      tbool CCL_API invokeMethod (CCL::Variant& returnValue, CCL::MessageRef msg);
    };


* Skip empty constructors and destructors.
* Do not repeat **virtual** keyword when overwriting base class methods.
* Do not repeat **virtual** keyword when overwriting base class destructor.


Members
=======

* Accessibility order (top to bottom):

    #. public
    #. protected
    #. private


* Declare static data members first:

  .. code-block:: cpp

    // Right
    class MyClass
    {
    protected:
      static int m;
      int x;
    };

    // Wrong
    class MyClass
    {
    protected:
      int x;
      static int m;
    };

* Declare data members before methods:

  .. code-block:: cpp

    // Right
    class MyClass
    {
    protected:
      int x;
      void foo ();
    };

    // Wrong
    class MyClass
    {
    protected:
      void foo ();
      int x;
    };


* Use framework PROPERTY macros to define getter and setter methods:

  .. code-block:: cpp

      // Right
      class MyClass
      {
      public:
        PROPERTY_BOOL (state, State)
      };

      // Wrong
      class MyClass
      {
      public:
        void setState (bool _state);
        bool getState () const;

      protected:
        bool state;
      };


Example
=======

With the member and method rules in mind, a class declaration may look like this:

.. code-block:: cpp

  class MyClass: public BaseClass
  {
  public:
      MyClass ();

      virtual void newPublicMethod ();

      // BaseClass
      void baseClassPublicMethod ();

  protected:
      static const int kMember1;

      int member1;

      void internalMethod ();

      // BaseClass
      void baseClassInternalMethod ();

  private:
      int member2;

      void privateMethod ();
  };


==============
Implementation
==============

Class **implementations** begin with asterisk separators:

.. Not the exact format, long lines shortened to fit PDF format.

.. code-block:: cpp

  //*************************************************************************
  // MyClass
  //*************************************************************************

  MyClass::MyClass ()
  {
  }

  ///////////////////////////////////////////////////////////////////////////

  void MyClass::testMethodOne ()
  {
  }

  ///////////////////////////////////////////////////////////////////////////

  void MyClass::testMethodTwo ()
  {
  }


Methods
=======

* Add forward slash separator lines between method definitions.
* Keep method definition order in sync with the declaration order in the header file.
* Prefer inline at the end of header files over inline in class scope. Typical exceptions are simple setter or return statements, as long as the class declarations remains readable:

  .. code-block:: cpp

    // Header file

    // Right
    class MyClass
    {
    public:
      inline void doWork ();
    };

    void MyClass::doWork ()
    {
      // long inline function...
    }

    // Wrong
    class MyClass
    {
    public:
      inline void doWork ()
      {
        // long inline function...
      }
    };


Members
=======

* Always initialize primitive types, omit initialization for data members with a ctor:

  .. code-block::

    class MyClass
    {
    public:
      MyClass ();
    protected:
      int state;
      String name;
    }

    // Right
    MyClass::MyClass ()
    : state (0)
    {}

    // Wrong
    MyClass::MyClass ()
    : state (0),
      name (String ())
    {}


* Keep initialization order in sync with member declaration order.
* Use class initializer lists in the constructor instead of C-style assignment:

  .. code-block:: cpp

    // Right
    MyClass::MyClass ()
    : member1 (0),
      member2 (0)
    {}

    // Wrong
    MyClass::MyClass ()
    {
        member1 = 0;
        member2 = 0;
    }


Inheritance
===========

* Inherit from implementation classes prior to interface classes:

  .. code-block:: cpp

    // Right
    class MyClass: public BaseClass,
                   public IMyInterface
    {};

    // Wrong
    class MyClass: public IMyInterface,
                   public BaseClass
    {};

