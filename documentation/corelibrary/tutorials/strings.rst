#######
Strings
#######

The Core Library supports strings via the :cref:`Core::ConstString` and :cref:`Core::CStringBuffer` classes which can both be viewed as wrappers for plain C strings. These classes can be used in all types of applications, including firmware for resource constrained devices as they don't use dynamic memory to store character data.

ConstString
###########

The :cref:`Core::ConstString` class wraps a pointer to a zero-terminated character sequence, usually to a literal string or character buffer that is managed elsewhere. It provides common string operations like comparisions, number conversions, or substring and character checks. Instances of the :cref:`Core::ConstString` class are, as the name implies, immutable and the character data cannot be modified. The following code block demonstrates some operations of the class.

.. code-block:: cpp

  ConsString str ("Hello, World!");

  // Check if string is empty
  bool empty = str.isEmpty (); // false

  // Get string length
  int length = str.length (); // 13

  // Get position of character
  int index = str.index (' '); // 6

  // Check for prefix
  bool prefix = str.startsWith ("Hello");

  // Access characters
  char firstChar = str.firstChar (); // 'H'
  char lastChar = str.lastChar ();   // '!'
  char charAt = str.at (4);          // 'o'

  // Get plain character pointer
  CStringPtr ptr = str;

  // Convert number
  ConstString num ("2304 bytes");
  int64 count = 0;
  num.getIntValue (count)

  // Compare strings
  if(str == num)
  {
  	...
  }

CStringBuffer
#############

Similar to plain character arrays, the :cref:`Core::CStringBuffer` template class allows to declare a fixed-size buffer holding a zero-terminated character sequence. In addition to all the operations that :cref:`Core::ConstString` provides, :cref:`Core::CStringBuffer` also provides operations to modify the character data.

The following statement declares an instance of :cref:`Core::CStringBuffer` that holds up to 40 characters (including the zero-terminator) and initializes it.

.. code-block:: cpp

  CStringBuffer<40> str ("   Hello, World!   ");

  // Remove leading and trailing whitespace
  str.trimWhitespace ();

  // Append strings
  str.append (' ');
  str.appendFormat ("The sky is %s.", "blue");
  str.append ('\n');
  str.append (42);
  str.append (" kByte");

  // Get substring
  CString16 result;
  str.subString (result, 7, 5);

  // Remove, insert, replace substrings
  str.remove (24, 4);
  str.insert (24, "orange");
  str.replace (24, 6, "green");
