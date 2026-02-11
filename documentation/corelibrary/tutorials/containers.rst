##########
Containers
##########

The Core Library offers a variety of container classes for common uses cases. The following sections give examples to demonstrate their use.

Array
#####

The :cref:`Core::Array` class wraps a block of memory containing elements of a particular type. The class can manage the underlying memory of the array itself or just point to it. There are several functions available to manage the memory and access it.

.. code-block:: cpp

  float data[4] = { 0.5f, 1.5f, 2,71f, 3.14f };
  
  // Create an empty array
  Array<float> a;
  
  // Create an array using existing data. The data is copied to heap memory.
  Array<float> b (data, sizeof(data));
  
  // Create an array using existing data. The buffer does
  // not own the memory, it just wraps pointer and size.
  Array<float> c (data, sizeof(data), false);
  
  // Get a pointer to the first element and the number of elements
  float* address = b.getAddress ();
  uint32 size = b.getSize ();
  
  // Fill the memory block with zeros
  b.zeroFill ();
  
  // Access the array
  c[2] = 3.2f;
  float f = c[4];

BitSet
######

A bitset manages a sequence of bits. Bits can be set, cleared, or toggled. There are also functions to find or count bits. The :cref:`Core::BitSet` class stores bit data on the heap and can grow dynamically if needed.

.. code-block:: cpp

  // Create a bitset that holds 64 bits.
  BitSet bitset (64);
  
  // Set and clear a particular bit
  bitset.setBit (17, true);
  bitset.setBit (29, true);
  
  // Obtain the value of a bit
  bool value = bitset.getBit (12);
  
  // Flip a bit
  bitset.toggleBit (17);
  
  // Find the first bit set
  int index = bitset.findFirst (true);
  
  // Determine the number of set bits
  int count = bitset.countBits (true);

Buffer
######

A buffer encapsulates a block of raw memory. It usually owns the memory determining its lifetime but can also point to memory that is managed elsewhere. To construct a new buffer that owns the memory call the :cref:`Core::Buffer` constructor optionally passing a pointer and the size of an existing block of memory. The ``Buffer`` class provides a variety of different functions to manage the memory and access it.

