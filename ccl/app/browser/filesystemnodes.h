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
// Filename    : ccl/app/browser/filesystemnodes.h
// Description : Filesystem Nodes
//
//************************************************************************************************

#ifndef _ccl_filesystemnodes_h
#define _ccl_filesystemnodes_h

#include "ccl/app/browser/browsernode.h"
#include "ccl/app/components/isearchprovider.h"

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/base/irecognizer.h"
#include "ccl/public/gui/idatatarget.h"

namespace CCL {

class Url;
class SignalSink;
struct VolumeInfo;

interface IFileIterator;
interface IFileSystem;
interface IPackageFile;

namespace Browsable {

class FileNode;

//************************************************************************************************
// Browsable::FileNodeConstructor
//************************************************************************************************

class FileNodeConstructor: public Object
{
public:
	virtual bool canCreateNode (UrlRef path) const = 0;

	virtual BrowserNode* createNode (UrlRef path) const = 0;
};

//************************************************************************************************
// Browsable::FileNodeFactory
//************************************************************************************************

class FileNodeFactory: public FileNodeConstructor,
					   public Singleton<FileNodeFactory>
{
public:
	FileNodeFactory ();

	void addConstructor (FileNodeConstructor* constructor);

	// FileNodeConstructor
	bool canCreateNode (UrlRef path) const override;
	BrowserNode* createNode (UrlRef path) const override;

protected:
	ObjectList constructors;
};

//************************************************************************************************
// Browsable::FileNode
//************************************************************************************************

class FileNode: public BrowserNode,
				public IFileNode
{
public:
	DECLARE_CLASS (FileNode, BrowserNode)

	FileNode (Url* path = nullptr, BrowserNode* parent = nullptr);
	~FileNode ();

	const Url* getPath () const;
	UrlRef CCL_API getFilePath () const override; ///< [IFileNode]

	enum FileCommands
	{
		kShowInShellBrowser	= 1<<0,
		kOpenWithExtShell	= 1<<1,
		kRenameFile			= 1<<2,
		kDeleteFile			= 1<<3,
		kCreateSubFolder	= 1<<4,
		kAllFileCommands	= kShowInShellBrowser|kRenameFile|kDeleteFile|kOpenWithExtShell,
		kAllDirCommands		= kAllFileCommands|kCreateSubFolder,
		kLastFileCommand	= 4
	};

	PROPERTY_VARIABLE (int, fileCommandMask, FileCommandMask)
	PROPERTY_FLAG (fileCommandMask, kShowInShellBrowser, canShowInShellBrowser)
	PROPERTY_FLAG (fileCommandMask, kOpenWithExtShell, canOpenWithExternalShell)
	PROPERTY_FLAG (fileCommandMask, kRenameFile, canRenameFile)
	PROPERTY_FLAG (fileCommandMask, kDeleteFile, canDeleteFile)
	PROPERTY_FLAG (fileCommandMask, kCreateSubFolder, canCreateSubFolder)

	// BrowserNode
	IImage* getIcon () override;
	IUnknown* createDragObject () override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes) override;
	bool interpretCommand (const CommandMsg& msg, const Container* selectedNodes) override;
	bool performRemoval (NodeRemover& remover) override;
	bool onOpen (bool deferred = true) override;

	CLASS_INTERFACE (IFileNode, BrowserNode)

protected:
	Url* path;

	void setPath (Url* path);

	// IFileNode
	DELEGATE_IBROWSERNODE_METHODS (IFileNode, BrowserNode)

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// Browsable::DirectoryNode
//************************************************************************************************

class DirectoryNode: public FileNode,
					 public IDataTarget,
					 public ISearchProvider
{
public:
	DECLARE_CLASS (DirectoryNode, FileNode)

	DirectoryNode (Url* path = nullptr, BrowserNode* parent = nullptr, IUrlFilter* urlFilter = nullptr);

	PROPERTY_POINTER (IUrlFilter, urlFilter, UrlFilter)

	bool addFile (UrlRef path);

	virtual bool isHiddenFile (UrlRef path) const;

	/** Get local target path for drag operations, defaults to directory itself. */
	virtual bool getTargetLocation (Url& path) const;
	
	/** Tells if the default drag & drop action on this folder should be copy instead of move (default: true on another volume). */
	virtual bool shouldCopyByDefault (UrlRef sourcePath) const;

	// BrowserNode
	bool isFolder () const override { return true; }
	bool hasSubNodes () const override;
	bool getSubNodes (Container& children, NodeFlags flags = NodeFlags::kAll) override;
	StringID getCustomBackground () const override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes) override;
	bool interpretCommand (const CommandMsg& msg, const Container* selectedNodes) override;
	ISearchProvider* getSearchProvider () override;

