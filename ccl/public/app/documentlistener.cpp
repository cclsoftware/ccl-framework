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
// Filename    : ccl/public/app/documentlistener.cpp
// Description : Document Listener
//
//************************************************************************************************

#include "ccl/public/app/documentlistener.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/iobjectnode.h"
#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// DocumentListener
//************************************************************************************************

DocumentListener::DocumentListener (IDocument* appDocument)
: appDocument (appDocument)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentListener::saveContent (IFileSystem& fileSystem, VariantRef data, IProgressNotify* progress) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentListener::loadContent (IFileSystem& fileSystem, VariantRef data, IProgressNotify* progress)
{
	return true;
}

//************************************************************************************************
// DocumentListenerFactory
//************************************************************************************************

IDocumentManager* DocumentListenerFactory::getDocumentManager ()
{
	UnknownPtr<IObjectNode> appRoot = System::GetObjectTable ().getObjectByName (IObjectTable::kHostApp);
	return appRoot ? UnknownPtr<IDocumentManager> (appRoot->findChild (CCLSTR (IDocumentManager::kComponentName))) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentListenerFactory::DocumentListenerFactory ()
: documentManager (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentListenerFactory::~DocumentListenerFactory ()
{
	ASSERT (listeners.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentListenerFactory::beginListen ()
{
	documentManager = getDocumentManager ();
	if(documentManager == nullptr)
		return false;

	documentManager->addHandler (this);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentListenerFactory::endListen ()
{
	if(documentManager)
	{
		documentManager->removeHandler (this);
		documentManager = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentListenerFactory::hasListeners () const
{
	return !listeners.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentListener* DocumentListenerFactory::findListener (IDocument& document) const
{
	VectorForEach (listeners, DocumentListener*, listener)
		if(listener->getAppDocument () == &document)
			return listener;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentListenerFactory::onDocumentEvent (IDocument& document, int eventCode)
{
	if(eventCode == IDocument::kCreated || eventCode == IDocument::kBeforeLoad)
		onDocumentAvailable (document, true);

	if(DocumentListener* listener = findListener (document))
		listener->onEvent (eventCode);

	if(eventCode == IDocument::kDestroyed || eventCode == IDocument::kLoadFailed)
		onDocumentAvailable (document, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentListenerFactory::onDocumentAvailable (IDocument& document, bool state)
{
	if(state)
	{
		if(DocumentListener* listener = createListener (document))
		{
			onListenerAvailable (*listener, true);

			listeners.add (listener);
			listener->initialize ();
		}
	}
	else
	{
		if(DocumentListener* listener = findListener (document))
		{
			listeners.remove (listener);
			listener->terminate ();

			onListenerAvailable (*listener, false);
			listener->release ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentListenerFactory::onListenerAvailable (DocumentListener& listener, bool state)
{
	if(state)
	{
		UnknownPtr<IStorageRegistry> storageRegistry (listener.getAppDocument ());
		if(storageRegistry)
			storageRegistry->registerHandler (&listener);
	}
	else
	{
		UnknownPtr<IStorageRegistry> storageRegistry (listener.getAppDocument ());
		if(storageRegistry)
			storageRegistry->unregisterHandler (&listener);
	}
}
