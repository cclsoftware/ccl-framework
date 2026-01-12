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
// Filename    : ccl/app/documents/documentmanager.h
// Description : Document Manager
//
//************************************************************************************************

#ifndef _ccl_documentmanager_h
#define _ccl_documentmanager_h

#include "ccl/app/documents/recentdocuments.h"

#include "ccl/base/signalsource.h"

#include "ccl/public/app/idocument.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/collections/linkedlist.h"

namespace CCL {

class Document;
class DocumentClass;
class DocumentTemplate;

interface IMenuBar;
interface IFileSelector;
interface IDocumentViewFactory;
interface IDragHandler;
interface IAsyncOperation;
struct DragEvent;

//************************************************************************************************
// DocumentManager
//************************************************************************************************

class DocumentManager:	public Component,
						public IDocumentManager,
						public CommandDispatcher<DocumentManager>,
						public ComponentSingleton<DocumentManager>
{
public:
	DECLARE_CLASS (DocumentManager, Component)
	DECLARE_METHOD_NAMES (DocumentManager)

	DocumentManager (StringRef name = nullptr);
	~DocumentManager ();

	struct DirtySuspender;

	PROPERTY_BOOL (multipleDocuments, MultipleDocuments)			///< allow multiple documents?
	PROPERTY_BOOL (externalFormatsEnabled, ExternalFormatsEnabled)	///< allow external document formats?
	PROPERTY_BOOL (previewEnabled, PreviewEnabled)					///< document preview in file selector?
	PROPERTY_BOOL (saveDisabled, SaveDisabled)						///< saving documents disabled?
	PROPERTY_BOOL (newDisabled, NewDisabled)						///< creating documents disabled?
	PROPERTY_BOOL (skipDirtyCheck, SkipDirtyCheck)				    ///< save document even if it is not dirty?
	PROPERTY_BOOL (skipAskSave, SkipAskSave)				        ///< save dirty documents without asking on close?
	PROPERTY_BOOL (dirtySuspended, DirtySuspended)					///< document dirty suspended (e.g. during load)?
	PROPERTY_BOOL (asyncAlertMode, AsyncAlertMode)					///< use async alert mode, suppresses some features
	PROPERTY_BOOL (asyncLoadMode, AsyncLoadMode)					///< use async load mode
	PROPERTY_BOOL (delayOpenDeferred, DelayOpenDeferred)			///< delay a deferred openDocument

	PROPERTY_POINTER (IMenuBar, menuBar, MenuBar)
	PROPERTY_POINTER (IMenu, convertMenu, ConvertMenu)

	void addDocumentClass (DocumentClass* documentClass, bool isDefault = false);	///< add internal document class
	bool addDocumentClass (UIDRef cid, bool isDefault = false);						///< add external document class
	DocumentClass* findDocumentClass (const FileType& fileType) const;
	void findDocumentClasses (Container& result, const FileType& fileType) const;
	const Container& getDocumentClasses () const;
	
	void setViewFactory (IDocumentViewFactory* viewFactory);
	IDocumentViewFactory& getViewFactory ();

	Document* getDocument (int index) const;
	Document* getActiveDocument () const;
	bool setActiveDocument (Document* doc);
	Document* findDocument (UrlRef path) const;
	void addDocument (Document* doc);
	bool showDocument (Document* doc);
	void deferOpenDocument (UrlRef path, bool checkUpdates = false);
	bool canOpenDocument (UrlRef path) const;
	void signalDocumentEvent (Document& doc, int eventCode);
	bool closeAll ();
	
	void setDocumentFolder (UrlRef folder);
	void getDefaultDocumentFile (Url& path, const FileType& fileType) const;
	Url* findDocumentInFolder (UrlRef folder) const; ///< creates url of found document

	bool renameDocument (UrlRef path, const String* newName = nullptr);		///< shows rename dialog if newName == 0
	bool renameDocument (Document* document, bool checkOnly = false);	///< shows rename dialog
	bool renameDocument (Document* doc, StringRef newName);				///< no dialog
	bool deleteDocument (UrlRef path);
	bool saveDocument (Document* doc);
	
	RecentDocuments& getRecentPaths ();
	void updateDirtyState (const Document* document = nullptr);
	void updateApplicationTitle ();

	IDragHandler* createDragHandler (const DragEvent& event, IView* view);

	Document* openPreviewDocument (UrlRef path, StringID previewMode);
	Document* openPreviewTemplate (UrlRef path, StringID previewMode, const FileType& docFileType);
	Document* openPreviewTemplate (const DocumentTemplate& docTemplate, StringID previewMode, const FileType& docFileType);
	
	void closePreviewDocument (Document* doc);
	class PreviewLoader;

	IAsyncOperation* openDocumentAsync (UrlRef path, int mode = 0);

	// IDocumentManager
	UrlRef CCL_API getDocumentFolder () const override;
	IDocument* CCL_API openDocument (UrlRef path, int mode = 0, const IAttributeList* args = nullptr) override;
	IDocument* CCL_API createDocument (const FileType* fileType = nullptr, int mode = 0, const IAttributeList* args = nullptr) override;
	tbool CCL_API closeDocument (IDocument* document, int mode = 0) override;
	int CCL_API countDocuments () const override;
	void CCL_API addHandler (IDocumentEventHandler* handler) override;
	void CCL_API removeHandler (IDocumentEventHandler* handler) override;

	// Command Methods
	virtual bool onFileOpen (CmdArgs);
	virtual bool onFileNew (CmdArgs);
	virtual bool onFileClose (CmdArgs);
	virtual bool onFileCloseAll (CmdArgs);
	virtual bool onFileSave (CmdArgs);
	virtual bool onFileSaveAs (CmdArgs);
	virtual bool onFileSaveAs (CmdArgs args, VariantRef data);
	virtual bool onFileSaveToNewFolder (CmdArgs);
	virtual bool onFileSaveNewVersion (CmdArgs);
	virtual bool onFileRestoreVersion (CmdArgs);
	virtual bool onFileRename (CmdArgs);
	virtual bool onFileRevert (CmdArgs);
	virtual bool onEditUndo (CmdArgs);
	virtual bool onEditRedo (CmdArgs);
	virtual bool onEditDeleteHistory (CmdArgs);
	virtual bool onEditShowHistory (CmdArgs);
	virtual bool onOpenRecent (CmdArgs);
	virtual bool onClearRecent (CmdArgs);

	DECLARE_COMMANDS (DocumentManager)
	tbool CCL_API checkCommandCategory (CStringRef category) const override;

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	tbool CCL_API canTerminate () const override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	IObjectNode* CCL_API findChild (StringRef id) const override;
	tbool CCL_API getChildDelegates (IMutableArray& delegates) const override;

	CLASS_INTERFACE (IDocumentManager, Component)

protected:
	ObjectArray documentClasses;
	DocumentClass* defaultClass;
	IDocumentViewFactory* viewFactory;
	ObjectArray documents;
	Document* activeDocument;
	LinkedList<IDocumentEventHandler*> handlers;
	RecentDocuments* recentPaths;
	Url documentFolder;
	
	SignalSink documentSink;
	SignalSink systemSink;
	bool anyDocumentDirty;

	enum SaveMode { kSave, kSaveAs, kSaveToNewFolder, kExportToNewFolder };

	class DocumentLoader;
	class DocumentSaver;
	struct DocumentUsageSuspender;

	void registerFormatHandlers ();
	void unregisterFormatHandlers ();

	String& makeDocumentTitle (String& title, StringRef defaultTitle);
	void onDocumentRenamed (Document* doc, UrlRef oldPath);
	bool askDescription (String& description, bool& isIncremental);
	void prepareFilters (IFileSelector& sel, Document* document = nullptr, StringRef typeString = nullptr) const;
	DocumentClass* getDefaultClass () const;
	DocumentClass* runClassSelector (const Container& classes);
	bool closeDocument (Document& doc, bool isRevert = false, int* shouldSaveResult = nullptr);
	bool saveDocument (Document* doc, SaveMode mode, StringRef typeString = nullptr);
	void checkSaveFolder (Url& newPath, Document& doc);
	void signalFileCreated (Document& doc);
	void updateMenuBar ();
	void updateConvertMenu ();
	Document* openPreviewDocument (Document* doc, StringID previewMode);
	bool showsDirtyStateInWindowTitle () const;

	// IDocumentManager
	IDocument* CCL_API getIDocument (int index) const override;
	IDocument* CCL_API getActiveIDocument () const override;
	IUnknownIterator* CCL_API newDocumentClassIterator () const override;
	IDocumentClass* CCL_API findIDocumentClass (const FileType& fileType) const override;
	void CCL_API listRecentDocuments (IUnknownList& urls) const override;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// DocumentManager::DirtySuspender
//************************************************************************************************

struct DocumentManager::DirtySuspender
{
	DirtySuspender ()
	: wasDirtySuspended (DocumentManager::instance ().isDirtySuspended ())
	{
		DocumentManager::instance ().setDirtySuspended (true);
	}

	~DirtySuspender ()
	{
		DocumentManager::instance ().setDirtySuspended (wasDirtySuspended);
	}

	bool wasDirtySuspended;
};

//************************************************************************************************
// DocumentManager::PreviewLoader
//************************************************************************************************

class DocumentManager::PreviewLoader
{
public:
	PreviewLoader (bool slient = false);
	~PreviewLoader ();
	
	bool openDocument (UrlRef path, StringID previewMode, DocumentClass* docClass = nullptr);
	bool openTemplate (UrlRef path, StringID previewMode, const FileType& docFileType);
	bool openTemplate (const DocumentTemplate& docTemplate, StringID previewMode, const FileType& docFileType);

	void closeDocument ();

	Document* getDocument () const {return document;}
	
private:
	SharedPtr<Document> document;
	bool silent;
};

//************************************************************************************************
// DocumentStrings
//************************************************************************************************

namespace DocumentStrings
{
	StringRef ExportingX ();
	StringRef ImportingX ();
	StringRef ConvertTo ();
	StringRef OldDocumentFormatWarning ();
}

} // namespace CCL

#endif // _ccl_documentmanager_h