	// IDataTarget
	tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;

	CLASS_INTERFACE2 (IDataTarget, ISearchProvider, FileNode)

protected:
	bool hasNoSubnodes;

	virtual BrowserNode* createNode (Url* forPath) const;
	virtual IFileIterator* createIterator (int mode) const;
	bool hasFileNodes () const;  ///< checks if this delivers any subNodes (truthful version of hasSubNodes)

	// ISearchProvider
	StringRef getTitle () const override;
	UrlRef getStartPoint () const override;
	IImage* getSearchIcon () const override;
	ISearcher* createSearcher (ISearchDescription& description) override;
	IUrlFilter* getSearchResultFilter () const override;
	IUnknown* customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem) override;
};

//************************************************************************************************
// Browsable::TranslatedDirectoryNode
/** A DirectoryNode with a translated name. */
//************************************************************************************************

class TranslatedDirectoryNode: public TranslatedNode<DirectoryNode>
{
public:
	TranslatedDirectoryNode (Url* path = nullptr, BrowserNode* parent = nullptr, IUrlFilter* urlFilter = nullptr)
	{
		if(path)
			setPath (path);
		setUrlFilter (urlFilter);
	}
};

//************************************************************************************************
// FilteredDirectoryNode
/** Mixin class for a DirectoryNode with an additional filter that only applies to one level. */
//************************************************************************************************

template<class BaseClass>
class FilteredDirectoryNode: public BaseClass
{
public:
	using BaseClass::BaseClass;

	PROPERTY_AUTO_POINTER (IUrlFilter, topLevelFilter, TopLevelFilter)

	// DirectoryNode
	bool isHiddenFile (UrlRef path) const
	{
		if(topLevelFilter && !topLevelFilter->matches (path))
			return true;

		return BaseClass::isHiddenFile (path);
	}
};

//************************************************************************************************
// Browsable::PackageNodeConstructor
//************************************************************************************************

class PackageNodeConstructor: public FileNodeConstructor
{
public:
	PackageNodeConstructor (const FileType& fileType, int fileCommandMask = 0);

	PROPERTY_OBJECT (FileType, fileType, FileType)
	PROPERTY_VARIABLE (int, fileCommandMask, FileCommandMask)

	// FileNodeConstructor
	bool canCreateNode (UrlRef path) const override;
	BrowserNode* createNode (UrlRef path) const override;
};

//************************************************************************************************
// Browsable::PackageNode
//************************************************************************************************

class PackageNode: public DirectoryNode
{
public:
	DECLARE_CLASS (PackageNode, DirectoryNode)

	PackageNode (Url* path = nullptr, BrowserNode* parent = nullptr, IUrlFilter* urlFilter = nullptr, bool shouldShowContentAlways = false);
	~PackageNode ();

	static void registerConstructor (const FileType& fileType, int fileCommandMask = 0);

	enum PackageCommands
	{
		kCanExtract = 1<<(kLastFileCommand+1)
	};

	PROPERTY_FLAG (fileCommandMask, kCanExtract, canExtract)

	// DirectoryNode
	ISearchProvider* getSearchProvider () override;
	bool isFolder () const override { return false; }
	bool hasSubNodes () const override;
	IImage* getIcon () override;
	StringID getCustomBackground () const override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes) override;
	bool interpretCommand (const CommandMsg& msg, const Container* selectedNodes) override;
	bool onOpen (bool deferred = true) override;

protected:
	bool shouldShowContent;
	bool shouldShowContentAlways;

	void getContentPath (Url& path) const;
	void showContent (bool state, bool force = false);

	// DirectoryNode
	IFileIterator* createIterator (int mode) const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	IPackageFile* package;
	String packageId;
	SignalSink* signalSink;

	void store ();
	void restore ();
	void mount ();
	void unmount (bool expectRemount);
	bool extract (bool checkOnly);
	void listenToFileSystemSignals (bool state);
};

//************************************************************************************************
// Browsable::VolumeNode
//************************************************************************************************

