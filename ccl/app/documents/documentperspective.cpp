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
// Filename    : ccl/app/documents/documentperspective.cpp
// Description : Document Perspective
//
//************************************************************************************************

#include "ccl/app/documents/documentperspective.h"

#include "ccl/app/documents/document.h"
#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/utilities/fileicons.h"

#include "ccl/base/storage/archivehandler.h"
#include "ccl/public/storage/istorage.h"

#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//************************************************************************************************
// DocumentPerspective
//************************************************************************************************

IWorkspace* DocumentPerspective::getWorkspace ()
{
	return System::GetWorkspaceManager ().getWorkspace (RootComponent::instance ().getApplicationID ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPerspective* DocumentPerspective::createPerspective (StringID perspectiveID)
{
	IWorkspace* workspace = getWorkspace ();
	ASSERT (workspace != nullptr)
	IPerspective* perspective = workspace ? workspace->clonePerspective (perspectiveID) : nullptr;
	if(perspective)
		perspective->retain ();
	return perspective;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (DocumentPerspective, Object)
	
//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentPerspective::DocumentPerspective (Document& document, IPerspective* perspective, StringID viewID, StringID altPerspectiveID)
: document (document),
  workspace (getWorkspace ()),
  perspective (nullptr),
  viewID (viewID),
  altPerspectiveID (altPerspectiveID),
  icon (nullptr),
  closing (false)
{
	document.setDocumentView (this);
	document.retain ();

	ASSERT (workspace != nullptr)
	ASSERT (perspective != nullptr)
	setPerspective (perspective, viewID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentPerspective::~DocumentPerspective ()
{
	document.setDocumentView (nullptr);
	document.release ();

	setPerspective (nullptr);

	if(icon)
		icon->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentPerspective::setPerspective (IPerspective* p, CCL::StringID viewID)
{
	if(perspective)
	{
		perspective->setActivator (nullptr);
		ISubject::removeObserver (perspective, this);
		perspective->release ();
	}

	perspective = p;
	this->viewID = viewID;

	if(perspective)
	{
		perspective->setActivator (this);
		perspective->retain ();
		ISubject::addObserver (perspective, this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentPerspective::activateDocumentView ()
{
	ASSERT (perspective != nullptr && workspace != nullptr)
	if(workspace && perspective)
	{
		if(closing)
			return;

		bool activated = DocumentManager::instance ().setActiveDocument (&document);

		workspace->selectPerspective (perspective);
		workspace->openView (viewID);

		if(activated)
			DocumentManager::instance ().signalDocumentEvent (document, Document::kViewActivated);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentPerspective::closeDocumentView ()
{
	{
		ScopedVar<bool> scope (closing, true);
		
		if(perspective)
			perspective->setActivator (nullptr);
		
		if(workspace)
			workspace->selectPerspective (altPerspectiveID);
	}

	release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentPerspective::isDocumentVisible () const
{
	return workspace ? workspace->isViewOpen (viewID) != 0 : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CCL_API DocumentPerspective::getPerspectiveTitle ()
{
	return document.getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CCL_API DocumentPerspective::getPerspectiveDescription ()
{
	return UrlDisplayString (document.getPath ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API DocumentPerspective::getPerspectiveIcon ()
{
	if(!icon)
		icon = FileIcons::instance ().createIcon (document.getPath ());
	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentPerspective::activatePerspective ()
{
	activateDocumentView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentPerspective::notifyPerspectiveSelected ()
{
	DocumentManager::instance ().setActiveDocument (&document);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentPerspective::notify (ISubject* subject, MessageRef msg)
{
	if(isEqualUnknown (subject, perspective) && msg == kChanged)
	{
		if(DocumentManager::instance ().isDirtySuspended () == false && document.ignoreDirtyUI () == false)
			document.setDirty ();
	}
	else
		SuperClass::notify (subject, msg);
}

//************************************************************************************************
// PerspectiveStorageHelper
//************************************************************************************************

PerspectiveStorageHelper::PerspectiveStorageHelper (IPerspective* perspective)
: perspective (perspective)
{
	ASSERT (perspective)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPerspective* PerspectiveStorageHelper::getPerspective ()
{
	return perspective;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PerspectiveStorageHelper::loadPerspective (ArchiveHandler& archiveHandler, StringRef progressText)
{
	if(!progressText.isEmpty ())
	{
		IProgressNotify* progress = archiveHandler.getProgress ();
		ASSERT (progress)
		if(progress)
			progress->updateAnimated (progressText);
	}

	bool result = false;

	// load perspective
	UnknownPtr<IStorable> perspectiveStorable (perspective);
	ASSERT (perspectiveStorable.isValid ())
	if(perspectiveStorable)
	{
		result = archiveHandler.loadStream (CCLSTR ("Workspace/perspective.xml"), *perspectiveStorable);
		ASSERT (result)
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PerspectiveStorageHelper::savePerspective (ArchiveHandler& archiveHandler, StringRef progressText, StringID debugName)
{
	IProgressNotify* progress = archiveHandler.getProgress ();
	ASSERT (progress)
	if(progress)
		progress->updateAnimated (progressText);

	// save perspective
	UnknownPtr<IStorable> perspectiveStorable (perspective);
	ASSERT (perspectiveStorable.isValid ())
	if(perspectiveStorable)
	{
		if(!archiveHandler.addSaveTask (CCLSTR ("Workspace/perspective.xml"), *perspectiveStorable, debugName))
			return false;
	}
	return true;
}
