#############################
GUI for embedded applications
#############################

The Core Library contains an optional toolkit for building graphical user interfaces for embedded applications. Depending on the available system resources, you can choose between two approaches, static or dynamic views. The basic view model is the same for both. Conceptually, a view is a rectangular area on the screen that can display images or text, and can optionally have subviews. The coordinate system starts at (0,0) in the top-left corner. Subviews are positioned relative to their parent and clip their content and children. So far, so common. 

The view tree can be rendered into a bitmap. The following pixel formats are currently supported:

* 32 bit RGBA
* 16 bit RGB565
* Monochrome (1 bit per pixel)

The library provides an implementation for software rendering. The mapping to specific graphics hardware needs to be added externally. Support is limited to lines, gradients, bitmaps, and text, whereas text is implemented with bitmap fonts. No fancy 2D vector algorithms, sorry. See :cref:`Core::Portable::Graphics` class for more details.

Instead of writing C++ code, views can be defined in JSON skin files. This simplifies collaboration with designers who can either write JSON directly or use a GUI builder tool and edit forms for the UI graphically. Assets can either be .png or .bmp files.

Static views
============

Static views should be used when memory is limited and there's no heap and no file system available. You need at least one :cref:`Core::Portable::StaticRootView` and a :cref:`Core::Portable::RootValueController` to provide values from the application to the GUI in a unified way.

When working with the GUI builder, static views can be exported from JSON to C++ code and linked to the application, including all assets.

Please note that static views are read-only, i.e. they can be used to reflect the status of an embedded system on a small LCD but they do not support direct user interaction. Input from a front panel needs to be processed by the application and then fed into the value controller, which in turn will cause UI updates automatically with the bult-in dependency mechnism.

Dynamic views
=============

When more system resources are available including a heap and a file system, dynamic views can be used. The application has a tree of :cref:`Core::Portable::Component` and :cref:`Core::Portable::Parameter` objects that can be addressed in JSON skin files. Views are created on the fly via the :cref:`Core::Portable::ViewBuilder` singleton which is also responsible to load the skin definition and assets from a .zip file or folder on a flash drive. You need one :cref:`Core::Portable::RootView` per screen.

Dynamic views support multi-touch input with gesture recognition, as well as text input from a built-in on-screen keyboard. Controls that support user input are buttons, text boxes, list views, etc.

Resources
=========

Resources to define the look and feel of an application typically are bitmaps, fonts, and styles. On embedded platforms without a file system, all resources can be linked directly into the code.

Bitmap fonts are based on the popular BMP font binary format (.fnt files). They can be created with the free `Bitmap Font Generator <https://www.angelcode.com/products/bmfont/>`_ tool which converts a vector font (i.e. True Type or Open Type fonts) into a bitmap, plus glyph information. The resulting files can then be converted into C arrays and referenced by the :cref:`Core::Portable::BitmapInplaceFont` class, or loaded dynamically via the :cref:`Core::Portable::FontManager`.

A style can reference a font and it adds additional attributes like text alignment, background, and foreground colors, etc.

The following example demonstrates how resources are set up for a custom user interface when exported as code from the GUI Builder tool. It starts with two arrays containing the bitmap and glyph data of a font named Tuffy that was rendered at 16pt.

.. code-block:: cpp

  const unsigned char Tuffy16_image_code[75926] =
  {
  	0x42,0x4d,0x96,0x28, ...
  };

  const unsigned char Tuffy16_data_code[3901] =
  {
  	0x42,0x4d,0x46,0x03, ...
  };

The ``Resources`` class accumulates the resources for our application, in this case the :cref:`Core::Portable::BitmapInplaceFont` object representing the Tuffy font and a :cref:`Core::Portable::Style` object that contains text attributes. The class also derives from :cref:`Core::Portable::FontProvider` and implement its ``getFont`` function used by the library.

.. code-block:: cpp

  class Resources: public Core::Portable::FontProvider,
                   public Core::Portable::StaticSingleton<Resources>
  {
  public:
  	Resources ();
  
  	// Core::Portable::FontProvider
  	const Core::Portable::BitmapFont* getFont (Core::CStringPtr name) const;
  
  	Core::Portable::Style Tuffy16White;
  	Core::Portable::BitmapInplaceFont Tuffy16;
  }

The class implementation is straightforward. The constructor initializes the ``Tuffy16`` object using the raw data arrays defined previously. It then adds itself to the global :cref:`Core::Portable::FontManager` instance to make the Tuffy font available to the text rendering engine. The ``getFont`` function will be called by the rendering engine whenever it references a font of a certain name. The constructor also initializes the rendering attributes of the ``Tuffy16White`` style object. Note that you don't need to write this by hand. It's all generated from JSON skin files.

