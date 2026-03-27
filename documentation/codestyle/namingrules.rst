######
Naming
######


* Use `CamelCase <https://en.wikipedia.org/wiki/Camel_case>`_ in general:

  .. code-block:: cpp

    class MyClass
    {
    public:
      void doWork ();

    protected:
      bool enabled;
      int currentState;
    };


* Data types (classes, enumerations) start with an uppercase letter.
  
  * Exception: simple data types like int32, float64, etc.;

* Variable names start with a lowercase letter.

  .. code-block:: cpp

    // Right
    bool enabled;

    // Wrong
    bool Enabled;

  * Exception: The upper case "This" is a viable variable name.

* Constants and enumerators start with the letter *k*:

  .. code-block:: cpp

    static const int kSomeConstant = 123;

* Do not use any kind of prefixing for data members (`Hungarian notation <https://en.wikipedia.org/wiki/Hungarian_notation>`_, C-style prefixes, etc...), except k for constants.

  .. code-block:: cpp

    // Right
    Object* object;

    // Wrong
    Object* pObject;


* Avoid *"is"* for bool data members as it's mostly used for methods:

  .. code-block:: cpp

    // Right
    bool enabled;

    // Wrong
    bool isEnabled;


* Do not use class name prefixes (*C*, *P*, *F*, etc.)
* Use *I* to prefix interfaces
* Choose natural identifiers:
  
  * Avoid abbreviations.
  * Acronyms are allowed.
  * Function names usually start with a verb like *"get"*, *"set"*, etc.
  * Single character variable names like *i*, *a*, *b*, *c*, are allowed.

  .. code-block:: cpp

    // Right
    getValue ()
    value

    // Wrong
    getVal ()
    val


* Acronyms:

  * Use uppercase in type.
  * Use uppercase in method names.
  * Use lowercase for members and variables.

  .. code-block:: cpp

    // Right
    NoteID noteId;
    NoteID getNoteID () const;

    // Wrong
    NoteId noteID;
    NoteId getNoteId () const;

    // Right
    URL url;
    URL getUrl () const;

    // Wrong
    Url url;
    Url getURL () const;

