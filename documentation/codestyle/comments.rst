########
Comments
########

=============
General Rules
=============

* Code and comments are written in American English.
* Keep comments short and specific.
* Do not leave your initials anywhere.
* Do not talk to yourself (*"I wonder why ..."*).
* Do not state the obvious or repeat the code:

  .. code-block:: cpp

    // Right
    // From testing: clients under load may take 600ms to respond.
    // Set timeout to 1s to avoid false positive connection issues.
    int timeout = 1000;

    // Wrong
    // Set timeout to 1 second.
    int timeout = 1000;

* Be precise:

  .. code-block:: cpp

    // Right
    void MyClass::foo ()
    {
      // Lazy init as fallback if parent component did not initialize prior.
      if(obj == nullptr)
        obj = NEW Object;
    }

    // Wrong
    void MyClass::foo ()
    {
      // May sometimes not be initialized.
      if(obj == nullptr)
        obj = NEW Object;
    }

* Do not express philosophical thoughts.
* Do not use comments to compensate missing social interaction between programmers.
* Do not add class method descriptions to file headers.
* Do not mention ticket numbers.
* Do not mention related code outside of current scope:

.. code-block:: cpp

    // Right
    void process ()
    {
      // Check if data is valid before processing
      if(!isValid (data))
        return;
    }

    // Wrong
    void process ()
    {
      // See validate() in utils.cpp for related logic
      if(!isValid (data))
        return;
    }


=======
Formats
=======

* Use regular C++ comments for implementation details:

  .. code-block:: cpp

    // Single-line example comment.
    int x = 0;

    /* Alternative single-line example comment. */
    int x = 0;

    // Multi-line example comment composed
    // of multiple rows of text.
    int x = 0;

    /* Alternative multi-line example comment
     composed of multiple rows of text. */
    int x = 0;


* Use Doxygen comments for interfaces:

  .. code-block:: cpp

    //********************************************************
    // IMyInterface
    /** Demo interface */
    //********************************************************

    interface IMyInterface
    {
      /** Do some work.
        \param state  some argument
        \return kResultOk for success, kResultFalse otherwise.
      */
      virtual tresult CCL_API doWork (bool state) = 0;
    };

* Use Doxygen comments for class declarations:

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

