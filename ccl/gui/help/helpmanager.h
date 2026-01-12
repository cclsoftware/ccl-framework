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
// Filename    : ccl/gui/help/helpmanager.h
// Description : Help Manager
//
//************************************************************************************************

#ifndef _ccl_helpmanager_h
#define _ccl_helpmanager_h

#include "ccl/base/singleton.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/gui/framework/ipresentable.h"

namespace CCL {

class View;
interface IDocumentViewer;

class HelpCatalog;
class HelpReference;
class QuickHelp;
class ViewHighlights;
class TutorialViewer;

//************************************************************************************************
// HelpManager
//************************************************************************************************

class HelpManager: public Object,
				   public IHelpManager,
				   public IdleClient,				   
				   public Singleton<HelpManager>
{
public:
	DECLARE_CLASS (HelpManager, Object)
	DECLARE_METHOD_NAMES (HelpManager)

	HelpManager ();
	~HelpManager ();

	// IHelpManager
	tresult CCL_API setHelpLocation (UrlRef path) override;
	tresult CCL_API addHelpCatalog (UrlRef path, StringID category) override;
	IUnknownIterator* CCL_API newCatalogIterator () const override;
	tresult CCL_API showHelpCatalog (IHelpCatalog* catalog) override;
	tresult CCL_API showLocation (StringRef location) override;
	tresult CCL_API showContextHelp (IUnknown* invoker = nullptr) override;
	
	tresult CCL_API addTutorials (UrlRef path) override;
	IUnknownIterator* CCL_API newTutorialIterator () const override;
	tresult CCL_API showTutorial (StringRef tutorialId, int delay = 0) override;
	tresult CCL_API alignActiveTutorial (StringRef helpId) override;
	tresult CCL_API centerActiveTutorial () override;
	tresult CCL_API focusActiveTutorial () override;

	tbool CCL_API hasInfoViewers () const override;
	tresult CCL_API addInfoViewer (IHelpInfoViewer* viewer) override;
	tresult CCL_API removeInfoViewer (IHelpInfoViewer* viewer) override;
	tresult CCL_API showInfo (IPresentable* info) override;
	
	tresult CCL_API highlightControl (StringRef helpId, IWindow* window = nullptr, tbool exclusive = true) override;
	tresult CCL_API discardHighlights () override;
	tresult CCL_API modifyHighlights (tbool begin) override;
	tresult CCL_API dimAllWindows () override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IdleClient
	void onIdleTimer () override;

	CLASS_INTERFACE2 (IHelpManager, ITimerTask, Object)

protected:
	AutoPtr<Url> helpFolder;
	IDocumentViewer* pdfViewer;
	IDocumentViewer* htmlViewer;
	IDocumentViewer* defaultViewer;
	AutoPtr<Url> currentFile;
	HelpCatalog* referenceList;
	ObjectList catalogs;
	ObjectList tutorials;
	TutorialViewer* activeTutorialViewer;
	LinkedList<IHelpInfoViewer*> infoViewers;
	IPresentable* currentInfo;
	QuickHelp& quickHelp;
	ViewHighlights& viewHighlights;

	AutoPtr<IMessage> pendingTutorialWindowCall;

	tresult deferTutorialWindowCall (IMessage* message);
	
	HelpCatalog& getReferences ();

	void makeHelpPath (Url& path, StringRef fileName, const Url* externalPath = nullptr);
	void makeHelpPath (Url& path, StringRef fileName, StringRef language, const Url* externalPath = nullptr);
	void makeExistingHelpPath (Url& path, StringRef fileName, const Url* externalPath = nullptr);

	IDocumentViewer* getViewerForDocument (UrlRef path) const;
	tresult openHelpFile (const HelpReference& reference);
	tresult openHelpFile (UrlRef path, StringRef location = nullptr);

	View* getActiveView ();

	bool setCurrentInfo (IPresentable* info);
	void updateInfoViewers ();
	void updateQuickHelp ();

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	
	static void composeHelpLocation (String& helpLocation, View* view);

	static View* findViewWithHelpId (View& startView, StringRef helpId);
	static View* findViewWithHelpIdPath (View& startView, StringRef helpId);
	static View* findViewWithHelpIdPath (StringRef helpIdPath); // tries all windows
};

} // namespace CCL

#endif // _ccl_helpmanager_h
