#####################
GUI for dynamic views
#####################

Dynamic views can be utilized in a system that has dynamic memory allocation and a file system.

The Skin Package
================

For a data driven GUI, you can create a ``skin`` folder containing:

- a bitmaps folder and a bitmaps.json file (containing array of bitmap descriptions)

.. code-block:: json

 {"name": "button1", "file": "bitmaps/button1.bmp", "frames": 2}

- a fonts folder and a fonts.json file (containing array of font descriptions)

.. code-block:: json

 {"name": "MyFont12", "file": "fonts/MyFont12.fnt", "monochrome": true, "default": true, "fontnumber": 0, "size": 12}

- a views folder and a views.json file (containing array of view descriptions)

.. code-block:: json

 {"name": "MyView", "file": "views/myview.json"}

- a styles.json file describing the styles used by views, controls, etc

.. code-block:: json

 {
	"name": "ButtonStyle", 
	"forecolor": "#000000", "backcolor": "#FFFFFF", 
	"font": "MyFont12", "textcolor": "#000000", "textalign": "center"
 }

Initialization of GUI resources
===============================

Instantiate :cref:`Core::Portable::BitmapManager` and load Bitmaps.

Instantiate :cref:`Core::Portable::FontManager` and load Fonts.

Instantiate :cref:`Core::Portable::StyleMangager` and load styles.

Instantiate :cref:`Core::Portable::ViewBuilder` and load views.

.. code-block:: cpp

	FileName skinDirectory = getSkinDirectory ();
	static FolderPackage skinPackage (skinDirectory, true);

	BitmapManager::instance ().loadBitmaps (*skinPackage);
	FontManager::instance ().loadFonts (*skinPackage);
	StyleManager::instance ().loadStyles (*skinPackage);
	ViewBuilder::instance ().loadViews (*skinPackage);

Root View
---------

Create a :cref:`Core::Portable::RootView` representing the LCD hardware. It will be the destination for building and rendering dynamic views.

.. code-block:: cpp

	// RootView CTOR
	RootView (RectRef size = Rect (), BitmapPixelFormat pixelFormat = kBitmapRGBAlpha, RenderMode renderMode = kOffscreenMode) {}

	// create a RootView representing your LCD View
	RootView* rootView = NEW RootView (Rect (0, 0, lcdWidth, lcdHeight));

Root View Controller
--------------------

Typically the root component would inherit :cref:`Core::Portable::ComponentViewController`.
If any subcomponents need to customize ``createView ()`` or ``getObjectForView ()`` they can inherit ``ComponentViewController`` also.

Get a :cref:`Core::Portable::ViewController` pointer from the root component.

.. code-block:: cpp

	ViewController* rootViewController = core_cast<ViewController> (getRootComponent ());

The :cref:`Core::Portable::ViewBuilder` will call ``ViewController`` functions with your ``rootViewController`` and other controllers defined in the view json file.

Building The View
=================

Dynamic views are created by reading in json files.

A json view file represents a view that can be loaded dynamically.

.. code-block:: cpp

	ViewBuilder::instance ().buildView (*rootView, "MyView", rootViewController);

The rootView now contains a C++ representation of the json file "MyView", but has not been rendered to an offscreen bitmap yet.

Rendering and Displaying the View
=================================

In your application, periodically check to see if the display needs to be updated. There are 2 parts:

- render the view to an offscreen buffer
- send the pixel data to hardware

.. code-block:: cpp

	void MyApplication::idle ()
	{

		// check display update interval
		// ...
		
		// check if update is needed
		if(!rootView.isUpdateSuspended () && !rootView->getDirtyRegion ().isEmpty ())
		{
			// render to offscreen buffer
			rootView->redraw ();
			
			// access offscreen content
			const BitmapData* data = rootView.accessForRead (); 
			
			// finally send the BitMapData to the hardware screen using its API.
			// ...
		}
	}