.. code-block::

  DEFINE_STATIC_SINGLETON (Resources)
  
  Resources::Resources ()
  : Tuffy16 ("Tuffy16", Tuffy16_image_code, sizeof(Tuffy16_image_code),
    Tuffy16_data_code, sizeof(Numbus16_data_code)),
  {
  	FontManager::instance ().setExternalFontProvider (this);
  	FontManager::instance ().setDefaultColorFont (&Tuffy16);
  
  	Tuffy16White.setFontName ("Tuffy16");
  	Tuffy16White.setTextAlign (Alignment::kLeft);
  	Tuffy16White.setTextColor (Color (0xFF, 0xFF, 0xFF));
  }
  
  const Core::Portable::BitmapFont* Resources::getFont (Core::CStringPtr name) const
  {
  	if(Tuffy16.getName () == name)
  		return &Tuffy16;
  
  	return nullptr;
  }

Creating a simple UI
====================

In this section we create a simple UI that presents two values to the user. We start with a ``SettingsView`` class that derives from :cref:`Core::Portable::StaticView` and add two :cref:`Core::Portable::StaticLabel` objects for static text and two :cref:`Core::Portable::StaticTextBox` objects that display the values. Again, this code is all generated from JSON skin files.

.. code-block:: cpp

  class SettingsView: public Core::Portable::StaticView
  {
  public:
  	SettingsView ();
  
  private:
  	Core::Portable::StaticLabel label1;
  	Core::Portable::StaticLabel label2;
  	Core::Portable::StaticTextBox value1;
  	Core::Portable::StaticTextBox value2;
  }

The class constructor sets the size of the view by calling ``setSize`` and then proceeds setting size, style, and the text of the labels. Each view is added to its parent via the ``addView`` function. Note that the names of :cref:`Core::Portable::StaticTextBox` objects equal the names of the value parameters managed by the :cref:`Core::Portable::RootValueController` class to connect UI elements with parameters.

.. code-block:: cpp

  SettingsView::SettingsView ()
  {
  	setSize (Rect (0, 0, 240, 135));
  
  	label1.setSize (Rect (0, 50, 50, 70));
  	label1.setStyle (&Resources::instance ().Tuffy16White);
  	label1.setTitle ("Value 1:");
  	addView (&label1);
  
  	label2.setSize (Rect (0, 70, 50, 90));
  	label2.setStyle (&Resources::instance ().Tuffy16White);
  	label2.setTitle ("Value 2:");
  	addView (&label2);
  
  	value1.setSize (Rect (60, 50, 240, 70));
  	value1.setName ("value1");
  	value1.setStyle (&Resources::instance ().Tuffy16White);
  	addView (&value1);
  
  	value2.setSize (Rect (60, 70, 240, 90));
  	value2.setName ("value2");
  	value2.setStyle (&Resources::instance ().Tuffy16White);
  	addView (&value2);
  }

To use static views for a user interface in an embedded project, declare a :cref:`Core::Portable::StaticRootView` object and a value controller object in the application class (see :ref:`Parameters <parameters>` for more information on creating a value controller class). 

.. code-block:: cpp

  class MyApp: public Core::Portable::Application
  {
  public:
  	renderUI ();

  	// Application
  	void startup () override;
  	void shutdown () override;
  
  private:
  	Bitmap renderBuffer;
  	SettingsView view;
  	Core::Portable::StaticRootView rootView;
  	MyController controller;
  };

Then connect the ``rootView`` object to the ``controller`` object by calling :cref:`Core::Portable::RootValueController::addObserver` and passing the ``rootView`` as the only argument. This will forward parameter changes to the :cref:`Core::Portable::StaticRootView` which would then update the UI accordingly. Next, the ``initOffscreen`` function is called to add a reference to :cref:`Core::Portable::Bitmap` object that represents the target buffer during the render operation. The ``SettingsView`` is then added to the root view and and instance of :cref:`Core::Portable::StaticViewConnector` ist used to establish the connection between UI elements and the value controller.

.. code-block:: cpp

  void MyApp::startup ()
  {
  	controller.addObserver (&rootView);

    rootView.initOffscreen (&renderBuffer, nullptr);
  	rootView.addView (view);
  	StaticViewConnector (controller).connect (view, true);
  }

Rendering the UI
================

Depending on your applications requirements you may want to regularly render the UI to the bitmap buffer in order to show it on the screen. The following code block uses the root view to check whether dirty region is empty, meaning the UI was updated and needs to be rendered. The function then calls :cref:`Core::Portable::RootViewBase::redraw` to initiate the render operation which will update the data in ``renderBuffer``. After the function completes, use the member functions of the :cref:`Core::Portable::Bitmap` to access the buffer's pixel data and send it to the screen.

.. code-block:: cpp

  void MyApp::renderUI ()
  {
  	if(!rootView.isUpdateSuspended () && !rootView.getDirtyRegion ().isEmpty ())
  	{
  		rootView.redraw ();

  		// Access the pixel data and send it to the screen.
		const BitmapData* bitmapData = rootView.accessForRead ();
  		...
  	}
  }