.. code-block:: cpp

  uint8 data[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  char hello[7] = "Hello!"; 
  
  // Create an empty buffer
  Buffer a;
  
  // Create a buffer using existing data. The data is copied to heap memory.
  Buffer b (data, sizeof(data));
  
  // Create a buffer using existing data. The buffer does
  // not own the memory, it just wraps pointer and size.
  Buffer c (data, sizeof(data), false);
  
  // Check if the buffer is empty
  bool empty = b.isNull ();
  
  // Get a pointer to the memory block and its size
  void* address = b.getAddress ();
  uint32 size = b.getSize ();
  
  // Fill the memory block with zeros
  b.zeroFill ();
  
  // Fill the memory block using a value
  b.byteFill (0xFF);
  
  // Copy memory
  b.copyFrom (hello, sizeof(hello));
  
  // Obtain a pointer of a certain type to the memory block
  const uint8* p = c.as<uint8> ();

Deque
#####

A double-ended queue (abbreviated to deque) is a data structure that allows to add or remove elements from either the front or back. It is implemented in :cref:`Core::Deque`.

.. code-block:: cpp

  Deque<int> deque;
  
  // Add elements
  deque.addFront (1);
  deque.addBack (2);
  deque.addFront (3);
  deque.addBack (4);
  deque.addFront (5);
  deque.addBack (6);
  
  // Remove elements
  int a = deque.popFront (); // returns 5
  int b = deque.popBack ();  // returns 6
  
  // Peek at first/last element
  a = deque.peekFront (); // returns 3
  b = deque.peekBack ();  // return 4

FixedDeque
##########

Similar to :cref:`Core::Deque`, :cref:`Core::FixedDeque` implements a double-ended queue that uses a user-provided memory block to store elements.

.. code-block:: cpp

  FixedDeque<int> deque;
  
  // Initialize the deque
  int buffer[16] = {};
  deque.initialize (buffer, sizeof(buffer));
  
  // Add elements
  deque.addFront (1);
  deque.addBack (2);
  deque.addFront (3);
  deque.addBack (4);
  deque.addFront (5);
  deque.addBack (6);
  
  // Remove elements
  int a = deque.popFront (); // returns 5
  int b = deque.popBack ();  // returns 6
  
  // Peek at first/last element
  a = deque.peekFront (); // returns 3
  b = deque.peekBack ();  // return 4

LinkedList
##########

The :cref:`Core::LinkedList` class implements a doubly linked list consisting of nodes that are not stored sequentially in memory.

.. code-block:: cpp

  LinkedList<int> list;

  // Add elements
  list.append (3);
  list.append (17);
  list.append (33);
  list.append (9);
  list.append (14);
  list.append (99);
  list.append (5);

  // Remove elements
  list.remove (9);
  list.removeAt (2);  // removes number 33
  list removeLast (); // remove number 5

  // Check if the list is empty
  bool empty = list.isEmpty ();

  // Check if element exists
  bool exists = list.contains (33);

  // Lookup elements
  int a = list.at (2);  // returns 14
  a = list.getFirst (); // returns 3
  a = list.getLast ();  // returns 99

  // Iterate elements
  for(int i : list)
  	printf ("%i\n", i);

IntrusiveLinkedList
###################

The :cref:`Core::IntrusiveLinkedList` class requires elements to provide storage for the links used to build the list. The elements added to the list must inherit from :cref:`Core::IntrusiveLink` and their lifetime is managed by the caller.

.. code-block:: cpp

  struct DataItem: public IntrusiveLink<DataItem>
  {
  	int data;
    DataItem (int data) : data (data) {}
  }
  
  IntrusiveLinkedList<DataItem> list;
  
  // Add elements
  list.append (NEW DataItem (3));
  list.append (NEW DataItem (17));
  list.append (NEW DataItem (33));
  list.append (NEW DataItem (9));
  list.append (NEW DataItem (14));
  list.append (NEW DataItem (99));
  list.append (NEW DataItem (5));
  
  // Lookup elements
  DataItem* item = itemlist.at (4);
  
  // Insert elements
  list.insertBefore (item, NEW DataItem (7));
  
  // Remove and free all remaining elements
  while((item = list.removeLast ()) != nullptr)
  	delete item;

ConstVector
###########

:cref:`Core::ConstVector` implements an immutable sequence of elements using external memory. It is functionality is extended by other classes, for instance, :cref:`Core::MutableVector`.

.. code-block:: cpp

  const int data[8] = { 3, 17, 33, 9, 14, 99, 5, 7 };

  ConstVector<int> vector (data, ARRAY_COUNT (data));

  // Check if the vector is empty
  bool empty = vector.isEmpty ();

  // Lookup elements
  int a = vector.at (4);   // returns 14
  int b = vector.first (); // returns 3
  int c = vector.last ();  // returns 7

  // Find elements or check if these exist
  int i = vector.index (99); // returns 5
  bool exists = vector.contains (17);

  // Iterate elements
  for(int i : list)
  	printf ("%i\n", i);

There are two methods available to iterate over the elements of a vector. The previous examples uses the for-range based loop to print the values of the vector that was introduced with C++11. This is the recommended method, but if your toolchain doesn't support C++11 or your code depends on a counter variable you can use the :cref:`VectorForEach`, :cref:`VectorForEachFast`, :cref:`VectorForEachReverse`, and :cref:`EndFor` macros. The following example prints the values of the vector using the :cref:`VectorForEach` macro.

.. code-block:: cpp

  VectorForEach (vector, int, value)
  	printf ("%i\n", value);
  EndFor

MutableVector
#############

:cref:`Core::MutableVector` derives from :cref:`Core::ConstVector` and adds functions to modify the data sequence. The memory for the sequence of elements is provided externally and it forms the base for :cref:`Core::Vector` and :cref:`Core::FixedSizeVector`. ``MutableVector`` cannot be used by itself, but instead requires the use of a derived class. See ``Vector`` or ``FixedSizeVector`` for example code.

Vector
######

:cref:`Core::Vector` wraps a mutable sequence of elements and manages the memory used by the sequence. It also grows the memory if needed. Since it inherits from :cref:`MutableVector` is provides all of its functions.

.. code-block:: cpp

  Vector<int> vector;

  // Add elements
  vector.add (3);
  vector.add (17);
  vector.add (33);
  vector.add (9);
  vector.add (14);
  vector.add (99);
  vector.add (5);

  // Add an item only if the vector doesn't contain it already
  vector.addOnce (7); // will be added
  vector.addOnce (9); // will not be added

  // Remove elements
  vector.remove (9);
  vector.removeAt (2);   // removes number 33
  vector.removeFirst (); // removes number 3
  vector.removeLast();   // removes number 5

  // Sort and reverse elements;
  vector.sort ();
  vector.reverse ();

  // Grow the vector for a specified number of elements.
  // This can be useful if you know the number of elements to expect in advance.
  vector.setCount (16);

  // Set all elements to zero
  vector.zeroFill ();

FixedSizeVector
###############

:cref:`Core::FixedSizeVector` wraps mutable sequence of elements that cannot grow beyond the initial size. With a few exceptions it has the same interface as :cref:`Core::Vector`. Hence, example code will be omitted.
