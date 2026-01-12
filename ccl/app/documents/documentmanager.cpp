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
// Filename    : ccl/app/documents/documentmanager.cpp
// Description : Document Manager
//
//************************************************************************************************

#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/documents/document.h"
#include "ccl/app/documents/documentwindow.h"
#include "ccl/app/documents/autosaver.h"
#include "ccl/app/documents/documentversions.h"
#include "ccl/app/documents/documentmetainfo.h"
#include "ccl/app/documents/documenttemplates.h"

#include "ccl/app/params.h"
#include "ccl/app/paramcontainer.h"
#include "ccl/app/application.h"
#include "ccl/app/actions/actionjournal.h"
#include "ccl/app/actions/actionjournalcomponent.h"
#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/fileinfo/filepreviewcomponent.h"
#include "ccl/app/components/filerenamer.h"
#include "ccl/app/controls/draghandler.h"
#include "ccl/app/safety/appsafety.h"

#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/settings.h"

#include "ccl/main/cclargs.h"

#include "ccl/public/app/signals.h"
#include "ccl/public/app/idocumentfilter.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/text/istringdict.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/gui/framework/controlclasses.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/iapplication.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/ifilemanager.h"
#include "ccl/public/plugins/stubobject.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"

namespace CCL {

//************************************************************************************************
// ExternalDocumentClass
//************************************************************************************************

class ExternalDocumentClass: public DocumentClass
{
public:
	DECLARE_CLASS (ExternalDocumentClass, DocumentClass)

	ExternalDocumentClass (IDocumentFilter* handler = nullptr);
	~ExternalDocumentClass ();

	// DocumentClass
	tbool CCL_API isNative () const override;
	Document* createDocument () override;
	void installFile (Url& path) override;
	bool canImportFile (UrlRef path) const override;
	bool loadDocument (Document& document) override;
	bool saveDocument (Document& document) override;
	bool canSaveDocument (Document& document) const override;
	bool saveDocumentAs (Document& document, UrlRef path) override;
	bool finalizeSaveDocumentAs (Document& document, UrlRef path) override;
	bool canMergeDocuments (Document& target, UrlRef sourcePath) override;
	bool mergeDocuments (Document& target, Document& source) override;

protected:
	IDocumentFilter* handler;

	bool saveDocumentInternal (Document& document, UrlRef path);
};

//************************************************************************************************
// DocumentSelectorHook
//************************************************************************************************

class DocumentSelectorHook: public Component,
							public IFileSelectorHook
{
public:
	DocumentSelectorHook ();

	// IFileSelectorHook
	void CCL_API onSelectionChanged (IFileSelector& fs, UrlRef path) override;
	void CCL_API onFilterChanged (IFileSelector& fs, int filterIndex) override;
	void CCL_API onCustomize (IFileSelectorCustomize& fsc) override;

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	CLASS_INTERFACE (IFileSelectorHook, Component)

protected:
	FilePreviewComponent* preview;
};

//************************************************************************************************
// DocumentDragHandler
//************************************************************************************************

class DocumentDragHandler: public DragHandler
{
public:
	DocumentDragHandler (IView* view);

	// DragHandler
	tbool CCL_API drop (const DragEvent& event) override;
	IUnknown* prepareDataItem (IUnknown& item, IUnknown* context) override;
	void finishPrepare () override;

private:
	void addSprite (UrlRef path);
};

//************************************************************************************************
// DocumentEventHandlerStub
//************************************************************************************************

class DocumentEventHandlerStub: public StubObject,
								public IDocumentEventHandler
{
public:
	DECLARE_STUB_METHODS (IDocumentEventHandler, DocumentEventHandlerStub)

	// IDocumentEventHandler
	void CCL_API onDocumentManagerAvailable (tbool state) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("onDocumentManagerAvailable", state));
	}

	void CCL_API onDocumentEvent (IDocument& document, int eventCode) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("onDocumentEvent", &document, eventCode));
	}

	void CCL_API onDocumentExported (IDocument& document, UrlRef exportPath) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("onDocumentExported", &document, &exportPath));
	}
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Stub registration
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (DocumentEventHandlerStub, kFirstRun)
{
	REGISTER_STUB_CLASS (IDocumentEventHandler, DocumentEventHandlerStub)
	return true;
}

//************************************************************************************************
// DocumentManager::DocumentUsageSuspender
//************************************************************************************************

struct DocumentManager::DocumentUsageSuspender
{
	Document* document;

	DocumentUsageSuspender (Document* d) 
	: document (d)
	{
		System::GetFileManager ().setFileUsed (document->getPath (), false);
	}
	~DocumentUsageSuspender	()
	{
		System::GetFileManager ().setFileUsed (document->getPath (), true);
	}
};

//************************************************************************************************
// DocumentManager::DocumentLoader
//************************************************************************************************

class DocumentManager::DocumentLoader: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (DocumentLoader, Object)

	DocumentLoader (DocumentManager& manager, UrlRef path, int mode, StringRef fileName, const IAttributeList* args = nullptr);
	~DocumentLoader ();

	IDocument* loadDocument ();
	IAsyncOperation* loadDocumentAsync ();

private:
	DocumentManager& manager;
	Url path;
	int mode;
	const IAttributeList* args;
	AutoPtr<Document> document;
	Document* toMergeInto;
	DocumentClass* docClass;
	AutoSaver::Suspender autoSaveSuspender;
	ErrorContextGuard errorContext;
	SafetyGuard safetyGuard;
	bool canceled;
	bool alertDisplaying;
	bool usingAutoSavedFile;

	IAsyncOperation* loadDocumentInternal (bool deferred = false);
	IDocument* checkAlreadyOpen ();
	bool prepare ();
	IAsyncOperation* loadPreparedDocument (AsyncOperation* resultOperation = nullptr);

	bool checkFileExists ();
	bool assignDocumentClass ();
	bool installFile ();
	void findToMergeInto ();

	bool makeDocument ();
	void resetDocument ();
	bool displayAlert ();
	IAsyncOperation* findAutoSaved (bool emergency);
	IAsyncOperation* openFileDialog ();
	
	bool isHiddenMode () const { return (mode & kHidden) != 0;}
	bool isSilentMode () const { return (mode & kSilent) != 0;}
	bool isSafetyMode () const { return (mode & kSafetyOptions) != 0;}
	bool isTemporaryMode () const { return (mode & kOpenTemporary) != 0;}
};

//************************************************************************************************
// DocumentManager::DocumentSaver
//************************************************************************************************

class DocumentManager::DocumentSaver: public Unknown
{
public:
	static bool setImportedToNativePath (Document& document);

	DocumentSaver (DocumentManager& manager, Document* doc, SaveMode mode, StringRef typeString);

	bool saveDocument ();

private:
	DocumentManager& manager;
	Document* doc;
	DocumentClass* docClass;
	SaveMode mode;
	String typeString;
	String attemptedTitle; // title used while trying to save
	Url oldFormatPath;
	AutoPtr<TempFile> preliminaryFile;
	AutoSaver::Suspender autoSaveSuspender;
	ErrorContextGuard errorContext;
	bool result;
	bool canceled;

