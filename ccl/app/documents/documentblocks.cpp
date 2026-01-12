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
// Filename    : ccl/app/documents/documentblocks.cpp
// Description : Document Blocks
//
//************************************************************************************************

#include "ccl/app/documents/documentblocks.h"
#include "ccl/app/documents/documentversions.h"
#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/documents/documentrenamer.h"
#include "ccl/app/documents/document.h"
#include "ccl/app/documents/autosaver.h"

#include "ccl/app/params.h"
#include "ccl/app/controls/treeviewmodel.h"
#include "ccl/app/controls/draghandler.h"
#include "ccl/app/fileinfo/fileinforegistry.h"
#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/utilities/fileoperations.h"
#include "ccl/app/utilities/shellcommand.h"
#include "ccl/app/components/pathselector.h"
#include "ccl/app/components/listvieweditcomponent.h"
#include "ccl/app/components/searchcomponent.h"
#include "ccl/app/components/searchprovider.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/file.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/base/iasyncoperation.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ifilemanager.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/diagnosticprofiler.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/framework/controlsignals.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/idragndrop.h"
#include "ccl/public/gui/framework/iviewanimation.h"
#include "ccl/public/gui/framework/icolorscheme.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/graphics/iuivalue.h"
#include "ccl/public/gui/iviewstate.h"
#include "ccl/public/guiservices.h"

