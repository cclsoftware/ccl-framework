##################
Application Phases
##################

Phases for desktop applications (Windows, similar for macOS).


=======
Startup
=======

.. code-block:: cpp
  :caption: Call Stack

  WinMain ()
    ccl_main_gui ()
      ccl_main_gui_init ()
         ApplicationStartup::beforeInit ()
           UserInterface::startup ()
             NativeGraphicsEngine::startup ()
             WindowsUserInterface::startupPlatform ()

           ccl_app_init ()
           Application::beforeInitialize ()

      WindowsUserInterface::runEventLoop ()
         ApplicationStartup::onInit ()
          Kernel::initialize ()
            Application::initialize ()
              Application::startup ()

          Application::processCommandLine () 

        Application::uiInitialized ()


**Call Details**

.. cpp:function:: WinMain ()
  
  :file: ccl/platform/win/winmain.cpp


.. cpp:function:: ccl_main_gui ()

  :file: ccl/main/cclmain.cpp


.. cpp:function:: ccl_main_gui_init ()

  :file: ccl/main/cclmain.cpp


.. cpp:function:: ApplicationStartup::beforeInit ()

  :file: ccl/main/cclmain.cpp


.. cpp:function:: UserInterface::startup ()
  
  :file: ccl/gui/gui.cpp


.. cpp:function:: NativeGraphicsEngine::startup ()

  :file: ccl/gui/graphics/nativegraphics.cpp

  Overridden by platform engine.


.. cpp:function:: WindowsUserInterface::startupPlatform ()

  :file: ccl/platform/win/gui/gui.win.cpp
         

.. cpp:function:: ccl_app_init ()
  
  Implemented in application code (create Application object).
        

.. cpp:function:: Application::beforeInitialize ()

  :file: ccl/app/application.cpp

  Load settings, strings, command table, ...
        

.. cpp:function:: WindowsUserInterface::runEventLoop ()

  :file: ccl/platform/win/gui/gui.win.cpp
    

.. cpp:function:: ApplicationStartup::onInit ()
  
  :file: ccl/main/cclmain.cpp


.. cpp:function:: Kernel::initialize ()
  
  :file: ccl/base/kernel.cpp

  Also see :cref:`CCL::Kernel::initialize`.


.. cpp:function:: Application::initialize ()

  :file: ccl/app/application.cpp

  Typically calls createWindow (), System::GetWindowManager ().createApplicationWindow ()  or initWindowlessApplication (), calls GUI.setApplication() in cclgui.


.. cpp:function:: Application::processCommandLine () 

  :file: ccl/app/application.cpp

  Overridden in derived application class.


.. cpp:function:: Application::uiInitialized ()

  :file: ccl/app/application.cpp

  Overridden in derived application class.
        

========
Shutdown
========

.. code-block:: cpp
  :caption: Call Stack

  ccl_main_gui_exit ()
    ApplicationStartup::cleanup ()
      Kernel::terminate ()
      PlugInManager::terminate ()
      ThreadPool::terminate ()

    ApplicationStartup::checkRestart ()


**Call Details**


.. cpp:function:: ccl_main_gui_exit ()
  
  :file: ccl/main/cclmain.cpp


.. cpp:function:: ApplicationStartup::cleanup ()

  :file: ccl/main/cclmain.cpp


.. cpp:function:: Kernel::terminate ()

  :file: ccl/base/kernel.cpp

  Also see :cref:`CCL::Kernel::terminate`


.. cpp:function:: PlugInManager::terminate ()

  :file: ccl/system/plugins/plugmanager.cpp
     

.. cpp:function:: ThreadPool::terminate ()

  :file: ccl/system/threading/threadpool.cpp


.. cpp:function:: ApplicationStartup::checkRestart ()

  :file: ccl/main/cclmain.cpp

