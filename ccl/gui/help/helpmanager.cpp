//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/gui/help/helpmanager.cpp
// Description : Help Manager
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "ccl/gui/help/helpmanager.h"
#include "ccl/gui/help/helpreferences.h"
#include "ccl/gui/help/htmlviewer.h"
#include "ccl/gui/help/helptutorial.h"
#include "ccl/gui/help/tutorialviewer.h"
#include "ccl/gui/help/quickhelp.h"
#include "ccl/gui/help/viewhighlights.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/popup/menu.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/layout/workspaceframes.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/message.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/text/language.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("HelpManager")
	XSTRING (HelpFileNotFound, "Help file not found at:\n")
	XSTRING (PDFViewerNotInstalled, "No compatible PDF Viewer installed!")
END_XSTRINGS

static const String kHelpFolder (CCLSTR ("help"));
static const String kHelpIndexFile (CCLSTR ("helpindex.xml"));
static const String kQuickHelpFile (CCLSTR ("quickhelp.xml"));
static const String kTutorialCollectionFile (CCLSTR ("tutorials.xml"));

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IHelpManager& CCL_API System::CCL_ISOLATED (GetHelpManager) ()
{
	return HelpManager::instance ();
}

//************************************************************************************************
// HelpManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (HelpManager, Object)
DEFINE_SINGLETON (HelpManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpManager::HelpManager ()
: pdfViewer (DocumentViewer::createPDFViewer ()),
  htmlViewer (NEW HtmlDocumentViewer),
  defaultViewer (DocumentViewer::createSystemViewer ()),
  referenceList (nullptr),
  activeTutorialViewer (nullptr),
  currentInfo (nullptr),
  quickHelp (*NEW QuickHelp),
  viewHighlights (*NEW ViewHighlights)
{
	catalogs.objectCleanup (true);
	tutorials.objectCleanup (true);

	Desktop.addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpManager::~HelpManager ()
{
	ASSERT (infoViewers.isEmpty ())
	ASSERT (!activeTutorialViewer)
	
	cancelSignals ();
		
	setCurrentInfo (nullptr);

	safe_release (htmlViewer);
	safe_release (pdfViewer);
	safe_release (defaultViewer);

	safe_release (referenceList);

	quickHelp.release ();
	viewHighlights.release ();

	Desktop.removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::setHelpLocation (UrlRef path)
{
	helpFolder = NEW Url (path);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::addHelpCatalog (UrlRef path, StringID category)
{
	Url indexPath (path);
	indexPath.descend (kHelpIndexFile);
	AutoPtr<HelpCatalog> catalog = NEW HelpCatalog;
	if(!catalog->loadFromFile (indexPath))
		return kResultFailed;

	catalog->setPath (AutoPtr<Url> (NEW Url (path)));
	Url basePath (path);
	Url quickHelpPath;
	makeHelpPath (quickHelpPath, kQuickHelpFile, &basePath);
	if(System::GetFileSystem ().fileExists (quickHelpPath))
		catalog->setQuickHelp (true);

	catalog->setCategory (category);

	if(catalogs.contains (*catalog))
		return kResultAlreadyExists;

	catalogs.add (catalog.detach ());

	safe_release (referenceList); // references need to be rebuild
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API HelpManager::newCatalogIterator () const
{
	return catalogs.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpManager::makeHelpPath (Url& path, StringRef fileName, const Url* externalPath)
{
	String language (System::GetLocaleManager ().getLanguage ());

	makeHelpPath (path, fileName, language, externalPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpManager::makeHelpPath (Url& path, StringRef fileName, StringRef language, const Url* externalPath)
{
	if(externalPath)
		path = *externalPath;
	else if(helpFolder)
		path = *helpFolder;
	else
	{
		System::GetSystem ().getLocation (path, System::kAppSupportFolder);
		path.descend (kHelpFolder, Url::kFolder);
	}

	if(path.isNativePath ())
	{
	 	path.descend (language, Url::kFolder);
		path.descend (fileName);
	}
	else
		path.descend (language, Url::kFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpManager::makeExistingHelpPath (Url& path, StringRef fileName, const Url* externalPath)
{
	makeHelpPath (path, fileName, externalPath);

	// fallback to English
	if(System::GetLocaleManager ().getLanguage () != LanguageCode::English)
	{
		if(path.isNativePath () && !System::GetFileSystem ().fileExists (path))
			makeHelpPath (path, fileName, String (LanguageCode::English), externalPath);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult HelpManager::deferTutorialWindowCall (IMessage* message)
{
	if(pendingTutorialWindowCall)
	{
		message->release ();
		return kResultFailed;
	}

	pendingTutorialWindowCall = message;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpCatalog& HelpManager::getReferences ()
{
#if DEBUG
	safe_release (referenceList);
#endif

	if(referenceList == nullptr)
	{
		Url path;
		makeHelpPath (path, kHelpIndexFile);

		if(path.isNativePath () && !System::GetFileSystem ().fileExists (path))
		{
			// try same for all languages
			path.ascend ();
			path.ascend ();
			path.descend (kHelpIndexFile);
		}

		referenceList = NEW HelpCatalog;
		bool loaded = referenceList->loadFromFile (path);
		SOFT_ASSERT (loaded == true, "Help references not loaded!\n")

		// add additional catalogs
		Vector<HelpCatalog*> primaryCatalogs;
		ForEach (catalogs, HelpCatalog, catalog)
			if(catalog->isPrimary ())
				primaryCatalogs.add (catalog);
			else
			{
				if(catalog->isQuickHelp ())
				{
					Url quickHelpPath;
					makeHelpPath (quickHelpPath, kQuickHelpFile, catalog->getPath ());
					quickHelp.loadMadcapFile (quickHelpPath);
				}
				referenceList->addShared (*catalog);
			}
		EndFor

		// check which primary catalog fits best
		auto findPrimaryCatalog = [&] ()
		{
			HelpCatalog* best = nullptr;
			if(!primaryCatalogs.isEmpty  ())
			{
				if(primaryCatalogs.count () == 1)
					best = primaryCatalogs[0];
				else
				{
					// try to find catalog for current language
					String English (LanguageCode::English);
					String userLanguage (System::GetLocaleManager ().getLanguage ());
					VectorForEach (primaryCatalogs, HelpCatalog*, c)
						String contentLanguage (c->getContentLanguage ());
						if(contentLanguage.isEmpty ()) // assume English
							contentLanguage << English;

						if(contentLanguage.contains (userLanguage))
						{
							best = c;
							break;
						}
					EndFor
					
					if(best == nullptr) // fall back to English
						VectorForEach (primaryCatalogs, HelpCatalog*, c)
							if(c->getContentLanguage ().isEmpty () || c->getContentLanguage () == English)
							{
								best = c;
								break;
							}
						EndFor

					if(best == nullptr) // no luck, pick any
						best = primaryCatalogs[0];
				}
			}
			return best;
		};

		if(HelpCatalog* catalog = findPrimaryCatalog ())
		{
			referenceList->addShared (*catalog);
			referenceList->setDefaultReference (catalog->getDefaultReference ());
			if(catalog->isQuickHelp ())
			{
				Url quickHelpPath;
				makeHelpPath (quickHelpPath, kQuickHelpFile, catalog->getPath ());
				quickHelp.loadMadcapFile (quickHelpPath);
			}
		}

		#if (0 && DEBUG)
		referenceList->dump ();
		#endif
	}
	return *referenceList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDocumentViewer* HelpManager::getViewerForDocument (UrlRef path) const
{
	if(pdfViewer && pdfViewer->canOpenDocument (path))
		return pdfViewer;
	
	if(htmlViewer && htmlViewer->canOpenDocument (path))
		return htmlViewer;

	return defaultViewer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult HelpManager::openHelpFile (const HelpReference& reference)
{
	Url path;
	ASSERT (reference.getCatalog () != nullptr)
	makeExistingHelpPath (path, reference.getFileName (), reference.getCatalog ()->getPath ());

	return openHelpFile (path, reference.getDestination ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult HelpManager::openHelpFile (UrlRef path, StringRef location)
{
	if(path.isNativePath ())
	{
		if(System::GetFileSystem ().fileExists (path))
		{
			CCL_PRINT ("[HelpManager] Open Help File \"")
			CCL_PRINT (UrlDisplayString (path))
			CCL_PRINT ("\" at location \"")
			CCL_PRINT (location)
			CCL_PRINTLN ("\"")

			IDocumentViewer* viewer = getViewerForDocument (path);
			ASSERT (viewer != nullptr)
			if(!viewer->isInstalled ())
			{
				if(viewer == pdfViewer)
				{
					// fallback to any application that can handle this document
					if(System::GetSystemShell ().openUrl (path) == kResultOk)
						return kResultOk;

					Alert::error (XSTR (PDFViewerNotInstalled));
				}
				return kResultFailed;
			}

			if(currentFile && !currentFile->isEqualUrl (path))
				viewer->closeAllDocuments ();

			viewer->openDocument (path, location);
			currentFile = NEW Url (path);

			return kResultOk;
		}
	}
	else if(System::GetSystemShell ().openUrl (path) == kResultOk)
		return kResultOk;

	Boxed::Variant handled;
	SignalSource (Signals::kHelpManager).signal (Message (Signals::kHelpFileNotFound, static_cast<IVariant*> (&handled)));
	if(handled.asVariant ().asBool () == false)
	{
		String message;
		message = XSTR (HelpFileNotFound);
		message << UrlDisplayString (path);
		Alert::error (message);
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::showHelpCatalog (IHelpCatalog* _catalog)
{
	HelpCatalog* catalog = unknown_cast<HelpCatalog> (_catalog);
	if(!catalog)
		return kResultInvalidArgument;

	return openHelpFile (catalog->getDefaultReference ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::showLocation (StringRef location)
{
	CCL_PRINTF ("[HelpManager] Show location \"%s\"\n", location.isEmpty () ? "(empty)" : MutableCString (location).str ())

	const HelpReference* ref = nullptr;
	HelpCatalog& list = getReferences ();

	if(!location.isEmpty ())
	{
		static const String kSeparator (CCLSTR (";"));
		if(location.contains (kSeparator)) // multiple alternatives
		{
			ForEachStringToken (location, kSeparator, token)
				ref = list.lookup (token);
				if(ref)
					break;
			EndFor
		}
		else
			ref = list.lookup (location);

		#if DEBUG
		if(ref == nullptr)
			Alert::error (String () << "[HelpManager (DEBUG)]: Undefined help location \"" << location << "\"!");
		#endif
	}

	if(ref == nullptr)
		ref = &getReferences ().getDefaultReference ();

	tresult result = openHelpFile (*ref);

	#if (1 && DEBUG) // allow editing of references during debugging
	safe_release (referenceList);
	#endif

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::showContextHelp (IUnknown* invoker)
{
	String helpId;

	if(MenuItem* item = unknown_cast<MenuItem> (invoker))
	{
		CCL_PRINTF ("[HelpManager] Invoked by menu item %s\n", MutableCString (item->getTitle ()).str ())

		helpId = item->getHelpIdentifier ();
	}
	else
	{
		View* target = unknown_cast<View> (invoker);
		if(target == nullptr)
			target = getActiveView ();

		CCL_PRINTF ("[HelpManager] Target view is %s (class %s)\n", 
					target ? MutableCString (target->getTitle ()).str () : nullptr,
					target ? target->myClass ().getPersistentName () : nullptr)

		if(target)
		{
			if(target->getName () == CCLSTR ("HelpViewer"))
				return kResultOk;
			composeHelpLocation (helpId, target);
		}
	}

	return showLocation (helpId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* HelpManager::getActiveView ()
{
	#if DEBUG && 0
	Window* window1 = Desktop.getTopWindow (kDialogLayer);
	Window* window2 = Desktop.getTopWindow (kWindowLayerFloating);
	Window* window3 = Desktop.getTopWindow (kWindowLayerIntermediate);
	Window* window4 = Desktop.getTopWindow (kWindowLayerBase);
	#endif

	// find active window in highest layer...
	Window* window = nullptr;
	for(int i = kNumWindowLayers-1; i >= 0; i--)
	{
		Window* w = Desktop.getTopWindow ((WindowLayer)i);
		if(w && w->isActive ())
		{
			window = w;
			break;
		}
	}

	if(window == nullptr)
		window = unknown_cast<Window> (Desktop.getApplicationWindow ());

	if(window)
	{
		#if 0
		// use focus view (if present)
		if(View* focusView = window->getFocusView ())
			return focusView;
		#endif

		// use deepest active window base
		if(WindowBase* deepest = window->getDeepestActiveWindow ())
			return deepest;
	}
	return window;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::addTutorials (UrlRef _path)
{
	Url baseFolder (_path);	
	Url collectionPath (baseFolder);
	collectionPath.descend (kTutorialCollectionFile);
	AutoPtr<HelpTutorialCollection> collection = NEW HelpTutorialCollection;
	if(!collection->loadFromFile (collectionPath))
		return kResultFailed;

	for(auto t : iterate_as<HelpTutorial> (collection->getTutorials ()))
	{
		t->setBaseFolder (baseFolder);
		tutorials.add (t);
		t->retain ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API HelpManager::newTutorialIterator () const
{
	return tutorials.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::showTutorial (StringRef tutorialId, int delay)
{
	if(tutorialId.isEmpty ())
		return kResultInvalidArgument;

	if(delay != 0)
	{
		(NEW Message ("showTutorial", tutorialId))->post (this, delay);
		return kResultOk;
	}

	HelpTutorial* tutorial = nullptr;
	tutorial = static_cast<HelpTutorial*> (tutorials.findIf ([tutorialId] (Object* obj)
							{ return static_cast<HelpTutorial*> (obj)->getID () == tutorialId; }));
	if(tutorial == nullptr)
		return kResultFailed;
	
	ASSERT (!activeTutorialViewer)
	if(activeTutorialViewer)
		return kResultUnexpected;
	
	AutoPtr<TutorialViewer> viewer = TutorialViewer::createViewerForTutorial (*tutorial);
	if(!viewer)
		return kResultClassNotFound;

	activeTutorialViewer = viewer;
	Promise p = viewer->runAsync ();
	p.then ([this] (IAsyncOperation& op) 		
	{
		activeTutorialViewer = nullptr;
	});

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::alignActiveTutorial (StringRef helpId)
{
	if(activeTutorialViewer == nullptr)
		return kResultFailed;

	Window* tutorialWindow = unknown_cast<Window> (Desktop.getWindowByOwner (activeTutorialViewer->asUnknown ()));
	if(tutorialWindow == nullptr) // The tutorial window doesn't exist yet, let's try again once it does
		return deferTutorialWindowCall (NEW Message ("alignActiveTutorial", helpId));

	View* referenceView = findViewWithHelpIdPath (helpId);
	if(referenceView == nullptr)
		return kResultFailed;

	// The aligned window should not be centered and should not have sheetstyle
	StyleFlags style = tutorialWindow->getStyle ();
	style.setCustomStyle (Styles::kWindowBehaviorCenter, false);
	style.setCustomStyle (Styles::kWindowBehaviorSheetStyle, false);
	tutorialWindow->setStyle (style);

	Point screenPos;
	referenceView->clientToScreen (screenPos);
	Rect viewRect (referenceView->getSize ());
	viewRect.moveTo (screenPos);

	static const Coord kWindowSpacing = 20; // offset of tutorial from reference view

	// we calculate with the window's frame rect (including non-client area), but finally need to set the "window size" (client size at frame pos)
	Rect tutorialRect (tutorialWindow->getSize ());
	Rect tutorialFrameRect;
	tutorialWindow->getFrameSize (tutorialFrameRect);

	if(tutorialFrameRect.isEmpty ())
		tutorialWindow->getClientRect (tutorialFrameRect);

	// align below reference view
	Point referencePos (viewRect.getLeftBottom () + Point (0, kWindowSpacing));
	tutorialFrameRect.moveTo (referencePos);

	// try to keep tutorial inside monitor / screen
	Rect monitorRect;
	int monitor = Desktop.findMonitor (referencePos, false);

	// falback to monitor of application window
	if(monitor < 0)
		if(auto appWindow = unknown_cast<Window> (Desktop.getApplicationWindow ()))
			monitor = Desktop.findMonitor (appWindow->getSize ().getCenter (), true);

	if(monitor >= 0)
		Desktop.getMonitorSize (monitorRect, monitor, true);
	else
		Desktop.getVirtualScreenSize (monitorRect, true);

	auto moveInsideScreenHorizontally = [&] ()
	{
		if(tutorialFrameRect.getWidth () < monitorRect.getWidth ()) // otherwise we can't help
		{
			Coord outside = 0;
			if((outside = tutorialFrameRect.right - monitorRect.right) > 0)
				tutorialFrameRect.offset (-outside, 0);
			else if((outside = monitorRect.left - tutorialFrameRect.left) > 0)
				tutorialFrameRect.offset (outside, 0);
		}
	};

	auto moveInsideScreenVertically = [&] ()
	{
		if(tutorialFrameRect.getHeight () < monitorRect.getHeight ())
		{
			Coord outside = 0;
			if((outside = tutorialFrameRect.bottom - monitorRect.bottom) > 0)
				tutorialFrameRect.offset (0, -outside);
			else if((outside = monitorRect.top - tutorialFrameRect.top) > 0)
				tutorialFrameRect.offset (0, outside);
		}
	};

	moveInsideScreenHorizontally ();

	// if tutorial bottom is outside screen, check if there's enough room to place the window above the view
	Coord outsideBottom = tutorialFrameRect.bottom - monitorRect.bottom;
	if(outsideBottom > 0)
	{
		Coord alternativeTopPos = viewRect.top - kWindowSpacing - tutorialFrameRect.getHeight ();
		if(alternativeTopPos >= monitorRect.top)
			tutorialFrameRect.moveTo (Point (tutorialFrameRect.left, alternativeTopPos));
		else
		{
			// not enough room either: move into screen vertically, adjust horizontally to keep reference view visible
			if(tutorialFrameRect.getHeight () < monitorRect.getHeight ())
			{
				tutorialFrameRect.moveTo (Point (tutorialFrameRect.left, viewRect.top));

				Coord availableLeft = viewRect.left - monitorRect.left;
				Coord availableRight = monitorRect.right - viewRect.right;
				if(availableRight >= tutorialFrameRect.getWidth () || availableLeft <= availableRight) // prefer right edge of reference view
				{
					// align to right edge of reference view
					tutorialFrameRect.moveTo (Point (viewRect.right + kWindowSpacing, tutorialFrameRect.top));
				}
				else
				{
					// align to left edge of reference view
					tutorialFrameRect.moveTo (Point (viewRect.left - kWindowSpacing - tutorialFrameRect.getWidth (), tutorialFrameRect.top));
				}

				moveInsideScreenHorizontally ();
				moveInsideScreenVertically ();
			}
			else
			{
				ASSERT (false) // tutorial is simply too big
			}
		}
	}

	tutorialRect.moveTo (tutorialFrameRect.getLeftTop ());

	tutorialWindow->setSize (tutorialRect);
	tutorialWindow->updateSize ();
	tutorialWindow->activate (); // bonus: bring to front
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::centerActiveTutorial ()
{
	if(activeTutorialViewer == nullptr)
		return kResultFailed;

	Window* tutorialWindow = unknown_cast<Window> (Desktop.getWindowByOwner (activeTutorialViewer->asUnknown ()));
	if(tutorialWindow == nullptr) // The tutorial window doesn't exist yet, let's try again once it does
		return deferTutorialWindowCall (NEW Message ("centerActiveTutorial"));

	tutorialWindow->center ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::focusActiveTutorial ()
{
	if(activeTutorialViewer == nullptr)
		return kResultFailed;

	Window* tutorialWindow = unknown_cast<Window> (Desktop.getWindowByOwner (activeTutorialViewer->asUnknown ()));
	if(tutorialWindow == nullptr) // The tutorial window doesn't exist yet, let's try again once it does
		return deferTutorialWindowCall (NEW Message ("focusActiveTutorial"));

	tutorialWindow->activate ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HelpManager::hasInfoViewers () const
{
	return !infoViewers.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::addInfoViewer (IHelpInfoViewer* viewer)
{
	ASSERT (viewer && !infoViewers.contains (viewer))
	infoViewers.append (viewer);
	if(currentInfo)
		viewer->updateHelpInfo (currentInfo);
	if(infoViewers.count () == 1)
		startTimer ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::removeInfoViewer (IHelpInfoViewer* viewer)
{
	ASSERT (viewer && infoViewers.contains (viewer))
	viewer->updateHelpInfo (nullptr);
	infoViewers.remove (viewer);

	if(infoViewers.isEmpty ())
	{
		stopTimer ();
		setCurrentInfo (nullptr);
	}
			
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::showInfo (IPresentable* info)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	if(setCurrentInfo (info))
		updateInfoViewers ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpManager::setCurrentInfo (IPresentable* info)
{
	if(currentInfo != info)
	{
		if(currentInfo)
		{
			ISubject::removeObserver (currentInfo, this);
			currentInfo->release ();
		}
		currentInfo = info;
		if(currentInfo)
		{
			currentInfo->retain ();
			ISubject::addObserver (currentInfo, this);
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpManager::updateInfoViewers ()
{
	ListForEach (infoViewers, IHelpInfoViewer*, viewer)
		viewer->updateHelpInfo (currentInfo);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpManager::updateQuickHelp ()
{
	ListForEach (infoViewers, IHelpInfoViewer*, viewer)
		viewer->updateHelpInfo (&quickHelp);
	EndFor			
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::highlightControl (StringRef helpId, IWindow* window, tbool exclusive)
{
	CCL_PRINTF ("highlightControl: %s\n", MutableCString (helpId).str ())

	if(Window* w = unknown_cast<Window> (window))
	{
		if(View* view = findViewWithHelpIdPath (*w, helpId))
		{
			viewHighlights.addView (view, exclusive != 0);
			return kResultOk;
		}
		else if(exclusive)
			viewHighlights.removeAll ();

		return kResultFailed;
	}
	else
	{
		// try application window and other windows
		IWindow* appWindow = Desktop.getApplicationWindow ();
		if(appWindow && highlightControl (helpId, appWindow, exclusive) == kResultOk)
			return kResultOk;

		for(auto i = 0, numWindows = Desktop.countWindows (); i < numWindows; i++)
		{
			IWindow* w = Desktop.getWindow (i);
			if(w != appWindow && highlightControl (helpId, w, exclusive) == kResultOk)
				return kResultOk;
		}
	}
	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::discardHighlights ()
{
	viewHighlights.removeAll ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::dimAllWindows ()
{
	viewHighlights.addView (nullptr, true);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpManager::modifyHighlights (tbool begin)
{
	viewHighlights.modifyHighlights (begin != 0);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API HelpManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		if(currentInfo && isEqualUnknown (subject, currentInfo))
			updateInfoViewers ();
	}
	else if(msg == "showTutorial")
	{
		showTutorial (msg[0].asString ());
	}
	else if(msg == DesktopManager::kWindowAdded && activeTutorialViewer != nullptr)
	{
		if(auto* addedWindow = unknown_cast<Window> (msg[0].asUnknown ()))
		{
			if(unknown_cast<TutorialViewer> (addedWindow->getController ()) == activeTutorialViewer && pendingTutorialWindowCall)
			{
				Variant returnValue;
				invokeMethod (returnValue, *pendingTutorialWindowCall);
				ASSERT (returnValue == kResultOk);
				pendingTutorialWindowCall.release ();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpManager::onIdleTimer ()
{
	// info views take precedence over quick help
	if(currentInfo)
		return;

	Point mousePos;
	GUI.getMousePosition (mousePos);
	if(View* target = quickHelp.findView (mousePos))
	{
		getReferences ();
		String helpLocation;
		composeHelpLocation (helpLocation, target);
		quickHelp.setByKey (helpLocation);
		updateQuickHelp ();
	}
	else if(quickHelp.showsHelpIds ())
		updateQuickHelp (); // to allow quickHelp context menu via presentable
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpManager::composeHelpLocation (String& helpId, View* target)
{
	static const String kSeparator (CCLSTR (";"));
	static const String kSubSeparator (CCLSTR ("."));
	
	ASSERT (target)	
	helpId.empty ();
	
	String targetId = target->getHelpIdentifier ();
	
	if(!targetId.contains (kSubSeparator))
	{
		if(View* parent = target->getParent ())
		{
			String parentHelpId = parent->getHelpIdentifier ();
			bool useWindowId = parentHelpId.isEmpty ();

			if(useWindowId)
				if(auto frameView = target->getParent<FrameView> ())
				{
					// fallback to workspace frame's window class ID as parentHelpId (returning this from FrameView::getHelpIdentifier would dominate too many detailed ids)
					String windowId (frameView->getFrameItem ()->getWindowID ());
					if(!windowId.isEmpty ())
						parentHelpId = windowId;
				}

			if(!parentHelpId.isEmpty ())
			{
				if(parentHelpId.contains (kSeparator))
				{
					// use first (most detailed) alternative
					ForEachStringToken (parentHelpId, kSeparator, token)
						helpId.append (token);
						break;
					EndFor
				}
				else	
					helpId.append (parentHelpId);

				if(!targetId.isEmpty () && targetId != parentHelpId)
				{
					// alternative 1: "parent.target"
					helpId << kSubSeparator << targetId;

					// alternative 2: plain target id (for legacy strings without window class ID))
					if(useWindowId)
						 helpId << kSeparator << targetId;

					// alternative 3: parent id only (fallback to more general area)
					helpId << kSeparator << parentHelpId;
				}
				return;
			}
		}
	}

	helpId.append (targetId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* HelpManager::findViewWithHelpId (View& startView, StringRef helpId)
{
	AutoPtr<IRecognizer> recognizer;

	if(helpId.startsWith ("@"))
	{
		// find a view whose controller has the given path, or ends with it
		String controllerPath (helpId.subString (1));
		String controllerPathEnding (String (Url::strPathChar) << controllerPath);
		recognizer = Recognizer::create ([=] (IUnknown* obj)
		{
			if(auto view = unknown_cast<View> (obj))
			{
				UnknownPtr<IObjectNode> controller = view->getController ();
				if(controller)
				{
					String path;
					controller->getChildPath (path);
					if(path == controllerPath || path.endsWith (controllerPathEnding))
						return true;
				}
			}
			return false;
		});
	}
	else
	{
		recognizer = Recognizer::create ([&] (IUnknown* obj)
		{
			if(auto view = unknown_cast<View> (obj))
			{
				// windowID of workspace frame (see composeHelpLocation)
				auto frameView = ccl_cast<FrameView> (view);
				if(frameView && String (frameView->getFrameItem ()->getWindowID ()) == helpId)
					return true;

				return view->getHelpIdentifier () == helpId;
			}
			return false;
		});
	}
	return startView.findView (*recognizer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* HelpManager::findViewWithHelpIdPath (StringRef helpIdPath)
{
	// try application window and other windows
	auto appWindow = unknown_cast<Window> (Desktop.getApplicationWindow ());
	if(appWindow)
		if(View* view = findViewWithHelpIdPath (*appWindow, helpIdPath))
			return view;

	for(auto i = 0, numWindows = Desktop.countWindows (); i < numWindows; i++)
	{
		auto w = unknown_cast<Window> (Desktop.getWindow (i));
		if(w != appWindow)
			if(View* view = findViewWithHelpIdPath (*w, helpIdPath))
				return view;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* HelpManager::findViewWithHelpIdPath (View& startView, StringRef helpIdPath)
{
	static const String kSubSeparator (CCLSTR (".")); // separates path segments
	static const String kEscape (CCLSTR ("^"));	 // escapes a following "." as part of a helpID

	static const String kDelimiters (String () << kSubSeparator << kEscape);

	View* currentView = &startView;

	String pendingId;
	bool wasEscape = false;
	ForEachStringTokenWithFlags (helpIdPath, kDelimiters, helpId, Text::kPreserveEmptyToken)
		bool isEscape = __delimiter == kEscape.at (0);
		if(isEscape)
		{
			pendingId << helpId;
		}
		else if(helpId.isEmpty ())
		{
			// empty token: could be an escaped delimiter
			if(wasEscape)
				pendingId.append (&__delimiter, 1);
		}
		else
		{
			pendingId << helpId;
			if(View* view = findViewWithHelpId (*currentView, pendingId))
				currentView = view;

			pendingId.empty ();
		}
		wasEscape = isEscape;
	EndFor

	if(currentView == &startView)
		currentView = nullptr;

	return currentView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (HelpManager)
	DEFINE_METHOD_ARGR ("showLocation", "location", "tresult")
	DEFINE_METHOD_ARGR ("showTutorial", "tutorial: string, delay: int = 0", "tresult")
	DEFINE_METHOD_ARGR ("alignActiveTutorial", "helpId: string", "tresult")
	DEFINE_METHOD_ARGR ("centerActiveTutorial", "", "tresult")
	DEFINE_METHOD_ARGR ("focusActiveTutorial", "", "tresult")
	DEFINE_METHOD_ARGR ("highlightControl", "helpId: string, exclusive: bool = true", "tresult")
	DEFINE_METHOD_ARGR ("discardHighlights", "", "tresult")
	DEFINE_METHOD_ARGR ("modifyHighlights", "begin: bool", "tresult")
	DEFINE_METHOD_ARGR ("dimAllWindows", "", "tresult")
END_METHOD_NAMES (HelpManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HelpManager::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "showLocation")
	{
		returnValue = showLocation (msg[0].asString ());
		return true;
	}
	else if(msg == "showTutorial")
	{
		String tutorialId (msg[0].asString ());
		int delay = msg.getArgCount () > 1 ? msg[1].asInt () : 0;
		returnValue = showTutorial (tutorialId, delay);
		return true;
	}
	else if(msg == "alignActiveTutorial")
	{
		returnValue = alignActiveTutorial (msg[0].asString ());
		return true;
	}
	else if(msg == "centerActiveTutorial")
	{
		returnValue = centerActiveTutorial ();
		return true;
	}
	else if(msg == "focusActiveTutorial")
	{
		returnValue = focusActiveTutorial ();
		return true;
	}
	else if(msg == "highlightControl")
	{
		bool exclusive = msg.getArgCount () > 1 ? msg[1].asBool () : true;
		returnValue = highlightControl (msg[0].asString (), nullptr, exclusive);
		return true;
	}
	else if(msg == "discardHighlights")
	{
		returnValue = discardHighlights ();
		return true;
	}
	else if(msg == "modifyHighlights")
	{
		tbool begin = msg.getArgCount () > 0 ? msg[0].asBool () : true;
		returnValue = modifyHighlights (begin);
		return true;
	}
	else if(msg == "dimAllWindows")
	{
		returnValue = dimAllWindows ();
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}
