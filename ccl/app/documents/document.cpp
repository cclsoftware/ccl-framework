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
// Filename    : ccl/app/documents/document.cpp
// Description : Document
//
//************************************************************************************************

#include "ccl/app/documents/document.h"
#include "ccl/app/documents/documentrenamer.h"
#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/documents/idocumentview.h"
#include "ccl/app/actions/actionjournal.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/file.h"

#include "ccl/public/text/istringdict.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/base/istream.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IDocumentView, 0x67f053a6, 0xfa5b, 0x4200, 0x97, 0x61, 0xd3, 0x93, 0xe9, 0x1b, 0x65, 0xc1)
DEFINE_IID_ (IDocumentViewFactory, 0x771c1a3b, 0x833b, 0x4f97, 0x98, 0x0, 0x3d, 0x60, 0xeb, 0xb8, 0x99, 0x69)

//************************************************************************************************
// DocumentClass
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DocumentClass, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentClass::DocumentClass (int _flags)
: flags (_flags),
  formatVersion (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentClass::isNative () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentClass::setFileType (const FileType& _fileType)
{
	fileType = _fileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API DocumentClass::getFileType () const
{ 
	return fileType; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API DocumentClass::getSubFolderName () const
{
	return getSubFolder ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String DocumentClass::makeTitle () const
{
	return getDefaultTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentTemplate* DocumentClass::createDefaultTemplate ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentClass::getUserTemplateFolders (StringList& folderNames)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Component* DocumentClass::createNewDialog (Document& document, StringID contextId)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentClass* DocumentClass::getTargetClass ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentClass::installFile (Url& path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentClass::canImportFile (UrlRef path) const
{
	return getFileType () == path.getFileType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentClass::loadDocument (Document& document)
{
	ActionJournalDisabler disabler (document.getActionJournal ()); 
	return document.load ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentClass::saveDocument (Document& document)
{
	return document.save ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentClass::saveDocumentAs (Document& document, UrlRef path)
{
	return document.saveAs (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentClass::finalizeSaveDocumentAs (Document& document, UrlRef path)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentClass::canSaveDocument (Document& document) const
{
	return document.getDocumentClass () == this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentClass::canMergeDocuments (Document& target, UrlRef sourcePath)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentClass::mergeDocuments (Document& target, Document& source)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (DocumentClass)
	DEFINE_METHOD_ARGR ("isNative", "", "bool")
	DEFINE_METHOD_ARGR ("isPrivate", "", "bool")
	DEFINE_METHOD_ARGR ("getFileType", "", "FileType")
END_METHOD_NAMES (DocumentClass)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentClass::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "isNative")
	{
		returnValue = isNative ();
		return true;
	}
	else if(msg == "isPrivate")
	{
		returnValue = isPrivate ();
		return true;
	}
	else if(msg == "getFileType")
	{
		AutoPtr<Boxed::FileType> ft = NEW Boxed::FileType (getFileType ());
		returnValue.takeShared (ft->asUnknown ());
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// Document::CancelGuard
//************************************************************************************************

Document::CancelGuard::CancelGuard (Document& document, IProgressNotify* progress)
: document (document),
  progress (progress)
{
	document.isCanceled (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Document::CancelGuard::~CancelGuard ()
{
	if(progress)
		document.isCanceled (document.isCanceled () || progress->isCanceled ());
}

//************************************************************************************************
// Document
//************************************************************************************************

bool Document::silentPreview = false;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (Document, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Document::Document (DocumentClass* _documentClass)
: documentClass (_documentClass),
  flags (0),
  dirty (false),
  autoSaveDirty (false),
  lastAutoSaveTime (0),
  documentView (nullptr),
  actionJournal (nullptr),
  eventHandler (nullptr)
{
	if(documentClass)
		documentClass->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Document::~Document ()
{
	if(actionJournal)
	{
		actionJournal->removeObserver (this);
		actionJournal->release ();
		ISubject::removeObserver (&System::GetCommandTable (), ccl_const_cast (this));
	}
	
	if(documentClass)
		documentClass->release ();

	ASSERT (documentView == nullptr)
	if(documentView)
		documentView->release ();

	ASSERT (eventHandler == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Document::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IDocument)
	QUERY_INTERFACE (IViewFactory)
	QUERY_INTERFACE (IActionContext)
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Document::toString (String& string, int flags) const
{
	string = getTitle ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::setTitle (StringRef _title)
{
	title = _title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::setPath (UrlRef _path)
{
	if(path != _path)
	{
		AutoPtr<Url> oldPath = NEW Url (path);
		path = _path;
		if(!isAutoSave ())
			updateTitle ();
		signal (Message (kPathChanged, oldPath->asUnknown (), isAutoSave ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::updateTitle ()
{
	setTitle (UrlDisplayString (path, Url::kStringDisplayName));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::initialize ()
{
	UnknownPtr<IComponent> component = getController ();
	if(component)
		component->initialize (this->asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::terminate ()
{	
	UnknownPtr<IComponent> component = getController ();
	if(component)
		component->terminate ();

	// cleanup action journal
	if(actionJournal)
		actionJournal->removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Document::canRemoveFolder (UrlRef folder) const
{
	return File::isFolderEmpty (folder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Document::prepare (const Attributes* args)
{
	setDirty (false);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Document::prepareImport ()
{
	setDirty (false);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Document::prepareLoading ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Document::load ()
{
	setDirty (false);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Document::save ()
{
	setDirty (false);

	if(actionJournal)
		actionJournal->setSavedNow ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Document::saveAs (UrlRef newPath)
{
	previousPath = getPath ();
	setPath (newPath);

	bool result = save ();
	if(!result)
		setPath (previousPath);

	previousPath = Url::kEmpty;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Document::prepareSaveToNewFolder (UrlRef newDocumentPath)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::finishSaveToNewFolder (UrlRef newDocumentPath)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Renamer* Document::createRenamer ()
{	
	return NEW DocumentRenamer (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Document::canClose () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::onEvent (int eventCode)
{
	switch(eventCode)
	{
	case kActivate :
	case kDeactivate :
		onActivate (eventCode == kActivate);
		break;

	case kViewActivated :
		onViewActivated ();
		break;

	case kLoadFinished :
	case kLoadFailed :
		onLoadFinished (eventCode == kLoadFailed);
		break;

	case kBeforeSave :
		onBeforeSave ();
		break;

	case kSaveFinished :
		onSaveFinished ();
		break;

	case kClose :
		onClose ();
		break;

	case kDestroyed :
		onDestroyed ();
		break;
	}

	if(eventHandler)
	{
		eventHandler->onDocumentEvent (*this, eventCode);
		if(eventCode == kDestroyed || eventCode == kLoadFailed)
		{
			setEventHandler (nullptr);
			ccl_markGC (this->asUnknown ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::setEventHandler (IDocumentEventHandler* newHandler)
{
	if(eventHandler != newHandler)
	{
		if(eventHandler)
		{
			eventHandler->onDocumentManagerAvailable (false);
			ccl_release (eventHandler);
			eventHandler = nullptr;
		}
		eventHandler = newHandler;
		if(eventHandler)
		{
			eventHandler->onDocumentManagerAvailable (true);
			eventHandler->retain ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDocumentEventHandler* Document::getEventHandler () const
{
	return eventHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::onLoadFinished (bool failed)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::onBeforeSave ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::onSaveFinished ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::onActivate (bool state)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::onViewActivated ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::onClose ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::onDestroyed ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API Document::createView (StringID name, VariantRef data, const Rect& bounds)
{
	// ask controller to create view
	UnknownPtr<IViewFactory> factory (getController ());
	return factory ? factory->createView (name, data, bounds) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionJournal* Document::getActionJournal () const
{
	if(!actionJournal)
	{
		actionJournal = NEW ActionJournal;
		actionJournal->addObserver (ccl_const_cast (this));
		ISubject::addObserver (&System::GetCommandTable (), ccl_const_cast (this));
	}
	return actionJournal;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API Document::getTitle () const
{
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API Document::getPath () const
{
	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Document::isDirty () const
{
	if(actionJournal && actionJournal->isModified ())
		return true;

	return dirty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::setDirty (bool _dirty)
{
	bool changed = dirty != _dirty;

	dirty = _dirty;
	autoSaveDirty = _dirty;

	if(changed)
		DocumentManager::instance ().updateDirtyState (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Document::setAutoSavedNow ()
{
	lastAutoSaveTime = System::GetSystemTicks ();
	autoSaveDirty = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Document::needsAutoSave () const
{
	return autoSaveDirty || (actionJournal && lastAutoSaveTime < actionJournal->getLastEditTime ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Document::getModel () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Document::getView () const
{
	return documentView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Document::getController () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Document::getMetaInfo () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IDocumentClass* CCL_API Document::getIDocumentClass () const
{
	return documentClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IActionJournal* CCL_API Document::getIActionJournal () const
{
	return getActionJournal ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Document::notify (ISubject* subject, MessageRef msg)
{
	if(msg == ActionJournal::kExecuted
	|| msg == ActionJournal::kUndone
	|| msg == ActionJournal::kRedone)
	{
		DocumentManager::instance ().updateDirtyState (this);
	}
	else if(msg == ICommandTable::kBeginTransaction 
		|| msg == ICommandTable::kEndTransaction)
	{
		if(isEqualUnknown (subject, &System::GetCommandTable ()) && actionJournal != nullptr)
		{
			if(msg == ICommandTable::kBeginTransaction)
				actionJournal->beginTransaction (msg[0].asString ());
			else
				actionJournal->endTransaction ();
		}	
		return;
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (Document)
	DEFINE_PROPERTY_NAME ("title")
	DEFINE_PROPERTY_NAME ("path")
END_PROPERTY_NAMES (Document)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Document::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "title")
	{
		var = getTitle ();
		return true;
	}
	else if(propertyId == "path")
	{
		AutoPtr<IObject> pathCopy = path.clone ();
		var.takeShared (pathCopy);
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// DocumentFile
//************************************************************************************************

DEFINE_CLASS (DocumentFile, Document)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentFile::load (IStream& stream)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentFile::save (IStream& stream)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentFile::load ()
{
	bool result = false;
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (getPath (), IStream::kOpenMode);
	if(stream)
		result = load (*stream);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentFile::save ()
{
	bool result = false;
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (getPath (), IStream::kCreateMode);
	if(stream)
		result = save (*stream);

	if(result)
		SuperClass::save ();

	return result;
}

