#######################
Spacing and Indentation
#######################

===========
Basic Rules
===========

* Use TAB based indentation with TAB size of 4. Make sure your IDE or text editor is configured accordingly.


==================
Control Statements
==================

* Do not insert whitespace after keywords:

  .. code-block:: cpp

    // Right
    while(true)
    for(int i = 0; i < count; i++)

    // Wrong
    while (true)
    for (int i=0;i<count;i++)


  .. code-block:: cpp

    // Right
    if(a == b)
    switch(a)
    {
    case kCase1 :
      break;
    }

    // Wrong
    if (a==b)
    switch (a)
    {
      case kCase1 :
      break;
    }

* Linebreak before opening brackets of a conditional expression:

  .. code-block:: cpp

    // Right
    if(expression)
    {
    }

    // Wrong
    if(expression) {
    }


=========
Functions
=========

* Insert whitespace before opening parenthesis of function arguments:

  .. code-block:: cpp

    // Right
    void saySomething (const char* what);
    saySomething ("Hi!");

    // Wrong
    void saySomething(const char*what);
    saySomething( "Hi!" );


* Linebreak before opening brackets of functions:

  .. code-block:: cpp

    // Right
    void myFunction ()
    {
    }

    // Wrong
    void myFunction () {
    }



======
Arrays
======

* Do not add whitespace before square brackets:

  .. code-block:: cpp

    // Right
    char string[256];
    string[0] = 'a';
    string[i + 1] = 'x';

    // Wrong
    char string [256];
    string [0]='a';
    string[i+1] = 'x';


=======
Classes
=======

* Linebreak before opening brackets of class declarations:

  .. code-block:: cpp

    // Right
    class MyClass
    {
    };

    // Wrong
    class MyClass {
    };


==========
Namespaces
==========

* Forward declarations: do not linebreak namespace opening bracket, do not indent inside the namespace, put closing } on same line as last forward declaration:
  
  .. code-block:: cpp

    // Right
    namespace MyNamespace {
    class MyClass; 
    interface IMyInterface; }

    // Wrong
    namespace MyNamespace 
    {
      class MyClass; 
      interface IMyInterface;
    }


* Indent namespaces that are used like classes:

  .. code-block:: cpp

    // Right
    namespace MyNamespace
    {
      static const int kConstant;
    };

    // Wrong
    namespace MyNamespace {
      static const int kConstant;
    };


======
Macros
======

* Iteration macros for container classes require spacing and indentation:

  .. code-block:: cpp

    // Right
    VectorForEach (data, Type, value)
      print (value);
    EndFor

    // Wrong
    VectorForEach(data,Type,value)
    print (value);
    EndFor


=====
Types
=====

* For pointer and reference types the asterisk and ampersand are part of the type:

  .. code-block:: cpp

    // Right
    Type* var
    Type& var

    // Wrong
    Type *var
    Type & var


=========
Templates
=========

* Do not insert whitespace before template type opening angle bracket:

  .. code-block:: cpp

    // Right
    MyClass<T> t;

    // Wrong
    MyClass <T> t;


========
Comments
========

* In comments, insert a space character before the text:

  .. code-block:: cpp

    // Right
    // A comment
    /* Another comment */

    // Wrong
    //Comment
    /*Another comment*/

=======================
Preprocessor Directives
=======================

* Align preprocessor directives with the code

  .. code-block:: cpp
  
	// Right
	void myFunction ()
	{
		if(condition)
		{
			#if PREPROCESSOR_DEFINE
			callFunction ();
			#else
			callSomethingElse ();
			#endif
		}
	}

	// Wrong
	void myFunction ()
	{
		if(condition)
		{
	#if PREPROCESSOR_DEFINE
			callFunction ();
	#else
			callSomethingElse ();
	#endif
		}
	}
	