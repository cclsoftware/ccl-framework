#######
General
#######

=====================
C++ language standard
=====================

As of September 2021, CCL and derived code is using C++17.

.. note::
  
  Certain standard library features are not supported by embedded platforms. This primarily affects the Core Libray meaning that feature testing macros need to be used in such cases.

============
Line endings
============

As of December 2020, source files on all platforms use Unix line ending style (LF). Make sure to configure your IDE or text editor accordingly.


=============
Text encoding
=============

Source files may use US-ASCII only.

