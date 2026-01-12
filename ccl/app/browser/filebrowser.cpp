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
// Filename    : ccl/app/browser/filebrowser.cpp
// Description : File Browser
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/browser/filebrowser.h"
#include "ccl/app/browser/filesystemnodes.h"
#include "ccl/app/browser/filexportdraghandler.h"

#include "ccl/app/controls/draghandler.h"
#include "ccl/app/fileinfo/filepreviewcomponent.h"
#include "ccl/app/utilities/pathclassifier.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/viewbox.h"

using namespace CCL;

using Browsable::FileNode;
using Browsable::DirectoryNode;
using Browsable::VolumeListNode;

namespace CCL {

//************************************************************************************************
// FileNodeTraverser
/** Calls processNode for every node with the given url (can be multiple nodes). */
//************************************************************************************************

class FileNodeTraverser: public FileNodeFinder
{
public:
	FileNodeTraverser (Browser& browser, UrlRef targetPath);

	void traverse ();
	void traverse (BrowserNode* startNode);

protected:
	Url targetPath;
	bool isDone;

	virtual void processNode (FileNode& fileNode) = 0;

	void traverse (DirectoryNode& directoryNode);
};

//************************************************************************************************
// FileNodeInserter
/** Inserts a new file node for the given url, if the parent node already exists. */
//************************************************************************************************

class FileNodeInserter: public FileNodeTraverser
{
public:
	FileNodeInserter (Browser& browser, UrlRef filePath);

protected:
	// FileNodeTraverser
	void processNode (FileNode& fileNode) override;
};

//************************************************************************************************
// FileNodeRemover
/** Removes a new file node with the given url, if it exists. */
//************************************************************************************************

class FileNodeRemover: public FileNodeTraverser
{
public:
	FileNodeRemover (Browser& browser, UrlRef filePath);

protected:
	// FileNodeTraverser
	void processNode (FileNode& fileNode) override;
};

//************************************************************************************************
// FileNodeCreator
/** Find the first node with the given url (creates the nodes and parents if necessary). */
//************************************************************************************************

class FileNodeCreator: public FileNodeTraverser
{
public:
	FileNodeCreator (Browser& browser, UrlRef filePath);

	FileNode* foundNode;

protected:
	// FileNodeTraverser
	void processNode (FileNode& fileNode) override;
};

} // namespace CCL

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Browser")
	XSTRING (Desktop, "Desktop")
	XSTRING (Documents, "Documents")
	XSTRING (Music, "Music")
END_XSTRINGS

//************************************************************************************************
// FileNodeFinder
//************************************************************************************************