class VolumeNode: public DirectoryNode
{
public:
	DECLARE_CLASS (VolumeNode, DirectoryNode)

	VolumeNode (Url* path = nullptr, BrowserNode* parent = nullptr, IUrlFilter* urlFilter = nullptr);

	int getVolumeType () const { return volumeType; }
	StringRef getVolumeSubType () const { return volumeSubType; }

	// BrowserNode
	bool hasSubNodes () const override;
	IImage* getIcon () override;
	bool getUniqueName (MutableCString& name) override;
	int compare (const Object& obj) const override;
	IUnknown* createDragObject () override { return nullptr; }

protected:
	int volumeType;
	String volumeSubType;
	MutableCString uniqueNodeName;
};

//************************************************************************************************
// Browsable::VolumeListNode
//************************************************************************************************

class VolumeListNode: public TranslatedDirectoryNode
{
public:
	DECLARE_CLASS (VolumeListNode, DirectoryNode)

	VolumeListNode (BrowserNode* parent = nullptr, IUrlFilter* urlFilter = nullptr);

	// DirectoryNode
	IImage* getIcon () override;
	IUnknown* createDragObject () override { return nullptr; }

protected:
	bool isHiddenFile (UrlRef path) const override;
	BrowserNode* createNode (Url* forPath) const override;
};

//************************************************************************************************
// Browsable::PackageVolumeNode
//************************************************************************************************

class PackageVolumeNode: public VolumeNode
{
public:
	DECLARE_CLASS (PackageVolumeNode, VolumeNode)

	PackageVolumeNode (Url* path = nullptr, BrowserNode* parent = nullptr, IUrlFilter* urlFilter = nullptr);

	// VolumeNode
	int compare (const Object& obj) const override;
	IUnknown* createDragObject () override;
};

//************************************************************************************************
// Browsable::PackageRootNode
//************************************************************************************************

class PackageRootNode: public TranslatedDirectoryNode
{
public:
	DECLARE_CLASS (PackageRootNode, DirectoryNode)

	PackageRootNode (BrowserNode* parent = nullptr, IUrlFilter* urlFilter = nullptr, StringRef volumeSubType = nullptr);
	~PackageRootNode ();

	PROPERTY_STRING (volumeSubType, VolumeSubType) ///< filter for package volume type (optional)
	PROPERTY_BOOL (packageDragEnabled, PackageDragEnabled)

	// DirectoryNode
	bool onRefresh () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	IUnknown* createDragObject () override { return nullptr; }

protected:
	SignalSink& packageSink;
	bool insideRefresh;

	IFileIterator* createIterator (int mode) const override;
	BrowserNode* createNode (Url* forPath) const override;
};

//************************************************************************************************
// OptionalPackageRootNode
//************************************************************************************************

class OptionalPackageRootNode: public PackageRootNode
{
public:
	OptionalPackageRootNode (FolderNode* parent = nullptr, IUrlFilter* urlFilter = nullptr, StringRef volumeSubType = nullptr);

	void init ();

private:
	FolderNode* targetParent;
	int targetIndex;

	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

//************************************************************************************************
// Browsable::SystemFolderNode
//************************************************************************************************

class SystemFolderNode: public TranslatedDirectoryNode
{
public:
	SystemFolderNode (int folderType, const LocalString& title, BrowserNode* parent = nullptr, IUrlFilter* urlFilter = nullptr);
	IUnknown* createDragObject () override { return nullptr; }
};

//************************************************************************************************
// Browsable::UserContentNode
//************************************************************************************************

class UserContentNode: public TranslatedDirectoryNode
{
public:
	DECLARE_CLASS (UserContentNode, DirectoryNode)

	UserContentNode (BrowserNode* parent = nullptr, IUrlFilter* urlFilter = nullptr);
	~UserContentNode ();

	// DirectoryNode
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	IUnknown* createDragObject () override { return nullptr; }

protected:
	SignalSink& systemSink;

	void updatePath ();
};

//************************************************************************************************
// FileNodeRecognizer
//************************************************************************************************

struct FileNodeRecognizer: public Recognizer
{
	UrlRef url;
	
	FileNodeRecognizer (UrlRef url): url (url) {}

	// Recognizer
	tbool CCL_API recognize (IUnknown* object) const override;
};

} // namespace Browsable
} // namespace CCL

#endif // _ccl_filesystemnodes_h
