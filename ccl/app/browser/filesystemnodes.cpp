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
// Filename    : ccl/app/browser/filesystemnodes.cpp
// Description : Filesystem Nodes
//
//************************************************************************************************

#include "ccl/app/browser/filesystemnodes.h"
#include "ccl/app/browser/filebrowser.h"
#include "ccl/app/browser/filedraghandler.h"
#include "ccl/app/browser/filexportdraghandler.h"

#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/utilities/pathclassifier.h"
#include "ccl/app/utilities/fileoperations.h"
#include "ccl/app/utilities/shellcommand.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/packageinfo.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/guiservices.h"

using namespace CCL;
using namespace Browsable;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Browser")
	XSTRING (Content, "Content")
	XSTRING (Volumes, "Volumes")
	XSTRING (Packages, "Packages")
	XSTRING (ShowPackageContents, "Show Package Contents")
	XSTRING (ExtractHere, "Extract Here")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_COMMAND_ ("Browser", "Show in Explorer/Finder",	CommandFlags::kNoRepeat)
REGISTER_COMMAND_ ("Browser", "Rename File",				CommandFlags::kNoRepeat)
REGISTER_COMMAND_ ("Browser", "New Folder",					CommandFlags::kNoRepeat)

//************************************************************************************************
// Browsable::FileNodeFactory
//************************************************************************************************

