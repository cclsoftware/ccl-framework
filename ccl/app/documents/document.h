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
// Filename    : ccl/app/documents/document.h
// Description : Document
//
//************************************************************************************************

#ifndef _ccl_document_h
#define _ccl_document_h

#include "ccl/base/object.h"
#include "ccl/base/storage/url.h"
#include "ccl/app/actions/iactioncontext.h"

#include "ccl/public/app/idocument.h"
#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

class StringList;
class Document;
class DocumentTemplate;
class Component;
class Renamer;

interface IDocumentView;
interface IProgressNotify;

//************************************************************************************************
// DocumentClass
//************************************************************************************************

class DocumentClass: public Object,
					 public IDocumentClass
{
public:
	DECLARE_CLASS_ABSTRACT (DocumentClass, Object)
	DECLARE_METHOD_NAMES (DocumentClass)

	enum Flags
	{
		kCanLoad = 1<<0,
		kCanSave = 1<<1,
		kNeedsFolder = 1<<2,
		kIsPrivate = 1<<3
	};

	DocumentClass (int flags = kCanLoad|kCanSave);

	void setFileType (const FileType& fileType);
	const FileType& CCL_API getFileType () const override;	///< [IDocumentClass]
	tbool CCL_API isNative () const override;				///< [IDocumentClass]

	PROPERTY_VARIABLE (int, formatVersion, FormatVersion)	///< document format version
	PROPERTY_OBJECT (FileType, templateType, TemplateType)	///< associated template file type (optional)

	PROPERTY_VARIABLE (int, flags, Flags)
	PROPERTY_FLAG (flags, kCanLoad, canLoad)
	PROPERTY_FLAG (flags, kCanSave, canSave)
	PROPERTY_FLAG (flags, kNeedsFolder, needsFolder)		///< document needs an own folder
	PROPERTY_FLAG (flags, kIsPrivate, isPrivate)		    ///< format should not be listed in GUI

	PROPERTY_STRING (subFolder, SubFolder)					///< subfolder in document folder
	PROPERTY_STRING (defaultTitle, DefaultTitle)			///< default document title
	PROPERTY_STRING (menuVariant, MenuVariant)				///< menubar variant

	virtual String makeTitle () const;						///< make title for new document

	virtual Document* createDocument () = 0;
		
	virtual DocumentTemplate* createDefaultTemplate ();
	virtual void getUserTemplateFolders (StringList& folderNames);
	virtual Component* createNewDialog (Document& document, StringID contextId); ///< create new document dialog component (optional)
	
	virtual DocumentClass* getTargetClass ();				///< specify the document to be created for import (optional, override default class)
	
	virtual void installFile (Url& path);
	virtual bool canImportFile (UrlRef path) const;
	virtual bool loadDocument (Document& document);
	virtual bool saveDocument (Document& document);
	virtual bool canSaveDocument (Document& document) const;
	virtual bool saveDocumentAs (Document& document, UrlRef path);
	virtual bool finalizeSaveDocumentAs (Document& document, UrlRef path);
	virtual bool canMergeDocuments (Document& target, UrlRef sourcePath);
	virtual bool mergeDocuments (Document& target, Document& source);

	CLASS_INTERFACE (IDocumentClass, Object)

protected:
	FileType fileType;

	// IDocumentClass
	StringRef CCL_API getSubFolderName () const override;
	tbool CCL_API isPrivateClass () const override { return isPrivate (); }

	// Object
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// Document
//************************************************************************************************

class Document: public Object,
				public IDocument,
				public IViewFactory,
				public IActionContext
{
public:
	DECLARE_CLASS (Document, Object)
	DECLARE_PROPERTY_NAMES (Document)

	Document (DocumentClass* documentClass = nullptr);
	~Document ();

	PROPERTY_SHARED (DocumentClass, documentClass, DocumentClass)
	PROPERTY_POINTER (IDocumentView, documentView, DocumentView)

	virtual void setTitle (StringRef title);
	virtual void setPath  (UrlRef path);
	virtual void setDirty (bool dirty = true);

	enum Flags
	{
		kImported				= 1<<0,	///< document has been imported
		kOlderFormat			= 1<<1,	///< document has been loaded from an older format version
		kSilent					= 1<<2,	///< suppress any warning dialogs
		kCanceled				= 1<<3,	///< document load has been canceled
		kIsAutoSave				= 1<<4,	///< auto-save in progress
		kIsSaveToNewFolder		= 1<<5,	///< save to new folder in progress
		kIsExportToNewFolder	= 1<<6,	///< export to new folder in progress
		kSafeMode				= 1<<7,	///< load with safety options
		kIsExport				= 1<<8,	///< export in progress
		kIsLoadingTemplate		= 1<<9, ///< loading from a document template in progress,
		kIgnoreDirtyUI			= 1<<10,///< do not mark document dirty if the data model is not modified
		kSavingSuspended		= 1<<11,///< document is currently not allowed to be saved
		kIsTemporary			= 1<<12 ///< document is temporary: it will be deleted from disk when closed; flag is automatically reset on save
	};

	PROPERTY_FLAG (flags, kImported, isImported)
	PROPERTY_FLAG (flags, kOlderFormat, isOlderFormat)
	PROPERTY_FLAG (flags, kSilent, isSilent)
	PROPERTY_FLAG (flags, kCanceled, isCanceled)
	PROPERTY_FLAG (flags, kIsAutoSave, isAutoSave)
	PROPERTY_FLAG (flags, kIsSaveToNewFolder, isSaveToNewFolder)
	PROPERTY_FLAG (flags, kIsExportToNewFolder, isExportToNewFolder)
	PROPERTY_FLAG (flags, kSafeMode, isSafeModeEnabled)
	PROPERTY_FLAG (flags, kIsExport, isExport)
	PROPERTY_FLAG (flags, kIsLoadingTemplate, isLoadingTemplate)
	PROPERTY_FLAG (flags, kIgnoreDirtyUI, ignoreDirtyUI)
	PROPERTY_FLAG (flags, kSavingSuspended, isSavingSuspended)
	PROPERTY_FLAG (flags, kIsTemporary, isTemporary)
	PROPERTY_MUTABLE_CSTRING (previewMode, PreviewMode)
	PROPERTY_OBJECT (Url, createdFolder, CreatedFolder)
	PROPERTY_STRING (sourceTemplateId, SourceTemplateID)

	/** Auto save. */
	bool needsAutoSave () const;
	void setAutoSavedNow ();

	/** Called after ctor (new/load/import). */
	virtual void initialize ();

	/** Called before dtor. */
	virtual void terminate ();

	/** Prepare new document. */
	virtual bool prepare (const Attributes* args = nullptr);

	/** Prepare document for import. */
	virtual bool prepareImport ();

	/** Prepare document for loading. */
	virtual bool prepareLoading ();

	/** Load document. */
	virtual bool load ();

	/** Save document. */
	virtual bool save ();

	/** Save document to new loaction. */
	virtual bool saveAs (UrlRef newPath);

	/** Prepare saving to a new folder. saveAs () will be called afterwards. */
	virtual bool prepareSaveToNewFolder (UrlRef newDocumentPath);

	/** Finish saving to a new folder. Called after saving is done. */
	virtual void finishSaveToNewFolder (UrlRef newDocumentPath);

	/** Rename the document. */
	virtual Renamer* createRenamer ();

	/** Check if document can be closed. */
	virtual bool canClose () const;

	/** Check if document folder can be removed. */
	virtual bool canRemoveFolder (UrlRef folder) const;

	/** Handle document event. */
	virtual void onEvent (int eventCode);
	void setEventHandler (IDocumentEventHandler* newHandler);
	IDocumentEventHandler* getEventHandler () const;

	virtual void onLoadFinished (bool failed);	///< kLoadFinished/kLoadFailed
	virtual void onBeforeSave ();				///< kBeforeSave
	virtual void onSaveFinished ();				///< kSaveFinished
	virtual void onActivate (bool state);		///< kActivate/kDeactivate
	virtual void onViewActivated ();			///< kViewActivated
	virtual void onClose ();					///< kClose
	virtual void onDestroyed ();				///< kDestroyed

	// IDocument
	StringRef CCL_API getTitle () const override;
	UrlRef CCL_API getPath () const override;
	tbool CCL_API isDirty () const override;
	IUnknown* CCL_API getModel () const override;
	IUnknown* CCL_API getView () const override;
	IUnknown* CCL_API getController () const override;
	IUnknown* CCL_API getMetaInfo () const override;
	const IDocumentClass* CCL_API getIDocumentClass () const override;
	IActionJournal* CCL_API getIActionJournal () const override;

	// IViewFactory
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	// IActionContext
	ActionJournal* getActionJournal () const override;

	// Object
	bool toString (String& string, int flags = 0) const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACES (Object)

	/** Helper to keep cancellation state. */
	struct CancelGuard
	{
		Document& document;
		IProgressNotify* progress;

		CancelGuard (Document& document, IProgressNotify* progress);
		~CancelGuard ();
	};

	/** Helper to suppress wait cursors on loading document previews. */
	struct SilentPreviewScope: ScopedVar<bool>
	{
		SilentPreviewScope (bool state): ScopedVar<bool> (silentPreview, state) {}
	};
	static bool isSilentPreview () {return silentPreview;}

protected:
	static bool silentPreview;

	String title;
	Url path;
	Url previousPath;
	int flags;
	bool dirty;
	bool autoSaveDirty;
	int64 lastAutoSaveTime;
	mutable ActionJournal* actionJournal;
	IDocumentEventHandler* eventHandler;

	void updateTitle ();

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// DocumentFile
//************************************************************************************************

class DocumentFile: public Document
{
public:
	DECLARE_CLASS (DocumentFile, Document)

	virtual bool load (IStream& stream);
	virtual bool save (IStream& stream);

	// Document
	bool load () override;
	bool save () override;
};

} // namespace CCL

#endif // _ccl_document_h
