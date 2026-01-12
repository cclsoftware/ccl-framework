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
// Filename    : ccl/public/app/idocument.h
// Description : Document Interfaces
//
//************************************************************************************************

#ifndef _ccl_idocument_h
#define _ccl_idocument_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

class FileType;
interface IUnknownList;
interface IUnknownIterator;
interface IAttributeList;
interface IActionJournal;
interface IDocumentEventHandler;

//************************************************************************************************
// IDocumentClass
/**	\ingroup app_inter */
//************************************************************************************************

interface IDocumentClass: IUnknown
{
	virtual tbool CCL_API isNative () const = 0;

	virtual const FileType& CCL_API getFileType () const = 0;

	virtual StringRef CCL_API getSubFolderName () const = 0;
	
	virtual tbool CCL_API isPrivateClass () const = 0;

	DECLARE_IID (IDocumentClass)
};

DEFINE_IID (IDocumentClass, 0xa79b0b09, 0x376e, 0x4643, 0x9b, 0x35, 0x27, 0xde, 0x57, 0xbc, 0x90, 0xa3)

//************************************************************************************************
// IDocument
/**	\ingroup app_inter */
//************************************************************************************************

interface IDocument: IUnknown
{
	/**	Event codes for IDocumentEventHandler. 
		ATTENTION: Keep constants in cclapp.js in sync! */
	enum EventCode
	{
		kCreated = 0,		///< new document 
		kBeforeLoad,		///< before loading document
		kLoadFinished,		///< after load finished
		kLoadFailed,		///< document load failed
		kBeforeSave,		///< before save
		kSaveFinished,		///< after save finished
		kClose,				///< on document close
		kActivate,          ///< on document activate
		kDeactivate,        ///< on document deactivate
		kViewActivated,		///< after document view is fully visible
		kDestroyed,			///< document is going away
		kBeforeAutoSave,	///< before auto save of document (also used for: save new version, save as template)
		kAutoSaveFinished	///< after auto save of document
	};

	virtual StringRef CCL_API getTitle () const = 0;
	
	virtual UrlRef CCL_API getPath () const = 0;
	
	virtual tbool CCL_API isDirty () const = 0;

	virtual IUnknown* CCL_API getModel () const = 0;
	
	virtual IUnknown* CCL_API getView () const = 0;

	virtual IUnknown* CCL_API getController () const = 0;

	virtual IUnknown* CCL_API getMetaInfo () const = 0;

	virtual const IDocumentClass* CCL_API getIDocumentClass () const = 0;

	virtual IActionJournal* CCL_API getIActionJournal () const = 0;
	
	DECLARE_STRINGID_MEMBER (kPathChanged) ///< args[0]: old path (IUrl), args[1]: auto-save (tbool)

	DECLARE_IID (IDocument)
};

DEFINE_IID (IDocument, 0x6522e5dc, 0xa72e, 0x4b79, 0x8d, 0x50, 0xce, 0xc4, 0x2f, 0xec, 0xbb, 0x47)
DEFINE_STRINGID_MEMBER (IDocument, kPathChanged, "pathChanged")

//************************************************************************************************
// IDocumentManager
/**	\ingroup app_inter */
//************************************************************************************************

interface IDocumentManager: IUnknown
{
	enum Modes
	{
		kHidden = 1<<0, ///< do not show document
		kSilent = 1<<1, ///< document should behave silently (no dialogs)
		kForceSave = 1<<2, ///< save when document is closed
		kSafetyOptions = 1<<3, ///< show safety options before opening the document
		kOpenTemporary = 1<<4 ///< open document as temporary, so it will be deleted from disk when it's closed in "unsaved" state
	};

	// Arguments
	DECLARE_STRINGID_MEMBER (kEventHandler) ///< document event handler for openDocument ()
	DECLARE_STRINGID_MEMBER (kInitialTitle) ///< initial title for createDocument()

	// Notifications
	DECLARE_STRINGID_MEMBER (kActiveDocumentChanged)

	virtual UrlRef CCL_API getDocumentFolder () const = 0;
	
	virtual IDocument* CCL_API openDocument (UrlRef path, int mode = 0, const IAttributeList* args = nullptr) = 0;
	
	virtual IDocument* CCL_API createDocument (const FileType* fileType = nullptr, int mode = 0, const IAttributeList* args = nullptr) = 0;
	
	virtual tbool CCL_API closeDocument (IDocument* document, int mode = 0) = 0;

	virtual int CCL_API countDocuments () const = 0;

	virtual IDocument* CCL_API getIDocument (int index) const = 0;

	virtual IDocument* CCL_API getActiveIDocument () const = 0;

	virtual void CCL_API addHandler (IDocumentEventHandler* handler) = 0;

	virtual void CCL_API removeHandler (IDocumentEventHandler* handler) = 0;

	virtual IUnknownIterator* CCL_API newDocumentClassIterator () const = 0;

	/** Get document class by file type, pass empty type for default document class. */
	virtual IDocumentClass* CCL_API findIDocumentClass (const FileType& fileType) const = 0;
	
	virtual void CCL_API listRecentDocuments (IUnknownList& urls) const = 0;	

	DECLARE_STRINGID_MEMBER (kComponentName) ///< document manager component name

	DECLARE_IID (IDocumentManager)
};

DEFINE_IID (IDocumentManager, 0x1e03d0d4, 0x9fc9, 0x41f7, 0xb9, 0x3e, 0x66, 0x74, 0x1a, 0x3f, 0x1b, 0x3d)
DEFINE_STRINGID_MEMBER (IDocumentManager, kComponentName, "DocumentManager")
DEFINE_STRINGID_MEMBER (IDocumentManager, kActiveDocumentChanged, "activeDocumentChanged")
DEFINE_STRINGID_MEMBER (IDocumentManager, kEventHandler, "eventHandler")
DEFINE_STRINGID_MEMBER (IDocumentManager, kInitialTitle, "initialTitle")

//************************************************************************************************
// IDocumentEventHandler
/**	\ingroup app_inter */
//************************************************************************************************

interface IDocumentEventHandler: IUnknown
{
	/** Document manager startup/shutdown notification. */
	virtual void CCL_API onDocumentManagerAvailable (tbool state) = 0;

	/** @see IDocument::EventCode. */
	virtual void CCL_API onDocumentEvent (IDocument& document, int eventCode) = 0;

	/** Document was exported to an external file format (the document's own path & format does not change). */
	virtual void CCL_API onDocumentExported (IDocument& document, UrlRef exportPath) = 0;

	DECLARE_IID (IDocumentEventHandler)
};

DEFINE_IID (IDocumentEventHandler, 0xa6968f4a, 0x7369, 0x49fa, 0xb3, 0xcc, 0xb1, 0x4b, 0x4d, 0x5a, 0x2f, 0xb2)

//************************************************************************************************
// AbstractDocumentEventHandler
//************************************************************************************************

class AbstractDocumentEventHandler: public IDocumentEventHandler
{
public:
	void CCL_API onDocumentManagerAvailable (tbool state) override {}
	void CCL_API onDocumentEvent (IDocument& document, int eventCode) override {}
	void CCL_API onDocumentExported (IDocument& document, UrlRef exportPath) override {}
};

} // namespace CCL

#endif // _ccl_idocument_h