DEFINE_SINGLETON (FileNodeFactory)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileNodeFactory::FileNodeFactory ()
{
	constructors.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileNodeFactory::addConstructor (FileNodeConstructor* constructor)
{
	constructors.add (constructor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileNodeFactory::canCreateNode (UrlRef path) const
{
	ForEach (constructors, FileNodeConstructor, c)
		if(c->canCreateNode (path))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* FileNodeFactory::createNode (UrlRef path) const
{
	ForEach (constructors, FileNodeConstructor, c)
		if(c->canCreateNode (path))
		{
			BrowserNode* node = c->createNode (path);
			if(node)
				return node;
		}
	EndFor
	return nullptr;
}

//************************************************************************************************
// Browsable::FileNode
//************************************************************************************************

DEFINE_CLASS (FileNode, BrowserNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileNode::FileNode (Url* _path, BrowserNode* parent)
: BrowserNode (nullptr, parent),
  path (nullptr),
  fileCommandMask (kAllFileCommands)
{
	if(_path)
		setPath (_path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileNode::~FileNode ()
{
	if(path)
		path->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Url* FileNode::getPath () const
{
	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileNode::setPath (Url* _path)
{
	take_shared<Url> (path, _path);
	ASSERT (path != nullptr)
	if(!path)
		return;

	path->getName (title);
	if(title.isEmpty ())
		title = path->getHostName ();

	icon = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API FileNode::getFilePath () const
{
	if(path)
		return *path;

	CCL_DEBUGGER ("FileNode path not set!\n")
	return Url::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* FileNode::getIcon ()
{
	if(icon == nullptr && path)
	{
		icon = FileIcons::instance ().createIcon (*path, FileIcons::kNoDefaultFolderIcon);
		if(icon)
			icon->release (); // shared pointer!
	}
	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* FileNode::createDragObject ()
{
	if(path)
	{
		IUrl* mountedPath = System::GetFileUtilities ().translatePathInMountedFolder (*path);
		if(mountedPath)
			return mountedPath;

		path->retain ();
	}
	return ccl_as_unknown (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileNode::appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes)
{
	if(path)
	{
		if(canShowInShellBrowser ())
		{
			if(!contextMenu.hasCommandItem (CSTR ("Browser"), CSTR ("Show in Explorer/Finder"))) // avoid duplicate entry for SearchResultNode, when "target" node adds this as well
				if(ShellCommand::showFileInSystem (*path, true))
					contextMenu.addCommandItem (ShellCommand::getShowFileInSystemTitle (), CSTR ("Browser"), CSTR ("Show in Explorer/Finder"), this);
		}

		// only if we are not tree root:
		if(contextMenu.getContextID () != Browser::kTreeRootContext)
		{
			if(canRenameFile ())
				contextMenu.addCommandItem (CommandWithTitle (CSTR ("Browser"), CSTR ("Rename File"), path->isFolder () ? FileStrings::RenameFolder () : FileStrings::RenameFile ()), nullptr, true); // avoid conflict with global File/Rename command!

			if(canDeleteFile ())
			{
				if(!System::GetFileSystem ().isWriteProtected (*path)) // filter files inside packages, or files that require admin privileges
					contextMenu.addCommandItem (CommandWithTitle (CSTR ("Edit"), CSTR ("Delete"), path->isFolder () ? FileStrings::DeleteFolder () : FileStrings::DeleteFile ()), nullptr, true);
			}
		}
	}
	return kResultFalse; // (continue)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileNode::interpretCommand (const CommandMsg& msg, const Container* selectedNodes)
{
	if(msg.category == "Browser")
	{
		if(msg.name == "Show in Explorer/Finder")
		{
			if(!canShowInShellBrowser ())
				return false;

			if(selectedNodes == nullptr || (selectedNodes && !selectedNodes->isEmpty ()))
				return path ? ShellCommand::showFileInSystem (*path, msg.checkOnly ()) : false;
		}
		else if(msg.name == "Rename File")
		{
			if(!canRenameFile ())
				return false;

			if(path)
			{
				if(msg.checkOnly ()) // filter files inside packages, or files that require admin privileges
					return System::GetFileSystem ().isWriteProtected (*path) == 0;
				else
				{
					Browser* browser = getBrowser ();
					FileBrowser* fileBrowser = ccl_cast<FileBrowser> (browser);
					AutoPtr<FileRenamer> renamer;
					if(fileBrowser)
						renamer = fileBrowser->createFileRenamer (this);
					if(!renamer)
						renamer = NEW FileRenamer (*path);

					if(renamer->runDialog (path->isFolder () ? FileStrings::RenameFolderTitle () : FileStrings::RenameFileTitle ()))
					{
						if(!System::GetFileSystem ().isLocalFile (*path))
							return true; // bail out here for remote files changed asynchronously

						AutoPtr<Url> newPath (renamer->createNewPath ());
						bool fileTypeChanged = path->getFileType () != newPath->getFileType ();

						setPath (newPath);

						if(browser)
						{
							if(fileTypeChanged && getParent ())
							{
								// refresh parent, select new node
								MutableCString nodePath;
								browser->makePath (nodePath, this);
								browser->refreshNode (getParent (), true);

								if(BrowserNode* newNode = browser->findNode (nodePath, true))
									browser->setFocusNode (newNode);
							}
							else
								browser->refreshNode (this, true);
						}
					}
				}
			}
			return true;
		}
	}
	return SuperClass::interpretCommand (msg, selectedNodes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileNode::performRemoval (NodeRemover& remover)
{
	bool result = false;
	bool checkOnly = remover.isCheckOnly ();
	remover.setRemoveDeferred (true);

	FileTransferOperation deleteOperation (FileTransferOperation::kDelete);
	ObjectArray preservedNodes;
	preservedNodes.objectCleanup (true);

	ForEach (remover, BrowserNode, node)
		if(FileNode* fileNode = ccl_cast<FileNode> (node))
		{
			if(fileNode->canDeleteFile () && fileNode->getPath ())
			{
				if(checkOnly)
					return true;

				deleteOperation.addFile (*fileNode->getPath (), fileNode);

				if(!System::GetFileSystem ().isLocalFile (*fileNode->getPath ()))
					preservedNodes.add (return_shared (fileNode)); // remote files are changed asynchronously
			}
			else if(!checkOnly)
				remover.keepNode (fileNode);
		}
	EndFor

	if(!checkOnly && !deleteOperation.isEmpty ())
	{
		result = true;
		deleteOperation.run (FileStrings::DeletingFiles ());

		// remove nodes of sucessfully deleted files
		ForEach (deleteOperation, BatchOperation::Task, deleteTask)
			if(FileNode* node = ccl_cast<FileNode> (deleteTask->getUserData ()))
			{
				if(deleteTask->succeeded () && !preservedNodes.contains (node))
					remover.removeNode (node);
				else
					remover.keepNode (node);
			}
		EndFor
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileNode::onOpen (bool deferred)
{
	if(path && path->isFile ())
	{
		int flags = deferred ? System::kDeferOpenURL : 0;
		if(!canOpenWithExternalShell ())
			flags |= System::kDoNotOpenExternally;

		return System::GetSystemShell ().openUrl (*path, flags) == kResultOk;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileNode::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "filePath")
	{
		var.takeShared (ccl_as_unknown (path));
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// Browsable::DirectoryNode
//************************************************************************************************

DEFINE_CLASS (DirectoryNode, FileNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

DirectoryNode::DirectoryNode (Url* path, BrowserNode* parent, IUrlFilter* urlFilter)
: FileNode (path, parent),
  hasNoSubnodes (false),
  urlFilter (urlFilter)
{
	fileCommandMask = kAllDirCommands;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DirectoryNode::isHiddenFile (UrlRef path) const
{
	String fileName;
	path.getName (fileName);
	return fileName.firstChar () == '.';

	// was: System::GetFileSystem ().isHiddenFile (path) != 0;
	// file system attribute is checked already by file iterator.
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DirectoryNode::getTargetLocation (Url& path) const
{
	path = getFilePath ();
	return !path.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DirectoryNode::shouldCopyByDefault (UrlRef sourcePath) const
{
	return !PathClassifier::isSameVolume (getFilePath (), sourcePath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* DirectoryNode::createNode (Url* forPath) const
{
	// try node factory first
	BrowserNode* node = FileNodeFactory::instance ().createNode (*forPath);
	if(node)
	{
		// assign filter
		if(DirectoryNode* dirNode = ccl_cast<DirectoryNode> (node))
			dirNode->setUrlFilter (urlFilter);

		return node;
	}

	// regular file/directory node
	if(forPath->isFolder ())
		return NEW DirectoryNode (forPath, nullptr, urlFilter);
	else
		return NEW FileNode (forPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* DirectoryNode::createIterator (int mode) const
{
	if(!path || (path && !path->isFolder ()))
		return nullptr;

	return System::GetFileSystem ().newIterator (*path, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DirectoryNode::hasSubNodes () const
{
#if 1
	return true;
#else
	// check if a sub-directory exists...
	AutoPtr<IFileIterator> iter = createIterator (IFileIterator::kAll);
	return iter && iter->next () != 0;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DirectoryNode::hasFileNodes () const
{
	// check if a sub-directory exists...
	AutoPtr<IFileIterator> iter = createIterator (IFileIterator::kAll);
	return iter && iter->next () != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DirectoryNode::getSubNodes (Container& children, NodeFlags flags)
{
	int mode = IFileIterator::kIgnoreHidden;
	if(flags.wantFolders ())
		mode |= IFileIterator::kFolders | IFileIterator::kBundlesAsFiles;
	if(flags.wantLeafs ())
		mode |= IFileIterator::kFiles;

	ObjectList files, folders; // sort files and folders
	ForEachFile (createIterator (mode), p)
		if(urlFilter && !urlFilter->matches (*p))
			continue;

		if(isHiddenFile (*p))
			continue;

		Url* twin = NEW Url (*p);
		BrowserNode* node = createNode (twin);
		if(node)
		{
			node->setParent (this);
			if(p->isFolder ())
				folders.addSorted (node);
			else
				files.addSorted (node);
		}
		twin->release ();
	EndFor

	hasNoSubnodes = folders.isEmpty () && files.isEmpty ();

	children.add (folders);
	children.add (files);

	return !hasNoSubnodes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DirectoryNode::addFile (UrlRef path)
{
	AutoPtr<Url> twin = NEW Url (path);
	BrowserNode* node = createNode (twin);
	Browser* browser = getBrowser ();
	if(node && browser)
		return browser->insertNode (this, node);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearchProvider* DirectoryNode::getSearchProvider ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef DirectoryNode::getTitle () const // ISearchProvider
{
	return SuperClass::getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef DirectoryNode::getStartPoint () const
{
	return getFilePath ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* DirectoryNode::getSearchIcon () const
{
	if(IImage* icon = const_cast<DirectoryNode*> (this)->getIcon ())
		return icon;
	return FileIcons::instance ().getDefaultFolderIcon ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* DirectoryNode::createSearcher (ISearchDescription& description)
{
	return System::GetFileSystem ().createSearcher (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrlFilter* DirectoryNode::getSearchResultFilter () const
{
	return getUrlFilter ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* DirectoryNode::customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem)
{
	// icon for folders
	UnknownPtr<IUrl> url (resultItem);
	if(url)
	{
		if(url->isFolder () && !args.presentation.getIcon ())
			args.presentation.setIcon (FileIcons::instance ().getDefaultFolderIcon ());
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID DirectoryNode::getCustomBackground () const
{
	return CSTR ("folder");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DirectoryNode::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	if(!path || System::GetFileSystem ().isWriteProtected (*path))
		return false;

	if(session && session->getDragHandler ())
		return true;

	AutoPtr<FileDraghandlerBase> dragHandler (NEW FileDraghandler (targetView, getBrowser ()));
	if(dragHandler->prepare (data, session))
	{
		if(session)
			session->setDragHandler (dragHandler);
		return true;
	}

	dragHandler = NEW FileExportDraghandler (targetView, getBrowser ());
	if(dragHandler->prepare (data, session))
	{
		if(session)
			session->setDragHandler (dragHandler);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DirectoryNode::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DirectoryNode::appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes)
{
	if(path)
		if(canCreateSubFolder () && contextMenu.getContextID () != Browser::kChildrenHiddenContext)
			contextMenu.addCommandItem (CommandWithTitle (CSTR ("Browser"), CSTR ("New Folder"), FileStrings::NewFolder ()), nullptr, true);

	return SuperClass::appendContextMenu (contextMenu, selectedNodes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DirectoryNode::interpretCommand (const CommandMsg& msg, const Container* selectedNodes)
{
	if(msg.category == "Browser")
	{
		if(msg.name == "New Folder")
		{
			if(path && canCreateSubFolder () && System::GetFileSystem ().isWriteProtected (*path) == 0)
			{
				if(!msg.checkOnly ())
					NewFolderOperation ().run (*path);
				return true;
			}
		}
	}
	return SuperClass::interpretCommand (msg, selectedNodes);
}

//************************************************************************************************
// Browsable::PackageNodeConstructor
//************************************************************************************************

PackageNodeConstructor::PackageNodeConstructor (const FileType& fileType, int fileCommandMask)
: fileType (fileType),
  fileCommandMask (fileCommandMask)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageNodeConstructor::canCreateNode (UrlRef path) const
{
	return path.getFileType () == fileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* PackageNodeConstructor::createNode (UrlRef path) const
{
	if(canCreateNode (path))
	{
		AutoPtr<Url> path2 = NEW Url (path);
		PackageNode* node = NEW PackageNode (path2);
		node->setFileCommandMask (node->getFileCommandMask ()|fileCommandMask);
		return node;
	}
	return nullptr;
}

//************************************************************************************************
// Browsable::PackageNode
//************************************************************************************************

static Container& getPackagePaths ()
{
	Attributes& a = Settings::instance ().getAttributes (CCLSTR ("PackageNode"));
	Container* paths = a.getObject<Container> ("paths");
	if(paths == nullptr)
	{
		paths = NEW ObjectList;
		paths->objectCleanup (true);
		a.set ("paths", paths, Attributes::kOwns);
	}
	return *paths;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageNode::registerConstructor (const FileType& fileType, int fileCommandMask)
{
	FileNodeFactory::instance ().addConstructor (NEW PackageNodeConstructor (fileType, fileCommandMask));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (PackageNode, DirectoryNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageNode::PackageNode (Url* path, BrowserNode* parent, IUrlFilter* urlFilter, bool shouldShowContentAlways)
: DirectoryNode (path, parent, urlFilter),
  package (nullptr),
  signalSink (nullptr),
  shouldShowContent (shouldShowContentAlways),
  shouldShowContentAlways (shouldShowContentAlways)
{
	restore ();

	fileCommandMask &= ~kAllDirCommands; // disable directory commands
	fileCommandMask |= kAllFileCommands; // but keep file commands
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageNode::~PackageNode ()
{
	unmount (false);
	listenToFileSystemSignals (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageNode::listenToFileSystemSignals (bool state)
{
	if(state)
	{
		if(signalSink == nullptr)
		{
			// listen to the kReleaseFile && kFileCreated signals
			signalSink = NEW SignalSink (Signals::kFileSystem);
			signalSink->setObserver (this);
			signalSink->enable (true);
		}
	}
	else
	{
		if(signalSink)
		{
			signalSink->enable (false);
			delete signalSink;
			signalSink = nullptr;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearchProvider* PackageNode::getSearchProvider ()
{
	return getAncestorNode<DirectoryNode> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageNode::restore ()
{
	ASSERT (path != nullptr)
	if(path == nullptr)
		return;

	if(shouldShowContentAlways)
		shouldShowContent = true;
	else
	{
		Container& paths = getPackagePaths ();
		if(paths.contains (*path))
			shouldShowContent = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageNode::store ()
{
	ASSERT (path != nullptr)
	if(path == nullptr)
		return;

	Container& paths = getPackagePaths ();
	if(shouldShowContent && shouldShowContentAlways == false)
	{
		if(!paths.contains (*path))
		{
			paths.add (path);
			path->retain ();
		}
	}
	else
	{
		Url* p = ccl_cast<Url> (paths.findEqual (*path));
		if(p)
		{
			paths.remove (p),
			p->release ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageNode::mount ()
{
	if(packageId.isEmpty ())
		packageId = UIDString::generate ();

	if(package == nullptr && path != nullptr)
	{
		AutoPtr<IPackageFile> p = System::GetPackageHandler ().openPackage (*path);
		ASSERT (p != nullptr)
		if(p)
		{
			tresult result = System::GetPackageHandler ().mountPackageVolume (p, packageId, IPackageVolume::kHidden);
			ASSERT (result == kResultOk)
			if(result == kResultOk)
			{
				p->retain ();
				package = p;

				listenToFileSystemSignals (true);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageNode::unmount (bool expectRemount)
{
	if(package)
	{
		System::GetPackageHandler ().unmountPackageVolume (package);
		package->close ();
		package->release ();
		package = nullptr;

		if(expectRemount == false)
			listenToFileSystemSignals (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageNode::getContentPath (Url& path) const
{
	ASSERT (shouldShowContent == true)

	const_cast<PackageNode*> (this)->mount ();

	path.assign (PackageUrl (packageId));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageNode::showContent (bool state, bool force)
{
	if(state == false && force == false && shouldShowContentAlways)
		return;

	shouldShowContent = state;
	if(shouldShowContent == false)
		unmount (force);

	store ();

	if(Browser* browser = getBrowser ())
		browser->refreshNode (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* PackageNode::createIterator (int mode) const
{
	if(shouldShowContent == false)
		return nullptr;

	Url contentPath;
	getContentPath (contentPath);
	return System::GetFileSystem ().newIterator (contentPath, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageNode::hasSubNodes () const
{
	return shouldShowContent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* PackageNode::getIcon ()
{
	return FileNode::getIcon (); // use icon of package file
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID PackageNode::getCustomBackground () const
{
	return FileNode::getCustomBackground (); // suppress folder background
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PackageNode::appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes)
{
	#if 1
	UnknownPtr<IMenu> menu (&contextMenu);
	if(menu)
	{
		MenuInserter inserter (menu, 0);
		if(shouldShowContentAlways == false || shouldShowContent == false)
			menu->addCommandItem (XSTR (ShowPackageContents), CSTR ("Browser"), CSTR ("Show Package Contents"), nullptr);
		if(canExtract ())
			menu->addCommandItem (XSTR (ExtractHere), CSTR ("Browser"), CSTR ("Extract Here"), nullptr);
		menu->addSeparatorItem ();
	}
	else
	#endif
	{
		if(shouldShowContentAlways == false || shouldShowContent == false)
		{
			contextMenu.addCommandItem (XSTR (ShowPackageContents), CSTR ("Browser"), CSTR ("Show Package Contents"), nullptr);
			contextMenu.addSeparatorItem ();
		}
	}

	return SuperClass::appendContextMenu (contextMenu, selectedNodes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageNode::interpretCommand (const CommandMsg& msg, const Container* selectedNodes)
{
	if(msg.category == "Browser" && msg.name == "Show Package Contents")
	{
		if(msg.checkOnly ())
		{
			UnknownPtr<IMenuItem> menuItem (msg.invoker);
			if(menuItem)
				menuItem->setItemAttribute (IMenuItem::kItemChecked, shouldShowContent);
			return true;
		}

		ObjectList candidates;
		if(selectedNodes)
			ForEach (*selectedNodes, BrowserNode, node)
				if(ccl_cast<PackageNode> (node))
					candidates.add (node);
			EndFor

		if(!candidates.contains (this))
			candidates.add (this);

		bool state = !shouldShowContent;
		ForEach (candidates, PackageNode, node)
			node->showContent (state);
		EndFor
		return true;
	}
	else if(msg.category == "Browser" && msg.name == "Extract Here")
	{
		return extract (msg.checkOnly ());
	}
	return SuperClass::interpretCommand (msg, selectedNodes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PackageNode::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kReleaseFile)
	{
		// unmount file
		UnknownPtr<IUrl> p (msg[0]);
		if(p && path && *path == *p && shouldShowContent == true)
			showContent (false, true);
	}
	else if(msg == Signals::kFileCreated)
	{
		// remount file
		UnknownPtr<IUrl> p (msg[0]);
		if(p && path && *path == *p && shouldShowContent == false)
			showContent (true);
	}
	return SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageNode::onOpen (bool deferred)
{
	if(extract (false)) // extract on doubleclick
		return true;

	return SuperClass::onOpen (deferred);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageNode::extract (bool checkOnly)
{
	if(!canExtract ())
		return false;

	Url folder (getFilePath ());
	folder.ascend ();
	if(System::GetFileSystem ().isWriteProtected (folder))
		return false;

	if(!checkOnly)
	{
		AutoPtr<IPackageFile> p;
		p.share (this->package);
		if(p == nullptr)
			p = System::GetPackageHandler ().openPackage (*path);

		if(p != nullptr)
		{
			AutoPtr<Url> dst = NEW Url (folder);
			String fileName;
			getFilePath ().getName (fileName, false);
			dst->descend (fileName, Url::kFolder);
			dst->makeUnique ();

			AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
			progress->setTitle (XSTR (ExtractHere));
			ProgressNotifyScope scope (progress);

			if(p->extractAll (*dst, true, nullptr, progress) > 0)
			{
				scope.finish ();
				SignalSource (Signals::kFileSystem).signal (Message (Signals::kFileCreated, static_cast<IUrl*> (dst)));
			}
		}
	}
	return true;
}

//************************************************************************************************
// Browsable::VolumeNode
//************************************************************************************************

DEFINE_CLASS (VolumeNode, DirectoryNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

VolumeNode::VolumeNode (Url* path, BrowserNode* parent, IUrlFilter* urlFilter)
: DirectoryNode (path, parent, urlFilter),
  volumeType (VolumeInfo::kUnknown)
{
	canRenameFile (false);
	canDeleteFile (false);

	ASSERT (path != nullptr)
	if(!path)
		return;

	VolumeInfo info;
	System::GetFileSystem ().getVolumeInfo (info, *path);

	title = PathClassifier::getVolumeLabel (*path, info);
	volumeType = info.type;
	volumeSubType = info.subType;

	String pathName;
	path->getName (pathName);
	if(!pathName.isEmpty ())
	{
		uniqueNodeName = pathName; // don't use the volume label (on Windows), might be renamed
		uniqueNodeName.replace ('/', '\\');
	}
	else if(!path->getHostName ().isEmpty ())
		uniqueNodeName = path->getHostName ();
	else
		uniqueNodeName = PathClassifier::getVolumeIdentifier (*path, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VolumeNode::getUniqueName (MutableCString& name)
{
	name = uniqueNodeName;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* VolumeNode::getIcon ()
{
	if(icon == nullptr)
	{
		// try sub type first
		if(!volumeSubType.isEmpty ())
		{
			MutableCString iconName ("VolumeIcon:");
			iconName += volumeSubType;
			icon = getThemeIcon (iconName);
		}

		if(icon == nullptr)
		{
			icon = FileIcons::instance ().createVolumeIcon (volumeType);
			if(icon)
				icon->release (); // shared pointer!
		}
	}

	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VolumeNode::hasSubNodes () const
{
	if(hasNoSubnodes)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int VolumeNode::compare (const Object& obj) const
{
	if(const VolumeNode* other = ccl_cast<VolumeNode> (&obj))
		return path->getPath ().compareWithOptions (other->getPath ()->getPath (), Text::kIgnoreCase|Text::kCompareNumerically);

	return SuperClass::compare (obj);
}

//************************************************************************************************
// Browsable::VolumeListNode
//************************************************************************************************

DEFINE_CLASS (VolumeListNode, DirectoryNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

VolumeListNode::VolumeListNode (BrowserNode* parent, IUrlFilter* urlFilter)
: TranslatedDirectoryNode (NEW Url ("file:///", Url::kFolder), parent, urlFilter)
{
	canRenameFile (false);
	canDeleteFile (false);
	canCreateSubFolder (false);
	canShowInShellBrowser (false);

	path->release (); // shared!
	setTranslatedTitle (XSTR_REF (Volumes));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* VolumeListNode::getIcon ()
{
	if(icon == nullptr)
	{
		icon = FileIcons::instance ().createVolumeIcon (FileIcons::kVolumeList);
		if(icon)
			icon->release (); // shared pointer!
	}
	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VolumeListNode::isHiddenFile (UrlRef path) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* VolumeListNode::createNode (Url* forPath) const
{
	return NEW VolumeNode (forPath, nullptr, urlFilter);
}

//************************************************************************************************
// Browsable::PackageVolumeNode
//************************************************************************************************

DEFINE_CLASS (PackageVolumeNode, VolumeNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageVolumeNode::PackageVolumeNode (Url* path, BrowserNode* parent, IUrlFilter* urlFilter)
: VolumeNode (path, parent, urlFilter)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PackageVolumeNode::compare (const Object& obj) const
{
	if(const PackageVolumeNode* other = ccl_cast<PackageVolumeNode> (&obj))
	{
		// sort by 1.) volume sub type
		int subTypeResult = getVolumeSubType ().compare (other->getVolumeSubType (), false);
		if(subTypeResult != 0)
			return subTypeResult;

		// 2.) displayed title
		return compareTitle (*other);
	}

	return SuperClass::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* PackageVolumeNode::createDragObject ()
{
	if(PackageRootNode* packageRootNode = ccl_cast<PackageRootNode> (getParent ()))
		if(packageRootNode->isPackageDragEnabled ())
		{
			AutoPtr<IPackageVolume> volume = System::GetPackageHandler ().openPackageVolume (getFilePath ().getHostName ());
			if(volume && volume->getPackage ())
				return ccl_as_unknown (NEW Url (volume->getPackage ()->getPath ()));
		}
	return nullptr;
}

//************************************************************************************************
// Browsable::PackageRootNode
//************************************************************************************************

DEFINE_CLASS (PackageRootNode, DirectoryNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageRootNode::PackageRootNode (BrowserNode* parent, IUrlFilter* urlFilter, StringRef volumeSubType)
: TranslatedDirectoryNode (NEW Url ("package:///", Url::kFolder), parent, urlFilter),
  volumeSubType (volumeSubType),
  packageDragEnabled (false),
  packageSink (*NEW SignalSink (Signals::kPackageHandler)),
  insideRefresh (false)
{
	canRenameFile (false);
	canDeleteFile (false);
	canCreateSubFolder (false);

	packageSink.setObserver (this);
	packageSink.enable (true);

	path->release (); // shared!
	setTranslatedTitle (XSTR_REF (Packages));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageRootNode::~PackageRootNode ()
{
	packageSink.enable (false);
	delete &packageSink;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageRootNode::onRefresh ()
{
	if(insideRefresh == false)
	{
		ScopedVar<bool> refreshScope (insideRefresh, true);
		SignalSource (Signals::kPackageHandler).signal (Message (Signals::kRescanPackages));
	}

	return SuperClass::onRefresh ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PackageRootNode::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kPackagesChanged)
	{
		if(insideRefresh == false)
		{
			ScopedVar<bool> refreshScope (insideRefresh, true);
			if(Browser* browser = getBrowser ())
				browser->refreshNode (this);
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* PackageRootNode::createIterator (int mode) const
{
	if(!volumeSubType.isEmpty ())
	{
		struct PackageVolumeFilter: UrlFilter
		{
			String subType;
			PackageVolumeFilter (StringRef subType): subType (subType) {}
			tbool CCL_API matches (UrlRef url) const override
			{
				VolumeInfo info;
				System::GetFileSystem ().getVolumeInfo (info, url);
				return info.subType == subType;
			}
		};

		if(AutoPtr<IFileIterator> iter = SuperClass::createIterator (mode))
			return File::filterIterator (*iter, PackageVolumeFilter (volumeSubType));
	}

	return SuperClass::createIterator (mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* PackageRootNode::createNode (Url* forPath) const
{
	return NEW PackageVolumeNode (forPath, nullptr, urlFilter);
}

//************************************************************************************************
// OptionalPackageRootNode
//************************************************************************************************

OptionalPackageRootNode::OptionalPackageRootNode (FolderNode* targetParent, IUrlFilter* urlFilter, StringRef volumeSubType)
: PackageRootNode (nullptr, urlFilter, volumeSubType),
  targetParent (targetParent),
  targetIndex (0)
{
	ASSERT (targetParent)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionalPackageRootNode::init ()
{
	if(targetParent && !getParent ())
	{
		// add immediately if possible, or remember where to insert
		if(hasFileNodes ())
			targetParent->add (return_shared (this));
		else
			targetIndex = targetParent->countNodes ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OptionalPackageRootNode::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kPackageMounted)
	{
		if(targetParent)
		{
			// check if package extension matches our volumeSubType (see PackageRootFileSystem::getVolumeInfo)
			UnknownPtr<IPackageFile> package (msg[0]);
			if(package
				&& package->getPath ().getFileType ().getExtension () == volumeSubType
				&& hasFileNodes ())
			{
				// add when required for the first time
				if(!getParent ())
					targetParent->insertAt (targetIndex, return_shared (this));

				if(Browser* browser = targetParent->getBrowser ())
					browser->refreshNode (targetParent, true);
			}
		}
	}
	SuperClass::notify (subject, msg);
}

//************************************************************************************************
// Browsable::SystemFolderNode
//************************************************************************************************

SystemFolderNode::SystemFolderNode (int folderType, const LocalString& title, BrowserNode* parent, IUrlFilter* urlFilter)
: TranslatedDirectoryNode (nullptr, parent, urlFilter)
{
	canRenameFile (false);
	canDeleteFile (false);

	AutoPtr<Url> location = NEW Url;
	if (System::GetSystem ().getLocation (*location, folderType))
	{
		setPath (location);
	}

	setTranslatedTitle (title);
}

//************************************************************************************************
// Browsable::UserContentNode
//************************************************************************************************

DEFINE_CLASS (UserContentNode, DirectoryNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserContentNode::UserContentNode (BrowserNode* parent, IUrlFilter* urlFilter)
: TranslatedDirectoryNode (nullptr, parent, urlFilter),
  systemSink (*NEW SignalSink (Signals::kSystemInformation))
{
	canRenameFile (false);
	canDeleteFile (false);

	systemSink.setObserver (this);
	systemSink.enable (true);

	setTranslatedTitle (XSTR_REF (Content));
	updatePath ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserContentNode::~UserContentNode ()
{
	systemSink.enable (false);
	delete &systemSink;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserContentNode::updatePath ()
{
	AutoPtr<Url> contentPath = NEW Url;
	System::GetSystem ().getLocation (*contentPath, System::kUserContentFolder);
	setPath (contentPath);

	updateTranslatedTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserContentNode::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kContentLocationChanged)
	{
		updatePath ();

		if(Browser* browser = getBrowser ())
			browser->refreshNode (this);
	}
	else
		SuperClass::notify (subject, msg);
}

//************************************************************************************************
// FileNodeRecognizer
//************************************************************************************************

tbool CCL_API FileNodeRecognizer::recognize (IUnknown* object) const
{
	if(FileNode* node = unknown_cast<FileNode> (object))
		if(const Url* path = node->getPath ())
			return url.isEqualUrl (*path);
	return false;
}
