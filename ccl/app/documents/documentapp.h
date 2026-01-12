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
// Filename    : ccl/app/documents/documentapp.h
// Description : Document-based Application
//
//************************************************************************************************

#ifndef _ccl_documentapp_h
#define _ccl_documentapp_h

#include "ccl/app/application.h"

#include "ccl/public/app/idocument.h"
#include "ccl/public/gui/framework/iworkspace.h"

namespace CCL {

class DocumentBlocks;

//************************************************************************************************
// DocumentApplication
//************************************************************************************************

class DocumentApplication: public Application,
                           public AbstractDocumentEventHandler,
                           public IWorkspaceEventHandler
{
public:
	DECLARE_CLASS (DocumentApplication, Application)

	using Application::Application;

	static void setupDocumentNavigation ();
	
	// Application
	void beforeQuit () override;
	bool startup () override;
	bool shutdown () override;
	void CCL_API extendMenu (IMenu& menu, StringID name) override;
	tbool CCL_API openFile (UrlRef path) override;
	IDragHandler* CCL_API createDragHandler (const DragEvent& event, IView* view) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE2 (IDocumentEventHandler, IWorkspaceEventHandler, Application)

protected:
	void addDocumentBlocks (); // append DocumentBlocks component - to be called in derived class constructor
	DocumentBlocks* getDocumentBlocks () const; // optional
	virtual void prepareDocumentBlocks (DocumentBlocks& blocks); // to be implemented in derived class

	// IWorkspaceEventHandler
	void CCL_API onWorkspaceEvent (const WorkspaceEvent& e) override;

	// IDocumentEventHandler
	void CCL_API onDocumentManagerAvailable (tbool state) override;
	void CCL_API onDocumentEvent (IDocument& document, int eventCode) override;
};

} // namespace CCL

#endif // _ccl_documentapp_h
