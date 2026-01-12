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
// Filename    : ccl/app/documents/documentblocks.h
// Description : Document Blocks
//
//************************************************************************************************

#ifndef _ccl_documentblocks_h
#define _ccl_documentblocks_h

#include "ccl/app/component.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/configuration.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/app/idocument.h"
#include "ccl/public/gui/framework/iworkspace.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/commanddispatch.h"

namespace CCL {

interface IMenu;
interface IColumnHeaderList;
interface IItemModel;
interface ISearcher;
interface ISearchDescription;
interface IProgressNotify;

class FileType;
class DocumentClass;
class DocumentDescription;
class SearchComponent;
class TreeViewModel;
class PathList;

//************************************************************************************************
// DocumentBlocks
//************************************************************************************************

class DocumentBlocks: public Component,
					  public CCL::AbstractDocumentEventHandler,
					  public CCL::IWorkspaceEventHandler,
					  public CommandDispatcher<DocumentBlocks>
{
public:
	DECLARE_CLASS (DocumentBlocks, Component)
	
	DocumentBlocks (StringRef name = nullptr);
	~DocumentBlocks ();

	class DocumentSink;
	class FileTreeEntry;
	class FileEntry;
	class FolderEntry;

	class Source: public Object
	{
	public:
		PROPERTY_STRING (title, Title)
		PROPERTY_MUTABLE_CSTRING (id, ID)
		PROPERTY_SHARED_AUTO (IImage, icon, Icon)
		PROPERTY_BOOL (orderedDocuments, OrderedDocuments)	///< source delivers documents in a meaningful order that should be respected
		PROPERTY_BOOL (flatContent, FlatContent)			///< presents all documents on top level, regardless of their subfolders on disk

		void addChildSource (Source* source);
		const ObjectArray& getChildSources () const;
		String getChildSourceFolderName (StringID id) const;

		virtual tresult getDocuments (DocumentSink& sink, IProgressNotify* progress = nullptr) = 0;

		virtual void appendDocumentMenu (IMenu& menu, const DocumentDescription& description, Container* selectedUrls = nullptr) {}

		virtual bool removeDocument (const DocumentDescription& description) { return false; }

		virtual ISearcher* createSearcher (ISearchDescription& description) = 0;

		virtual FolderEntry* getFileTree () { return nullptr; } // source might provide an already scanned tree of folders and files

		// Object
		bool toString (String& string, int flags = 0) const override;

	protected:
		ObjectArray childSources;

		Source ();
		~Source ();

		// Object
		void CCL_API notify (ISubject* subject, MessageRef msg) override;

		String getFolderName () const { return getTitle (); };
		void getChildSourcesDocumentsTree (DocumentSink& sink, IProgressNotify* progress = nullptr);
	};

	void addSource (Source* source);
	Source& addRecentDocuments ();
	void addPinnedDocumentsFolder (Source& parentSource);
	Source& addDocumentFolder (UrlRef path, const FileType& fileType, StringRef title, StringID id = nullptr);
	Source& addDocumentFolder (const DocumentClass& documentClass, StringRef title, StringID id = nullptr);

	Source* getActiveSource () const;
	void setActiveSource (StringID id);

	void appendDocumentMenu (IMenu& menu, const DocumentDescription& description);

	IColumnHeaderList& getColumns () const;
	DECLARE_STRINGID_MEMBER (kPinID)
	DECLARE_STRINGID_MEMBER (kAgeID)

	void selectDocuments (UrlRef path, StringID childSourceId = nullptr, bool cancelSearch = false);

	void beginBulkOperation () { inBulkOperation = true; }
	void endBulkOperation () { inBulkOperation = false; rebuildList (); }

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	
	// IWorkspaceEventHandler
	void CCL_API onWorkspaceEvent (const WorkspaceEvent& e) override;

	class ViewState;

	DECLARE_COMMANDS (DocumentBlocks)
	CLASS_INTERFACE2 (IDocumentEventHandler, IWorkspaceEventHandler, Component)

protected:
	class RecentSource;
	class PinnedSource;
	class FolderSource;
	class TreeModel;
	class Item;
	class FolderNode;
	class RootFolderNode;
	class FolderDragHandler;
	class RootFolderDataTarget;
	class DocumentTreeSink;
	class DocumentSearchProvider;
	class DocumentSearcher;
	class DocumentSearchResult;

	ObjectArray sources;
	ObjectArray sourceStates;
	Source* activeSource;
	TreeModel* treeModel;
	AutoPtr<Object> rootFolderTarget;
	DocumentSearchResult* searchResult;
	SearchComponent* search;
	bool sourceDirty;
	bool inBulkOperation;

	static Configuration::BoolValue locationIcons;

	bool isEditMode () const;
	void popupDocumentInfo (const DocumentDescription& description);

	// IDocumentEventHandler
	void CCL_API onDocumentEvent (CCL::IDocument& document, int eventCode) override;
	
	Source* getSourceByID (StringID id) const;
	void setActiveSource (Source* source);
	ViewState* getSourceState (Source* source, bool create);
	PathList* getNewFolders (bool create = false);
	Attributes& getSettings () const;
	void saveSettings () const;
	void loadSettings ();
	void rebuildList ();
	bool onNewFolder (CmdArgs args, VariantRef data);
	bool onMoveToFolder (CmdArgs args, VariantRef data);
	bool onRenameFolder (CmdArgs args, VariantRef data);
	bool onDeleteFolder (CmdArgs args, VariantRef data);
	bool showFileInSystem (CmdArgs args, VariantRef data);
	bool openWithOptions (CmdArgs args, VariantRef data);

	UrlRef getTargetFolder (FolderNode* folderNode) const;
	bool createNewFolder (UrlRef focusUrl, const Container* urlsToMove = nullptr);
	bool moveToFolder (UrlRef targetFolder, const Container& urlsToMove);
	void onFolderMoved (UrlRef oldPath, UrlRef newPath);
	void appendMoveToFolderMenu (IMenu& menu, const DocumentDescription& description);
};

//************************************************************************************************
// DocumentBlocks::DocumentSink
//************************************************************************************************

class DocumentBlocks::DocumentSink
{
public:
	virtual void addDocument (UrlRef url, bool sort = true) = 0;

