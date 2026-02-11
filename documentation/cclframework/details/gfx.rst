.. include:: extlinks.rst

#######################
Graphics Implementation
#######################

.. list-table::
   :widths: 20 20 20 20 20
   :header-rows: 1

   * - Platform
     - API
     - GPU Support
     - Printing
     - PDF Export
   
   * - Windows
     - `Direct 2D`_ + `Direct Composition`_
     - Primitives (+), Composition (+)
     - Yes, implemented
     - Yes, implemented

   * - macOS
     - `Skia`_ (with `Metal`_ backend) + `Core Animation`_, Legacy: `Quartz`_
     - Primitives (+), Composition (+)
     - Yes, implemented
     - Yes, implemented

   * - Linux
     - `Skia`_ (with `Vulkan`_ backend) + `Wayland`_
     - Primitives (+), Composition (+)
     - Yes, implemented
     - Yes, implemented

   * - iOS
     - `Quartz`_ + `Core Animation`_
     - Primitives (-), Composition (+)
     - Yes, implemented
     - Yes, implemented

   * - Android
     - `Java Canvas API`_ (`Skia`_ via JNI), `OpenGL ES`_ (experimental)
     - Primitives (-), Composition (-)
     - Yes, implemented
     - Yes, implemented

   * - Embedded
     - Software rendering (RGB565, RGBA, Monochrome)
     - Primitives (-), Composition (-)
     - Not required
     - Not required
