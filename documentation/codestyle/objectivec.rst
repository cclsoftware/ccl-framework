###########
Objective-C
###########

When code needs to be written in Objective-C, we use source files with an .mm extension, which denotes the Objective-C++ file type to the compiler. This allows to mix Objective-C and C++ code in the same file.
All the C++ coding conventions listed above apply to Objective-C accordingly, but there are some additional rules for special syntax elements.
In general, demo code snippets from Apple's documentation need to be reformatted to match our coding conventions.

=======
Spacing
=======

Declarations and definitions of class and instance methods require a space before the return type (in brackets) and each selector element, but nowhere else.

.. code-block:: cpp
  :caption: Example: Declaring Instance Methods

  // Right
  - (void)observeValueForKeyPath:(NSString*)keyPath ofObject:(id)object change:(NSDictionary<NSString*,id>*)change context:(void*)context;

  // Wrong
  -(void) observeValueForKeyPath: (NSString *)keyPath ofObject: (id)object change: (NSDictionary<NSString *,id> *)change context: (void *)context;

When nesting method calls, no additional spaces are inserted.

.. code-block:: cpp
  :caption: Example: Calling Instance Methods

  // Right
  NSURL* tempFileURL = [[[NSURL fileURLWithPath:NSTemporaryDirectory () isDirectory:YES] URLByAppendingPathComponent:uniqueString] URLByAppendingPathExtension:@"mov"];

  // Wrong
  NSURL* tempFileURL = [ [ [NSURL fileURLWithPath:NSTemporaryDirectory () isDirectory:YES ] URLByAppendingPathComponent:uniqueString ] URLByAppendingPathExtension:@"mov" ];

===========
Line Breaks
===========

We do not insert line breaks to create a table like appearance for decalarations, simply use one long line.

.. code-block:: cpp
  :caption: Example: Instance Methods

  // Wrong
  - (void)observeValueForKeyPath: (NSString*)keyPath 
                        ofObject: (id)object 
                          change: (NSDictionary<NSString*,id>*)change 
                         context: (void*)context;


For block parameters use a new line for the opening bracket.

.. code-block:: cpp
  :caption: Example: Passing a Block Parameter Value

  // Right
  [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted)
  {
      requestPending = false;
  }];

  // Wrong
  [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted){
      requestPending = false;
  }
  ];


========
Literals
========

When available, use the appropriate literals for Objetive-C types. 

.. code-block:: cpp
  :caption: Example: Some Objective-C Literals
  
  // Right
  NSString* message = nil;
  BOOL success = YES;
  
  // Wrong
  NSString* message = 0;
  BOOL success = false;