	virtual DocumentSink* addFolder (StringRef name, IImage* icon, UrlRef url, int sortPriority = 0) { return this; }  // returns sink for new folder

	virtual void removeFolder () {} // remove current folder

	virtual void flattenFolder () {} // remove current folder, move all content up to parent folder
};

//************************************************************************************************
// DocumentBlocks::FileTreeEntry
//************************************************************************************************

class DocumentBlocks::FileTreeEntry: public Object
{
public:
	FileTreeEntry (UrlRef url);

	PROPERTY_OBJECT (Url, url, Url)
	PROPERTY_BOOL (ignored, Ignored)
};

//************************************************************************************************
// DocumentBlocks::FileEntry
//************************************************************************************************

class DocumentBlocks::FileEntry: public DocumentBlocks::FileTreeEntry
{
public:
	FileEntry (UrlRef url);
	FileEntry (const DocumentDescription& description);
	~FileEntry ();

	DocumentDescription& getDescription ();

private:
	DocumentDescription* description;
};

//************************************************************************************************
// DocumentBlocks::FolderEntry
//************************************************************************************************

class DocumentBlocks::FolderEntry: public DocumentBlocks::FileTreeEntry
{
public:
	FolderEntry (UrlRef url);

	ObjectArray& getSubFolders (bool scan);
	ObjectArray& getFiles (bool scan);

	void addFile (UrlRef url);
	void addDocument (const DocumentDescription& description);

private:
	ObjectArray subFolders;
	ObjectArray files;
	bool contentScanned;

	void scanContent ();
};

} // namespace CCL

#endif // _ccl_documentblocks_h
