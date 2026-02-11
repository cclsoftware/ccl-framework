#######
Storage
#######

Attributes
##########

In the Core Library, the primary way to store and recall data is via the :cref:`Core::Portable::Attributes` class. This class implements a list of key-value pairs. The keys are strings, while the values can be ``int``, ``int64``, ``float``, and string types. In addition, the :cref:`Core::Portable::Attributes` class supports nesting attributes, thus, forming a tree-like structure. The Core Library also supports serializing and deserializing attributes into JSON (text) and UBJSON (binary) formats. This simplifies the implementation of settings files or exchanging data via network protocols. The following example demonstrates how to use the :cref:`Core::Portable::Attributes` class to store application data in a file.

.. code-block:: cpp

  #include "core/portable/corestorage.h"
  #include "core/portable/corepersistence.h"
  
  using namespace Core;
  using namespace Portable;
  
  void saveSettings ()
  {
  	AttributeAllocator& allocator = AttributeAllocator::getDefault ();
  
  	Attributes attributes (allocator);
  	attributes.set ("greeting", "Hello, World!");
  	attributes.set ("the answer to life", 42);
  	attributes.set ("PI", 3.14f);
  
  	Attributes recentFileList (allocator);
  	recentFileList.set ("1", "Lyrics.txt");
  	recentFileList.set ("2", "Cover.png");
  	recentFileList.set ("3", "Bach.mid");
  	recentFileList.set ("4", "Drums.wav");
  
  	attributes.set ("recent file list", recentFileList);
  
  	ArchiveUtils::saveToFile ("Settings.json", attributes);
  }

Objects of the :cref:`Core::Portable::Attributes` class can use the allocator supplied in the class constructor to allocate and free the memory used for the attribute data. Platforms with a heap can use the default allocator returned by :cref:`Core::Portable::AttributeAllocaor::getDefault` whereas embedded applications may prefer providing a custom allocator for their needs.