	bool saveWithFileSelector ();
	void finishSave ();
	void cleanup ();	
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Documents")
	XSTRING (Untitled, "Untitled")
	XSTRING (AskSaveDocument, "Do you want to save your changes in %(1)?")
	XSTRING (AskRevertDocument, "Are you sure you want to revert all changes?")
	XSTRING (FileNotFound, "The file %(1) could not be found!")
	XSTRING (AskMerge, "Do you want to merge into %(1)?")
	XSTRING (AskRemoveFromRecent, "Do you want to remove the reference to it from the Recent list?")
	XSTRING (LoadFailed, "Could not load %(1)!")
	XSTRING (FileIsBroken, "The file is broken or could not be opened.")
	XSTRING (SaveFailed, "Could not save %(1)!")
	XSTRING (FileIsInUse, "The file is in use.")
	XSTRING (FileIsWriteProtected, "You do not have write permissions at this file location.")
	XSTRING (AskSaveAsCopy, "Do you want to save a copy of %(1) as file format \"%(2)\"?")	
	XSTRING (WarnExportFormat, "The file format may not preserve all content.")
	XSTRING (UseFileFormat, "Use %(1) Format")
	XSTRING (WarnOldDocumentFormat, "The file has been created with an older version of $APPNAME. After saving the file you will not be able to load it in the old version again.\n\nAre you sure you want to continue?")
	XSTRING (CanNotCloseDocument, "Can not close %(1) right now!")
	XSTRING (Importing, "Importing %(1)...")
	XSTRING (Exporting, "Exporting %(1)...")
	XSTRING (Merging, "Merging %(1)...")
	XSTRING (Merge, "Merge")
	XSTRING (OpenX, "Open %(1):")
	XSTRING (OpenFiles, "Open files:")
	XSTRING (RenameDoc, "Rename")
	XSTRING (SaveNewVersion, "Save New Version")
	XSTRING (ConvertTo, "Convert To")
	XSTRING (AskDeleteUndoHistory, "Do you really want to delete the Undo History?\n\nThis action can not be undone.")
	XSTRING (Description, "Description")
	XSTRING (IncrementalVersion, "Incremental version")
	XSTRING (ImportAsFormat, "Import as format")
	XSTRING (OpenAction, "Open %(1)")
	XSTRING (CanNotSaveDocument, "%(1) was modified but can't be saved right now. Do you want to save a copy?")
END_XSTRINGS

//************************************************************************************************
// DocumentStrings
//************************************************************************************************

StringRef DocumentStrings::ExportingX () { return XSTR (Exporting); }
StringRef DocumentStrings::ImportingX () { return XSTR (Importing); }
StringRef DocumentStrings::ConvertTo () { return XSTR (ConvertTo); }
StringRef DocumentStrings::OldDocumentFormatWarning () { return XSTR (WarnOldDocumentFormat); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (DocumentManager)
	DEFINE_COMMAND ("File",	"New",			DocumentManager::onFileNew)
	DEFINE_COMMAND ("File",	"Open",			DocumentManager::onFileOpen)
	DEFINE_COMMAND_ARGS ("File", "Open with Options",	DocumentManager::onFileOpen, 0, "Options")
	DEFINE_COMMAND ("File",	"Close",		DocumentManager::onFileClose)
	DEFINE_COMMAND ("File",	"Close All",	DocumentManager::onFileCloseAll)
	DEFINE_COMMAND ("File",	"Save",			DocumentManager::onFileSave)
	DEFINE_COMMAND_ARGS ("File", "Save As",	DocumentManager::onFileSaveAs, 0, "Type")
	DEFINE_COMMAND_ARGS ("File",	"Save to New Folder",	DocumentManager::onFileSaveToNewFolder, 0, "Export")
	DEFINE_COMMAND_ARGS ("File", "Save New Version",DocumentManager::onFileSaveNewVersion, 0, "Description,Incremental")
	DEFINE_COMMAND ("File",	"Restore Version",		DocumentManager::onFileRestoreVersion)
	DEFINE_COMMAND ("File",	"Rename",		DocumentManager::onFileRename)
	DEFINE_COMMAND ("File",	"Revert",		DocumentManager::onFileRevert)
	DEFINE_COMMAND ("Edit",	"Undo",			DocumentManager::onEditUndo)
	DEFINE_COMMAND ("Edit",	"Redo",			DocumentManager::onEditRedo)
	DEFINE_COMMAND ("Edit",	"Delete Undo History",	DocumentManager::onEditDeleteHistory)
	DEFINE_COMMAND_("Edit",	"Undo History",	DocumentManager::onEditShowHistory, CommandFlags::kHidden)
	DEFINE_COMMAND ("Recent File", nullptr,		DocumentManager::onOpenRecent)
	DEFINE_COMMAND ("File", "Clear Recent Files",	DocumentManager::onClearRecent)
END_COMMANDS (DocumentManager)


//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum DocumentManagerTags
	{
		kActiveDocumentTitle = 100,
	};
}


//************************************************************************************************
// DocumentManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DocumentManager, Component)
DEFINE_COMPONENT_SINGLETON (DocumentManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentManager::DocumentManager (StringRef name)
: Component (name.isEmpty () ? CCLSTR (kComponentName) : name),
  documentSink (Signals::kDocumentManager),
  systemSink (Signals::kSystemInformation),
  activeDocument (nullptr),
  recentPaths (nullptr),
  menuBar (nullptr),
  convertMenu (nullptr),
  multipleDocuments (false),
  externalFormatsEnabled (false),
  saveDisabled (false),
  newDisabled (false),
  previewEnabled (false),
  skipDirtyCheck (false),
  asyncAlertMode (false),
  asyncLoadMode (false),
  dirtySuspended (false),
  skipAskSave (false),
  delayOpenDeferred (false),
  anyDocumentDirty (false),
  defaultClass (nullptr),
  viewFactory (nullptr)
{
	documentSink.setObserver (this);
	systemSink.setObserver (this);

	documentClasses.objectCleanup (true);
	documents.objectCleanup (true);
	paramList.addString ("activeDocumentTitle", Tag::kActiveDocumentTitle);
		
	addComponent (recentPaths = NEW RecentDocuments (CCLSTR ("RecentDocuments"), 250, 20, RecentDocuments::kShowFullPathInMenu));

	System::GetSystem ().getLocation (documentFolder, System::kUserContentFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentManager::~DocumentManager ()
{
	if(viewFactory)
		viewFactory->release ();

	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::addDocumentClass (DocumentClass* documentClass, bool isDefault)
{
	const FileType& fileType = documentClass->getFileType ();

	// prevent clashes of native and external document classes
	if(!documentClass->isNative ())
		ArrayForEach (documentClasses, DocumentClass, c)
			if(fileType == c->getFileType () && c->isNative ())
			{
				documentClass->release ();
				return;
			}
		EndFor
	
	if(isDefault)
		defaultClass = documentClass;

	if(documentClass->isNative ())
	{
		int insertIndex = 0; // insert before external formats
		ArrayForEach (documentClasses, DocumentClass, c)
			if(!c->isNative ())
				break;
			insertIndex++;
		EndFor
				
		if(!documentClasses.insertAt (insertIndex, documentClass)) 
			documentClasses.add (documentClass);
	}
	else
		documentClasses.add (documentClass);

	System::GetFileTypeRegistry ().registerFileType (fileType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::addDocumentClass (UIDRef cid, bool isDefault)
{
	IDocumentFilter* handler = ccl_new<IDocumentFilter> (cid);
	ASSERT (handler != nullptr)
	if(handler == nullptr)
		return false;
	
	addDocumentClass (NEW ExternalDocumentClass (handler), isDefault);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::registerFormatHandlers ()
{
	ForEachPlugInClass (PLUG_CATEGORY_DOCUMENTFILTER, desc)
		addDocumentClass (desc.getClassID ());
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::unregisterFormatHandlers ()
{
	ForEachReverse (documentClasses, DocumentClass, docClass)
		ExternalDocumentClass* externalClass = ccl_cast<ExternalDocumentClass> (docClass);
		if(externalClass)
		{
			documentClasses.remove (externalClass);
			externalClass->release ();
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::setViewFactory (IDocumentViewFactory* _viewFactory)
{
	take_shared<IDocumentViewFactory> (viewFactory, _viewFactory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDocumentViewFactory& DocumentManager::getViewFactory ()
{
	if(!viewFactory)
		viewFactory = NEW DocumentWindowFactory;
	return *viewFactory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& DocumentManager::makeDocumentTitle (String& title, StringRef defaultTitle)
{
	String titleFormat (defaultTitle);
	if(titleFormat.isEmpty ())
		titleFormat = XSTR (Untitled);
	titleFormat << "%(1)";

	int counter = 0;
	while(1)
	{
		title.empty ();
		Variant args[1];
		args[0] = ++counter;
		title.appendFormat (titleFormat, args, 1);

		bool found = false;
		ForEach (documents, Document, doc)
			if(doc->getTitle () == title)
			{
				found = true;
				break;
			}
		EndFor

		if(!found)
			break;
	}
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::prepareFilters (IFileSelector& sel, Document* document, StringRef typeString) const
{
	bool isSave = document != nullptr;
	ForEach (documentClasses, DocumentClass, docClass)
		bool verified = false;
		if(docClass->isPrivate () == false)
		{
			if(isSave && docClass->canSave ())
				verified = docClass->canSaveDocument (*document);
			else
				verified = !isSave && docClass->canLoad ();

			// optional: limit to requested file type
			if(verified && !typeString.isEmpty ())
				if(docClass->getFileType ().getExtension () != typeString)
					verified = false;
		}

		if(verified)
			sel.addFilter (docClass->getFileType ());
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentClass* DocumentManager::findDocumentClass (const FileType& fileType) const
{
	if(fileType == FileType ())
		return getDefaultClass ();

	ForEach (documentClasses, DocumentClass, docClass)
		if(docClass->getFileType () == fileType)
			return docClass;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::findDocumentClasses (Container& result, const FileType& fileType) const
{
	ForEach (documentClasses, DocumentClass, docClass)
		if(docClass->getFileType () == fileType)
			result.add (docClass);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& DocumentManager::getDocumentClasses () const
{
	return documentClasses;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentClass* DocumentManager::getDefaultClass () const
{
	if(defaultClass)
		return defaultClass;
	return (DocumentClass*)documentClasses.at (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentClass* DocumentManager::runClassSelector (const Container& classes)
{
	ASSERT (!classes.isEmpty ())

	AutoPtr<ListParam> formatList = NEW ListParam (XSTR_REF (ImportAsFormat).getKey ());

	ViewBox frame (ClassID::DialogGroup, Rect (), Styles::kDialogGroupAppearancePrimary);
	frame.setName (CCLSTR ("DocumentFormatSelector"));
	frame.setTitle (XSTR (ImportAsFormat));

	ViewBox innerFrame (ClassID::AnchorLayoutView, Rect (), Styles::kVertical);
	innerFrame.setSizeMode (IView::kFitSize);
	frame.getChildren ().add (innerFrame);
	innerFrame.getChildren ().add (ViewBox (ClassID::View, Rect (0, 0, 1, 10))); // space

	int value = 0;
	ForEach (classes, DocumentClass, docClass)
		String title (docClass->getFileType ().getDescription ());
		formatList->appendString (title);

		ControlBox radioButton (ClassID::RadioButton, formatList, Rect (), 0, title);
		radioButton.setAttribute ("value", value++);
		radioButton.autoSize ();
		innerFrame.getChildren ().add (radioButton);
	EndFor

	frame.autoSize ();
	if(DialogBox ()->runDialog (frame, Styles::kWindowCombinedStyleDialog, Styles::kOkayButton) != DialogResult::kOkay)
		return nullptr;

	return (DocumentClass*)classes.at (formatList->getValue ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Document* DocumentManager::findDocument (UrlRef path) const
{
	ForEach (documents, Document, doc)
		if(doc->getPath ().isEqualUrl (path, false))
			return doc;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* DocumentManager::openDocumentAsync (UrlRef path, int mode)
{
	String fileName;
	path.getName (fileName);
	AutoPtr<DocumentLoader> loader = NEW DocumentLoader (*this, path, mode, fileName); // loader must be released after load. Otherwise autosave remains suspended.
	return loader->loadDocumentAsync ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDocument* CCL_API DocumentManager::openDocument (UrlRef _path, int mode, const IAttributeList* args)
{
	String fileName;
	_path.getName (fileName);
	AutoPtr<DocumentLoader> loader = NEW DocumentLoader (*this, _path, mode, fileName, args);
	return loader->loadDocument ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDocument* CCL_API DocumentManager::createDocument (const FileType* fileType, int mode, const IAttributeList* _args)
{
	bool show = (mode & kHidden) == 0;
	bool silent = (mode & kSilent) != 0;
	Attributes* args = unknown_cast<Attributes> (const_cast<IAttributeList*> (_args));
	ASSERT (args || !_args)

	// if there's only one document allowed,
	// try to close the old one first:
	if(!isMultipleDocuments ())
		if(!closeAll ())
			return nullptr;
	
	DocumentClass* docClass = fileType ? findDocumentClass (*fileType) : getDefaultClass ();
	ASSERT (docClass != nullptr)
	if(!docClass)
		return nullptr;

	ScopedVar<bool> guard (dirtySuspended, true);

	AutoPtr<Document> doc = docClass->createDocument ();
	if(doc)
	{
		String initialTitle (args ? args->getString (kInitialTitle) : String ());
		if(!initialTitle.isEmpty ())
			doc->setTitle (initialTitle);

		doc->isSilent (silent);
		doc->initialize ();
		if(doc->prepare (args))
		{
			if(doc->getTitle ().isEmpty ())
			{
				String title;
				makeDocumentTitle (title, docClass->makeTitle ());
				doc->setTitle (title);
			}

			addDocument (doc);

			if(show)
				showDocument (doc);

			return doc;
		}

		doc->terminate ();
	}

	// TODO: alert?
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::addDocument (Document* doc)
{
	System::GetFileManager ().setFileUsed (doc->getPath (), true);
	documents.add (doc);
	doc->retain ();

	signalDocumentEvent (*doc, Document::kCreated);

	signal (Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentManager::closeDocument (IDocument* document, int mode)
{
	Document* doc = unknown_cast<Document> (document);
	if(!doc)
		return false;

	bool forceSave = (mode & kForceSave) != 0;

	bool result = false;
	if(forceSave)
	{
		int shouldSave = Alert::kYes;
		result = closeDocument (*doc, false, &shouldSave);
	}
	else
		result = closeDocument (*doc);
		
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::closeDocument (Document& doc, bool isRevert, int* shouldSaveResult)
{
	{
		ErrorContextGuard errorContext;
		if(!doc.canClose ())
		{
			Promise (Alert::errorWithContextAsync (String ().appendFormat (XSTR (CanNotCloseDocument), doc.getTitle ())));
			return false;
		}
	}

	if(doc.isDirty () && !isRevert && !saveDisabled)
	{
		int result = Alert::kNo;
		SaveMode saveMode = kSave;
		if(shouldSaveResult)
			result = *shouldSaveResult;
		else if(skipAskSave)
		{
			if(doc.isSavingSuspended ())
			{
				Promise (Alert::errorAsync (String ().appendFormat (XSTR (CanNotCloseDocument), doc.getTitle ())));
				return false;
			}
			result = Alert::kYes;
		}
		else
		{
			if(doc.isSavingSuspended ())
			{
				result = Alert::ask (String ().appendFormat (XSTR (CanNotSaveDocument), doc.getTitle ()), Alert::kYesNoCancel);
				if(result == Alert::kYes)
					saveMode = kSaveAs;
			}
			else
				result = Alert::ask (String ().appendFormat (XSTR (AskSaveDocument), doc.getTitle ()), Alert::kYesNoCancel);
		}

		if(result == Alert::kCancel)
				return false;
		
		if(result == Alert::kYes)
		{
			if(saveDocument (&doc, saveMode) == false && skipAskSave == false)
				return false;
		}
	}

	ScopedVar<bool> guard (dirtySuspended, true);

	System::GetFileManager ().setFileUsed (doc.getPath (), false);

	if(!documents.remove (&doc))
	{
		ASSERT (false)
		return false;
	}

	// do it before view closes to show alerts over correct document
	{
		ScopedVar<Document*> activeDocumentNuller (activeDocument, 0);
		signalDocumentEvent (doc, Document::kClose); 
	}

	if(doc.getDocumentView ())
		doc.getDocumentView ()->closeDocumentView ();

	if(&doc == getActiveDocument ())
	{
		// try to activate another one...
		Document* first = (Document*)documents.at (0);
		if(first && !isRevert)
			showDocument (first);
		else
			setActiveDocument (nullptr);
	}

	AutoSaver::instance ().removeAutoSaveFile (doc);

	doc.terminate ();

	Url createdFolder (doc.getCreatedFolder ());
	bool canRemoveFolder = createdFolder.isEmpty () == false && doc.canRemoveFolder (createdFolder);
	bool removeEmptyParentFolders = true;

	if(!canRemoveFolder && doc.isTemporary () && doc.getDocumentClass ())
	{
		ASSERT (doc.getDocumentClass ()->needsFolder ()) // not supported for single file documents yet
		if(doc.getDocumentClass ()->needsFolder ())
		{
			if(createdFolder.isEmpty ())
			{
				Url docFolder (doc.getPath ());
				docFolder.ascend ();
				createdFolder = docFolder;
			}

			canRemoveFolder = true;
			removeEmptyParentFolders = false;

			recentPaths->removeRecentPath (doc.getPath ());
		}
	}

	signalDocumentEvent (doc, Document::kDestroyed); 
	doc.release ();

	// remove doc folder if it's considered as empty
	if(canRemoveFolder)
	{
		Url docFolder (createdFolder);
		System::GetFileSystem ().removeFolder (docFolder, IFileSystem::kDeleteRecursively);

		if(removeEmptyParentFolders)
		{
			Url baseFolder (getDocumentFolder ());
			while(true)
			{
				docFolder.ascend ();
				if(docFolder != baseFolder && baseFolder.contains (docFolder) && File::isFolderEmpty (docFolder))
					System::GetFileSystem ().removeFolder (docFolder, 0);
				else
					break;
			}
		}
	}

	updateDirtyState ();

	signal (Message (kPropertyChanged));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Document* DocumentManager::openPreviewDocument (Document* doc, StringID previewMode)
{
	ASSERT (previewMode.isEmpty () == false)

	ASSERT (doc)
	if(doc == nullptr)
		return nullptr;

	DocumentClass* docClass = doc->getDocumentClass ();
	ASSERT (docClass)
	if(docClass == nullptr)
		return nullptr;

	doc->isSilent (true);	
	doc->setPreviewMode (previewMode);

	doc->initialize ();

	ErrorContextGuard errorContext;

	bool loaded = docClass->loadDocument (*doc);
	if(loaded)
		return return_shared (doc);
	else
	{
		doc->terminate ();
		
		String fileName;
		doc->getPath ().getName (fileName);
		
		if(!errorContext.hasErrors ())
			ccl_raise (XSTR (FileIsBroken));

		if(asyncAlertMode == true)
			Promise (Alert::errorWithContextAsync (String ().appendFormat (XSTR (LoadFailed), fileName)));
		else
		{
			Alert::errorWithContext (String ().appendFormat (XSTR (LoadFailed), fileName));
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Document* DocumentManager::openPreviewDocument (UrlRef path, StringID previewMode)
{
	DocumentClass* docClass = findDocumentClass (path.getFileType ());
	ASSERT (docClass)
	if(docClass == nullptr)
		return nullptr;

	AutoPtr<Document> doc = docClass->createDocument ();
	ASSERT (doc)
	if(doc == nullptr)
		return nullptr;

	doc->setPath (path);
	return openPreviewDocument (doc, previewMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Document* DocumentManager::openPreviewTemplate (const DocumentTemplate& docTemplate, StringID previewMode, const FileType& docFileType)
{
	DocumentClass* docClass = findDocumentClass (docFileType);
	if(docClass == nullptr)
		return nullptr;
			
	AutoPtr<Document> templateDoc = docClass->createDocument ();
	if(templateDoc == nullptr)
		return nullptr;

	templateDoc->setPath (docTemplate.getDataPath ());

	return openPreviewDocument (templateDoc, previewMode);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Document* DocumentManager::openPreviewTemplate (UrlRef path, StringID previewMode, const FileType& docFileType)
{
	AutoPtr<DocumentTemplate> docTemplate = DocumentTemplate::loadTemplate (path);
	if(docTemplate.isValid () == false)
		return nullptr;

	return openPreviewTemplate (*docTemplate, previewMode, docFileType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::closePreviewDocument (Document* document)
{
	if(document)
	{
		WaitCursor wc (System::GetGUI (), Document::isSilentPreview () == false);

		document->terminate ();
		
		signalDocumentEvent (*document, Document::kDestroyed); 

		document->release ();
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::signalFileCreated (Document& doc)
{
	SignalSource signalSource (Signals::kFileSystem);

	// document file
	IUrl* url = const_cast<IUrl*> (&doc.getPath ());
	signalSource.signal (Message (Signals::kFileCreated, url));

	// document folder may have also been just created
	Url path (*url);
	path.ascend ();
	signalSource.signal (Message (Signals::kFileCreated, path.asUnknown ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::checkSaveFolder (Url& newPath, Document& doc)
{
	// don't create a folder when overwriting an existing file
	if(System::GetFileSystem ().fileExists (newPath))
		return;

	Url folder (newPath);
	folder.ascend ();

	String fileName;
	String folderName;
	newPath.getName (fileName, false);
	folder.getName (folderName);

	// create a new folder with the same name as the document filename
	// exception: not when selected folder is empty and already has that name (assuming the user has created an empty matching folder in advance)
	AutoPtr<IFileIterator> iter (System::GetFileSystem ().newIterator (folder));
	bool folderIsEmpty = !iter || !iter->next ();

	bool useExistingFolder = folderIsEmpty && folderName == fileName;
	if(!useExistingFolder)
	{
		Url docFolder (doc.getPath ());
		docFolder.ascend ();
		if(folder != docFolder)
		{
			// create new folder for document
			String newFolderName;
			newPath.getName (newFolderName, false);
			folder.descend (LegalFolderName (newFolderName), Url::kFolder);
			folder.makeUnique ();

			// place document in that folder
			String docName;
			newPath.getName (docName);
			newPath = folder;
			newPath.descend (docName);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::saveDocument (Document* doc)
{
	return DocumentManager::saveDocument (doc, kSave);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::saveDocument (Document* doc, SaveMode mode, StringRef typeString)
{
	AutoPtr<DocumentSaver> saver (NEW DocumentSaver (*this, doc, mode, typeString));
	return saver->saveDocument ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::deferOpenDocument (UrlRef path, bool checkUpdates)
{
	AutoPtr<Url> pathCopy = NEW Url (path);
	AutoPtr<Message> message = NEW Message ("openDocument", pathCopy->asUnknown ());

	if(checkUpdates)
	{
		Promise p (System::GetFileManager ().triggerFileUpdate (path));
		p.then ([this, message] (IAsyncOperation& operation) 
		{			
			if(operation.getState () == IAsyncOperation::kCompleted)		
				return_shared<Message> (message)->post (this);			
		});	
	}
	else
	{	
		return_shared<Message> (message)->post (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentManager::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kActiveDocumentTitle:
		if(Document* doc = getActiveDocument ())
		{
			renameDocument (doc, param->getValue ().asString ());
			param->fromString (doc->getTitle ()); // back to old name if failed, or get possibly legalized name
		}
		return true;
	}
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kDocumentDirty)
	{
		if(dirtySuspended) // avoid during load/create/close
			return;

		Document* document = nullptr;
		if(msg.getArgCount () >= 1)
			document = unknown_cast<Document> (msg[0].asUnknown ());

		if(document == nullptr)
			document = getActiveDocument ();

		if(document)
			document->setDirty ();
	}
	else if(msg == Signals::kContentLocationChanged)
	{
		UnknownPtr<IUrl> path (msg[0]);
		UnknownPtr<IUrl> oldPath (msg[1]);
		ASSERT (path != nullptr)
		setDocumentFolder (*path);

		if(path && oldPath)
			recentPaths->relocate (*oldPath, *path);
	}
	else if(msg == IApplication::kAppSuspended || msg == IApplication::kAppTerminates || msg == IApplication::kAppDeactivated)
	{
#if CCL_PLATFORM_DESKTOP
		if(msg == IApplication::kAppDeactivated)
			return;
#endif		
		if(isSaveDisabled () == false)
		{
			if(Document* activeDoc = getActiveDocument ())
			{
				if(activeDoc->isDirty ())
				{
					if(isSkipAskSave ()) // save now without asking
						saveDocument (activeDoc);
					else   // auto save now 
						AutoSaver::instance ().doSave (*activeDoc);					
				}		
			}

			if(msg == IApplication::kAppTerminates)
			{
				if(skipAskSave == false)
					setSaveDisabled (true);		
			}
		}
	}
	else if(msg == "openDocument")
	{
		if(delayOpenDeferred)
		{
			(NEW Message (msg))->post (this, 25);		
		}
		else
		{
			UnknownPtr<IUrl> path = msg.getArg (0).asUnknown ();
			if(path)
			{
				if(isAsyncLoadMode ())
					Promise (openDocumentAsync (*path));
				else
					openDocument (*path);
			}
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::closeAll ()
{
	while(!documents.isEmpty ())
	{
		Document* doc = (Document*)documents.at (0);
		if(!closeDocument (*doc))
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
UrlRef DocumentManager::getDocumentFolder () const
{
	return documentFolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::setDocumentFolder (UrlRef folder)
{
	System::GetFileManager ().setFileUsed (documentFolder, false);
	documentFolder = folder;
	System::GetFileManager ().setFileUsed (folder, true);
		
	// create subfolders for document classes
	ForEach (documentClasses, DocumentClass, docClass)
		if(!docClass->getSubFolder ().isEmpty ())
		{
			Url classFolder (documentFolder);
			classFolder.descend (docClass->getSubFolder (), Url::kFolder);
			if(!System::GetFileSystem ().fileExists (classFolder))
				System::GetFileSystem ().createFolder (classFolder);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::getDefaultDocumentFile (Url& path, const FileType& fileType) const
{
	path.assign (documentFolder);

	DocumentClass* docClass = findDocumentClass (fileType);
	ASSERT (docClass != nullptr)
	if(docClass)
	{
		if(!docClass->getSubFolder ().isEmpty ())
			path.descend (docClass->getSubFolder (), Url::kFolder);
	}

	path.descend (CCLSTR ("default"));
	path.setFileType (fileType, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url* DocumentManager::findDocumentInFolder (UrlRef folder) const
{
	if(folder.isFolder ())
	{
		String folderName;
		folder.getName (folderName);

		// try folder based document classes
		ForEach (documentClasses, DocumentClass, docClass)
			if(docClass->needsFolder ())
			{
				Url fileInside (folder);
				fileInside.descend (folderName);
				fileInside.setExtension (docClass->getFileType ().getExtension (), false);
				if(System::GetFileSystem ().fileExists (fileInside))
					return NEW Url (fileInside);
			}
		EndFor
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::renameDocument (UrlRef path, const String* newName)
{
	AutoPtr<Document> tempDoc;

	Document* doc = findDocument (path);

	// if not already open, try to create a temporary document object for renaming
	if(!doc) 
		if(DocumentClass* docClass = findDocumentClass (path.getFileType ()))
			if(tempDoc = doc = docClass->createDocument ())
			{
				tempDoc->isSilent (true);
				tempDoc->initialize ();
				tempDoc->setPath (path);
			}

	if(doc)
	{
		bool result = newName ? renameDocument (doc, *newName) : renameDocument (doc);

		if(tempDoc)
			tempDoc->terminate (); // note: we don't save it

		return result;
	}
	else
	{
		// fallback: only rename the file
		AutoPtr<FileRenamer> renamer (NEW FileRenamer (path, false));

		Url oldPath (path);

		bool renamed = newName
			? renamer->tryRename (*newName)
			: renamer ->runDialog (XSTR (RenameDoc));

		if(renamed)
		{
			AutoPtr<Url> newPath (renamer->createNewPath ());
			if(recentPaths->removeRecentPath (oldPath))
				recentPaths->setRecentPath (*newPath);

			SignalSource signalSource (Signals::kFileSystem);
			signalSource.signal (Message (Signals::kFileCreated, Variant (newPath->asUnknown ())));
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::renameDocument (Document* doc, bool checkOnly)
{
	AutoPtr<Renamer> renamer = doc->createRenamer ();
	if(!renamer || !renamer->canRenameNow ())
		return false;

	if(!checkOnly)
	{
		AutoSaver::Suspender suspender;
		DocumentUsageSuspender usageSuspender (doc);

		Url oldPath (doc->getPath ());
		if(renamer->runDialog (XSTR (RenameDoc)))
			onDocumentRenamed (doc, oldPath);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::renameDocument (Document* doc, StringRef newName)
{
	AutoPtr<Renamer> renamer = doc->createRenamer ();
	if(!renamer || !renamer->canRenameNow ())
		return false;

	AutoSaver::Suspender suspender;
	DocumentUsageSuspender usageSuspender (doc);

	Url oldPath (doc->getPath ());
	if(renamer->tryRename (newName))
	{
		onDocumentRenamed (doc, oldPath);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::onDocumentRenamed (Document* doc, UrlRef oldPath)
{
	bool isActive = doc == getActiveDocument ();
	if(isActive)
		updateApplicationTitle ();

	if(recentPaths->removeRecentPath (oldPath))
		recentPaths->setRecentPath (doc->getPath ());

	signalFileCreated (*doc);
	if(isActive)
	{
		signalDocumentEvent (*doc, Document::kDeactivate);
		signalDocumentEvent (*doc, Document::kActivate);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::deleteDocument (UrlRef path)
{
	bool canRemoveFolder = false;
	Url docFolder (path);
	docFolder.ascend ();

	Document* openDoc = findDocument (path);
	if(openDoc)
	{
		canRemoveFolder = openDoc->getDocumentClass () && openDoc->getDocumentClass ()->needsFolder () && openDoc->canRemoveFolder (docFolder);
		closeDocument (*openDoc);
	}

	File file (path);
	file.signalRelease ();
	if(System::GetFileSystem ().removeFile (path, IFileSystem::kDeleteToTrashBin))
	{
		file.signalRemoved ();
		getRecentPaths ().removeRecentPath (path);

		if(!openDoc)
		{
			// create temporary document object for canRemoveFolder
			DocumentClass* docClass = findDocumentClass (path.getFileType ());
			if(docClass && docClass->needsFolder ())
			{
				AutoPtr<Document> tempDoc  = docClass->createDocument ();
				if(tempDoc)
				{
					tempDoc->setPath (path);
					tempDoc->isSilent (true);
					tempDoc->initialize ();

					canRemoveFolder = tempDoc->canRemoveFolder (docFolder);

					tempDoc->terminate ();
					tempDoc.release ();
				}
			}
		}

		if(canRemoveFolder)
			System::GetFileSystem ().removeFolder (docFolder, IFileSystem::kDeleteRecursively);

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RecentDocuments& DocumentManager::getRecentPaths ()
{
	ASSERT (recentPaths)
	return *recentPaths;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentManager::initialize (IUnknown* context)
{
	System::GetSystem ().getLocation (documentFolder, System::kUserContentFolder);
    
	documentSink.enable (true);
	systemSink.enable (true);

	recentPaths->restore ();

	if(isExternalFormatsEnabled ())
		registerFormatHandlers ();

	// update menus
	updateConvertMenu ();

	// startup notification
	ListForEach (handlers, IDocumentEventHandler*, handler)
		handler->onDocumentManagerAvailable (true);
	EndFor

	ISubject::addObserver (&System::GetGUI (), this);

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentManager::terminate ()
{
	ISubject::removeObserver (&System::GetGUI (), this);

	// shutdown notification
	ListForEach (handlers, IDocumentEventHandler*, handler)
		handler->onDocumentManagerAvailable (false);
	EndFor

	documentSink.enable (false);
	systemSink.enable (false);

	recentPaths->store ();
	ASSERT (!recentPaths->hasMenus ())

	// all documents must be closed before!
	ASSERT (documents.isEmpty () == true)

	unregisterFormatHandlers ();

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentManager::canTerminate () const
{
	// try to close all documents
	if(!const_cast<DocumentManager*> (this)->closeAll ())
		return false;

	return SuperClass::canTerminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::canOpenDocument (UrlRef path) const
{
	if(path.isFile () && System::GetFileSystem ().fileExists (path))
		if(DocumentClass* documentClass = findDocumentClass (path.getFileType ()))
			return documentClass->canImportFile (path);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* DocumentManager::createDragHandler (const DragEvent& event, IView* view)
{
	AutoPtr<DocumentDragHandler> handler (NEW DocumentDragHandler (view));
	if(handler->prepare (event.session.getItems (), &event.session))
	{
		event.session.setResult (IDragSession::kDropCopyReal);
		handler->retain ();
		return handler;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* DocumentManager::findChild (StringRef id) const
{
	if(id == CCLSTR ("ActiveDocument") && activeDocument)
	{
		IObjectNode* result = UnknownPtr<IObjectNode> (activeDocument->getController ());
		return result;
	}
	return SuperClass::findChild (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentManager::getChildDelegates (IMutableArray& delegates) const
{
	delegates.addArrayElement (CCLSTR ("ActiveDocument"));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Document* DocumentManager::getDocument (int index) const
{
	return (Document*)documents.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Document* DocumentManager::getActiveDocument () const
{
	return activeDocument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::setActiveDocument (Document* doc)
{
	if(doc == activeDocument)
		return false;

	if(activeDocument)
		signalDocumentEvent (*activeDocument, Document::kDeactivate);
		
	activeDocument = doc;

	if(activeDocument)
	{
		// bring to top
		ASSERT (documents.contains (doc))
		documents.remove (doc);
		documents.insertAt (0, doc);
		
		signalDocumentEvent (*activeDocument, Document::kActivate);
	}

	updateApplicationTitle ();
	updateMenuBar ();
	signal (Message (kActiveDocumentChanged));
	signalPropertyChanged ("hasActiveDocument");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::showDocument (Document* doc)
{
	if(!doc)
		return false;

	IDocumentView* view = doc->getDocumentView ();
	if(view == nullptr)
		view = getViewFactory ().createDocumentView (*doc);

	ASSERT (view != nullptr)
	if(view)
	{
		view->activateDocumentView ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::showsDirtyStateInWindowTitle () const
{
	#if CCL_PLATFORM_WINDOWS || CCL_PLATFORM_LINUX
	return !isSkipAskSave ();
	#else
	return false;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::updateDirtyState (const Document* document)
{
	bool anyDirty = false;

	if(showsDirtyStateInWindowTitle () && document == getActiveDocument ())
		updateApplicationTitle ();

	if(document && document->isDirty ())
		anyDirty = true;
	else
		for(auto doc : iterate_as<Document> (documents))
			if(doc->isDirty ())
			{
				anyDirty = true;
				break;
			}

	if(anyDirty != anyDocumentDirty)
	{
		anyDocumentDirty = anyDirty;

		// indicate in application window
		if(IWindow* appWindow = System::GetDesktop ().getApplicationWindow ())
			UnknownPtr<IObject> (appWindow)->setProperty (IWindow::kDocumentDirty, anyDocumentDirty);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::updateApplicationTitle ()
{
	IWindow* appWindow = System::GetDesktop ().getApplicationWindow ();
	if(appWindow)
	{
		const IUrl* path = nullptr;
		String appTitle (RootComponent::instance ().getApplicationTitle ());
		if(Document* document = getActiveDocument ())
		{
			appTitle.append (CCLSTR (" - "));
			appTitle.append (document->getTitle ());

			if(showsDirtyStateInWindowTitle () && document->isDirty ())
				appTitle.append (CCLSTR ("*"));

			path = &document->getPath ();
		}

		appWindow->setWindowTitle (appTitle);
		if(path)
			// if file does not (yet) exist, hide the file icon
			if(!System::GetFileSystem ().fileExists (*path))
				path = nullptr;
		UnknownPtr<IObject> (appWindow)->setProperty (IWindow::kRepresentedFile, const_cast<IUrl*> (path));
	}

	paramList.byTag (Tag::kActiveDocumentTitle)->fromString (activeDocument ? activeDocument->getTitle () : String::kEmpty);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::updateMenuBar ()
{
	UnknownPtr<IVariantMenuBar> variantBar (menuBar);
	if(variantBar == nullptr)
		return;

	String variant;
	if(activeDocument)
		variant = activeDocument->getDocumentClass ()->getMenuVariant ();

	variantBar->setVariant (variant);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::updateConvertMenu ()
{
	if(!convertMenu)
		return;

	convertMenu->removeAll ();

	Vector<FileType> fileTypes;
	Vector<String> descriptonsDone;
	ForEach (documentClasses, DocumentClass, documentClass)
		if(!documentClass->isPrivate () && documentClass->canSave ())
		{
			// filter duplicates using the same description but different file extension
			auto& fileType = documentClass->getFileType ();
			if(descriptonsDone.contains (fileType.getDescription ()))
				continue;
			descriptonsDone.add (fileType.getDescription ());

			struct Sorter
			{
				static DEFINE_VECTOR_COMPARE_OBJECT (compareDescription, FileType, lhs, rhs)
					return lhs->getDescription ().compare (rhs->getDescription ());
				}
			};

			fileTypes.addSorted (fileType, Sorter::compareDescription);
		}
	EndFor

	for(auto& fileType : fileTypes)
	{
		convertMenu->addCommandItem (String () << fileType.getDescription () << IMenu::strFollowIndicator,
									CSTR ("File"), CSTR ("Save As"),
									CommandDelegate<DocumentManager>::make (this, &DocumentManager::onFileSaveAs, fileType.getExtension ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DocumentManager::countDocuments () const
{
	return documents.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDocument* CCL_API DocumentManager::getIDocument (int index) const
{
	return getDocument (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDocument* CCL_API DocumentManager::getActiveIDocument () const
{
	return getActiveDocument ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentManager::addHandler (IDocumentEventHandler* handler)
{
	handlers.append (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentManager::removeHandler (IDocumentEventHandler* handler)
{
	handlers.remove (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API DocumentManager::newDocumentClassIterator () const
{
	return documentClasses.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDocumentClass* CCL_API DocumentManager::findIDocumentClass (const FileType& fileType) const
{
	if(fileType.isValid () == false) // use default class
		return getDefaultClass ();
	else
		return findDocumentClass (fileType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentManager::listRecentDocuments (IUnknownList& urls) const
{	
	if(recentPaths)
	{
		for(int i = 0; i < recentPaths->count (); i++)
		{
			const IUrl* url = recentPaths->at (i);
			urls.add (const_cast<IUrl*> (url), true);
		}
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::signalDocumentEvent (Document& doc, int eventCode)
{
	if(eventCode == Document::kBeforeSave || eventCode == Document::kBeforeAutoSave)
		System::GetFileManager ().setFileWriting (doc.getPath (), true);

	else if(eventCode == Document::kSaveFinished || eventCode == Document::kAutoSaveFinished)
		System::GetFileManager ().setFileWriting (doc.getPath (), false);
	
	bool after = eventCode == Document::kDeactivate ||
				 eventCode == Document::kSaveFinished ||
				 eventCode == Document::kAutoSaveFinished;

	if(!after)
		doc.onEvent (eventCode);

	ListForEach (handlers, IDocumentEventHandler*, handler)
		handler->onDocumentEvent (doc, eventCode);
	EndFor

	if(after)
		doc.onEvent (eventCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentManager::checkCommandCategory (CStringRef category) const
{
	if(category == "File" || category == "Edit" || category == "Recent File")
		return true;

	// ask active document controller...
	Document* doc = getActiveDocument ();
	if(doc && doc->getDocumentView () && doc->getDocumentView ()->isDocumentVisible ())
	{
		UnknownPtr<ICommandHandler> docHandler (doc->getController ());
		if(docHandler && docHandler->checkCommandCategory (category))
			return true;
	}

	return SuperClass::checkCommandCategory (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentManager::interpretCommand (const CommandMsg& msg)
{
	if(CommandDispatcher<DocumentManager>::dispatchCommand (msg))
		return true;

	// ask active document controller...
	Document* doc = getActiveDocument ();
	if(doc && doc->getDocumentView () && doc->getDocumentView ()->isDocumentVisible ())
	{
		UnknownPtr<ICommandHandler> docHandler (doc->getController ());
		if(docHandler && docHandler->interpretCommand (msg))
			return true;
	}

	return SuperClass::interpretCommand (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onFileOpen (CmdArgs args)
{
	if(args.checkOnly ())
		return true;

	if(System::GetDesktop ().closePopupAndDeferCommand (this, args))
		return true;

	AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);
	if(!fs)
		return false;
		
	fs->setFolder (documentFolder);

	if(isPreviewEnabled ())
	{
		AutoPtr<IFileSelectorHook> hook (NEW DocumentSelectorHook);
		fs->setHook (hook);
	}

	ASSERT (fs != nullptr)
	prepareFilters (*fs);

	bool openWithOptions = args.name.contains ("Options");
	int mode = openWithOptions ? kSafetyOptions : 0;

	Promise fsPromise (fs->runAsync (IFileSelector::kOpenFile));
	fsPromise.then ([this, fs, mode] (IAsyncOperation& operation) 
	{
		if(operation.getResult ().asBool ())
			if(const IUrl* path = fs->getPath ())
			{
				if(isAsyncLoadMode ())
				{
					Promise (openDocumentAsync (*path));
				}
				else
					openDocument (*fs->getPath (), mode);
			}
	});
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onOpenRecent (CmdArgs args)
{
	if(args.checkOnly ())
		return true;
	
	bool openWithOptions = args.name.contains ("Options");
	int mode = openWithOptions ? kSafetyOptions : 0;

	int64 index = 0;
	args.name.getIntValue (index);
	if(index >= 1)
	{
		const Url* path = recentPaths->at ((int)index - 1);
		if(path)
			openDocument (*path, mode);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onClearRecent (CmdArgs args)
{
	if(recentPaths->count () < 1)
		return false;

	if(!args.checkOnly ())
		recentPaths->clearAll ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onFileNew (CmdArgs args)
{
	if(newDisabled)
		return false;
	
	if(args.checkOnly ())
		return getDefaultClass () != nullptr;

	if(System::GetDesktop ().closePopupAndDeferCommand (this, args))
		return true;

	createDocument ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onFileClose (CmdArgs args)
{
	static bool isClosing = false;
	if(isClosing)
		return false;

	ScopedVar<bool> guard (isClosing, true);

	if(args.checkOnly ())
		return getActiveDocument () != nullptr;

	if(System::GetDesktop ().closePopupAndDeferCommand (this, args))
		return true;

	// check dirty here in case platform requires async alerts
	if(asyncAlertMode == true)
		if(Document* doc = getActiveDocument ())
			if(doc->isDirty () && saveDisabled == false && skipAskSave == false)
			{
				String message;
				message.appendFormat (XSTR (AskSaveDocument), doc->getTitle ());
				Promise (Alert::askAsync (message, Alert::kYesNoCancel)).then ([this] (IAsyncOperation& operation)
				{
					int shouldSaveResult = operation.getResult ().asInt ();
					if(getActiveDocument ())
						closeDocument (*getActiveDocument (), false, &shouldSaveResult);
				});
				return true;
			}

	if(getActiveDocument ())
		closeDocument (*getActiveDocument ());

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onFileCloseAll (CmdArgs args)
{
	if(documents.isEmpty ())
		return false;

	if(!args.checkOnly ())
	{
		if(System::GetDesktop ().closePopupAndDeferCommand (this, args))
			return true;

		closeAll ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onFileSave (CmdArgs args)
{
	if(saveDisabled)
		return false;

	auto document = getActiveDocument ();
	if(document == nullptr)
		return false;

	if(document->isSavingSuspended ())
		return false;

	if(document->isDirty () || skipDirtyCheck)
	{
		if(!args.checkOnly ())
			saveDocument (document);

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onFileSaveAs (CmdArgs args)
{
	if(saveDisabled)
		return false;

	if(!getActiveDocument ())
		return false;

	String typeString;
	CommandAutomator::Arguments (args).getString ("Type", typeString);

	if(args.checkOnly ())
	{
		if(!typeString.isEmpty ())
		{
			auto documentClass = findDocumentClass (FileType (nullptr, MutableCString (typeString)));
			return documentClass && documentClass->canSaveDocument (*getActiveDocument ());
		}
		else
			return true;
	}
	else
	{
		saveDocument (getActiveDocument (), kSaveAs, typeString);
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onFileSaveAs (CmdArgs args, VariantRef data)
{
	Attributes commandArgs;
	commandArgs.set ("Type", data.asString ());	
	CommandMsg args2 (args);
	args2.invoker = static_cast<IAttributeList*> (&commandArgs);
	return onFileSaveAs (args2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onFileSaveToNewFolder (CmdArgs args)
{
	if(saveDisabled)
		return false;

	if(args.checkOnly ())
		return getActiveDocument () != nullptr;

	bool isExport = false;
	CommandAutomator::Arguments (args).getBool ("Export", isExport);
	SaveMode mode = isExport ? kExportToNewFolder : kSaveToNewFolder;

	if(getActiveDocument ())
		saveDocument (getActiveDocument (), mode);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::askDescription (String& description, bool& isIncremental)
{
	ParamContainer params;
	IParameter* descriptionParam = params.addString (XSTR_REF (Description).getKey ());
	descriptionParam->fromString (description);

	IParameter* incrementalParam = params.addParam (XSTR_REF (IncrementalVersion).getKey ());
	incrementalParam->setValue (isIncremental);
	
	ITheme* theme = getTheme ();
	ASSERT (theme != nullptr)
	IView* dialogView = theme ? theme->createView ("CCL/SaveNewVersionDialog", params.asUnknown ()) : nullptr;

	int answer = dialogView
		? DialogBox ()->runDialog (dialogView)
		: DialogBox ()->runWithParameters (CCLSTR ("DocumentDescriptionDialog"), params, XSTR (SaveNewVersion));

	if(answer != DialogResult::kOkay)
		return false;

	description = descriptionParam->getValue ().asString ();
	isIncremental = incrementalParam->getValue ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onFileSaveNewVersion (CmdArgs args)
{
	if(saveDisabled)
		return false;

	Document* doc = getActiveDocument ();
	if(doc == nullptr)
		return false;

	if(args.checkOnly ())
		return true;

	UnknownPtr<IAttributeList> metaAttribs (doc->getMetaInfo ());
	String description, oldDescription;
	String metaTitle;
	if(metaAttribs)
	{
		description = oldDescription = DocumentMetaInfo (*metaAttribs).getDescription ();
		metaTitle = DocumentMetaInfo (*metaAttribs).getTitle ();
	}

	bool isIncremental = false; // 1. default
	Settings::instance ().getAttributes ("DocumentVersions").getBool (isIncremental, "Incremental"); // 2. last dialog state
	CommandAutomator::Arguments (args).getBool ("Incremental", isIncremental); // 3. argument can override

	// ask for description if not provided as argument
	if(!CommandAutomator::Arguments (args).getString ("Description", description))
	{
		if(!DocumentManager::askDescription (description, isIncremental))
			return true;

		Settings::instance ().getAttributes ("DocumentVersions").set ("Incremental", isIncremental);
	}

	Url oldPath (doc->getPath ());
	String oldTitle (doc->getTitle ());
	bool wasDirty = doc->isDirty () != 0;

	if(metaAttribs)
		DocumentMetaInfo (*metaAttribs).setDescription (description);

	DocumentUsageSuspender usageSuspender (doc);	

	DocumentVersions versions (doc->getPath ());
	Url newDocumentPath;

	if(!isIncremental)
	{
		// save the current (possibly modified) state in history
		Url historyPath;
		versions.makeVersionPath (historyPath);
		doc->setPath (historyPath);
	}
	else
	{ 
		// make version path right in document folder
		newDocumentPath = versions.makeVersionPathInDocumentFolder (oldPath); // already uses the new description
		doc->setPath (newDocumentPath);
	}

	if(!isIncremental)
		doc->setTitle (metaTitle);

	signalDocumentEvent (*doc, Document::kBeforeAutoSave);
	bool result = doc->save ();	// don't call saveAs, it might do too much
	signalDocumentEvent (*doc, Document::kAutoSaveFinished);
	if(result)
		signalFileCreated (*doc);

	if(!isIncremental)
	{
		// continue working on the original file
		if(metaAttribs)
		{
			DocumentMetaInfo (*metaAttribs).setDescription (oldDescription);
			DocumentMetaInfo (*metaAttribs).setTitle (metaTitle);
		}
		doc->setPath (oldPath);
		doc->setTitle (oldTitle);
	}
	else
	{
		// keep the saved file as active document; move old document to history
		if(result && oldPath != newDocumentPath)
		{
			versions.moveDocumentVersionToHistory ();
			versions.onActiveVersionChanged (oldPath, newDocumentPath);
			updateApplicationTitle ();
		}
	}

	if(wasDirty)
		doc->setDirty (true);
	doc->setAutoSavedNow ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onFileRestoreVersion (CmdArgs args)
{
	Document* doc = getActiveDocument ();
	if(doc == nullptr)
		return false;

	if(args.checkOnly ())
		return true;

	if(System::GetDesktop ().closePopupAndDeferCommand (this, args))
		return true;

	DocumentVersionSelector ().runDialog (*doc);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onFileRename (CmdArgs args)
{
	if(saveDisabled)
		return false;

	auto document = getActiveDocument ();
	if(document == nullptr)
		return false;

	if(document->isSavingSuspended ())
		return false;

	return renameDocument (document, args.checkOnly ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onFileRevert (CmdArgs args)
{
	Document* doc = getActiveDocument ();
	bool canRevert = doc && doc->isDirty () && !doc->getPath ().isEmpty ();

	if(args.checkOnly ())
		return canRevert;

	if(canRevert)
	{
		if(System::GetDesktop ().closePopupAndDeferCommand (this, args))
			return true;

		if(System::GetFileSystem ().fileExists (doc->getPath ()))
		{
			if(Alert::ask (XSTR (AskRevertDocument), Alert::kYesNo) == Alert::kYes)
			{
				Url path (doc->getPath ());
				closeDocument (*doc, true);
				openDocument (path);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onEditUndo (CmdArgs args)
{
	Document* doc = getActiveDocument ();
	ActionJournal* journal = doc ? doc->getActionJournal () : nullptr;

	if(args.checkOnly ())
	{
		// update title if it is a menu item...
		IMenuItem* menuItem = UnknownPtr<IMenuItem> (args.invoker);
		if(menuItem)
		{
			String title;
			ActionJournal::getUndoString (title, journal);
			menuItem->setItemAttribute (IMenuItem::kItemTitle, title);
		}
	}

	if(!journal)
		return false;

	// disable if user can't see the document! 
	if(doc->getDocumentView () && !doc->getDocumentView ()->isDocumentVisible ())
		return false;

	if(!journal->canUndo ())
		return false;

	if(!args.checkOnly ())
		journal->undo ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onEditRedo (CmdArgs args)
{
	Document* doc = getActiveDocument ();
	ActionJournal* journal = doc ? doc->getActionJournal () : nullptr;

	if(args.checkOnly ())
	{
		// update title if it is a menu item...
		IMenuItem* menuItem = UnknownPtr<IMenuItem> (args.invoker);
		if(menuItem)
		{
			String title;
			ActionJournal::getRedoString (title, journal);
			menuItem->setItemAttribute (IMenuItem::kItemTitle, title);
		}
	}

	if(!journal)
		return false;

	// disable if user can't see the document! 
	if(doc->getDocumentView () && !doc->getDocumentView ()->isDocumentVisible ())
		return false;

	if(!journal->canRedo ())
		return false;

	if(!args.checkOnly ())
		journal->redo ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onEditDeleteHistory (CmdArgs args)
{
	Document* doc = getActiveDocument ();
	ActionJournal* journal = doc ? doc->getActionJournal () : nullptr;
	if(!journal)
		return false;

	// disable if user can't see the document! 
	if(doc->getDocumentView () && !doc->getDocumentView ()->isDocumentVisible ())
		return false;

	bool canDelete = journal->canUndo () || journal->canRedo ();
	if(!canDelete)
		return false;

	if(!args.checkOnly ())
	{
		if(Alert::ask (XSTR (AskDeleteUndoHistory)) == Alert::kYes)
			journal->removeAll ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::onEditShowHistory (CmdArgs args)
{
	Document* doc = getActiveDocument ();
	
	if(ActionJournal* journal = doc ? doc->getActionJournal () : nullptr)
	{
		// disable if user can't see the document! 
		if(doc->getDocumentView () && !doc->getDocumentView ()->isDocumentVisible ())
			return false;

		bool canDelete = journal->canUndo () || journal->canRedo ();
		if(!canDelete)
			return false;

		if(!args.checkOnly ())
			ActionJournalComponent (*journal).runDialog ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentManager::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "saveDisabled")
	{
		var = isSaveDisabled ();
		return true;
	}
	else if(propertyId == "activeDocument")
	{
		var = ccl_as_unknown (activeDocument);
		return true;
	}
	else if(propertyId == "hasActiveDocument")
	{
		var = (activeDocument != nullptr);
		return true;
	}
	else if(propertyId == "documentCount")
	{
		var = countDocuments ();
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (DocumentManager)
	DEFINE_METHOD_ARGR ("newDocumentClassIterator", "", "Iterator")
END_METHOD_NAMES (DocumentManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentManager::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "newDocumentClassIterator")
	{
		AutoPtr<Iterator> iter;
		if(Iterator* it = unknown_cast<Iterator> (newDocumentClassIterator ()))
			iter = NEW HoldingIterator (this, it);
		returnValue.takeShared (ccl_as_unknown (iter));
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// DocumentManager::DocumentLoader
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DocumentManager::DocumentLoader, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentManager::DocumentLoader::DocumentLoader (DocumentManager& manager, UrlRef path, int mode, StringRef fileName, const IAttributeList* args)
: manager (manager),
  path (path),
  safetyGuard (SafetyID::kOpenDocumentAction, {fileName}),
  docClass (nullptr),
  mode (mode),
  toMergeInto (nullptr),
  canceled (false),
  alertDisplaying (false),
  usingAutoSavedFile (false),
  args (args)
{
	manager.dirtySuspended = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentManager::DocumentLoader::~DocumentLoader ()
{
	manager.dirtySuspended = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDocument* DocumentManager::DocumentLoader::loadDocument ()
{
	AutoPtr<IAsyncOperation> op (loadDocumentInternal ());
	if(op->getState () != IAsyncOperation::kCompleted)
		return nullptr;

	UnknownPtr<IDocument> doc (op->getResult ().asUnknown ());
	return doc;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* DocumentManager::DocumentLoader::loadDocumentAsync ()
{
	return loadDocumentInternal (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* DocumentManager::DocumentLoader::loadDocumentInternal (bool deferred)
{
	if(IDocument* doc = checkAlreadyOpen ())
		return AsyncOperation::createCompleted (doc);

	// clear old errors
	int errorCode = 0;
	System::GetFileSystem ().getFirstError (errorCode);

	AutoPtr<AsyncStepMachine> stepMachine = NEW AsyncStepMachine;
	AsyncStep* prepareStep = stepMachine->createStep ();
	AsyncStep* loadStep = stepMachine->createStep ();
	AsyncStep* tryAutosaved = stepMachine->createStep ();
	AsyncStep* handleErrorStep = stepMachine->createStep ();

	SharedPtr<DocumentLoader> me (this);

	prepareStep->onStart ([me, deferred]() -> IAsyncOperation*
	{
		if(me->prepare ())
			return AsyncOperation::createCompleted (0, deferred);

		return AsyncOperation::createFailed (deferred);
	});
	prepareStep->onCompletion ([loadStep, handleErrorStep](IAsyncOperation& op)
	{
		if(op.getState () == IAsyncInfo::kCompleted)
			loadStep->start ();
		else
			handleErrorStep->start ();
	});

	loadStep->onStart ([me]() -> IAsyncOperation*
	{
		return me->loadPreparedDocument ();
	});
	loadStep->onCompletion ([me, prepareStep, handleErrorStep](IAsyncOperation& op)
	{
		if(op.getState () == IAsyncInfo::kCompleted)
		{
			if(op.getResult ().asBool () == false) // no retry needed
			{
				if(me->document)
					op.setResult (me->document->asUnknown ());
				else
					op.setResult (Variant ());
			}
			else
				prepareStep->start ();
		}
		else
			handleErrorStep->start ();
	});

	tryAutosaved->onStart ([me]() -> IAsyncOperation*
	{
		return me->findAutoSaved (true);
	});
	tryAutosaved->onCompletion ([prepareStep, me](IAsyncOperation& op)
	{
		if(op.getState () == IAsyncInfo::kCompleted)
			prepareStep->start ();
	});

	handleErrorStep->onStart ([me]() -> IAsyncOperation*
	{
		if(me->isSilentMode () || me->canceled)
			return AsyncOperation::createFailed ();

		int errorCode = 0;
		if(me->errorContext.hasErrors ())
			System::GetFileSystem ().getFirstError (errorCode);

		if(errorCode == INativeFileSystem::kAccesDenied)
			return me->openFileDialog ();
		else if(errorCode == INativeFileSystem::kFileNotFound)
			return AsyncOperation::createCompleted (true); // true means tryAutosave

		return AsyncOperation::createFailed ();
	});
	handleErrorStep->onCompletion ([prepareStep, tryAutosaved, me](IAsyncOperation& op)
	{
		if(op.getState () == IAsyncInfo::kCompleted)
		{
			if(op.getResult ().asBool () == true)
				tryAutosaved->start ();
			else
				prepareStep->start ();
		}
		else if(me->displayAlert () && !me->alertDisplaying)
			tryAutosaved->start ();
	});

	return stepMachine->start (prepareStep);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* DocumentManager::DocumentLoader::openFileDialog ()
{
	AsyncOperation* result = NEW AsyncOperation;
	if(AutoPtr<IFileSelector> fileSelector = ccl_new<IFileSelector> (ClassID::FileSelector))
	{
		Url folderUrl (path);
		folderUrl.ascend ();
		fileSelector->setFolder (folderUrl);
		String fileName;
		path.getName (fileName);
		fileSelector->setFileName (fileName);
		IAsyncOperation* op = fileSelector->runAsync (IFileSelector::kOpenFile);
		Promise (op).then ([this, result, fileSelector] (IAsyncOperation& operation)
		{
			const IUrl* newPath = nullptr;
			if(operation.getResult ().asBool ())
				newPath = fileSelector->getPath (0);

			if(newPath)
			{
				path = *newPath;
				AutoPtr<IStream> fileStream = System::GetFileSystem ().openStream (path);
				if(fileStream)
				{
					// refresh the recent documents entry
					if(DocumentManager::instance ().getRecentPaths ().removeRecentPath (path))
						DocumentManager::instance ().getRecentPaths ().setRecentPath (path);

					result->setState (IAsyncOperation::kCompleted);
					return;
				}
			}

			result->setState (IAsyncOperation::kFailed);
		});
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDocument* DocumentManager::DocumentLoader::checkAlreadyOpen ()
{
	// check if already open!
	Document* oldDoc = manager.findDocument (path); 
	if(oldDoc)
	{
		oldDoc->isSilent (isSilentMode ());
		if(isHiddenMode () == false)
			manager.showDocument (oldDoc);
		
		return oldDoc; 
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::DocumentLoader::prepare ()
{
	if(checkFileExists () == false)
		return false;

	if(assignDocumentClass () == false)
		return false;

	findToMergeInto ();

	if(manager.isMultipleDocuments () == false && toMergeInto == nullptr)
		if(!manager.closeAll ())
			return false;

	if(installFile () == false)
		return false;

	if(makeDocument () == false)
		return false;

	// do not spit out errors from file system if things are OK until now	
	errorContext.reset ();
	int errorCode = 0;
	System::GetFileSystem ().getFirstError (errorCode);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::DocumentLoader::checkFileExists ()
{
	if(!System::GetFileSystem ().fileExists (path))
	{
		if(isSilentMode () == false)
		{
			String fileName;
			path.getName (fileName);
			String message;
			message.appendFormat (XSTR (FileNotFound), fileName);

			alertDisplaying = true; // prevent multiple alerts

			if(manager.recentPaths->contains (path))
			{
				message << "\n\n" << XSTR (AskRemoveFromRecent);
								
				Url pathToRemove (path);
				
				Promise (Alert::askAsync (message)).then ([pathToRemove] (IAsyncOperation& operation)
				{
					if(operation.getResult ().asInt () == Alert::kYes)
						DocumentManager::instance ().recentPaths->removeRecentPath (pathToRemove);
				});
			}
			else
				Promise (Alert::errorAsync (message));
		}
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::DocumentLoader::assignDocumentClass ()
{
	ObjectArray docClasses;
	manager.findDocumentClasses (docClasses, path.getFileType ());
	if(docClasses.count () == 1)
		docClass = (DocumentClass*)docClasses.at (0);
	else if(docClasses.count () > 1)
	{
		if(manager.isAsyncAlertMode ())
			docClass = (DocumentClass*)docClasses.at (0);
		else
		{
			// file type is ambigous, let user pick the format
			if(isSilentMode () == false)
			{
				docClass = manager.runClassSelector (docClasses);
				if(!docClass)
					return false;
			}
		}
	}

	if(!docClass)
	{
		CCL_PRINTLN ("Unknown file type, using default document class!")
		docClass = manager.getDefaultClass ();
	}

	ASSERT (docClass != nullptr)
	if(!docClass)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::DocumentLoader::findToMergeInto ()
{
	ASSERT (docClass)

	// check if document merge is possible
	if(toMergeInto == nullptr)
	{
		String merge (path.getParameters ().lookupValue (CCLSTR ("merge"))); // (copy string: param entry is removed below)
		if(merge.isEmpty () == false)
		{
			path.getParameters ().removeEntry (CCLSTR ("merge")); // don't store this

			Document* activeDoc = manager.getActiveDocument ();
			if(activeDoc)
			{		
				if(merge == CCLSTR ("true"))
				{
					if(docClass->canMergeDocuments (*activeDoc, Url::kEmpty))
						toMergeInto = activeDoc;		
				}
				else if(merge == CCLSTR ("option") && isSilentMode () == false && manager.isAsyncAlertMode () == false)
				{
					if(docClass->canMergeDocuments (*activeDoc, path))
						toMergeInto = activeDoc;		
			
					if(toMergeInto)
					{
						String question;
						question.appendFormat (XSTR (AskMerge), activeDoc->getTitle ());
						if(Alert::ask (question, Alert::kYesNo) != Alert::kYes)
							toMergeInto = nullptr;						
					}
				}
			}	
		}
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::DocumentLoader::installFile ()
{
	ASSERT (docClass)

	Url preInstallPath = path;
	docClass->installFile (path); // give document class a chance to move/copy the file
	if(path != preInstallPath && docClass->getFileType () != path.getFileType ())
	{
		// file type changed during install
		docClass = manager.findDocumentClass (path.getFileType ());
		if(docClass == nullptr)
			return false;
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::DocumentLoader::makeDocument ()
{
	ASSERT (docClass)

	auto initDocument = [&] ()
	{
		document->isSilent (isSilentMode ());
		document->isSafeModeEnabled (isSafetyMode ());
		document->isTemporary (isTemporaryMode ());
		document->initialize ();
		document->setPath (path); 

		Variant eventHandlerArg;
		if(args && args->getAttribute (eventHandlerArg, IDocumentManager::kEventHandler))
		{
			if(UnknownPtr<IDocumentEventHandler> handler = eventHandlerArg.asUnknown ())
			{
				document->setEventHandler (handler);
				const_cast<IAttributeList*> (args)->remove (IDocumentManager::kEventHandler); // ownership must be transferred completely
			}
		}
	};

	document = docClass->createDocument ();
	if(document)
	{
		initDocument ();
		bool result = document->prepareLoading ();
		if(result == false)
			resetDocument ();
	}
	else
	{
		// prepare import of foreign document...
		DocumentClass* defaultClass = docClass->getTargetClass ();
		if(!defaultClass)
			defaultClass = manager.getDefaultClass ();
		document = defaultClass ? defaultClass->createDocument () : nullptr;
		if(document)
		{
			document->isImported (true);

			initDocument ();
			
			if(docClass->isPrivate () && defaultClass->needsFolder ())
			{
				Url docFolder (document->getPath ());
				docFolder.ascend ();
				document->setCreatedFolder (docFolder);	
			}

			bool result = document->prepareImport (); 
			canceled = document->isCanceled ();
			if(!result)
				resetDocument ();
			else if(docClass->isPrivate ())
				document->setDirty (true);
		}
	}

	return document.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::DocumentLoader::resetDocument ()
{
	if(document)
	{
		document->terminate ();
		document.release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* DocumentManager::DocumentLoader::loadPreparedDocument (AsyncOperation* result)
{
	bool ignoreAutosaved = result != nullptr;
	if(result == nullptr)
		result = NEW AsyncOperation ();

	if(document)
	{
		if(!ignoreAutosaved && toMergeInto == nullptr)
		{
			Promise (findAutoSaved (false)).then ([this, result](IAsyncOperation& operation)
			{
				usingAutoSavedFile = operation.getState () == IAsyncInfo::kCompleted;
				if(usingAutoSavedFile && document->getPath ().isEqualUrl (path) == false)
				{
					// restart with new document class if autosaved file has different filetype
					path = document->getPath ();
					resetDocument ();
					docClass = nullptr;
					canceled = false;
					result->setResult (true); // retry
					result->setState (IAsyncInfo::kCompleted);
				}
				else
					loadPreparedDocument (result);
			});

			return result;
		}

		manager.signalDocumentEvent (*document, Document::kBeforeLoad);

		bool loaded = false;
		bool merged = false;

		if(toMergeInto)
			merged = docClass->mergeDocuments (*toMergeInto, *document);
		if(merged == false)		
			loaded = docClass->loadDocument (*document);

		manager.signalDocumentEvent (*document, loaded ? Document::kLoadFinished : Document::kLoadFailed); // report merged as failed
		canceled = document->isCanceled ();
		if(canceled)
			loaded = false; // ???

		if(loaded)
		{
			manager.documents.add (document);
			document->retain ();

			bool addToRecentPath = (docClass->isPrivate () == false);
			if(manager.isSkipAskSave () && document->isImported ()) // on import with automatic saving, set the path for the native format now to show the final title
			{
				DocumentSaver::setImportedToNativePath (*document);
				document->isImported (false);
				document->setDirty (false); // save only if modified
				addToRecentPath = false;
			}
			else
				System::GetFileManager ().setFileUsed (document->getPath (), true);

			// create view
			if(isHiddenMode () == false)
			{
				manager.showDocument (document);
				manager.dirtySuspended = false;
			}

			if(addToRecentPath)
				manager.recentPaths->setRecentPath (path);
			
			AutoSaver::instance ().resetTimer ();

			manager.signal (Message (kPropertyChanged));
			result->setResult (false); // no retry
			result->setState (IAsyncInfo::kCompleted);
			return result;
		}
		else if(merged)
		{
			resetDocument ();
			result->setResult (false); // no retry
			result->setState (IAsyncInfo::kCompleted);
			return result;
		}
		else
			document->terminate ();
	}

	result->setState (IAsyncInfo::kFailed);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::DocumentLoader::displayAlert ()
{
	if(isSilentMode () || canceled || alertDisplaying)
		return false;

	String fileName;
	path.getName (fileName);

	if(!errorContext.hasErrors ())
		ccl_raise (XSTR (FileIsBroken));

	if(manager.isAsyncAlertMode () == true)
	{
		alertDisplaying = true;
		Promise (Alert::errorWithContextAsync (String ().appendFormat (XSTR (LoadFailed), fileName)));
	}
	else
		Alert::errorWithContext (String ().appendFormat (XSTR (LoadFailed), fileName));

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* DocumentManager::DocumentLoader::findAutoSaved (bool emergency)
{
	if(isSilentMode () || canceled)
		return AsyncOperation::createFailed ();

	// try an autosaved file instead (even if older)
	if(document && !usingAutoSavedFile)
		return AutoSaver::instance ().tryAutoSavedFile (*document, emergency);

	return AsyncOperation::createFailed ();
}

//************************************************************************************************
// DocumentManager::DocumentSaver
//************************************************************************************************

bool DocumentManager::DocumentSaver::setImportedToNativePath (Document& document)
{
	DocumentClass* docClass = document.getDocumentClass ();
	if(document.isImported () && document.getPath ().getFileType () != docClass->getFileType ())
	{
		Url newPath (document.getPath ());
		if(document.getCreatedFolder ().isEmpty () == false)
		{
			String documentName;
			document.getPath ().getName (documentName, true);
			newPath = document.getCreatedFolder ();
			newPath.descend (documentName, Url::kFile);
		}
		
		newPath.setFileType (docClass->getFileType (), true);
		newPath.makeUnique ();
		bool pathValid = System::GetFileSystem ().fileExists (newPath);
		if(!pathValid)
		{
			// Try to create an empty file at the desired location to check if we are allowed to write there.
			// This can fail in a sandboxed environment.
			AutoPtr<IStream> fileStream = System::GetFileSystem ().openStream (newPath, IStream::kCreateMode);
			pathValid = fileStream.isValid ();
		}

		if(pathValid)
			System::GetFileSystem ().removeFile (newPath); // If the test file was successfully created, clean it up
		else
		{
			// If we are not allowed to write in the desired location, fall back to the documents folder.
			// This is guaranteed to be writable even in a sandboxed environment.
			newPath = DocumentManager::instance ().getDocumentFolder ();
			UrlDisplayString fileName (document.getPath (), Url::kStringDisplayName);
			newPath.descend (fileName, Url::kFile);
			newPath.setFileType (docClass->getFileType (), true);
			newPath.makeUnique ();
		}

		document.setPath (newPath);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentManager::DocumentSaver::DocumentSaver (DocumentManager& manager, Document* doc, SaveMode mode, StringRef typeString)
: manager (manager),
  doc (doc),
  docClass (doc->getDocumentClass ()),
  mode (mode),
  typeString (typeString),
  attemptedTitle (doc->getTitle ()),
  result (false),
  canceled (false)
{
	ASSERT (!manager.isSaveDisabled ())
	ASSERT (docClass)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::DocumentSaver::saveDocument ()
{
	// if not yet saved, force "Save As"
	if(doc->getPath ().isEmpty () && mode == kSave)
		mode = kSaveAs;

	// warn before overwriting old format
	if(doc->isOlderFormat () && mode == kSave)
	{
		oldFormatPath = doc->getPath ();

		int result = Alert::ask (XSTR (WarnOldDocumentFormat), Alert::kYesNoCancel);
		if(result == Alert::kCancel)
			return false;
		if(result != Alert::kYes)
			mode = kSaveAs;
	}

	// update extension after import
	setImportedToNativePath (*doc);

	if(mode == kSaveAs || mode == kSaveToNewFolder || mode == kExportToNewFolder)
	{
		return saveWithFileSelector ();
	}
	else
	{
		// copy old format document to history before overwriting
		if(!oldFormatPath.isEmpty ())
			DocumentVersions (oldFormatPath).copyOldFormatToHistory (*doc);

		manager.signalDocumentEvent (*doc, Document::kBeforeSave);
		File::signalFile (Signals::kReleaseFile, doc->getPath ());

		result = docClass->saveDocument (*doc);
		if(!result)
		{
			// To prevent data loss, we change the document path to a location that is known to be writable.
			Url newPath;
			System::GetSystem ().getLocation (newPath, System::kUserDocumentFolder);

			Url path (doc->getPath ());
			String fileName;
			path.getName (fileName);
			newPath.descend (fileName);
			if(newPath != path && !docClass->needsFolder ())
			{
				newPath.makeUnique ();

				System::GetFileManager ().setFileUsed (path, false);
				manager.recentPaths->removeRecentPath (path);

				doc->setPath (newPath);

				System::GetFileManager ().setFileUsed (newPath, true);
				manager.recentPaths->setRecentPath (newPath);

				result = docClass->saveDocument (*doc);
			}
		}

		manager.signalDocumentEvent (*doc, Document::kSaveFinished);
		if(result)
		{
			manager.updateDirtyState (doc);
			manager.updateApplicationTitle ();
		}

		finishSave ();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::DocumentSaver::saveWithFileSelector ()
{
	AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);
	ASSERT (fs != nullptr)
	if(!doc->getPath ().isEmpty ())
	{
		Url url (doc->getPath ());
		url.ascend ();
		fs->setFolder (url);
	}

	if(!doc->getTitle ().isEmpty ())
		fs->setFileName (doc->getTitle ());

	manager.prepareFilters (*fs, doc, typeString);

	if(fs->getSaveBehavior () & IFileSelector::kSaveNeedsContent)
	{
		// save to temp file
		preliminaryFile = NEW TempFile (doc->getTitle ());
		Url tempUrl = preliminaryFile->getPath ();
		tempUrl.setName (LegalFileName (doc->getTitle ()));
		tempUrl.setFileType (docClass->getFileType ());
		preliminaryFile->setPath (tempUrl);

		doc->setPath (preliminaryFile->getPath ());

		manager.signalDocumentEvent (*doc, Document::kBeforeSave);
		File::signalFile (Signals::kReleaseFile, doc->getPath ());

		result = docClass->saveDocumentAs (*doc, preliminaryFile->getPath ());

		manager.signalDocumentEvent (*doc, Document::kSaveFinished);

		// provide saved file to selector
		if(result)
			fs->setSaveContent (preliminaryFile->getPath ());
	}

	retain ();

	Promise fsPromise (fs->runAsync (IFileSelector::kSaveFile));
	fsPromise.then ([this, fs] (IAsyncOperation& operation) 
	{
		Url newPath;
		const IUrl* path = operation.getResult ().asBool () ? fs->getPath () : nullptr;
		if(path)
		{
			newPath = *path;

			if(preliminaryFile)
			{				
				preliminaryFile = nullptr;

				finishSave ();
				release ();
				return;
			}
	
			// check if saved as other type...
			if(newPath.getFileType () != docClass->getFileType () && newPath.getFileType ().isValid ())
			{
				DocumentClass* exportClass = manager.findDocumentClass (newPath.getFileType ());
				if(exportClass)
				{
					doc->isExport (true);

					int result = Alert::kFirstButton;
					if(typeString.isEmpty ()) // don't ask if file type provided explicitly
					{
						String exportFormat (exportClass->getFileType ().getDescription ());

						String text;
						text.appendFormat (XSTR (AskSaveAsCopy), doc->getTitle (), exportFormat);
						text << "\n\n";
						text << XSTR (WarnExportFormat);

						String firstButton;
						firstButton.appendFormat (XSTR (UseFileFormat), exportFormat);
						//String secondButton;
						//secondButton.appendFormat (XSTR (UseFileFormat), docClass->getFileType ().getDescription ());
						String thirdButton (Alert::button (Alert::kCancel));

						result = Alert::ask (text, firstButton, /*secondButton,*/ thirdButton);
					}

					if(result == Alert::kFirstButton)
						docClass = exportClass;
					/*else if(result == Alert::kSecondButton)
					{
						newPath.setFileType (docClass->getFileType (), true);
						newPath.makeUnique ();
						doc->setPath (newPath);
					}*/
					else
						canceled = true;
				}
			}

			if(!canceled)
			{
				// copy old format document to history before overwriting
				if(!oldFormatPath.isEmpty ())
					DocumentVersions (oldFormatPath).copyOldFormatToHistory (*doc);

				if(docClass->needsFolder ())
					manager.checkSaveFolder (newPath, *doc);

				result = true;

				doc->isSaveToNewFolder (mode == kSaveToNewFolder);
				doc->isExportToNewFolder (mode == kExportToNewFolder);

				bool isNewFolder = mode == kSaveToNewFolder || mode == kExportToNewFolder;
				if(isNewFolder)
					result = doc->prepareSaveToNewFolder (newPath);

				if(result)
				{
					manager.signalDocumentEvent (*doc, Document::kBeforeSave);
					File::signalFile (Signals::kReleaseFile, newPath);

					bool wasDirty = doc->isDirty ();

					Url oldPath (doc->getPath ());
					Url oldFolder (oldPath);
					oldFolder.ascend ();

					Url newFolder (newPath);
					newFolder.ascend ();

					// keep the title used while trying to save (for error dialog in case of failure, document will restore old title)
					attemptedTitle = UrlDisplayString (newPath, Url::kStringDisplayName);

					result = docClass->saveDocumentAs (*doc, newPath);
					canceled = doc->isCanceled ();
					doc->isCanceled (false);
					
					if(isNewFolder || oldFolder != newFolder)
					{
						if(mode == kExportToNewFolder)
						{
							doc->setPath (oldPath);
							if(wasDirty)
								doc->setDirty (true);
							
							newPath = oldPath;
						}
						doc->finishSaveToNewFolder (newPath);
					}

					manager.signalDocumentEvent (*doc, Document::kSaveFinished); 

					if(result)
						manager.updateApplicationTitle ();
				}
				
				doc->isSaveToNewFolder (false);
				doc->isExportToNewFolder (false);
			}
		}
		else
			canceled = true;
			
		finishSave ();

		ListForEach (manager.handlers, IDocumentEventHandler*, handler)
			handler->onDocumentExported (*doc, newPath);
		EndFor

		doc->isExport (false);

		if(result && !canceled && !newPath.isEmpty ())
			docClass->finalizeSaveDocumentAs (*doc, newPath);

		release ();
	});

	return !canceled; // (in case of a synchronous file selector implementation, otherwise we don't know yet)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::DocumentSaver::finishSave ()
{
	if(!canceled)
	{
		String fileName;
		doc->getPath ().getName (fileName);
		SafetyGuard safetyGuard (SafetyID::kSaveAction, { fileName });

		if(result)
		{
			if(mode != kExportToNewFolder)
			{
				if(System::GetFileSystem ().fileExists (doc->getPath ())) // does not exist if saved as copy
					manager.recentPaths->setRecentPath (doc->getPath ());

				if(!doc->isExport ()) // keep autosave on export to external format
					AutoSaver::instance ().removeAutoSaveFile (*doc);

				// successfully saved: remove temporary flag
				if(doc->isTemporary ())
					doc->isTemporary (false);
			}
		}
		else
		{
			String text;
			text.appendFormat (XSTR (SaveFailed), attemptedTitle);

			if(!errorContext.hasErrors ())
				if(System::GetFileSystem ().isWriteProtected (doc->getPath ()))
					ccl_raise (XSTR (FileIsWriteProtected));
				else
					ccl_raise (XSTR (FileIsInUse));

			retain ();
			Promise (Alert::errorWithContextAsync (text)).then ([this] (IAsyncOperation& operation)
			{
				cleanup ();
				release ();
			});
			return;
		}
	}

	cleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::DocumentSaver::cleanup ()
{
	if(result)
	{
		// reset imported flag if document was saved in it's own format
		if(doc->isImported () && docClass == doc->getDocumentClass ())
			doc->isImported (false);

		doc->isOlderFormat (false);	// reset old format flag
		doc->setAutoSavedNow ();	// reset autosave timer
		manager.signalFileCreated (*doc);
	}
}

//************************************************************************************************
// ExternalDocumentClass
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ExternalDocumentClass, DocumentClass)

//////////////////////////////////////////////////////////////////////////////////////////////////

ExternalDocumentClass::ExternalDocumentClass (IDocumentFilter* handler)
: DocumentClass (0),
  handler (handler)
{
	ASSERT (handler != nullptr)
	setFileType (handler->getFileType ());

	int handlerFlags = handler->getFlags ();
	if(handlerFlags & IDocumentFilter::kCanImport)
		flags |= kCanLoad;
	if(handlerFlags & IDocumentFilter::kCanExport)
		flags |= kCanSave;
	if(handlerFlags & IDocumentFilter::kIsPrivate)
		flags |= kIsPrivate;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExternalDocumentClass::~ExternalDocumentClass ()
{
	if(handler)
		ccl_release (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExternalDocumentClass::canImportFile (UrlRef path) const
{
	if(handler)
	{
		tresult result = handler->canImportFile (path);
		return (result == kResultOk || result == kResultNotImplemented);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExternalDocumentClass::isNative () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Document* ExternalDocumentClass::createDocument ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExternalDocumentClass::installFile (Url& path)
{
	if(handler->getFlags () & IDocumentFilter::kInstallRequired)
		handler->installFile (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExternalDocumentClass::loadDocument (Document& document)
{
	// show import options
	if(handler->getFlags () & IDocumentFilter::kHasImportOptions)
	{
		if(handler->showImportOptions (document) != kResultOk)
		{
			document.isCanceled (true);
			return false;
		}
	}

	// disable undo stack
	ActionJournalDisabler disabler (document.getActionJournal ()); 

	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	UnknownPtr<IProgressDialog> (progress)->setOpenDelay (0.5, true);
	ProgressNotifyScope progressScope (progress);
	progress->updateAnimated (String ().appendFormat (XSTR (Importing), fileType.getDescription ()));

	Document::CancelGuard cancelGuard (document, progress);
	tresult result = handler->importDocument (document, progress);
	document.isCanceled (result == kResultAborted);
	document.setDirty (false);
	return result == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExternalDocumentClass::saveDocumentInternal (Document& document, UrlRef path)
{
	// show export options
	if(handler->getFlags () & IDocumentFilter::kHasExportOptions)
	{
		if(handler->showExportOptions (document) != kResultOk)
		{
			document.isCanceled (true);
			return false;
		}
	}

	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	if(!(handler->getFlags () & IDocumentFilter::kNeedsCancel))
		progress->setCancelEnabled (false);
	ProgressNotifyScope progressScope (progress);
	progress->updateAnimated (String ().appendFormat (XSTR (Exporting), fileType.getDescription ()));

	IDocumentFilter::ExportParams params (path);
	tresult result = handler->exportDocument (document, params, progress);
	document.isCanceled (result == kResultAborted);
	return result == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExternalDocumentClass::saveDocument (Document& document)
{
	return saveDocumentInternal (document, document.getPath ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExternalDocumentClass::canSaveDocument (Document& document) const
{
	return handler->canExportDocument (document) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExternalDocumentClass::saveDocumentAs (Document& document, UrlRef path)
{
	return saveDocumentInternal (document, path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExternalDocumentClass::finalizeSaveDocumentAs (Document& document, UrlRef path)
{
	if(handler->getFlags () & IDocumentFilter::kNeedsExportFinalization)
	{
		IDocumentFilter::ExportParams params (path);
		return handler->finalizeDocumentExport (document, params) == kResultOk;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExternalDocumentClass::canMergeDocuments (Document& target, UrlRef sourcePath)
{
	return handler->canMergeDocuments (target, sourcePath) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExternalDocumentClass::mergeDocuments (Document& target, Document& source)
{
	ActionJournalDisabler disabler (source.getActionJournal ()); 
	ActionJournal* targetJournal = target.getActionJournal ();
	if(targetJournal)
		targetJournal->beginMultiple (XSTR (Merge));

	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	ProgressNotifyScope progressScope (progress);
	progress->updateAnimated (String ().appendFormat (XSTR (Merging), fileType.getDescription ()));

	Document::CancelGuard cancelGuard (source, progress);

	bool result = handler->mergeDocuments (target, source, progress) == kResultOk;

	if(targetJournal)
		targetJournal->endMultiple (result == false);
	
	return result;
}

//************************************************************************************************
// DocumentDragHandler
//************************************************************************************************

DocumentDragHandler::DocumentDragHandler (IView* view)
: DragHandler (view)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* DocumentDragHandler::prepareDataItem (IUnknown& item, IUnknown* context)
{
	UnknownPtr<IUrl> path (&item);
	if(path)
	{
		if(DocumentManager::instance ().canOpenDocument (*path))
		{
			addSprite (*path);
			path->retain ();
			return path;
		}
		else if(IUrl* filePath = DocumentManager::instance ().findDocumentInFolder (*path))
		{
			addSprite (*filePath);
			return filePath;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDragHandler::addSprite (UrlRef path)
{
	AutoPtr<IImage> icon (FileIcons::instance ().createIcon (path));
	String fileName;
	path.getName (fileName, false);
	spriteBuilder.addItem (icon, fileName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDragHandler::finishPrepare ()
{
	if(!getData ().isEmpty ())
	{
		String text;

		IUnknown* first = getData ().getFirst ();
		IUnknown* last = getData ().getLast();
		if(first == last) // one file
		{
			UnknownPtr<IUrl> path (first);
			if(path)
			{
				ObjectArray docClasses;
				DocumentManager::instance ().findDocumentClasses (docClasses, path->getFileType ());
				if(docClasses.count () == 1)
				{
					DocumentClass* docClass = (DocumentClass*)docClasses.at (0);
					const FileType* knownType = &docClass->getFileType ();
					Variant args [] = { knownType->getDescription () };
					text.appendFormat (XSTR (OpenX), args, ARRAY_COUNT (args));
				}
			}
		}

		if(text.isEmpty ())
			text = XSTR (OpenFiles);
		spriteBuilder.addHeader (text, -1);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentDragHandler::drop (const DragEvent& event)
{
	ForEachUnknown (getData (), obj)
		UnknownPtr<IUrl> path (obj);
		if(path)
			DocumentManager::instance ().deferOpenDocument (*path);
	EndFor

	return DragHandler::drop (event);
}

//************************************************************************************************
// DocumentManager::PreviewLoader
//************************************************************************************************

DocumentManager::PreviewLoader::PreviewLoader (bool silent)
: silent (silent)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentManager::PreviewLoader::~PreviewLoader ()
{
	closeDocument ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
bool DocumentManager::PreviewLoader::openDocument (UrlRef path, StringID previewMode, DocumentClass* docClass)
{
	closeDocument ();
	Document::SilentPreviewScope silenceScope (silent);
	if(docClass)
	{
		AutoPtr<Document> doc = docClass->createDocument ();
		ASSERT (doc)
		if(doc == nullptr)
			return false;

		doc->setPath (path);

		document = DocumentManager::instance ().openPreviewDocument (doc, previewMode);	
	}
	else
		document = DocumentManager::instance ().openPreviewDocument (path, previewMode);
	return document != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::PreviewLoader::openTemplate (UrlRef path, StringID previewMode, const FileType& docFileType)
{
	closeDocument ();
	Document::SilentPreviewScope silenceScope (silent);
	document = DocumentManager::instance ().openPreviewTemplate (path, previewMode, docFileType);
	return document != nullptr;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentManager::PreviewLoader::openTemplate (const DocumentTemplate& docTemplate, StringID previewMode, const FileType& docFileType)
{
	closeDocument ();
	Document::SilentPreviewScope silenceScope (silent);
	document = DocumentManager::instance ().openPreviewTemplate (docTemplate, previewMode, docFileType);
	return document != nullptr;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentManager::PreviewLoader::closeDocument ()
{
	if(document)
	{
		Document::SilentPreviewScope silenceScope (silent);
		DocumentManager::instance ().closePreviewDocument (document);
		document = nullptr;
	}
}

//************************************************************************************************
// DocumentSelectorHook
//************************************************************************************************

DocumentSelectorHook::DocumentSelectorHook ()
: Component (CCLSTR ("DocumentSelectorHook")),
  preview (nullptr)
{
	addComponent (preview = NEW FilePreviewComponent (CCLSTR ("Preview")));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentSelectorHook::onSelectionChanged (IFileSelector& fs, UrlRef path)
{
	preview->setFile (path, nullptr, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentSelectorHook::onFilterChanged (IFileSelector& fs, int filterIndex)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentSelectorHook::onCustomize (IFileSelectorCustomize& fsc)
{
	preview->customizeFileSelector (fsc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API DocumentSelectorHook::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "FileSelectorView")
	{
		ITheme* theme = getTheme ();
		ASSERT (theme != nullptr)
		return theme ? theme->createView ("CCL/DocumentFileSelector", this->asUnknown ()) : nullptr;
	}
	return nullptr;
}