namespace CCL {

//************************************************************************************************
// DocumentBlocks::FileTreeEntry
//************************************************************************************************

DocumentBlocks::FileTreeEntry::FileTreeEntry (UrlRef url)
: url (url),
  ignored (false)
{}

//************************************************************************************************
// DocumentBlocks::FileEntry
//************************************************************************************************

DocumentBlocks::FileEntry::FileEntry (UrlRef url)
: FileTreeEntry (url),
  description (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::FileEntry::FileEntry (const DocumentDescription& d)
: FileTreeEntry (d.getPath ()),
  description (NEW DocumentDescription (d))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::FileEntry::~FileEntry ()
{
	if(description)
		description->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentDescription& DocumentBlocks::FileEntry::getDescription ()
{
	if(!description)
	{
		description = NEW DocumentDescription;
		description->assign (getUrl ());
	}
	return *description;
}

//************************************************************************************************
// DocumentBlocks::FolderEntry
//************************************************************************************************

DocumentBlocks::FolderEntry::FolderEntry (UrlRef url)
: FileTreeEntry (url),
  contentScanned (false)
{
	subFolders.objectCleanup (true);
	files.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectArray& DocumentBlocks::FolderEntry::getSubFolders (bool scan)
{
	if(scan && !contentScanned)
		scanContent ();
	return subFolders;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectArray& DocumentBlocks::FolderEntry::getFiles (bool scan)
{
	if(scan && !contentScanned)
		scanContent ();
	return files;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::FolderEntry::scanContent ()
{
	if(!url.isEmpty ())
	{
		ForEachFile (System::GetFileSystem ().newIterator (getUrl ()), p)
			if(p->isFolder ())
				subFolders.add (NEW FolderEntry (*p));
			else
				files.add (NEW FileEntry (*p));
		EndFor
		contentScanned = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::FolderEntry::addFile (UrlRef url)
{
	files.add (NEW FileEntry (url));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::FolderEntry::addDocument (const DocumentDescription& description)
{
	files.add (NEW FileEntry (description));
}

//************************************************************************************************
// DocumentBlocks::RecentSource
//************************************************************************************************

class DocumentBlocks::RecentSource: public DocumentBlocks::Source
{
public:
	DECLARE_CLASS_ABSTRACT (RecentSource, Source)
	
	static bool isDeleteOnRemove ();

	RecentSource ();
	~RecentSource ();

	PROPERTY_OBJECT (FileType, fileType, FileType) // default (not set): any type
	PROPERTY_BOOL (failOnEmptyList, FailOnEmptyList)

	bool removeFromRecentList (UrlRef path);

	// Source
	tresult getDocuments (DocumentSink& sink, IProgressNotify* progress = nullptr) override;
	void appendDocumentMenu (IMenu& menu, const DocumentDescription& description, Container* selectedUrls = nullptr) override;
	bool removeDocument (const DocumentDescription& description) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	ISearcher* createSearcher (ISearchDescription& description) override;
	FolderEntry* getFileTree () override;

protected:
	RecentDocuments& recentPaths;
	AutoPtr<FolderEntry> folderEntry;
	static Configuration::BoolValue deleteOnRemove;
	static Configuration::BoolValue hideMissing;

	virtual Iterator* newPathIterator () const;
	tresult getDocumentsInternal (Container& list, IProgressNotify* progress, Iterator* iterator);

	bool onRemoveRecent (CmdArgs args, VariantRef data);
};

//************************************************************************************************
// DocumentBlocks::PinnedSource
//************************************************************************************************

class DocumentBlocks::PinnedSource: public DocumentBlocks::RecentSource
{
public:
	DECLARE_CLASS_ABSTRACT (PinnedSource, RecentSource)

protected:
	// RecentSource
	Iterator* newPathIterator () const override;
	void appendDocumentMenu (IMenu& menu, const DocumentDescription& description, Container* selectedUrls = nullptr) override;
};

//************************************************************************************************
// DocumentBlocks::FolderSource
//************************************************************************************************

class DocumentBlocks::FolderSource: public DocumentBlocks::Source
{
public:
	DECLARE_CLASS_ABSTRACT (FolderSource, Source)

	FolderSource (UrlRef path, const FileType& fileType);
	~FolderSource ();

	PROPERTY_OBJECT (Url, path, Path)
	PROPERTY_OBJECT (FileType, fileType, FileType)

	bool canCreateFolderIn (Url& parentFolder) const;

	// Source
	tresult getDocuments (DocumentSink& sink, IProgressNotify* progress = nullptr) override;
	bool removeDocument (const DocumentDescription& description) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	ISearcher* createSearcher (ISearchDescription& description) override;
	FolderEntry* getFileTree () override;

protected:
	SignalSink signalSink;
	bool documentNeedsFolder;
	AutoPtr<FolderEntry> rootFolderEntry;

	enum Constants { kMaxDepth = 3 };
	tresult scan (Container& list, UrlRef folder, IProgressNotify* progress = nullptr, int depth = 0);
	int scanTree (DocumentSink& sink, FolderEntry& folderEntry, UrlRef folder, IProgressNotify* progress = nullptr, int depth = 0);

	MutableCString getDiagnosticID () const;
	String getDiagnosticLabel () const;

	static DEFINE_ARRAY_COMPARE (SortByName, DocumentDescription, lhs, rhs)
		return lhs->getTitle ().compare (rhs->getTitle ());
	}
};

//************************************************************************************************
// DocumentBlocks::Item
//************************************************************************************************

class DocumentBlocks::Item: public TreeViewNode
{
public:
	DECLARE_CLASS_ABSTRACT (Item, TreeViewNode)

	Item ();

	void setDocumentUrl (UrlRef url);
	UrlRef getDocumentUrl () const	{ return documentUrl; }

	DocumentDescription& getDescription () const;
	PROPERTY_SHARED_AUTO (IParameter, pinParameter, PinParam)

	String getFileName () const;

	CStringRef getSortName () const	{ return sortName; }
	static void makeSortName (MutableCString& name, UrlRef documentUrl);

	IView* getTitleView () const	{ return titleView; }
	void setTitleView (IView* view)	{ titleView = view; }

	IView* getPinView () const		{ return pinView; }
	void setPinView (IView* view)	{ pinView = view; }

	void assignDocument (const DocumentDescription& document);

	// ListViewItem
	bool getDetail (Variant& value, StringID id) const override;
	bool getTooltip (String& tooltip, StringID id) override;
	int compare (const Object& obj) const override;

protected:
	Url documentUrl;
	MutableCString sortName;
	mutable AutoPtr<DocumentDescription> description;
	ViewPtr titleView;
	ViewPtr pinView;
};

//************************************************************************************************
// DocumentBlocks::FolderNode
//************************************************************************************************

class DocumentBlocks::FolderNode: public TreeViewFolderNode,
								  public DocumentSink
{
public:
	DECLARE_CLASS_ABSTRACT (FolderNode, TreeViewFolderNode)

	FolderNode ();

	PROPERTY_OBJECT (Url, url, Url)
	PROPERTY_POINTER (FolderNode, parentFolder, ParentFolder)
	PROPERTY_VARIABLE (int, sortPriority, SortPriority)

	FolderNode* addFolderNode (StringRef name, IImage* icon, UrlRef url, int sortPriority = 0);
	Item* findDocumentItem (Url url) const;

	virtual DocumentBlocks* getComponent ();

	// DocumentSink
	void addDocument (UrlRef url, bool sort = true) override;
	DocumentSink* addFolder (StringRef name, IImage* icon, UrlRef url, int sortPriority = 0) override;
	void removeFolder () override;
	void flattenFolder () override;

	// TreeViewFolderNode
	int compare (const Object& obj) const override;
};

//************************************************************************************************
// DocumentBlocks::RootFolderNode
//************************************************************************************************

class DocumentBlocks::RootFolderNode: public DocumentBlocks::FolderNode
{
public:
	RootFolderNode (DocumentBlocks* component)
	: component (component)
	{}

	// FolderNode
	DocumentBlocks* getComponent () override { return component; }

private:
	DocumentBlocks* component;
};

//************************************************************************************************
// DocumentBlocks::TreeModel
//************************************************************************************************

class DocumentBlocks::TreeModel: public TreeViewModel,
								 public IParamObserver
{
public:
	TreeModel (DocumentBlocks& component);
	~TreeModel ();

	void addDocumentItem (UrlRef url, bool sort = true);
	void makeViews (bool state);

	void storeViewState ();
	void restoreViewState ();

	static const IUrl* getUrl (ListViewItem& item);
	void getSelectedUrls (Container& urls, bool wantFolders, bool wantDocuments);
	void selectDocuments (UrlRef url, StringRef folderName);

	DocumentBlocks& getComponent () const { return component; }
	FolderNode* getRootFolder () const;

	void rebuild (Source* source);

	FolderNode* findFolderNode (UrlRef url, FolderSource& folderSource, bool create);

	// TreeViewModel
	void onVisibleChanged (bool state) override;
	void onColumnRectsChanged () override;
	tbool CCL_API getUniqueItemName (MutableCString& name, ItemIndexRef index) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API canRemoveItem (ItemIndexRef index) override;
	tbool CCL_API removeItem (ItemIndexRef index) override;
	tbool CCL_API appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection) override;
	tbool CCL_API canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView) override;
	tbool CCL_API insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session) override;
	tbool CCL_API interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection) override;

	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IParamObserver, TreeViewModel)

protected:
	DocumentBlocks& component;
	SignalSink schemeSink;
	Item* editItem;
	Object* editData;
	Color separatorColor;
	bool hasViews;
	bool inRebuild;

	using SuperClass = TreeViewModel;

	enum ActionCode { kNone, kPopupActionMenu, kOpenDocument, kRenameDocument, kDragDocument, kPinDocument, kPopupDocumentInfo };

	bool isEditMode () const;
	void enableEditCommands (bool state);

	void addEmptyFolders ();
	Item* resolveDocumentItem (ItemIndexRef index);

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override {}
};

//************************************************************************************************
// DocumentBlocks::ViewState
//************************************************************************************************

class DocumentBlocks::ViewState: public Object
{
public:
	DECLARE_CLASS (ViewState, Object)

	PROPERTY_MUTABLE_CSTRING (name, Name)
	PROPERTY_OBJECT (PathList, newFolders, NewFolders) // folders created by the user

	IViewStateHandler* getExpandState (ITreeItem& rootItem);
	void setExpandState (IViewStateHandler* state);

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

private:
	mutable AutoPtr<Attributes> expandStateAttribs;
	AutoPtr<IViewStateHandler> expandState;
};

//************************************************************************************************
// DocumentBlocks::FolderDragHandler
//************************************************************************************************

class DocumentBlocks::FolderDragHandler: public DragHandler,
										 public IItemDragVerifier
{
public:
	DECLARE_CLASS_ABSTRACT (FolderDragHandler, DragHandler)

	FolderDragHandler (IView* view = nullptr, TreeModel* model = nullptr);

	PROPERTY_BOOL (dragToRoot, DragToRoot)

	virtual bool setTargetNode (TreeViewNode* node);
	UrlRef getTargetFolder () const;
	UrlRef getTargetFolder (FolderNode* folderNode) const;

	// DragHandler
	IUnknown* prepareDataItem (IUnknown& item, IUnknown* context) override;
	void finishPrepare () override;
	tbool CCL_API dragOver (const DragEvent& event) override;

	// IItemDragVerifier
	tbool CCL_API verifyTargetItem (ItemIndex& item, int& relation) override;

	CLASS_INTERFACE (IItemDragVerifier, DragHandler)

protected:
	UnknownPtr<IItemView> itemView;
	TreeModel* model;
	SharedPtr<FolderNode> targetNode;
	PathList forbiddenTargetFolders;		///< can't drag into these folders
	PathList forbiddenTargetFoldersDeep;	///< can't drag into childs of these folders

	bool checkTargetNode (FolderNode& node) const;
	bool canMoveInto (UrlRef targetFolder) const;
};

//************************************************************************************************
// DocumentBlocks::RootFolderDataTarget
/** DataTarget for dragging into the root folder of a folder source (to be used outside the ItemView). */
//************************************************************************************************

class DocumentBlocks::RootFolderDataTarget: public Object,
											public IDataTarget
{
public:
	PROPERTY_POINTER (DocumentBlocks, component, DocumentBlocks)

	// IDataTarget
	tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;

	CLASS_INTERFACE (IDataTarget, Object)
};

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::RootFolderDataTarget::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	DocumentBlocks* component = getDocumentBlocks ();
	if(component && session && ccl_cast<FolderSource> (component->getActiveSource ()))
	{
		// we expect not to be called for an ItemView here (that would be TreeModel::canInsertData), e.g. for "header" view
		ASSERT (!UnknownPtr<IItemView> (targetView).isValid ())
		targetView = ViewBox (component->treeModel->getTreeView ());

		AutoPtr<FolderDragHandler> handler (NEW FolderDragHandler (targetView, component->treeModel));
		if(handler->prepare (data, session))
		{
			handler->setDragToRoot (true);

			session->setDragHandler (handler);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::RootFolderDataTarget::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	if(DocumentBlocks* component = getDocumentBlocks ())
	{
		ItemIndex rootIndex (ccl_as_unknown (component->treeModel->getRootFolder ()));
		return component->treeModel->insertData (rootIndex, 0, data, session);
	}
	return false;
}

//************************************************************************************************
// DocumentBlocks::DocumentSearchProvider
//************************************************************************************************

class DocumentBlocks::DocumentSearchProvider: public SearchProvider
{
public:
	DocumentSearchProvider (DocumentBlocks& component);

	// SearchProvider
	ISearcher* createSearcher (ISearchDescription& description) override;

protected:
	DocumentBlocks& component;
};

//************************************************************************************************
// DocumentBlocks::DocumentSearcher
//************************************************************************************************

class DocumentBlocks::DocumentSearcher: public Object,
										public AbstractSearcher,
										public DocumentBlocks::DocumentSink
{
public:
	DocumentSearcher (ISearchDescription& description, Source& source, bool preload = false);

	tresult loadDocumentList (IProgressNotify* progress = nullptr);

	// ISearcher
	tresult CCL_API find (ISearchResultSink& resultSink, IProgressNotify* progress) override;

	// DocumentSink
	void addDocument (UrlRef url, bool sort = true) override;

	CLASS_INTERFACE (ISearcher, Object)

private:
	Source& source;
	ObjectArray documentUrls;
};

//************************************************************************************************
// DocumentBlocks::DocumentSearchResult
//************************************************************************************************

class DocumentBlocks::DocumentSearchResult: public TreeModel,
											public ISearchResultViewer
{
public:
	DocumentSearchResult (DocumentBlocks& component);

	// ISearchResultViewer
	bool isViewVisible () override { return false; };
	IView* createView (const Rect& bounds) override { return nullptr; };
	void onSearchStart (ISearchDescription& description, ISearchProvider* searchProvider) override;
	void onSearchEnd (bool canceled) override;
	void onResultItemsAdded (const IUnknownList& items) override;

	CLASS_INTERFACE (ISearchResultViewer, TreeModel)
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum DocumentBlockTags
	{
		kActiveSource = 100,
		kRenameDocument = 200
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Documents")
	XSTRING (NoVersionsFound, "No versions found.")
	XSTRING (AdditionalVersions, "Additional versions available.")
	XSTRING (RemoveFromRecentFiles, "Remove from Recent Files list")
	XSTRING (PinDocument, "Pin document to Recent Files list")
	XSTRING (UnpinDocument, "Unpin document from Recent Files list")
	XSTRING (Pinned, "Pinned")
END_XSTRINGS

//************************************************************************************************
// DocumentBlocks
//************************************************************************************************

Configuration::BoolValue DocumentBlocks::locationIcons ("Application.DocumentBlocks", "locationIcons", false);
DEFINE_STRINGID_MEMBER_ (DocumentBlocks, kPinID, "pin")
DEFINE_STRINGID_MEMBER_ (DocumentBlocks, kAgeID, "age")

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (DocumentBlocks, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::DocumentBlocks (StringRef name)
: Component (name.isEmpty () ? CCLSTR ("DocumentBlocks") : name),
  treeModel (nullptr),
  searchResult (nullptr),
  activeSource (nullptr),
  search (nullptr),
  sourceDirty (false),
  inBulkOperation (false)
{
	sources.objectCleanup (true);
	sourceStates.objectCleanup (true);

	treeModel = NEW TreeModel (*this);
	addObject ("documentTree", treeModel);
	addObject ("documentList", treeModel->getListViewAdapter ());

	auto rootTarget = NEW RootFolderDataTarget;
	rootTarget->setDocumentBlocks (this);
	addObject ("rootFolderTarget", (rootFolderTarget = rootTarget));

	ListViewEditComponent* editComponent = NEW ListViewEditComponent (treeModel);
	editComponent->addEditCommand ("deleteDocuments", "Edit", "Delete");
	editComponent->enableEditCommands (false);
	addComponent (editComponent);

	paramList.addList (CSTR ("activeSource"), Tag::kActiveSource);

	search = NEW SearchComponent ();

	searchResult = NEW DocumentSearchResult (*this);
	addObject ("searchResult", searchResult->getListViewAdapter ());
	search->setResultViewer (searchResult);

	AutoPtr<SearchProvider> searchProvider (NEW DocumentSearchProvider (*this));
	search->setSearchProvider (searchProvider);

	addComponent (search);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::~DocumentBlocks ()
{
	setActiveSource (nullptr);
	treeModel->release ();
	searchResult->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::isEditMode () const
{
	if(ListViewEditComponent* editComponent = findChildNode<ListViewEditComponent> ())
		return editComponent->isEditMode ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::addSource (Source* source)
{
	sources.add (source);

	if(activeSource == nullptr)
		setActiveSource (source);

	ListParam* listParam = paramList.byTag<ListParam> (Tag::kActiveSource);
	listParam->appendObject (return_shared (source));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::Source& DocumentBlocks::addRecentDocuments ()
{
	Source* source = NEW RecentSource ();
	source->setTitle (RecentDocuments::getTranslatedTitle ());
	source->setID ("RecentFiles");

	addSource (source);

	if(RecentSource::isDeleteOnRemove ())
	{
		if(ListViewEditComponent* editComponent = findChildNode<ListViewEditComponent> ())
		{
			editComponent->addEditCommand ("removeFromList", "Edit", "Remove From List");
			editComponent->enableEditCommands (false);
		}
	}
	return *source;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::Source& DocumentBlocks::addDocumentFolder (UrlRef path, const FileType& fileType, StringRef title, StringID id)
{
	Source* source = NEW FolderSource (path, fileType);
	source->setTitle (title);
	source->setID (id);
	addSource (source);
	return *source;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::Source& DocumentBlocks::addDocumentFolder (const DocumentClass& documentClass, StringRef title, StringID id)
{
	Url path (DocumentManager::instance ().getDocumentFolder ());
	if(!documentClass.getSubFolder ().isEmpty ())
		path.descend (documentClass.getSubFolder (), Url::kFolder);

	return addDocumentFolder (path, documentClass.getFileType (), title, id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::addPinnedDocumentsFolder (Source& parentSource)
{
	auto pinned = NEW PinnedSource;
	pinned->setTitle (XSTR (Pinned));
	pinned->setFailOnEmptyList (true); // hides empty "pinned" folder

	if(auto folderSource = ccl_cast<FolderSource> (&parentSource))
		pinned->setFileType (folderSource->getFileType ());

	parentSource.addChildSource (pinned);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::Source* DocumentBlocks::getActiveSource () const
{
	return activeSource;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::setActiveSource (StringID id)
{
	setActiveSource (getSourceByID (id));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::Source* DocumentBlocks::getSourceByID (StringID id) const
{
	for(auto source : iterate_as<Source> (sources))
		if(source->getID () == id)
			return source;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::setActiveSource (Source* source)
{
	if(activeSource != source)
	{
		treeModel->storeViewState ();

		share_and_observe (this, activeSource, source);

		rebuildList ();
		getParameterByTag (Tag::kActiveSource)->setValue (sources.index (source));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::ViewState* DocumentBlocks::getSourceState (Source* source, bool create)
{
	ViewState* state = nullptr;
	if(source && !source->getID ().isEmpty ())
	{
		state = sourceStates.findIf<ViewState> ([&] (const ViewState& s) { return s.getName () == source->getID (); });
		if(!state && create)
		{
			state = NEW ViewState;
			state->setName (source->getID ());
			sourceStates.add (state);
		}
	}
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PathList* DocumentBlocks::getNewFolders (bool create)
{
	ViewState* state = getSourceState (getActiveSource (), create);
	return state ? ccl_const_cast (&state->getNewFolders ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& DocumentBlocks::getSettings () const
{
	String path ("DocumentBlocks");
	return Settings::instance ().getAttributes (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::saveSettings () const
{
	Attributes& attributes = getSettings ();
	sourceStates.save (attributes);

	if(search)
		saveChild (Storage (attributes), *search); // (search field visibility)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::loadSettings ()
{
	Attributes& attributes = getSettings ();
	sourceStates.load (attributes);

	if(search)
		loadChild (Storage (attributes), *search);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentBlocks::notify (ISubject* subject, MessageRef msg)
{
	if(activeSource && subject == activeSource)
	{
		if(treeModel->getItemView ())
			rebuildList ();
		else
			sourceDirty = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::rebuildList ()
{
	if(inBulkOperation)
		return;

	sourceDirty = false;

	if(search)
		search->cancelSearch ();

	treeModel->rebuild (activeSource);

	signal (Message (kPropertyChanged));

	if(search && activeSource)
		if(search->isShowingResult ())
			search->startSearch (search->getSearchTerms ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentBlocks::initialize (IUnknown* context)
{
	DocumentManager::instance ().addHandler (this);
	loadSettings ();

	// update icons of special folders (addPinnedDocumentsFolder is called before loading skin)
	struct IconUpdater
	{
		IImage* pinnedFolderIcon = RootComponent::instance ().getTheme ()->getImage ("FolderIcon:Pinned");

		void updateIcons (const Container& sources)
		{
			for(auto source : iterate_as<Source> (sources))
			{
				for(auto source : iterate_as<Source> (sources))
					if(ccl_cast<PinnedSource> (source))
						source->setIcon (pinnedFolderIcon);

				updateIcons (source->getChildSources ()); // recursion
			}
		}
	};

	IconUpdater ().updateIcons (sources);
	rebuildList ();

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentBlocks::terminate ()
{
	DocumentManager::instance ().removeHandler (this);
	saveSettings ();

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kActiveSource)
	{
		WaitCursor wc (System::GetGUI ());
		int index = param->getValue ();
		Source* source = (Source*)sources.at (index);
		setActiveSource (source);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentBlocks::appendContextMenu (IContextMenu& contextMenu)
{
	if(ccl_cast<RecentSource> (activeSource))
		DocumentManager::instance ().getRecentPaths ().appendContextMenu (contextMenu);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::appendMoveToFolderMenu (IMenu& menu, const DocumentDescription& description)
{
	struct MenuBuilder
	{
		DocumentBlocks& component;
		Url urltoMove;
		Url oldParentFolder;

		MenuBuilder (DocumentBlocks& component, const DocumentDescription& description)
		: component (component)
		{
			urltoMove = description.getPath ();
			bool hasDedicatedFolder = DocumentPathHelper (urltoMove).hasDedicatedFolder ();
			if(hasDedicatedFolder)
				urltoMove.ascend ();

			oldParentFolder = urltoMove;
			oldParentFolder.ascend ();
		}

		void traverseFolders (IMenu& parentMenu, FolderNode& parentFolder, bool createSubMenu)
		{
			IMenuItem* parentFolderItem = addMoveToFolderCommand (parentMenu, parentFolder, createSubMenu);

			AutoPtr<IMenu> targetMenu;
			if(createSubMenu)
				targetMenu = parentMenu.createMenu ();
			else
			{
				parentMenu.addSeparatorItem ();
				targetMenu.share (&parentMenu);
			}

			for(auto node : parentFolder.getContent ())
				if(auto folderNode = ccl_cast<FolderNode> (node))
					traverseFolders (*targetMenu, *folderNode, true);

			if(createSubMenu && targetMenu->countItems () > 0)
			{
				// add as split menu to parent folder item if possible, or normal subMenu otherwise
				if(parentFolderItem && targetMenu->isExtendedMenu ())
				{
					parentFolderItem->setItemAttribute (IMenuItem::kSplitMenu, static_cast<IMenu*> (targetMenu));

					// even if parent folder is not a possible destination, we must enable the split item to allow access to the submenu
					Variant enabled;
					parentFolderItem->getItemAttribute (enabled, IMenuItem::kItemEnabled);
					if(!enabled)
					{
						AutoPtr<IUrl> targetFolder (NEW Url (component.getTargetFolder (&parentFolder)));
						AutoPtr<ICommandHandler> handler (makeCommandDelegate (&component, &DocumentBlocks::onMoveToFolder, Variant (targetFolder, true)));
						parentFolderItem->setItemAttribute (IMenuItem::kItemHandler, Variant (handler, true));
						parentFolderItem->setItemAttribute (IMenuItem::kItemEnabled, true);
					}
				}
				else
					parentMenu.addMenu (targetMenu.detach ());
			}
		}

		IMenuItem* addMoveToFolderCommand (IMenu& subMenu, FolderNode& folderNode, bool withIcon)
		{
			IMenuItem* menuItem = nullptr;
			Url targetFolder (component.getTargetFolder (&folderNode));
			if(!targetFolder.isEmpty ())
			{
				String name;
				targetFolder.getName (name);
				AutoPtr<Url> folder (NEW Url (targetFolder));

				bool enabled = targetFolder != oldParentFolder && targetFolder != urltoMove;

				AutoPtr<ICommandHandler> handler;
				if(enabled)
					handler = makeCommandDelegate (&component, &DocumentBlocks::onMoveToFolder, Variant (folder->asUnknown (), true));

				menuItem = subMenu.addCommandItem (name, CSTR ("File"), CSTR ("Move to Folder"), handler);
				menuItem->setItemAttribute (IMenuItem::kItemEnabled, enabled);
				if(withIcon)
					menuItem->setItemAttribute (IMenuItem::kItemIcon, FileIcons::instance ().getDefaultFolderIcon ());
			}
			return menuItem;
		}
	};

	AutoPtr<IMenu> subMenu (menu.createMenu ());
	subMenu->setMenuAttribute (IMenu::kMenuTitle, FileStrings::MoveToFolder ());

	MenuBuilder builder (*this, description);
	builder.traverseFolders (*subMenu, *treeModel->getRootFolder (), false);
	menu.addMenu (subMenu.detach ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::appendDocumentMenu (IMenu& menu, const DocumentDescription& description)
{
	AutoPtr<IUrl> path (NEW Url (description.getPath ()));

	menu.addSeparatorItem ();

	if(ccl_cast<FolderSource> (activeSource))
	{
		menu.addCommandItem (CommandWithTitle (CSTR ("File"), CSTR ("New Folder"), FileStrings::MoveToNewFolder ()),
			makeCommandDelegate (this, &DocumentBlocks::onNewFolder, Variant (path, true)), true);

		appendMoveToFolderMenu (menu, description);
	}

	menu.addSeparatorItem ();
	menu.addCommandItem (ShellCommand::getShowFileInSystemTitle (), "File", "Show in Explorer/Finder",
		makeCommandDelegate (this, &DocumentBlocks::showFileInSystem, Variant (path, true)));

	if(activeSource)
	{
		AutoPtr<ObjectList> selectedUrls (NEW ObjectList);
		treeModel->getSelectedUrls (*selectedUrls, false, true);

		if(!selectedUrls->contains (description.getPath ()))
			selectedUrls.release ();

		activeSource->appendDocumentMenu (menu, description, selectedUrls);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::onMoveToFolder (CmdArgs args, VariantRef data)
{
	UnknownPtr<IUrl> targetFolder (data);
	if(targetFolder)
	{
		if(!args.checkOnly ())
		{
			ObjectList urlsToMove;
			static_cast<TreeModel*> (treeModel)->getSelectedUrls (urlsToMove, false, true);

			if(!urlsToMove.isEmpty ())
				moveToFolder (*targetFolder, urlsToMove);
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::onNewFolder (CmdArgs args, VariantRef data)
{
	UnknownPtr<IUrl> url (data);
	if(url)
	{
		if(auto folderSource = ccl_cast<FolderSource> (getActiveSource ()))
		{
			Url parentFolder (*url);
			if(parentFolder.isFile ())
				parentFolder.ascend ();

			if(!folderSource->canCreateFolderIn (parentFolder))
				return false;
		}

		if(!args.checkOnly ())
		{
			if(url->isFolder ())
				return createNewFolder (*url);
			else
			{
				ObjectList urlsToMove;
				static_cast<TreeModel*> (treeModel)->getSelectedUrls (urlsToMove, false, true);
				createNewFolder (*url, &urlsToMove);
			}
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::onRenameFolder (CmdArgs args, VariantRef data)
{
	UnknownPtr<IUrl> url (data);
	if(url && url->isFolder ())
	{
		if(!args.checkOnly ())
		{
			AutoPtr<FileRenamer> renamer (NEW FileRenamer (*url));
			Promise (renamer->runDialogAsync (FileStrings::RenameFileTitle ())).then ([&] (IAsyncOperation& operation)
			{
				if(operation.getState () == IAsyncInfo::kCompleted && operation.getResult ().asInt () == DialogResult::kOkay)
				{
					renamer->tryRename ();

					AutoPtr<Url> newPath (renamer->createNewPath ());
					onFolderMoved (renamer->getOldPath (), *newPath);

					treeModel->storeViewState ();
					rebuildList ();
				}
			});
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::onDeleteFolder (CmdArgs args, VariantRef data)
{
	UnknownPtr<IUrl> url (data);
	if(url && url->isFolder ())
	{
		if(!args.checkOnly ())
		{
			ObjectList urls;
			treeModel->getSelectedUrls (urls, true, false);

			AutoPtr<FileTransferOperation> batchOperation (NEW FileTransferOperation (FileTransferOperation::kDelete));
			for(auto url : iterate_as<Url> (urls))
			{
				Url docFolder;
				if(url->isFolder ())
					batchOperation->addFile (*url);
			}
			Promise (batchOperation->runAsync (FileStrings::DeletingFiles ())).then ([this, batchOperation] (IAsyncOperation& operation)
			{
				PathList* newFolders = getNewFolders ();

				for(auto task : iterate_as<BatchOperation::Task> (batchOperation->getTasks ()))
					if(task->getState () == BatchOperation::Task::kSucceeded)
						if(newFolders && task->getSourcePath ().isFolder ())
							newFolders->removePath (task->getSourcePath ());

				treeModel->storeViewState ();
				rebuildList ();
			});
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::showFileInSystem (CmdArgs args, VariantRef data)
{
	UnknownPtr<IUrl> path (data);
	if(!path)
		return false;

	return ShellCommand::showFileInSystem (*path, args.checkOnly ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::openWithOptions (CmdArgs args, VariantRef data)
{
	UnknownPtr<IUrl> path (data);
	if(!path)
		return false;

	if(args.checkOnly ())
		return true;

	DocumentManager& manager = DocumentManager::instance ();
	
	if(Document* document = manager.findDocument (*path))
		if(!manager.closeDocument (document))
			return false;
	
	return DocumentManager::instance ().openDocument (*path, DocumentManager::kSafetyOptions);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef DocumentBlocks::getTargetFolder (FolderNode* folderNode) const
{
	if(folderNode)
	{
		// root node: base url of folder source
		if(treeModel && folderNode == treeModel->getRootNode ())
			if(auto folderSource = ccl_cast<FolderSource> (getActiveSource ()))
				return folderSource->getPath ();

		return folderNode->getUrl ();
	}
	return Url::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::createNewFolder (UrlRef focusUrl, const Container* urlsToMove)
{
	String folderName;
	if(DialogBox ()->askForString (folderName, CSTR ("Name"), FileStrings::NewFolderTitle (), CCLSTR ("NewFolder")) && !folderName.isEmpty ())
	{
		Url newFolder (focusUrl);
		if(focusUrl.isFile ())
		{
			// new folder in parent folder, or parent of document folder
			newFolder.ascend ();
			if(DocumentPathHelper (focusUrl).hasDedicatedFolder ())
				newFolder.ascend ();
		}
		newFolder.descend (folderName, Url::kFolder);

		// create the new folder
		newFolder.makeUnique ();
		if(File (newFolder).create ())
		{
			// move given documents / folders into the new folder
			if(urlsToMove && !urlsToMove->isEmpty ())
				moveToFolder (newFolder, *urlsToMove);
			else
			{
				if(PathList* folderList = getNewFolders (true))
				{
					folderList->addPath (newFolder);

					treeModel->storeViewState ();
					rebuildList ();
				}
			}
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::moveToFolder (UrlRef targetFolder, const Container& urlsToMove)
{
	AutoPtr<FileTransferOperation> batchOperation (NEW FileTransferOperation (FileTransferOperation::kMove));
	batchOperation->setDestFolder (targetFolder);

	for(auto url : iterate_as<Url> (urlsToMove))
	{
		Url urlToMove (*url);

		Url docFolder;
		if(url->isFile () && DocumentPathHelper (*url).getDedicatedFolder (docFolder))
			urlToMove = docFolder;

		Url oldParentFolder (urlToMove);
		oldParentFolder.ascend ();

		if(urlToMove == targetFolder || oldParentFolder == targetFolder)
			continue;

		batchOperation->addFile (urlToMove); // (can be a folder)
	}

	if(batchOperation->isEmpty ())
		return true;

	Promise (batchOperation->runAsync (FileStrings::MovingFiles ())).then ([this, batchOperation] (IAsyncOperation& operation)
	{
		// adjust recent path entries for moved files
		for(auto task : iterate_as<BatchOperation::Task> (batchOperation->getTasks ()))
			if(task->getState () == BatchOperation::Task::kSucceeded)
			{
				DocumentManager::instance ().getRecentPaths ().relocate (task->getSourcePath (), task->getDestPath ()); // can be file or folder

				onFolderMoved (task->getSourcePath (), task->getDestPath ());
			}

		treeModel->storeViewState ();
		rebuildList ();
	});
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::onFolderMoved (UrlRef oldPath, UrlRef newPath)
{
	if(oldPath.isFolder ())
	{
		// if moved folder was an added (empty) folder, replace it with the target folder, if still empty
		if(PathList* newFolders = getNewFolders ())
			if(newFolders->removePath (oldPath))
				if(File::isFolderEmpty (newPath))
					newFolders->addPath (newPath);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::popupDocumentInfo (const DocumentDescription& description)
{
	StringRef endLine = String::getLineEnd ();

	Url path (description.getPath ());	
	path.ascend ();
	UrlDisplayString pathString (path, Url::kStringDisplayPath);

	String infoText = description.getTitle ();
	infoText << endLine << pathString << endLine << endLine;
	infoText << description.getDateString () << endLine;

	AutoPtr<IAsyncOperation> op = Alert::infoAsync (infoText);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IColumnHeaderList& DocumentBlocks::getColumns () const
{
	ASSERT (treeModel)
	return (treeModel->getColumns ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::selectDocuments (UrlRef path, StringID childSourceId, bool cancelSearch)
{
	if(cancelSearch && search && search->isShowingResult ())
		search->clearSearchTerms ();

	if(activeSource && treeModel)
	{
		String folderName = activeSource->getChildSourceFolderName (childSourceId);
		treeModel->selectDocuments (path, folderName);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentBlocks::onDocumentEvent (IDocument& document, int eventCode)
{
	switch(eventCode)
	{
	case Document::kCreated :
	case Document::kSaveFinished :
	case Document::kDestroyed :
		if(!RootComponent::instance ().isQuitRequested ())
			rebuildList ();
		break;

	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentBlocks::onWorkspaceEvent (const WorkspaceEvent& e)
{
	if(e.animator)
	{
		// provide rect of focus item for transition
		ItemIndex focusIndex;
		IItemView* listView = treeModel->getItemView ();
		if(listView && listView->getFocusItem (focusIndex))
		{
			Rect rect;
			listView->getItemRect (rect, focusIndex, 3); // title column

			Point pos;
			ViewBox (listView).clientToWindow (pos);
			rect.offset (pos);

			AutoPtr<IUIValue> rectValue = GraphicsFactory::createValue ();
			rectValue->fromRect (rect);
			e.animator->setTransitionProperty (IViewAnimator::kFromRect, (IUIValue*)rectValue);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::interpretCommand (const CommandMsg& msg)
{	
	if(search && msg.category == "Edit" && msg.name == "Search")
	{
		if(treeModel->getItemView ())
			return search->interpretCommand (msg);
		else
			return false;
	}
	else if(msg.category == "Navigation" && msg.name == "Back")
	{
		// leave edit or search mode
		auto editComponent = findChildNode<ListViewEditComponent> ();
		if(editComponent && editComponent->isEditMode ())
		{
			if(!msg.checkOnly ())
				editComponent->setEditMode (false);
			return true;
		}
		else if(search && search->isVisible ())
		{
			if(!msg.checkOnly ())
				search->setVisible (false);
			return true;
		}
	}
	else if(msg.category == "Browser" && msg.name == "New Folder")
	{
		if(auto folderSource = ccl_cast<FolderSource> (getActiveSource ()))
		{
			// in root folder of source
			Url parentFolder (folderSource->getPath ());
			return onNewFolder (msg, parentFolder.asUnknown ());
		}
		return false;
	}
	return SuperClass::interpretCommand (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isEmpty")
	{
		var = treeModel->countFlatItems () == 0;
		return true;
	}
	else if(propertyId == "isFolderSource")
	{
		var = ccl_cast<FolderSource> (getActiveSource ()) != nullptr;
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// DocumentBlocks::Item
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DocumentBlocks::Item, TreeViewNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::Item::Item ()
: pinParameter (NEW Parameter)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::Item::setDocumentUrl (UrlRef url)
{
	ASSERT (!description) // not expecting re-assign, otherwise todo: release here, reset icon etc
	documentUrl = url;

	makeSortName (sortName, url); // used for faster comparison
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::Item::assignDocument (const DocumentDescription& document)
{
	setIcon (document.getIcon ());
	getPinParam ()->setValue (DocumentManager::instance ().getRecentPaths ().isPathPinned (document.getPath ()));

	setDocumentUrl (document.getPath ());
	description = NEW DocumentDescription (document);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentDescription& DocumentBlocks::Item::getDescription () const
{
	if(!description)
	{
		// create on demand from Url
		description = NEW DocumentDescription;
		description->assign (getDocumentUrl ());

		if(locationIcons)
		{
			auto getLocationIcon = [] (StringID _locationType)
			{
				MutableCString iconName ("FileLocation:");
				MutableCString locationType (_locationType);
				iconName.append (locationType.toLowercase ());
				return RootComponent::instance ().getTheme ()->getImage (iconName);
			};

			StringID locationType = System::GetFileManager ().getFileLocationType (getDocumentUrl ());			
			IImage* image = getLocationIcon (locationType);
			if(image == nullptr && System::GetFileManager ().isCloudLocationType (locationType))
				image = getLocationIcon (FileLocationType::kCloud);
			if(image)
				description->setIcon (image);		
		}

		ccl_const_cast (this)->setIcon (description->getIcon ());
		getPinParam ()->setValue (DocumentManager::instance ().getRecentPaths ().isPathPinned (getDocumentUrl ()));
	}
	return *description;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::Item::getDetail (Variant& value, StringID id) const
{
	if(id == kAgeID)
	{
		value = getDescription ().getAge ();
		value.share ();
		return true;
	}
	return ListViewItem::getDetail (value, id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::Item::getTooltip (String& tooltip, StringID id)
{
	if(id == TreeModel::kTitleID)
	{		
		tooltip = UrlDisplayString (getDocumentUrl (), Url::kStringDisplayPath);
		return true;
	}
	else if(id == kAgeID)
	{
		tooltip = getDescription ().getDateString ();
		return true;
	}
	else if(id == kPinID)
	{
		if(pinParameter->getValue ().asBool ())
			tooltip = XSTR (UnpinDocument);
		else
			tooltip = XSTR (PinDocument);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::Item::makeSortName (MutableCString& name, UrlRef documentUrl)
{
	String fileName;
	documentUrl.getName (fileName, false);
	name = fileName; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String DocumentBlocks::Item::getFileName () const
{
	String name;
	getDocumentUrl ().toDisplayString (name, IUrl::kStringDisplayName);
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DocumentBlocks::Item::compare (const Object& obj) const
{
	if(auto folderNode = ccl_strict_cast<FolderNode> (&obj))
		return 1; // folder before document

	// using the cached CString (instead of Url::getName) is much faster when sorting large folders
	else if(auto otherItem = ccl_strict_cast<Item> (&obj))
		return sortName.compare (otherItem->sortName, false); // (SuperClass compares title, which is not set for document items)

	ASSERT (0)
	return SuperClass::compare (obj);
}

//************************************************************************************************
// DocumentBlocks::FolderNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DocumentBlocks::FolderNode, TreeViewFolderNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::FolderNode::FolderNode ()
: parentFolder (nullptr),
  sortPriority (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks* DocumentBlocks::FolderNode::getComponent ()
{
	return parentFolder ? parentFolder->getComponent () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::FolderNode::addDocument (UrlRef url, bool sort)
{
	auto item = NEW Item;
	item->setDocumentUrl (url);

	if(sort)
		addSorted (item);
	else
		add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::DocumentSink* DocumentBlocks::FolderNode::addFolder (StringRef name, IImage* icon, UrlRef url, int sortPriority)
{
	return addFolderNode (name, icon, url, sortPriority);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::FolderNode* DocumentBlocks::FolderNode::addFolderNode (StringRef name, IImage* icon, UrlRef url, int sortPriority)
{
	auto subFolderNode = NEW FolderNode;
	subFolderNode->setTitle (name);
	subFolderNode->setIcon (icon);
	subFolderNode->setSortPriority (sortPriority);
	subFolderNode->setUrl (url);
	subFolderNode->setParentFolder (this);

	addSorted (subFolderNode);
	return subFolderNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::Item* DocumentBlocks::FolderNode::findDocumentItem (Url url) const
{
	MutableCString sortName;
	Item::makeSortName (sortName, url);

	return getContent ().findIf<Item> ([&] (Item& item)
	{
		return item.getSortName () == sortName // quick check first, isEqualUrl is slow
			&& item.getDocumentUrl ().isEqualUrl (url);
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::FolderNode::removeFolder ()
{
	if(parentFolder && parentFolder->remove (this))
		release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::FolderNode::flattenFolder ()
{
	// don't flatten an explicitly added folder
	DocumentBlocks* component = getComponent ();
	PathList* folderList = component  ? component->getNewFolders () : nullptr;
	if(folderList && folderList->contains (getUrl ()))
		return;

	if(parentFolder && parentFolder->remove (this))
	{
		// "skip" this folder: move all sub nodes to our parent
		for(auto n : content)
		{
			auto childNode = ccl_cast<TreeViewNode> (n);
			ASSERT (childNode)
			if(childNode)
			{
				parentFolder->addSorted (return_shared (childNode));
				if(auto subFolder = ccl_cast<FolderNode> (childNode))
					subFolder->setParentFolder (parentFolder);
			}
		}
		release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DocumentBlocks::FolderNode::compare (const Object& obj) const
{
	auto otherFolderNode = ccl_strict_cast<FolderNode> (&obj);
	if(!otherFolderNode)
		return -1; // folder before document

	int otherSortPriority = otherFolderNode->getSortPriority ();
	if(sortPriority != otherSortPriority)
		return otherSortPriority - sortPriority;

	return SuperClass::compare (obj);
}

//************************************************************************************************
// DocumentBlocks::ViewState
//************************************************************************************************

DEFINE_CLASS (DocumentBlocks::ViewState, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

IViewStateHandler* DocumentBlocks::ViewState::getExpandState (ITreeItem& rootItem)
{
	if(!expandState && expandStateAttribs)
	{
		expandState = rootItem.createExpandState ();
		ASSERT (expandState)
		expandState->loadViewState (nullptr, nullptr, *expandStateAttribs, nullptr);
	}
	return expandState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::ViewState::setExpandState (IViewStateHandler* state)
{
	expandState = state;
	expandStateAttribs = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::ViewState::load (const Storage& storage)
{
	Attributes& attribs (storage.getAttributes ());
	attribs.get (name, "name");

	expandStateAttribs.share (attribs.getAttributes ("state"));
	expandState = nullptr;

	if(Attributes* a2 = storage.getAttributes ().getAttributes ("folders"))
		newFolders.load (Storage (*a2, storage));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::ViewState::save (const Storage& storage) const
{
	Attributes& attribs (storage.getAttributes ());
	attribs.set ("name", name);    
	
	if(expandStateAttribs || expandState)
	{
		if(!expandStateAttribs)
		{
			expandStateAttribs = NEW Attributes;
			expandState->saveViewState (nullptr, nullptr, *expandStateAttribs, nullptr);
		}
		attribs.set ("state", expandStateAttribs, Attributes::kShare);
	}

	AutoPtr<Attributes> a2 = NEW Attributes;
	if(newFolders.save (Storage (*a2, storage)) && !a2->isEmpty ())
		storage.getAttributes ().set ("folders", a2, Attributes::kShare);
	return true;
}

//************************************************************************************************
// DocumentBlocks::TreeModel
//************************************************************************************************

static const CString kRestoreVersion ("restoreVersion");

DocumentBlocks::TreeModel::TreeModel (DocumentBlocks& component)
: component (component),
  editItem (nullptr),
  editData (nullptr),
  separatorColor (Colors::kTransparentBlack),
  schemeSink (Signals::kGUI),
  hasViews (false),
  inRebuild (false)
{
	schemeSink.setObserver (this);

	rootNode = NEW RootFolderNode (&component);

	getColumns ().addColumn (260, nullptr, kTitleID); // (tree column is always index 0)
	getColumns ().addColumn (20, nullptr, kPinID);
	getColumns ().addColumn (44, nullptr, kEditSelectID, 0, IColumnHeaderList::kEditMode);
	getColumns ().addColumn (24, nullptr, kIconID);
	getColumns ().addColumn (150, nullptr, kAgeID);

	getColumns ().moveColumn (kTitleID, 4);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::TreeModel::~TreeModel ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::Item* DocumentBlocks::TreeModel::resolveDocumentItem (ItemIndexRef index)
{
	return ccl_cast<Item> (resolveNode (index));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::FolderNode* DocumentBlocks::TreeModel::getRootFolder () const
{
	return static_cast<FolderNode*> (getRootNode ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::TreeModel::isEditMode () const
{
	return component.isEditMode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::TreeModel::enableEditCommands (bool state)
{
	if(ListViewEditComponent* editComponent = component.findChildNode<ListViewEditComponent> ())
		editComponent->enableEditCommands (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::TreeModel::addDocumentItem (UrlRef url, bool sort)
{
	if(auto rootFolder = getRootFolder ())
		if(!rootFolder->findDocumentItem (url)) // check for duplicates
			rootFolder->addDocument (url, sort);

	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::FolderNode* DocumentBlocks::TreeModel::findFolderNode (UrlRef url, FolderSource& folderSource, bool create)
{
	if(url.isEqualUrl (folderSource.getPath ()))
		return getRootFolder ();
	else
	{
		// find parent folder node (recursion)
		Url parentFolder (url);
		if(parentFolder.ascend () == false)
			return nullptr;
		FolderNode* parentNode = findFolderNode (parentFolder, folderSource, create);
		if(parentNode)
		{
			// find folder in parent node
			for(auto node : parentNode->getContent ())
				if(auto subFolder = ccl_cast<FolderNode> (node))
					if(subFolder->getUrl () == url)
						return subFolder;

			if(create)
			{
				String name;
				url.getName (name);
				return parentNode->addFolderNode (name, nullptr, url);
			}
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::TreeModel::addEmptyFolders ()
{
	// create nodes for folders the user has created (but contain no documents yet, so they wouldn't appear)
	if(PathList* folderList = component.getNewFolders ())
		if(auto folderSource = ccl_cast<FolderSource> (component.getActiveSource ()))
		{
			AutoPtr<Iterator> iter (folderList->newIterator ());
			for(auto url : iterate_as<Url> (*iter))
				findFolderNode (*url, *folderSource, true);
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::TreeModel::rebuild (Source* source)
{
	makeViews (false);

	if(auto rootFolder = getRootFolder ())
	{
		ScopedVar scope (inRebuild, true);

		rootFolder->removeAll ();

		if(source)
			source->getDocuments (*rootFolder);

		addEmptyFolders ();

		signal (Message (kChanged));
		if(getTreeView ())
			signal (Message (IItemModel::kNewRootItem));

		restoreViewState ();
	}

	makeViews (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::TreeModel::storeViewState ()
{
	ITreeView* treeView = getTreeView ();
	ITreeItem* rootItem = treeView ? treeView->getRootItem () : nullptr;
	if(rootItem)
		if(ViewState* state = component.getSourceState (component.getActiveSource (), true))
			state->setExpandState (treeView->getRootItem ()->storeExpandState ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::TreeModel::restoreViewState ()
{
	ITreeView* treeView = getTreeView ();
	ITreeItem* rootItem = treeView ? treeView->getRootItem () : nullptr;
	ViewState* state = component.getSourceState (component.getActiveSource (), false);
	if(rootItem && state)
		if(IViewStateHandler* expandState = state->getExpandState (*rootItem))
		{
			rootItem->restoreExpandState (expandState);
			if(ViewBox (treeView).getStyle ().isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
				treeView->expandItem (rootItem, true);

			UnknownPtr<IObserver> treeViewObj (treeView);
			if(treeViewObj)
				treeViewObj->notify (nullptr, Message ("updateSize"));
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::TreeModel::makeViews (bool state)
{
	if(state == hasViews || inRebuild)
		return;

	IItemView* itemView = getItemView ();
	if(!itemView)
		return;

	ViewBox itemViewBox (itemView);
	if(!itemViewBox.isAttached ())
		return;

	const IVisualStyle& style = itemViewBox.getVisualStyle ();
	Color linkColor = style.getColor ("linkcolor");
	this->separatorColor = style.getColor ("separatorcolor", Colors::kTransparentBlack);

	AutoPtr<IVisualStyle> linkStyle = ccl_new<IVisualStyle> (ClassID::VisualStyle);
	linkStyle->setColor ("linkcolor", linkColor);
	linkStyle->setFont (StyleID::kTextFont, style.getTextFont ());
	linkStyle->setOptions (StyleID::kTextAlign, Alignment::kLeftCenter);

	AutoPtr<IVisualStyle> renameStyle = ccl_new<IVisualStyle> (ClassID::VisualStyle);
	renameStyle->setColor (StyleID::kBackColor, style.getColor ("renameBackColor"));
	renameStyle->setColor (StyleID::kTextColor, style.getTextColor ());
	renameStyle->setFont (StyleID::kTextFont, style.getTextFont ());
	renameStyle->setOptions (StyleID::kTextAlign, Alignment::kLeftCenter);
	bool fullRenameHeight = style.getMetric ("fullRenameHeight", false);

	AutoPtr<IVisualStyle> pinStyle = ccl_new<IVisualStyle> (ClassID::VisualStyle);
	IImage* pinIcon = style.getImage ("pinicon");
	Rect pinSize (0, 0, 12, 12);
	if(pinIcon)
	{
		pinSize (0, 0, pinIcon->getWidth (), pinIcon->getHeight ());
		pinStyle->setImage (StyleID::kBackground, pinIcon);
	}

	hasViews = state;
	if(state)
	{
		int pinIndex = getColumnIndex (kPinID);
		int titleIndex = getColumnIndex (kTitleID);

		UnknownPtr<ITreeView> treeView (itemView);

		visitItems ([&] (ListViewItem& item)
		{
			if(auto docItem = ccl_cast<Item> (&item))
			{
				ItemIndex itemIndex;
				getIndex (itemIndex, &item);

				Rect titleRect;
				itemView->getItemRect (titleRect, itemIndex, titleIndex); // rect of title column
				if(!titleRect.isEmpty ())
				{
					titleRect.top++;
					titleRect.bottom--;

					docItem->getDescription ();

					if(treeView)
						titleRect.left += treeView->getItemTextInset (itemIndex.getTreeItem ());

					IView* titleView = nullptr;
					if(isEditMode ())
					{
						String name (docItem->getFileName ());

						AutoPtr<IParameter> param = NEW StringParam;
						param->fromString (name);
						param->connect (this, Tag::kRenameDocument);

						String urlString;
						docItem->getDocumentUrl ().getUrl (urlString, true);
						param->setName (MutableCString (urlString, Text::kUTF8));

						if(fullRenameHeight)
							titleRect.top--;
						else
							titleRect.contract (6);
						ControlBox editBox (CCL::ClassID::EditBox, param, titleRect, StyleFlags (Styles::kTransparent));
						editBox.setVisualStyle (renameStyle);
						titleView = editBox;
					}
					else
					{
			 			ControlBox linkView (ClassID::LinkView, nullptr, titleRect, StyleFlags (0, Styles::kLinkViewAppearanceFitTitle), docItem->getDescription ().getTitle ());
			 			linkView.setVisualStyle (linkStyle);
			 			titleView = linkView;
					}

					if(titleView)
					{
						itemViewBox.getChildren ().add (titleView);
						ASSERT (!docItem->getTitleView ())
						docItem->setTitleView (titleView);
					}

					if(pinIcon)
					{
						Rect pinRect;
						itemView->getItemRect (pinRect, itemIndex, pinIndex); // rect of pin column
						ASSERT (!pinRect.isEmpty ())

						ControlBox pinButton (ClassID::Toggle, docItem->getPinParam (), Rect (pinSize).center (pinRect));
						pinButton.setVisualStyle (pinStyle);
						itemViewBox.getChildren ().add (pinButton);
						ASSERT (!docItem->getPinView ())
						docItem->setPinView (pinButton);
					}
				}
			}
			return true;
		});
	}
	else
	{
		visitItems ([&] (ListViewItem& item)
		{
			if(auto docItem = ccl_cast<Item> (&item))
			{
				if(IView* view = docItem->getTitleView ())
					if(IView* parent = view->getParentView ())
					{
						parent->getChildren ().remove (view);
						view->release ();
					}
				if(IView* view = docItem->getPinView ())
					if(IView* parent = view->getParentView ())
					{
						parent->getChildren ().remove (view);
						view->release ();
					}
			}
			return true;
		});
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::TreeModel::onVisibleChanged (bool state)
{
	if(state)
	{
		// hide icon column in tree mode (already part of tree column)
		getColumns ().hideColumn (kIconID, getTreeView () != nullptr);
		updateColumns ();

		makeViews (false);
		
		if(component.sourceDirty && !RootComponent::instance ().isQuitRequested ())
			component.rebuildList ();

		restoreViewState ();
	}
	else
		storeViewState ();

	makeViews (state);
	schemeSink.enable (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::TreeModel::onColumnRectsChanged ()
{
	ViewBox view (getItemView ());
	if(view && view.isAttached ())
	{
		makeViews (false);
		makeViews (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::TreeModel::getUniqueItemName (MutableCString& name, ItemIndexRef index)
{
	name.empty ();
	if(TreeViewNode* node = resolveNode (index))
	{
		if(node == rootNode && component.getActiveSource ())
			name = component.getActiveSource ()->getID ();
		else
			name.append (node->getTitle (), Text::kUTF8);
	}
	return !name.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::TreeModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	// fill background of all columns for delete candidates if visual style provides color
	ListViewItem* item = resolve (index);
	if(item && item->isChecked () && component.isEditMode ())
	{
		const IVisualStyle& vs = ViewBox (info.view).getVisualStyle ();
		Color deleteColor = vs.getColor ("deleteItemBackColor", Colors::kTransparentBlack);
		if(deleteColor.alpha != 0)
		{
			Rect r (info.rect);
			r.bottom--;
			info.graphics.fillRect (r, SolidBrush (deleteColor));		
		}
	}

	tbool result = SuperClass::drawCell (index, column, info);

	// draw bottom separator
	Coord y = info.rect.bottom - 1;

	#if 0
	Coord left = info.rect.left;
	if(getColumnID (column) == kTitleID && getTreeView ())
	{
		// tree view shortens the given rect by folder indent + icon, wee need the full cell rect
		Rect titleRect;
		getItemView ()->getItemRect (titleRect, index, getColumnIndex (kTitleID));
		left = titleRect.left;
		left -= ViewBox (getItemView ()).getVisualStyle ().getMetric ("marginH", 4); // hmm, getItemRect still applies a margin for the tree column...

		info.graphics.drawLine (Point (left, y), Point (info.rect.right, y), Pen (separatorColor));
	}
	#else
	// alternative approach: draw over full view width, but only when called for column 0 (must not draw multiple times when color is half transparent)
	if(column == 0)
	{
		Coord width = ViewBox (getItemView ()).getWidth ();
		info.graphics.drawLine (Point (0, y), Point (width, y), Pen (separatorColor));
	}
	#endif

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::TreeModel::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	SharedPtr<Item> item (ccl_cast<Item> (resolve (index)));
	if(!item)
		return false;

	bool isLeftClick = true;
	bool isDrag = false;
	bool isTap = false;
	bool canSelect = getItemView () && ViewBox (getItemView ()).getStyle ().isCustomStyle (Styles::kItemViewBehaviorSelection);

	const MouseEvent* mouseEvent = info.editEvent.as<MouseEvent> ();
	if(mouseEvent)
	{
		if(info.editEvent.eventType != MouseEvent::kMouseDown)
			return false;

		isLeftClick = mouseEvent->keys.isSet (KeyState::kLButton);

		if(info.view)
		{
			bool dragItems = (info.view->getStyle ().isCustomStyle (Styles::kItemViewBehaviorNoDrag) == false);
			if(dragItems)
				isDrag = info.view->detectDrag (*mouseEvent) > 0;
		}
	}
	else if(const GestureEvent* gesture = info.editEvent.as<GestureEvent> ())
	{
		auto type = gesture->getType ();
		if(type != GestureEvent::kSingleTap && type != GestureEvent::kDoubleTap)
			return false;

		isTap = (type == GestureEvent::kSingleTap);
	}
	else
		return false;

	ActionCode actionCode = kNone;

	// determine click action
	MutableCString columnID;
	getColumnType (columnID, column);

	// tree view does snot distinguish icon and title (tree) column
	if(getTreeView () && mouseEvent && columnID == kTitleID)
		Debugger::printf ("%\n", mouseEvent->where.x - info.rect.left);
	if(getTreeView () && mouseEvent && columnID == kTitleID)
		if(mouseEvent->where.x - info.rect.left < getTreeView ()->getItemTextInset (index.getTreeItem ()))
			columnID = kIconID;

	if(columnID == kIconID)
	{
		if(isDrag)
			actionCode = kDragDocument;
		#if 0 // showing a menu on left click can be confusing at least in tree mode
		else if(DocumentVersions::isSupported ())
			actionCode = kPopupActionMenu;
		#endif
		else if(isEditMode () == false)
		{
			if(isTap)
				actionCode = kPopupDocumentInfo;
			
			else if(mouseEvent && isLeftClick)
			{
				if(info.view->detectDoubleClick (*mouseEvent) != 0)
					actionCode = kOpenDocument;			
			}
		}
	}
	else if(columnID == kTitleID || columnID == kAgeID)
	{
		if(isLeftClick)
		{
			actionCode = isEditMode () ?  kRenameDocument : kOpenDocument;

			if(columnID == kAgeID && canSelect)
				actionCode = kNone; // let itemview perfom selection, dragging on the age column
		}
		else
			actionCode = kPopupActionMenu;
	}
	else if(columnID == kPinID)
		actionCode = kPinDocument;
	else if(columnID == kEditSelectID)
	{
		SuperClass::editCell (index, column, info);
		enableEditCommands (isAnyItemChecked ());
		return true;
	}
	
	if(actionCode == kPopupActionMenu)
	{
		if(DocumentVersions::isSupported ())
		{
			ObjectArray versions;
			versions.objectCleanup (true);
			bool result = false;
			{
				WaitCursor wc (System::GetGUI ());
				result = DocumentVersions (item->getDocumentUrl ()).buildHistory (versions);
			}

			ScopedVar<Item*> editScope1 (editItem, item);
			ScopedVar<Object*> editScope2 (editData, &versions);

			AutoPtr<ListParam> versionList = NEW CustomizedMenuParam (nullptr, MenuPresentation::kExtended);
			if(versions.isEmpty ())
				versionList->appendString (XSTR (NoVersionsFound));
			else
				ForEach (versions, DocumentDescription, v)
					versionList->appendString (v->getSummary ());
				EndFor

			if(!result)
				versionList->appendString (XSTR (AdditionalVersions));

			versionList->connect (this, 'Vers');
			versionList->setSignalAlways (true);
			versionList->setOutOfRange (true); // no selection

			UnknownPtr<IItemView> itemView (info.view);
			if(itemView && !itemView->getSelection ().isSelected (index))
			{
				itemView->selectAll (false);
				itemView->selectItem (index, true);
			}

			doPopup (versionList, info);
			return true;
		}
		else
		{
			ScopedVar<Item*> editScope1 (editItem, item);

			AutoPtr<ListParam> menu = NEW CustomizedMenuParam (nullptr, MenuPresentation::kExtended);
			menu->connect (this, 'Cntx');			
			menu->setOutOfRange (true); // no selection
			doPopup (menu, info);
			return true;
		}
	}
	else if(actionCode == kPopupDocumentInfo)
	{
		component.popupDocumentInfo (item->getDescription ());
		return true;
	}
	else if(actionCode == kOpenDocument)
	{
		DocumentManager::instance ().deferOpenDocument (item->getDocumentUrl (), true);
		return true;
	}
	else if(actionCode == kRenameDocument)
	{
		return true;
	}
	else if(actionCode == kDragDocument)
	{
		AutoPtr<IDragSession> session = ccl_new<IDragSession> (ClassID::DragSession);
		session->setSource (info.view);
		session->getItems ().add (ccl_as_unknown (NEW Url (item->getDocumentUrl ())));
		session->drag ();
		return true;
	}
	else if(actionCode == kPinDocument)
	{
		storeViewState ();

		IParameter* p = item->getPinParam ();
		bool state = !p->getValue ().asBool ();
		UrlRef path = item->getDocumentUrl ();
		RecentDocuments& recentPaths = DocumentManager::instance ().getRecentPaths ();
		if(state)
		{
			if(!recentPaths.contains (path))
				recentPaths.setRecentPath (path);
			recentPaths.setPathPinned (path, true);
		}
		else
			recentPaths.setPathPinned (path, false);
		p->setValue (state);
		return true;
	}

	return SuperClass::editCell (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::TreeModel::canRemoveItem (ItemIndexRef index)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::TreeModel::removeItem (ItemIndexRef index)
{
	if(SharedPtr<Item> item = resolveDocumentItem (index))
		if(component.getActiveSource () && component.getActiveSource ()->removeDocument (item->getDescription ()))
		{
			component.rebuildList ();
			// don't return true, view would try to select the now deleted TreeItem
		}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::TreeModel::canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView)
{
	if(session && ccl_cast<FolderSource> (component.getActiveSource ()))
		if(UnknownPtr<IItemView> itemView = targetView)
		{
			AutoPtr<DragHandler> handler (NEW FolderDragHandler (targetView, this));
			if(handler->prepare (data, session))
			{
				session->setDragHandler (handler);
				return true;
			}
		}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::TreeModel::insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session)
{
	if(auto folderSource = ccl_cast<FolderSource> (component.getActiveSource ()))
	{
		Url targetFolder;

		if(auto folderDragHandler = session ? unknown_cast<FolderDragHandler> (session->getDragHandler ()) : nullptr)
			targetFolder = folderDragHandler->getTargetFolder ();
		else
		{
			auto folderNode = ccl_cast<FolderNode> (resolveNode (index));
			if(folderNode)
				targetFolder = folderNode->getUrl ();
			else if(!index.isValid ())
				targetFolder = folderSource->getPath ();
		}

		DragDataExtractor dataExtractor;
		dataExtractor.construct<FolderDragHandler> (data, session);
		if(dataExtractor.getData () && !targetFolder.isEmpty ())
		{
			ObjectList urlsToMove;
			urlsToMove.objectCleanup (true);
			ForEachUnknown (*dataExtractor.getData (), unk)
				UnknownPtr<IUrl> url (unk);
				if(url)
					urlsToMove.add (NEW Url (*url));
			EndFor

			return component.moveToFolder (targetFolder, urlsToMove);
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* DocumentBlocks::TreeModel::getUrl (ListViewItem& item)
{
	if(auto documentItem = ccl_cast<Item> (&item))
		return &documentItem->getDocumentUrl ();

	else if(auto folderNode = ccl_cast<FolderNode> (&item))
		return &folderNode->getUrl ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::TreeModel::getSelectedUrls (Container& urls, bool wantFolders, bool wantDocuments)
{
	urls.objectCleanup (true);

	auto addUrl = [&] (ListViewItem& item)
	{
		if(auto documentItem = ccl_cast<Item> (&item))
		{
			if(wantDocuments)
				urls.add (NEW Url (documentItem->getDocumentUrl ()));
		}
		else if(auto folderNode = ccl_cast<FolderNode> (&item))
		{
			if(wantFolders)
				urls.add (NEW Url (folderNode->getUrl ()));
		}
	};

	visitSelectedItems ([&] (ListViewItem& item)
	{
		addUrl (item);
		return true;
	});

	// fallback to focus item
	if(urls.isEmpty ())
		if(ListViewItem* focusItem = getFocusItem ())
			addUrl (*focusItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::TreeModel::selectDocuments (UrlRef path, StringRef folderName)
{
	ITreeView* treeView = getTreeView ();
	ASSERT (treeView)
	if(!treeView)
		return;
	
	FolderNode* folderNode = nullptr;
	if(folderName.isEmpty ())
	{
		auto folderSource = ccl_cast<FolderSource> (component.getActiveSource ());
		folderNode = findFolderNode (path, *folderSource, false);
	}
	else
	{
		auto rootFolder = getRootFolder ();
		for(auto folder : iterate_as<FolderNode> (rootFolder->getContent ()))
			if(folder->getTitle () == folderName)
			{
				folderNode = folder;
				break;
			}
	}
	if(folderNode)
	{
		ITreeItem* rootItem = treeView ? treeView->getRootItem () : nullptr;
		if(rootItem)
		{
			ITreeItem* treeItem = rootItem->findChild (folderNode->asUnknown ());
			treeView->expandItem (treeItem, true, ITreeView::ExpandModes::kExpandParents);
		}
	}

	bool itemVisible = false;
	UnknownPtr<IItemView> itemView (getTreeView ());
	if(itemView)
	{
		itemView->selectAll (false);
		visitItems ([&] (ListViewItem& listItem)
		{
			if(Item* fileItem = ccl_cast<Item> (&listItem))
			{
				Url filePath (fileItem->getDocumentUrl ());
				Url folderPath (path);
				if(folderPath.contains (filePath))
				{
					ItemIndex index;
					if(getIndex (index, &listItem))
					{
						itemView->selectItem (index, true);
						if(!itemVisible)
						{
							itemView->makeItemVisible (index);
							itemVisible = true;
						}
					}
					
				}
			}
			return true;
		});
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::TreeModel::appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection)
{
	if(ccl_cast<FolderSource> (component.getActiveSource ()))
	{
		if(auto folderNode = ccl_cast<FolderNode> (resolveNode (item)))
		{
			AutoPtr<Url> url (NEW Url (folderNode->getUrl ()));
			menu.addCommandItem (CommandWithTitle (CSTR ("Browser"), CSTR ("New Folder"), FileStrings::NewFolder ()),
				makeCommandDelegate (&component, &DocumentBlocks::onNewFolder, Variant (url->asUnknown (), true)), true);

			menu.addCommandItem (CommandWithTitle (CSTR ("Browser"), CSTR ("Rename Folder"), FileStrings::RenameFolder ()),
				makeCommandDelegate (&component, &DocumentBlocks::onRenameFolder, Variant (url->asUnknown (), true)), true);

			menu.addCommandItem (CommandWithTitle (CSTR ("Browser"), CSTR ("New Folder"), FileStrings::DeleteFolder ()),
				makeCommandDelegate (&component, &DocumentBlocks::onDeleteFolder, Variant (url->asUnknown (), true)), true);
			
			menu.addSeparatorItem ();
			menu.addCommandItem (ShellCommand::getShowFileInSystemTitle (), "File", "Show in Explorer/Finder",
				makeCommandDelegate (&component, &DocumentBlocks::showFileInSystem, Variant (url->asUnknown (), true)));
		}
	}
	return SuperClass::appendItemMenu (menu, item, selection);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::TreeModel::interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection)
{
	if(msg.category == "Edit" && (msg.name == "Remove From List" || msg.name == "Delete"))
	{
		Source* source = component.getActiveSource ();
		RecentSource* recent = ccl_cast<RecentSource> (source);
		FolderSource* folderSource = ccl_cast<FolderSource> (source);
				
		if(recent || folderSource)
		{				
			if(msg.checkOnly () || selection.isEmpty ())
				return true;

			if(folderSource && msg.name != "Delete")
				return true;

			IItemView* itemView = getItemView ();
	
			LinkedList<int> toDelete;
			ForEachItem (selection, idx)
				toDelete.addSorted (idx.getIndex ());
			EndFor

			component.beginBulkOperation ();

			ListForEachReverse (toDelete, int, idx)
				if(SharedPtr<Item> item = resolveDocumentItem (idx))
				{
					if(folderSource)
					{
						folderSource->removeDocument (item->getDescription ());
					}
					else if(recent)
					{
						if(msg.name == "Delete")
							recent->removeDocument (item->getDescription ());
						else
							recent->removeFromRecentList (item->getDocumentUrl ());
					}
					if(itemView)
						itemView->selectItem (idx, false);
				}
			EndFor

			component.endBulkOperation ();
		}		
		return true;
	}
	return SuperClass::interpretCommand (msg, item, selection);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::TreeModel::paramChanged (IParameter* param)
{
	if(param->getTag () == 'Vers')
	{
		ASSERT (editItem && editData)
		if(editItem && editData)
		{
			DocumentDescription* version = nullptr;
			Container* versions = (Container*)editData;
			if(!versions->isEmpty ())
				version = (DocumentDescription*)versions->at (param->getValue ());

			if(version)
				(NEW Message (kRestoreVersion, ccl_const_cast (editItem->getDescription ()).asUnknown (), version->asUnknown ()))->post (this);
		}
	}
	else if(param->getTag () >= Tag::kRenameDocument)
	{
		// from EditBox
		String urlString (Text::kUTF8, param->getName ());
		AutoPtr<IUrl> url (NEW Url (urlString));
		String name (param->getValue ().asString ());
		(NEW Message ("renameItem", Variant (url, true), Variant (name, true)))->post (this);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentBlocks::TreeModel::notify (ISubject* subject, MessageRef msg)
{
	if(msg == ITreeView::kItemExpanded)
	{
		if(hasViews)
		{
			makeViews (false);
			makeViews (true);
		}
	}
	else if(msg == Signals::kColorSchemeChanged)
	{
		UnknownPtr<IColorScheme> scheme (msg[0].asUnknown ());
		ASSERT (scheme)

		ViewBox view (getItemView ());
		if(hasViews && view && view.isAttached () && !view.getChildren ().isEmpty ())
		{
			const IVisualStyle& visualStyle = view.getVisualStyle ();
			if(visualStyle.hasReferences (*scheme))
			{
				makeViews (false);
				makeViews (true);
			}
		}
	}
	else if(msg == kRestoreVersion)
	{
		DocumentDescription* d = unknown_cast<DocumentDescription> (msg[0].asUnknown ());
		DocumentDescription* v = unknown_cast<DocumentDescription> (msg[1].asUnknown ());
		ASSERT (d && v)
		DocumentVersions (d->getPath ()).restoreDocumentVersion (v->getPath ());
	}
	else if(msg == IParameter::kExtendMenu)
	{
		UnknownPtr<IMenu> menu (msg.getArg (0));
		ASSERT (menu && editItem)
		if(menu && editItem)
		{
			if(editData) // versions not supported in any app
			{
				Container* versions = (Container*)editData;
				if(!versions->isEmpty ())
					for(int i = 0; i < menu->countItems (); i++)
					{
						IMenuItem* menuItem = menu->getItem (i);
						menuItem->setItemAttribute (IMenuItem::kItemIcon, Variant (editItem->getIcon ()));
					}

				menu->addSeparatorItem ();

				AutoPtr<IUrl> path (NEW Url (editItem->getDocumentUrl ()));
				menu->addCommandItem (CommandRegistry::find ("File", "Open with Options"),
					CommandDelegate<DocumentBlocks>::make (&component, &DocumentBlocks::openWithOptions, Variant (path, true)), true);
			}

			if(menu->isExtendedMenu ())
			{
				UnknownPtr<IExtendedMenu> extendedMenu (menu);
				if(extendedMenu)
				{
					AutoPtr<IFileInfoComponent> component = FileInfoRegistry::instance ().createComponent (editItem->getDocumentUrl ());
					if(component)
					{
						String fileInfo;
						component->getFileInfoString (fileInfo, IFileInfoComponent::kFileInfo2);
						if(fileInfo.isEmpty () == false)
						{
							fileInfo.append (CCLSTR (" - "));
							fileInfo.append (editItem->getDescription ().getDateString ());
							MenuInserter inserter (menu, 0);
							extendedMenu->addHeaderItem (fileInfo);
							menu->addSeparatorItem ();
						}
					}
				}
			}

			component.appendDocumentMenu (*menu, editItem->getDescription ());
		}
	}
	else if(msg == "renameItem")
	{
		UnknownPtr<IUrl> url (msg[0]);
		String name = msg[1].asString ();
		if(url)
		{
			DocumentManager::instance ().renameDocument (*url, &name);
			component.rebuildList (); 
		}	
	}
	else
		SuperClass::notify (subject, msg);
}

//************************************************************************************************
// DocumentBlocks::FolderDragHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DocumentBlocks::FolderDragHandler, DragHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::FolderDragHandler::FolderDragHandler (IView* view, TreeModel* model)
: DragHandler (view),
  model (model),
  itemView (view),
  dragToRoot (false)
{
	if(itemView)
		setChildDragHandler (itemView->createDragHandler (IItemView::kCanDragOnItem|IItemView::kDropInsertsData, this));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* DocumentBlocks::FolderDragHandler::prepareDataItem (IUnknown& item, IUnknown* context)
{
	UnknownPtr<IUrl> url (&item);
	if(!url)
	{
		if(auto node = unknown_cast<TreeViewNode> (&item))
			url = ccl_const_cast (TreeModel::getUrl (*node));
	}

	if(url && url->isNativePath () && !File (*url).isWriteProtected ())
		return ccl_as_unknown (NEW Url (*url));

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::FolderDragHandler::finishPrepare ()
{
	spriteBuilder.addHeader (nullptr);

	ForEachUnknown (data, unk)
		UnknownPtr<IUrl> url (unk);
		if(url)
		{
			// can't drag into same parent folder
			Url movedUrl (*url);
			if(DocumentPathHelper (movedUrl).hasDedicatedFolder ())
				movedUrl.ascend ();

			Url parentFolder (movedUrl);
			if(parentFolder.ascend ())
				forbiddenTargetFolders.addPath (parentFolder);

			forbiddenTargetFolders.addPath (movedUrl);     // can't drag folder into itself
			forbiddenTargetFoldersDeep.addPath (movedUrl); // can't drag folder into childs of itself

			// sprite
			AutoPtr<IImage> icon (FileIcons::instance ().createIcon (*url));
			String fileName;
			url->getName (fileName, true);
			spriteBuilder.addItem (icon, fileName);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::FolderDragHandler::canMoveInto (UrlRef targetFolder) const
{
	if(forbiddenTargetFolders.contains (targetFolder) || forbiddenTargetFoldersDeep.containsSubPath (targetFolder))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::FolderDragHandler::checkTargetNode (FolderNode& node) const
{
	Url folder (getTargetFolder (&node));
	if(File (folder).isWriteProtected ())
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::FolderDragHandler::setTargetNode (TreeViewNode* node)
{
	auto folderNode = ccl_cast<FolderNode> (node);
	if(folderNode && checkTargetNode (*folderNode))
	{
		targetNode = folderNode;
		CCL_PRINTF ("target folder: %s\n", MutableCString (UrlFullString (getTargetFolder ())).str ())
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef DocumentBlocks::FolderDragHandler::getTargetFolder (FolderNode* folderNode) const
{
	return model->getComponent ().getTargetFolder (folderNode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef DocumentBlocks::FolderDragHandler::getTargetFolder () const
{
	return getTargetFolder (targetNode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::FolderDragHandler::verifyTargetItem (ItemIndex& item, int& relation)
{
	targetNode = nullptr;

	if(model)
	{
		TreeViewNode* dragNode = nullptr;
		if(!isDragToRoot ())
			dragNode = ccl_cast<TreeViewNode> (model->resolveNode (item));

		if(dragNode)
		{
			if(setTargetNode (dragNode))
				return true;

			if(auto itemNode = ccl_cast<Item> (dragNode))
			{
				// find parent folder containing the document item
				AutoPtr<IRecognizer> r (Recognizer::create ([&] (IUnknown* data)
				{
					auto folder = unknown_cast<FolderNode> (data);
					return folder && folder->getContent ().contains (itemNode);
				}));

				UnknownPtr<ITreeView> treeView (itemView);
				ITreeItem* rootItem = treeView ? treeView->getRootItem () : nullptr;
				ITreeItem* parentItem = rootItem ? rootItem->findItem (r, false) : nullptr;
				auto parentFolder = parentItem ? unknown_cast<FolderNode> (parentItem->getData ()) : nullptr;
				if(setTargetNode (parentFolder))
				{
					item = ItemIndex (ccl_as_unknown (parentFolder));

					Url targetFolder = getTargetFolder (parentFolder);
					if(canMoveInto (targetFolder) && parentFolder == model->getRootNode ())
						relation = IItemViewDragHandler::kFullView;
					return true;
				}
			}
		}
		else if(setTargetNode (model->getRootNode ()))
		{
			// not on a node: try current root node
			item = ItemIndex ();
			relation = IItemViewDragHandler::kFullView;
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentBlocks::FolderDragHandler::dragOver (const DragEvent& event)
{
	SuperClass::dragOver (event);

	int result = IDragSession::kDropNone;
	String header;

	Url targetFolder (getTargetFolder ());
	if(!targetFolder.isEmpty ())
	{
		if(!forbiddenTargetFoldersDeep.containsSubPath (targetFolder))
			result = IDragSession::kDropMove;

		if(canMoveInto (targetFolder))
		{
			String fileName;
			targetFolder.getName (fileName);

			Variant args[] = { fileName };
			header.appendFormat (FileStrings::MoveTo (), args, 1);
		}
		else
			header = FileStrings::Move ();
	}

	event.session.setResult (result);
	if(sprite)
		spriteBuilder.replaceItemText (*sprite, 0, header);
	return true;
}

//************************************************************************************************
// DocumentBlocks::Source
//************************************************************************************************

DocumentBlocks::Source::Source ()
: orderedDocuments (false),
  flatContent (false)
{
	childSources.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::Source::~Source ()
{
	for(auto source : iterate_as<Source> (childSources))
		source->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::Source::addChildSource (Source* source)
{
	childSources.add (source);

	source->addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& DocumentBlocks::Source::getChildSources () const
{
	return childSources;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String DocumentBlocks::Source::getChildSourceFolderName (StringID id) const
{
	String folderName;
	for(auto source : iterate_as<Source> (childSources))
		if(source->getID () == id)
		{
			folderName = source->getFolderName ();
			break;
		}
	return folderName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::Source::toString (String& string, int flags) const
{
	string = title;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::Source::getChildSourcesDocumentsTree (DocumentSink& sink, IProgressNotify* progress)
{
	// folder for each child source
	int sortPriority = childSources.count (); // before other folders
	for(auto source : iterate_as<Source> (childSources))
	{
		DocumentSink* subFolderSink = sink.addFolder (source->getFolderName (), source->getIcon (), Url::kEmpty, sortPriority--);
		tresult result = source->getDocuments (*subFolderSink, progress);

		// remove folder if source failed
		if(result != kResultOk)
			subFolderSink->removeFolder ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentBlocks::Source::notify (ISubject* subject, MessageRef msg)
{
	// forward change notification from child sources to item model
	if(msg == kChanged && childSources.contains (unknown_cast<Object> (subject)))
		signal (msg);
}

//************************************************************************************************
// DocumentBlocks::RecentSource
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DocumentBlocks::RecentSource, Source)
Configuration::BoolValue DocumentBlocks::RecentSource::deleteOnRemove ("Application.DocumentBlocks.RecentSource", "deleteOnRemove", false);
Configuration::BoolValue DocumentBlocks::RecentSource::hideMissing ("Application.DocumentBlocks.RecentSource", "hideMissing", false);

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::RecentSource::isDeleteOnRemove ()
{
	return deleteOnRemove;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::RecentSource::RecentSource ()
: recentPaths (DocumentManager::instance ().getRecentPaths ()),
  failOnEmptyList (false)
{
	recentPaths.retain ();
	recentPaths.addObserver (this);

	setOrderedDocuments (true); // keep own order of recent / pinned paths (don't order by name)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::RecentSource::~RecentSource ()
{
	recentPaths.removeObserver (this);
	recentPaths.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* DocumentBlocks::RecentSource::newPathIterator () const
{
	// all recent documents (including pinned ones) in "recent order" 
	return recentPaths.newRecentPathsIterator (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult DocumentBlocks::RecentSource::getDocuments (DocumentSink& sink, IProgressNotify* progress)
{
	AutoPtr<Iterator> iterator (newPathIterator ());
	if(!iterator)
		return kResultFailed;

	int count = 0;
	for(auto path : iterate_as<Url> (*iterator))
	{
		if(progress && progress->isCanceled ())
			return kResultAborted;

		if(hideMissing)
			if(System::GetFileSystem ().fileExists (*path) == false)
				continue;

		if(fileType.isValid () && path->getFileType () != fileType)
			continue;

		sink.addDocument (*path, !isOrderedDocuments ());
		count++;
	}

	getChildSourcesDocumentsTree (sink, progress);

	return (isFailOnEmptyList () && count == 0) ? kResultFailed : kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::FolderEntry* DocumentBlocks::RecentSource::getFileTree ()
{
	ObjectArray documents;
	documents.objectCleanup (true);
	AutoPtr<Iterator> iterator (recentPaths.newRecentPathsIterator (true)); // including pinned
	getDocumentsInternal (documents, nullptr, iterator);

	folderEntry.release ();
	folderEntry = NEW FolderEntry (Url ());

	for(auto url : iterate_as<Url> (documents))
		folderEntry->addFile (*url);

	return folderEntry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult DocumentBlocks::RecentSource::getDocumentsInternal (Container& list, IProgressNotify* progress, Iterator* iterator)
{
	ASSERT (list.isObjectCleanup ())

	if(!iterator)
		return kResultFailed;

	for(auto path : iterate_as<Url> (*iterator))
	{
		if(progress && progress->isCanceled ())
			return kResultAborted;

		if(hideMissing)
			if(System::GetFileSystem ().fileExists (*path) == false)
				continue;

		if(fileType.isValid () && path->getFileType () != fileType)
			continue;

		list.add (return_shared (path));
	}
	return isFailOnEmptyList () && list.isEmpty () ? kResultFailed : kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::RecentSource::appendDocumentMenu (IMenu& menu, const DocumentDescription& description, Container* selectedUrls)
{
	menu.addSeparatorItem ();

	AutoPtr<Container> urlsToRemove;
	if(selectedUrls && selectedUrls->contains (description.getPath ()))
		urlsToRemove.share (selectedUrls);
	else
	{
		urlsToRemove = NEW ObjectList;
		urlsToRemove->objectCleanup (true);
		urlsToRemove->add (NEW Url (description.getPath ()));
	}

	menu.addCommandItem (XSTR (RemoveFromRecentFiles), "File", "Remove Recent File",
		CommandDelegate<DocumentBlocks::RecentSource>::make (this, &DocumentBlocks::RecentSource::onRemoveRecent, Variant (static_cast<ISubject*> (urlsToRemove), true)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::RecentSource::removeDocument (const DocumentDescription& description)
{
	if(deleteOnRemove)
		return DocumentManager::instance ().deleteDocument (description.getPath ());
	else
		return DocumentManager::instance ().getRecentPaths ().removeRecentPath (description.getPath ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentBlocks::RecentSource::notify (ISubject* subject, MessageRef msg)
{
	if(subject == &recentPaths && msg == kChanged)
		signal (msg);
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* DocumentBlocks::RecentSource::createSearcher (ISearchDescription& description)
{
	return NEW DocumentSearcher (description, *this, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::RecentSource::removeFromRecentList (UrlRef path)
{
	return DocumentManager::instance ().getRecentPaths ().removeRecentPath (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::RecentSource::onRemoveRecent (CmdArgs args, VariantRef data)
{
	auto urlsToRemove = unknown_cast<Container> (data);
	if(!urlsToRemove)
		return false;

	if(args.checkOnly ())
		return true;

	for(auto url : iterate_as<Url> (*urlsToRemove))
		removeFromRecentList (*url);

	return true;
}

//************************************************************************************************
// DocumentBlocks::PinnedSource
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DocumentBlocks::PinnedSource, DocumentBlocks::RecentSource)

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* DocumentBlocks::PinnedSource::newPathIterator () const
{
	#if 1 // most-recently opened first ("recent order" filtered for pinned)
	return makeFilteringIterator (recentPaths.newRecentPathsIterator (false), [&] (IUnknown* obj)
	{
		auto url = unknown_cast<Url> (obj);
		return recentPaths.isPathPinned (*url);
	});
	#else // most-recently pinned first
	Iterator* iterator = recentPaths.newPinnedPathsIterator ();
	return iterator ? NEW ReverseIterator (iterator) : nullptr;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::PinnedSource::appendDocumentMenu (IMenu& menu, const DocumentDescription& description, Container* selectedUrls)
{
	SuperClass::SuperClass::appendDocumentMenu (menu, description);
}

//************************************************************************************************
// DocumentBlocks::FolderSource
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DocumentBlocks::FolderSource, DocumentBlocks::Source)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::FolderSource::FolderSource (UrlRef path, const FileType& fileType)
: path (path),
  fileType (fileType),
  signalSink (Signals::kSystemInformation),
  documentNeedsFolder (false)
{
	signalSink.setObserver (this);
	signalSink.enable (true);

	if(DocumentClass* docClass = DocumentManager::instance ().findDocumentClass (getFileType ()))
		documentNeedsFolder = docClass->needsFolder ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::FolderSource::~FolderSource ()
{
	signalSink.enable (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::FolderSource::canCreateFolderIn (Url& parentFolder) const
{
	int depth = 0;
	Url folder (parentFolder);
	while(folder != getPath () && folder.ascend ())
		depth++;

	return depth < kMaxDepth;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentBlocks::FolderSource::removeDocument (const DocumentDescription& description)
{
	return DocumentManager::instance ().deleteDocument (description.getPath ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentBlocks::FolderSource::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kContentLocationChanged)
	{
		UnknownPtr<IUrl> newLocation (msg[0].asUnknown ());
		UnknownPtr<IUrl> oldLocation (msg[1].asUnknown ());
		ASSERT (newLocation && oldLocation)

		if(Url (*oldLocation).contains (path))
		{
			Url newPath (path);
			newPath.makeRelative (*oldLocation);
			newPath.makeAbsolute (*newLocation);

			path = newPath;
			signal (Message (kChanged));
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* DocumentBlocks::FolderSource::createSearcher (ISearchDescription& description)
{
	return NEW DocumentSearcher (description, *this, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentBlocks::FolderEntry* DocumentBlocks::FolderSource::getFileTree ()
{
	if(!rootFolderEntry)
		rootFolderEntry = NEW FolderEntry (getPath ());

	return rootFolderEntry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult DocumentBlocks::FolderSource::scan (Container& list, UrlRef folder, IProgressNotify* progress, int depth)
{
	if(depth > kMaxDepth)
		return kResultOk;

	ForEachFile (System::GetFileSystem ().newIterator (folder), p)
		if(progress && progress->isCanceled ())
			return kResultAborted;

		if(p->isFolder ())
		{
			scan (list, *p, progress, depth + 1);
		}
		else if(p->getFileType () == fileType)
		{
			list.add (NEW Url (*p));
		}
	EndFor
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult DocumentBlocks::FolderSource::getDocuments (DocumentSink& sink, IProgressNotify* progress)
{
	//CCL_RPROFILE_START (getDocuments)
	rootFolderEntry.release (); // TODO: keep for next time, only rescan on file system changes

	tresult result = kResultFailed;

	{
		MutableCString contextId = getDiagnosticID ();
		String label = getDiagnosticLabel ();
		DiagnosticProfilingScope profilingScope (contextId, DiagnosticID::kScanDuration, label);
		profilingScope.setEnabled (System::IsInMainThread ());

		result = scanTree (sink, *getFileTree (), path, progress) >= 0 ? kResultOk : kResultAborted;
	}

	getChildSourcesDocumentsTree (sink, progress);
	//CCL_RPROFILE_STOP (getDocuments)
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DocumentBlocks::FolderSource::scanTree (DocumentSink& sink, FolderEntry& folderEntry, UrlRef folder, IProgressNotify* progress, int depth)
{
	if(depth > kMaxDepth)
		return 0;

	MutableCString contextId;
	contextId.appendFormat ("depth/%d", depth);
	String label = getDiagnosticLabel ();
	DiagnosticProfilingScope profilingScope (contextId, DiagnosticID::kScanDuration, label);
	profilingScope.setEnabled (System::IsInMainThread ());

	String folderName;
	folder.getName (folderName, true);

	// check if this folder contains at least one document or autosave file
	bool hasDocumentOrAutoSave = false;
	for(auto fileEntry : iterate_as<FileEntry> (folderEntry.getFiles (true)))
	{
		if(fileEntry->getUrl ().getFileType () == fileType
			|| AutoSaver::isAutoSaveFile (fileEntry->getUrl ())) // an ".autosave" file is another indicator for a document folder (e.g. document was never saved) -> ignore History folder (below))
		{
			hasDocumentOrAutoSave = true;
			break;
		}
	}

	bool isDocumentFolder = false;
	int numDocuments = 0;
	int numDocumentsDeep = 0;

	// scan sub folders
	if(depth < kMaxDepth)
		for(auto subFolderEntry : iterate_as<FolderEntry> (folderEntry.getSubFolders (true)))
		{
			if(progress && progress->isCanceled ())
				return -1;

			String subFolderName;
			subFolderEntry->getUrl ().getName (subFolderName);

			if(hasDocumentOrAutoSave && subFolderName == DocumentVersions::getHistoryFolderName ()) // do not scan history folder (if there is at least one document besides)
			{
				subFolderEntry->setIgnored (true);
				continue;
			}

			DocumentSink* subFolderSink = sink.addFolder (subFolderName, nullptr, subFolderEntry->getUrl ());
			int num = scanTree (*subFolderSink, *subFolderEntry, subFolderEntry->getUrl (), progress, depth + 1);
			if(num > 0)
				numDocumentsDeep += num;
		}

	// add documents from this folder
	for(auto fileEntry : iterate_as<FileEntry> (folderEntry.getFiles (true)))
	{
		if(progress && progress->isCanceled ())
			return -1;

		if(fileEntry->getUrl ().getFileType () == fileType)
		{
			String docName;
			fileEntry->getUrl ().getName (docName, false);

			// ignore autosave snapshots in History (in case we scan a History folder by accident, e.g. when document folder has no document)
			if(docName.endsWith (DocumentVersions::strAutosaveSnapshotSuffix) && folderName == DocumentVersions::getHistoryFolderName ())
			{
				fileEntry->setIgnored (true);
				continue;
			}

			sink.addDocument (fileEntry->getUrl ());

			numDocuments++;
			numDocumentsDeep++;

			if(documentNeedsFolder)
			{
				// check if the parent folder we're scanning is the document's inherent folder
				if(docName == folderName)
					isDocumentFolder = true;
			}
		}
	}

	// avoid showing folders that are just document folders for a single folder based document:
	// flatten folder if it contains no document at all (deep), or exactly 1 document of the same name
	auto shouldFlattenSingleDocFolder = [&] ()
	{
		#if 0
		return isDocumentFolder; // flatten	if folder name matches document name
		#else
		return numDocuments > 0; // flatten any folder with a single document
		#endif
	};

	bool mustFlatten = isFlatContent ();
	if(!mustFlatten && depth > 0)
		mustFlatten = numDocumentsDeep == 0 || (numDocumentsDeep == 1 && shouldFlattenSingleDocFolder ());

	if(mustFlatten)
		sink.flattenFolder ();

	return numDocumentsDeep;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString DocumentBlocks::FolderSource::getDiagnosticID () const
{
	MutableCString id (DiagnosticID::kFileTypePrefix);
	id += fileType.getExtension ();
	return id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String DocumentBlocks::FolderSource::getDiagnosticLabel () const
{
	return fileType.getDescription ();
}

//************************************************************************************************
// DocumentBlocks::DocumentSearchProvider
//************************************************************************************************

DocumentBlocks::DocumentSearchProvider::DocumentSearchProvider (DocumentBlocks& component)
: component (component)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* DocumentBlocks::DocumentSearchProvider::createSearcher (ISearchDescription& description)
{
	if(Source* source = component.getActiveSource ())
		return source->createSearcher (description);

	return nullptr;
}

//************************************************************************************************
// DocumentBlocks::DocumentSearcher
//************************************************************************************************

DocumentBlocks::DocumentSearcher::DocumentSearcher (ISearchDescription& description, Source& source, bool preload)
: AbstractSearcher (description),
  source (source)
{
	documentUrls.objectCleanup (true);
	if(preload)
		loadDocumentList ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::DocumentSearcher::addDocument (UrlRef url, bool sort)
{
	// note: this is the DocumentSink callback from getDocuments;
	// a check for duplicates here (before filtering) would be very expensive (happens later in addDocumentItem)
	documentUrls.add (NEW Url (url));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult DocumentBlocks::DocumentSearcher::loadDocumentList (IProgressNotify* progress)
{
	documentUrls.removeAll ();
	return source.getDocuments (*this, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DocumentBlocks::DocumentSearcher::find (ISearchResultSink& resultSink, IProgressNotify* progress)
{
	if(loadDocumentList (progress) == kResultAborted)
		return kResultAborted;

	ForEach (documentUrls, Url, url)
		if(progress && progress->isCanceled ())
			return kResultAborted;

		String name;
		url->getName (name, false);
		if(searchDescription.matchesName (name))
			resultSink.addResult (ccl_as_unknown (NEW Url (*url)));
	EndFor

	return kResultOk;
}

//************************************************************************************************
// DocumentBlocks::DocumentSearchResult
//************************************************************************************************

DocumentBlocks::DocumentSearchResult::DocumentSearchResult (DocumentBlocks& component)
: TreeModel (component)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::DocumentSearchResult::onSearchStart (ISearchDescription& description, ISearchProvider* searchProvider)
{
	rebuild (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::DocumentSearchResult::onSearchEnd (bool canceled)
{
	if(canceled)
		rebuild (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentBlocks::DocumentSearchResult::onResultItemsAdded (const IUnknownList& items)
{
	makeViews (false);

	// sort by name except for recent documents
	bool sort = ccl_cast<RecentSource> (component.getActiveSource ()) == nullptr;

	ForEachUnknown (items, unknown)
		if(auto url = unknown_cast<Url> (unknown))
			addDocumentItem (*url, sort);
	EndFor

	makeViews (true);
}
