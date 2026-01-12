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
// Filename    : ccl/app/documents/documentapp.cpp
// Description : Document-based Application
//
//************************************************************************************************

#include "ccl/app/documents/documentapp.h"
#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/documents/documentnavigation.h"
#include "ccl/app/documents/documentblocks.h"

#include "ccl/app/navigation/navigator.h"

#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// DocumentApplication
//************************************************************************************************

void DocumentApplication::setupDocumentNavigation ()
{
	DocumentNavigationServer::instance (); // create server
	DocumentManager::instance ().setViewFactory (AutoPtr<IDocumentViewFactory> (NEW DocumentNavigationPage::Factory));
	Navigator::instance ().setHomeUrl (Url (String () << "theme://" << RootComponent::instance ().getApplicationID () << "/HomePage"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (DocumentApplication, Application)

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentApplication::addDocumentBlocks ()
{
	addComponent (NEW DocumentBlocks);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks* DocumentApplication::getDocumentBlocks () const
{
	return findChildNode<DocumentBlocks> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentApplication::startup ()
{
	if(getDocumentBlocks ())
		DocumentManager::instance ().addHandler (this);
	
	return SuperClass::startup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentApplication::shutdown ()
{
	if(getDocumentBlocks ())
		DocumentManager::instance ().removeHandler (this);

	return SuperClass::shutdown ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentApplication::beforeQuit ()
{
	// The menu should not be used anymore because it is going away
	DocumentManager::instance ().getRecentPaths ().removeMenus ();
	DocumentManager::instance ().setConvertMenu (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentApplication::prepareDocumentBlocks (DocumentBlocks& blocks)
{
	CCL_NOT_IMPL ("DocumentApplication::prepareDocumentBlocks");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentApplication::onDocumentManagerAvailable (tbool state)
{
	if(state)
	{
		if(auto documentBlocks = getDocumentBlocks ())
			prepareDocumentBlocks (*documentBlocks);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentApplication::onDocumentEvent (IDocument& document, int eventCode)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentApplication::onWorkspaceEvent (const WorkspaceEvent& e)
{
	if(auto documentBlocks = getDocumentBlocks ())
		documentBlocks->onWorkspaceEvent (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentApplication::extendMenu (IMenu& menu, StringID name)
{
	if(name.startsWith ("Recent Files"))
	{
		IMenu* subMenu = menu.createMenu ();
		subMenu->setMenuAttribute (IMenu::kMenuTitle, RecentDocuments::getTranslatedTitle ());
		menu.addMenu (subMenu);

		DocumentManager::instance ().getRecentPaths ().addMenu (subMenu);
	}
	else if(name == "Convert To")
	{
		IMenu* subMenu = menu.createMenu ();
		subMenu->setMenuAttribute (IMenu::kMenuTitle, DocumentStrings::ConvertTo ());
		menu.addMenu (subMenu);

		DocumentManager::instance ().setConvertMenu (subMenu);
	}
	else
		SuperClass::extendMenu (menu, name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentApplication::openFile (UrlRef path)
{
	if(DocumentManager::instance ().canOpenDocument (path))
	{
		DocumentManager::instance ().deferOpenDocument (path);
		return true;
	}
	return SuperClass::openFile (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* CCL_API DocumentApplication::createDragHandler (const DragEvent& event, IView* view)
{
	return DocumentManager::instance ().createDragHandler (event, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentApplication::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kAppTerminates || msg == kAppSuspended || msg == kAppDeactivated)
		DocumentManager::instance ().notify (subject, msg);

	SuperClass::notify (subject, msg);
}
