#################
Application Model
#################

The Core Library supports developers by providing a lightweight application model through the :cref:`Core::Portable::Application` class. This class maintains the application state, including initialization, runnning, and termination. It is called directly by the ``main`` function and provides the basic skeleton of the application. Each application should have only one :cref:`Core::Portable::Application` object.

Using the Application class
###########################

To create the application object derive an application class from :cref:`Core::Portable::Application` and implement the ``startup`` and ``shutdown`` methods. 

.. code-block:: cpp

  #include "core/portable/coreapplication.h"
  
  using namespace Core;
  using namespace Portable;
  
  class MyApp: public Application
  {
  public:
  	// Application
  	void startup () override;
  	void shutdown () override;
  };

.. code-block:: cpp

  void MyApp::startup ()
  {
  	// Put the application initialization in here
  }
  
  void MyApp::shutdown ()
  {
  	// Clean up and free resources
  }

Since the Core Library runs on a variety of different operating systems and platforms, there's no general recipe for the ``main`` function. In many cases, however, the application's main entry point just obtains a pointer to the application object and calls its ``startup`` and ``shutdown`` functions. It may also need to perform other tasks, for instance, it may need to run an application loop.

.. code-block:: cpp

  int main ()
  {
  	MyApp* instance = MyApp::instance ();
  
  	instance.startup ();
  
  	while(running)
  	{
  		// Do work
  	}
  
  	instance.shutdown ();
  	return 0;
  }