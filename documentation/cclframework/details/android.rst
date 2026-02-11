########################
Android Startup Sequence
########################


==========
Call stack
==========

.. code-block:: cpp

  static block 
    FrameworkActivity.loadNativeLibraries ("ccldemo") // Java ...

    System.loadLibrary (mainModuleID)
    System.loadLibrary ("cclgui")
    System.loadLibrary ("ccltext")
    System.loadLibrary ("cclsystem")

      JNI_OnLoad () // cclgui

  // main module ...
  
  FrameworkActivity.onCreate () // Java ...
    FrameworkView ()
      FrameworkView::constructNative () // cclgui ...

    FrameworkActivity::onCreateNative ()
      AndroidSystemInformation::setNativeActivity () // cclsystem ...
      AndroidSystemInformation::callAndroidMain ()
        CCLAndroidMain () // main module ...
          FrameworkInitializer::init ()
            FrameworkInitializer::callModuleMain ()
              CCLModuleMain ()

          ccl_main_gui_init ()
            ApplicationStartup::beforeInit ()
              UserInterface::startup () // cclgui ...
                AndroidUserInterface::startupPlatform ()

              ccl_app_init () // main module

          AndroidUserInterface::runEventLoop () // cclgui
            ApplicationStartup::onInit () // main module
            FrameworkView::createApplicationView () // cclgui
            Application::uiInitialized () // main module

            // main module ...


============
Call details
============

.. js:function:: FrameworkActivity.loadNativeLibraries ()

  :scope: Java
  :file: Application, example: CCLDemo.java

  Performed in application's Activity class.


.. js:function:: System.loadLibrary ()
  
  :scope: Java
  :file: ccl/platform/android/java/FrameworkActivity.java

  Adds native main module and framework modules that require JNI access.


.. cpp:function:: JNI_OnLoad ()
  
  :scope: cclgui
  :file: ccl/platform/android/jni_onload.cpp

  Called in all CCL modules that export JNI_OnLoad.


.. js:function:: FrameworkActivity.onCreate ()
  
  :scope: Java
  :file: ccl/platform/android/java/FrameworkActivity.java

  Prepares Window, creates main FrameworkView.


.. js:function:: FrameworkView ()
  
  :scope: Java
  :file: ccl/platform/android/java/FrameworkView.java

    
.. cpp:function:: FrameworkView::constructNative ()
  
  :scope: cclgui
  :file: ccl/platform/android/native/frameworkview.cpp

  Creates C++ counterpart of FrameworkView.


.. cpp:function:: FrameworkActivity::onCreateNative ()
  
  :scope: cclgui
  :file: ccl/platform/android/native/frameworkactivity.cpp

  Creates FrameworkActivity (C++ counterpart), creates FrameworkGraphicsFactory (C++).
   

.. cpp:function:: AndroidSystemInformation::setNativeActivity ()

  :scope: cclsystem
  :file: ccl/platform/android/system/system.android.cpp
  
  Initialize cclsystem module (android specific part).
  

.. cpp:function:: AndroidSystemInformation::callAndroidMain ()
  
  :scope: cclsystem
  :file: ccl/platform/android/system/system.android.cpp

  Finds CCLAndroidMain function (exported by main module) and calls it.


.. cpp:function:: CCLAndroidMain ()
  
  :scope: main module
  :file: ccl/platform/android/system/system.android.cpp


.. cpp:function:: FrameworkInitializer::init ()
  
  :scope: main module
  :file: ccl/main/cclinit.h


.. cpp:function:: FrameworkInitializer::callModuleMain ()
  
  :scope: main module
  :file: ccl/main/cclinit.h

  Called for all CCL modules (ccltext, cclsystem, ...).
  
 
.. cpp:function:: CCLModuleMain ()
  
  :scope: main module
  :file: ccl/main/cclmodmain.cpp	


.. cpp:function:: ccl_main_gui_init ()
  
  :scope: main module
  :file: ccl/main/cclmain.cpp	

        
.. cpp:function:: ApplicationStartup::beforeInit ()
  
  :scope: main module
  :file: ccl/main/cclmain.cpp	


.. cpp:function:: UserInterface::startup ()
  
  :scope: cclgui
  :file: ccl/gui/gui.cpp


.. cpp:function:: AndroidUserInterface::startupPlatform ()
  
  :scope: cclgui
  :file: ccl/platform/android/gui/gui.android.cpp	


.. cpp:function:: ccl_app_init ()
  
  :scope: main module
  :file: ccl/main/cclmain.cpp	


.. cpp:function::  AndroidUserInterface::runEventLoop ()
  
  :scope: cclgui
  :file: ccl/platform/android/gui/gui.android.cpp	

  No loop, returns after initialization.


.. cpp:function:: ApplicationStartup::onInit ()
  
  :scope: main module
  :file: ccl/main/cclmain.cpp	


.. cpp:function:: FrameworkView::createApplicationView ()
  
  :scope: cclgui
  :file: ccl/platform/android/native/frameworkview.cpp	


.. cpp:function:: Application::uiInitialized ()
  
  :scope: main module
  :file: ccl/app/application.cpp
            