FileNodeFinder::FileNodeFinder (Browser& browser)
: browser (browser),
  acceptAncestor (true),
  createNodes (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileNode* FileNodeFinder::findFileNode (DirectoryNode& directoryNode, UrlRef targetPath)
{
	if(const Url* folderPath = directoryNode.getPath ())
	{
		if(folderPath->contains (targetPath))
		{
			// make path relative to the directoryNode's url
			Url relativePath (targetPath);
			relativePath.makeRelative (*folderPath);
			String relativePathString (relativePath.getPath ());
			if(relativePathString.startsWith (CCLSTR ("./")))
				relativePathString = relativePathString.subString (2);

			String relativeBrowserPath;
			if(!targetPath.getHostName ().isEmpty () && folderPath->getHostName ().isEmpty ()) 
			{
				relativeBrowserPath.append (targetPath.getHostName ());
				relativeBrowserPath.append (Url::strPathChar);
			}
			relativeBrowserPath.append (relativePathString);

			MutableCString relativeBrowserPathID (relativeBrowserPath, Text::kUTF8);
			return findFileNode (directoryNode, relativeBrowserPathID);
		}
		else if(*folderPath == targetPath)
			return &directoryNode; 
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileNode* FileNodeFinder::findFileNode (DirectoryNode& directoryNode, StringID relativePathString)
{
	// build browser path of the target node
	MutableCString path;
	browser.makePath (path, &directoryNode);
	if(!path.isEmpty ())
		path.append ("/");
	path.append (relativePathString);

	return ccl_cast<FileNode> (browser.findNode (path, createNodes, acceptAncestor));
}

//************************************************************************************************
// FileNodeTraverser
//************************************************************************************************

FileNodeTraverser::FileNodeTraverser (Browser& browser, UrlRef targetPath)
: FileNodeFinder (browser),
  targetPath (targetPath),
  isDone (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileNodeTraverser::traverse ()
{
	traverse (browser.getTreeRoot ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileNodeTraverser::traverse (BrowserNode* startNode)
{
	if(VolumeListNode* volumeListNode = ccl_cast<VolumeListNode> (startNode))
	{
		if(isCreateNodes ())
		{
			// create filenode deep, relative to volume list node
			MutableCString relativeBrowserPath (targetPath.getPath ());
			if(FileNode* fileNode = findFileNode (*volumeListNode, relativeBrowserPath))
				processNode (*fileNode);
		}
		else
		{
		IterForEachUnknown (browser.iterateChildNodes (*startNode), obj)
			if(BrowserNode* node = unknown_cast<BrowserNode> (obj))
				traverse (node);
			if(isDone)
				return;
		EndFor
	}
	}
	else if(DirectoryNode* directoryNode = ccl_cast<DirectoryNode> (startNode))
	{
		traverse (*directoryNode);
	}
	else if(FolderNode* folderNode = ccl_cast<FolderNode> (startNode))
	{
		ArrayForEach (folderNode->getContent (), BrowserNode, node)
			traverse (node);
			if(isDone)
				return;
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileNodeTraverser::traverse (DirectoryNode& directoryNode)
{
	if(FileNode* fileNode = findFileNode (directoryNode, targetPath))
		processNode (*fileNode);
}

//************************************************************************************************
// FileNodeInserter
//************************************************************************************************

FileNodeInserter::FileNodeInserter (Browser& browser, UrlRef filePath)
: FileNodeTraverser (browser, filePath)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileNodeInserter::processNode (FileNode& fileNode)
{
	if(DirectoryNode* directoryNode = ccl_cast<DirectoryNode> (&fileNode))
		if(*directoryNode->getPath () != targetPath) // found an ancestor, refresh it
			if(browser.wasExpanded (directoryNode))
			{
				browser.refreshNode (directoryNode, true);

				#if DEBUG_LOG
				MutableCString p; browser.makePath (p, directoryNode);
				UrlDisplayString urlStr (targetPath);
				CCL_PRINTF ("FileBrowser: new file \"%s\"\n        refresh node: %s\n", MutableCString (urlStr).str (), p.str ())
				#endif
			}
}

//************************************************************************************************
// FileNodeRemover
//************************************************************************************************

FileNodeRemover::FileNodeRemover (Browser& browser, UrlRef filePath)
: FileNodeTraverser (browser, filePath)
{
	setAcceptAncestor (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileNodeRemover::processNode (FileNode& fileNode)
{
	ASSERT (*fileNode.getPath () == targetPath)

	#if DEBUG_LOG
	MutableCString p; browser.makePath (p, &fileNode);
	UrlDisplayString urlStr (targetPath);
	CCL_PRINTF ("FileBrowser: remove file \"%s\"\n        node: %s\n", MutableCString (urlStr).str (), p.str ())
	#endif

	browser.removeNode (&fileNode);
}

//************************************************************************************************
// FileNodeCreator
//************************************************************************************************

FileNodeCreator::FileNodeCreator (Browser& browser, UrlRef filePath)
: FileNodeTraverser (browser, filePath),
  foundNode (nullptr)
{
	setCreateNodes (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileNodeCreator::processNode (FileNode& fileNode)
{
	foundNode = &fileNode;
	isDone = true;
}

//************************************************************************************************
// FileBrowser::NewFileTabTarget
//************************************************************************************************

class FileBrowser::NewFileTabTarget: public NewTabTarget
{
public:
	NewFileTabTarget (Browser* browser)
	: NewTabTarget (browser)
	{}

	// NewTabTarget
	bool canCreateTab (Browser& browser, const IUnknownList& data) override
	{
		UnknownPtr<IUrl> url (data.getFirst ());
		return url && url->isFolder () && System::GetFileSystem ().fileExists (*url);
		}

	BrowserNode* findNewTabRoot (Browser& browser, const IUnknownList& data) override
	{
		// find (create) the file node and set as root
		UnknownPtr<IUrl> url (data.getFirst ());
		if(url)
		{
			FileNodeCreator finder (browser, *url);
			finder.traverse ();
			return finder.foundNode;
		}
		return nullptr;
	}
};

//************************************************************************************************
// FileBrowser
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FileBrowser, Browser)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileBrowser::FileBrowser (StringRef name, StringRef title, FilePreviewComponent* _preview)
: Browser (name, title),
  urlFilter (NEW UrlFilter ()),
  fileSystemSink (Signals::kFileSystem)
{
	setFormName ("CCL/FileBrowser");

	preview = _preview;
	if(!preview)
		preview = NEW FilePreviewComponent (CCLSTR ("Preview"));
	addComponent (preview);

	fileSystemSink.setObserver (this);
	fileSystemSink.enable (true);

	addComponent (NEW NewFileTabTarget (this));
	addSearch ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileBrowser::~FileBrowser()
{
	fileSystemSink.enable (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileRenamer* FileBrowser::createFileRenamer (BrowserNode* node) const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileBrowser::addDesktop ()
{
	addBrowserNode (NEW Browsable::SystemFolderNode (System::kDesktopFolder, XSTR_REF (Desktop), nullptr, urlFilter));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileBrowser::addUserDocumentFolder ()
{
	addBrowserNode (NEW Browsable::SystemFolderNode (System::kUserDocumentFolder, XSTR_REF (Documents), nullptr, urlFilter));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileBrowser::addUserMusicFolder ()
{
	addBrowserNode (NEW Browsable::SystemFolderNode (System::kUserMusicFolder, XSTR_REF (Music), nullptr, urlFilter));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* FileBrowser::addUserContent ()
{
	Browsable::UserContentNode* node = NEW Browsable::UserContentNode (nullptr, urlFilter);
	addBrowserNode (node);
	return node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileBrowser::addVolumes ()
{
	addBrowserNode (NEW Browsable::VolumeListNode (nullptr, urlFilter));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* FileBrowser::addPackages (StringRef subType)
{
	BrowserNode* node = NEW Browsable::PackageRootNode (nullptr, urlFilter, subType);
	addBrowserNode (node);
	return node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileBrowser::addLocation (UrlRef _path, StringRef title)
{
	AutoPtr<Url> path = NEW Url (_path);
	BrowserNode* node = NEW Browsable::DirectoryNode (path, nullptr, urlFilter);
	addBrowserNode (node);
	if(!title.isEmpty ())
		node->setTitle (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileBrowser::selectFocusFileNode (UrlRef url, bool selectExclusive)
{
	FileNodeCreator finder (*this, url);
	finder.traverse ();
	if(finder.foundNode)
	{
		expandNode (finder.foundNode);
		return setFocusNode (finder.foundNode, selectExclusive);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* FileBrowser::findNodeWithUrl (UrlRef url)
{
	FileNodeCreator finder (*this, url);
	finder.traverse ();
	return finder.foundNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileBrowser::onNodeFocused (BrowserNode* node, bool inList)
{
	Url path;
	IImage* icon = nullptr;
	String title;

	if(node)
	{
		icon = node->getIcon ();
		title = node->getTitle ();

		UnknownPtr<Browsable::IFileNode> fileNode (node->asUnknown ());
		if(fileNode)
		{
			path = fileNode->getFilePath ();

			if(PathClassifier::isRoot (path)) // display root paths as virtual folders 
				path = Url::kEmpty;		
		}
	}

	if(path.isEmpty ())
	{
		path.setProtocol (CCLSTR ("virtual"));

		// path of virtual folder
		String pathString;
		for(BrowserNode* n = node; n && n->getParent (); n = n->getParent ())
		{
			StringRef t (n->getTitle ());
			if(!t.isEmpty ())
			{
				if(!pathString.isEmpty ())
					pathString.prepend (Url::strPathChar);
				pathString.prepend (t);
			}
		}

		path.setPath (pathString, Url::kFolder);
	}

	if(path.isEqualUrl (preview->getFile ()) == false)
		preview->setFile (path, icon, title);

	SuperClass::onNodeFocused (node, inList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileBrowser::onEditNode (BrowserNode& node, StringID columnID, const IItemModel::EditInfo& info)
{
	return SuperClass::onEditNode (node, columnID, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool FileBrowser::canInsertData (BrowserNode* node, const IUnknownList& data, IDragSession* session, IView* targetView)
{
	if(SuperClass::canInsertData (node, data, session, targetView))
		return true;

	AutoPtr<Browsable::FileDraghandlerBase> dragHandler (NEW Browsable::FileDraghandler (targetView, this));
	if(dragHandler->prepare (data, session))
	{
		if(session)
			session->setDragHandler (dragHandler);
		return true;
	}

	dragHandler = NEW Browsable::FileExportDraghandler (targetView, this);
	if(dragHandler->prepare (data, session))
	{
		if(session)
			session->setDragHandler (dragHandler);
		return true;
	}

	#if 0
	// reject files dragged by ourselves
	if(session && isDragSource (*session))
	{
		NullDragHandler::attachToSession (session, targetView);
		return true;
	}
	#endif
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileBrowser::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kFileCreated)
	{
		// a file was created: we may have to add it to the parent folder node
		UnknownPtr<IUrl> url (msg[0]);
		if(url)
		{
			FileNodeInserter inserter (*this, *url);
			inserter.traverse ();
		}
	}
	else if(msg == Signals::kFileRemoved)
	{
		// a file was removed: we may have to remove it from the parent folder node
		UnknownPtr<IUrl> url (msg[0]);
		if(url)
		{
			FileNodeRemover remover (*this, *url);
			remover.traverse ();
		}
	}
	else
		SuperClass::notify (subject, msg);
}
