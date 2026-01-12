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
// Filename    : ccl/extras/webfs/webfilenodes.cpp
// Description : Web File Nodes
//
//************************************************************************************************

#include "ccl/extras/webfs/webfilenodes.h"
#include "ccl/extras/webfs/webfilemethods.h"
#include "ccl/extras/webfs/webfiledraghandler.h"

#include "ccl/app/browser/filebrowser.h"
#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/controls/itemviewmodel.h"

#include "ccl/base/signalsource.h"

#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/network/web/iwebfileservice.h"
#include "ccl/public/netservices.h"

using namespace CCL;
using namespace Web;
using namespace Browsable;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("WebFS")
	XSTRING (WebRoot, "Cloud")
END_XSTRINGS

//************************************************************************************************
// WebNodesBuilder
//************************************************************************************************

WebNodesBuilder::WebNodesBuilder ()
: insideNodeUpdate (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNodesBuilder::onRefresh (DirectoryNode& node)
{
	if(insideNodeUpdate == false) // avoid recursion
	{
		System::GetWebFileService ().discardDirectory (node.getFilePath ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebNodesBuilder::getSubNodes (DirectoryNode& node, Container& children, BrowserNode::NodeFlags flags)
{
	UrlRef path = node.getFilePath ();

	int mode = 0;// TODO? IFileIterator::kIgnoreHidden;
	if(flags.wantFolders ())
		mode |= IFileIterator::kFolders;
	if(flags.wantLeafs ())
		mode |= IFileIterator::kFiles;

	IFileIterator* iter = System::GetFileSystem ().newIterator (path, mode);
	if(iter)
	{
		ObjectList files, folders; // sort files and folders

		IUrlFilter* urlFilter = node.getUrlFilter ();
		ForEachFile (iter, p)
			if(urlFilter && !urlFilter->matches (*p))
				continue;

			if(node.isHiddenFile (*p))
				continue;

			if(p->isFolder ())
			{
				BrowserNode* child = NEW WebDirectoryNode (AutoPtr<Url> (NEW Url (*p)), &node, urlFilter);
				folders.addSorted (child);
			}
			else
			{
				BrowserNode* child = NEW WebFileNode (AutoPtr<Url> (NEW Url (*p)), &node);
				files.addSorted (child);
			}
		EndFor

		children.add (folders);
		children.add (files);
	}
	else // start directory request
	{
		setWebFSUrl (path);
		restart ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNodesBuilder::updateNode (BrowserNode& node)
{
	ScopedVar<bool> scope (insideNodeUpdate, true);

	if(state == kCompleted)
	{
		if(Browser* browser = node.getBrowser ())
			browser->refreshNode (&node);
	}
	else
	{
		if(Browser* browser = node.getBrowser ())
			browser->redrawNode (&node);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNodesBuilder::drawStateOverlay (BrowserNode& node, const IItemModel::DrawInfo& info)
{
	if(state == kPending || state == kFailed)
	{
		static SharedPtr<IImage> pendingIcon, failedIcon;
		static bool iconsChecked = false;
		if(!iconsChecked)
		{
			iconsChecked = true;
			ITheme& theme = ViewBox (info.view).getTheme ();
			pendingIcon = theme.getImage ("OverlayIcon:WebPending");
			failedIcon = theme.getImage ("OverlayIcon:WebFailed");
		}

		IImage* icon = state == kPending ? pendingIcon : failedIcon;
		if(icon)
			ItemModelPainter ().drawIcon (info, icon, true, false);
		else
		{
			Color color = state == kPending ? Colors::kGreen : Colors::kRed; 
			color.setAlphaF (.2f); 
			info.graphics.fillRect (info.rect, SolidBrush (color)); 
		}
	}
}

//************************************************************************************************
// WebNodeCustomizer
//************************************************************************************************

WebNodeCustomizer::WebNodeCustomizer ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Interface>
Interface* WebNodeCustomizer::openHandler (FileNode& node)
{
	return System::GetWebFileService ().openHandler<Interface> (node.getFilePath ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNodeCustomizer::customizeFile (WebFileNode& node)
{
	customize (node, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNodeCustomizer::customizeSearchResult (FileNode& node)
{
	if(AutoPtr<IWebFileBrowserModel> model = openHandler<IWebFileBrowserModel> (node))
		if(AutoPtr<IFileDescriptor> webfsItem = System::GetWebFileService ().openFileItem (node.getFilePath ()))
			model->triggerThumbnailDownload (webfsItem, node.getFilePath ());

	customize (node, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNodeCustomizer::customize (FileNode& node, bool isThisNode)
{
	// file-level customization
	node.canShowInShellBrowser (false);
	UrlRef path = node.getFilePath ();
	node.canRenameFile (FileMethods ().canRenameFile (path));
	node.canDeleteFile (FileMethods ().canDeleteFile (path));

	// browser-level customization
	if(AutoPtr<IWebFileBrowserModel> model = openHandler<IWebFileBrowserModel> (node))
	{
		if(AutoPtr<IFileDescriptor> webfsItem = System::GetWebFileService ().openFileItem (path))
		{
			String title;
			if(webfsItem->getTitle (title))
				node.setTitle (title);

			if(IImage* icon = model->getItemIcon (webfsItem))
				node.setIcon (icon);

			if(IImage* image = model->getItemThumbnail (webfsItem))
				node.setThumbnail (image);
		}

		if(isThisNode == true)
			backgroundId = model->getCustomBackground (path);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNodeCustomizer::customizeFolder (WebDirectoryNode& node)
{
	// file-level customization
	UrlRef path = node.getFilePath ();
	node.canCreateSubFolder (FileMethods ().canCreateFolder (path));

	customize (node, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNodeCustomizer::customizeVolume (WebVolumeNode& node, bool isUpdate)
{
	// file-level customization
	node.canShowInShellBrowser (false);
	node.canRenameFile (false);
	node.canDeleteFile (false);

	UrlRef path = node.getFilePath ();
	node.canCreateSubFolder (FileMethods ().canCreateFolder (path));

	// browser-level customization
	if(AutoPtr<IWebFileBrowserModel> model = openHandler<IWebFileBrowserModel> (node))
	{
		if(isUpdate == false)
			model->attachToBrowser (&node, true);

		if(IImage* icon = model->getVolumeIcon (path))
			node.setIcon (icon);

		if(IUrlFilter* filter = model->getUrlFilter ())
			node.setUrlFilter (filter);

		backgroundId = model->getCustomBackground (path);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNodeCustomizer::uncustomizeVolume (WebVolumeNode& node)
{
	// reset browser-level customization
	if(AutoPtr<IWebFileBrowserModel> model = openHandler<IWebFileBrowserModel> (node))
		model->attachToBrowser (&node, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID WebNodeCustomizer::getBackgroundID (StringID defaultId) const
{
	if(!backgroundId.isEmpty ())
		return backgroundId;
	else
		return defaultId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNodeCustomizer::refresh ()
{
	// nothing here
}

//************************************************************************************************
// WebRootNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebRootNode, TranslatedDirectoryNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebRootNode::WebRootNode (BrowserNode* parent, IUrlFilter* urlFilter)
: TranslatedDirectoryNode (NEW Url (String () << IWebFileService::kProtocol << ":///", Url::kFolder), parent, urlFilter),
  signalSink (*NEW SignalSink (Signals::kWebFiles))
{
	path->release (); // shared!
	setTranslatedTitle (XSTR_REF (WebRoot));

	canRenameFile (false);
	canDeleteFile (false);
	canCreateSubFolder (false);
	canShowInShellBrowser (false);

	signalSink.setObserver (this);
	signalSink.enable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebRootNode::~WebRootNode ()
{
	signalSink.enable (false);
	delete &signalSink;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearchProvider* WebRootNode::getSearchProvider ()
{
	return nullptr; // no search here!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* WebRootNode::createDragObject ()
{
	return nullptr; // nothing to drag here!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* WebRootNode::getIcon ()
{
	if(icon == nullptr)
		icon = getThemeIcon ("VolumeIcon:WebRoot");
	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* WebRootNode::createNode (Url* forPath) const
{
	return NEW WebVolumeNode (forPath, nullptr, urlFilter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebRootNode::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kDirectoryChanged) // caused by upload via transfer manager or remote file operations
	{
		UnknownPtr<IUrl> webfsUrl (msg[0]);
		ASSERT (webfsUrl)

		if(Browser* browser = getBrowser ())
		{
			FileNodeFinder finder (*browser);
			if(FileNode* dirNode = finder.findFileNode (*this, *webfsUrl))
				browser->refreshNode (dirNode);
		}
	}
	else if(msg == Signals::kVolumesChanged)
	{
		if(Browser* browser = getBrowser ())
		{
			BrowserNode* toRefresh = this;
			if(msg.getArgCount () >= 2)
			{
				String volumeName (msg[0].asString ());
				MutableCString type (msg[1].asString ());
				if(type == Signals::kVolumeChangeMounted || type == Signals::kVolumeChangeUnmounted)
				{
					Url targetPath (getFilePath ());
					targetPath.descend (volumeName, Url::kFolder);

					FileNodeFinder finder (*browser);
					if(FileNode* volumeNode = finder.findFileNode (*this, targetPath))
					{
						if(type == Signals::kVolumeChangeMounted)
							toRefresh = volumeNode;
						else
						{
							browser->removeNode (volumeNode);
							return;
						}
					}
				}
			}
			browser->refreshNode (toRefresh);
		}
	}
	else if(msg == Signals::kRevealVolume)
	{
		if(Browser* browser = getBrowser ())
		{
			String volumeName (msg[0].asString ());
			Url targetPath (getFilePath ());
			targetPath.descend (volumeName, Url::kFolder);

			FileNodeFinder finder (*browser);
			if(FileNode* volumeNode = finder.findFileNode (*this, targetPath))
				browser->expandNode (volumeNode);
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//************************************************************************************************
// IWebNode
//************************************************************************************************

DEFINE_IID_ (IWebNode, 0xa8bd09c5, 0x6bef, 0x4c11, 0x87, 0xb0, 0x30, 0xd9, 0x70, 0xc6, 0x5f, 0x85)

//************************************************************************************************
// WebVolumeNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebVolumeNode, VolumeNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebVolumeNode::WebVolumeNode (Url* path, BrowserNode* parent, IUrlFilter* urlFilter)
: VolumeNode (path, parent, urlFilter)
{
	builder.addObserver (this);

	customizer.customizeVolume (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebVolumeNode::~WebVolumeNode ()
{
	customizer.uncustomizeVolume (*this);

	builder.removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebVolumeNode::getTargetLocation (Url& path) const
{
	return FileMethods ().getUploadFolder (path, getFilePath ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebVolumeNode::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	ForEachUnknown (data, unk)
		if(UnknownPtr<IDownloadable> downloadable = unk)
			if(FileMethods ().isSameVolume (downloadable->getSourceUrl (), *getPath ()) == false)
				return false;
	EndFor

	if(AutoPtr<IDataTarget> dataTarget = customizer.openHandler<IDataTarget> (*this))
		return dataTarget->canInsertData (data, session, targetView, insertIndex);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebVolumeNode::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	if(AutoPtr<IDataTarget> dataTarget = customizer.openHandler<IDataTarget> (*this))
		return dataTarget->insertData (data, session, insertIndex);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebVolumeNode::getSubNodes (Container& children, NodeFlags flags)
{
	return builder.getSubNodes (*this, children, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID WebVolumeNode::getCustomBackground () const
{
	return customizer.getBackgroundID (CSTR ("webvolume"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebVolumeNode::drawIconOverlay (const IItemModel::DrawInfo& info)
{
	builder.drawStateOverlay (*this, info);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebVolumeNode::onRefresh ()
{
	builder.onRefresh (*this);
	customizer.refresh ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebVolumeNode::onOpen (bool deferred)
{
	return onOpenVolume (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebVolumeNode::onEdit (StringID id, const IItemModel::EditInfo& info)
{
	return onOpenVolume (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebVolumeNode::onOpenVolume (bool isEdit)
{
	if(AutoPtr<IWebFileBrowserModel> model = customizer.openHandler<IWebFileBrowserModel> (*this))
		if(model->onOpenVolume (getFilePath (), isEdit))
			if(Browser* browser = getBrowser ())
			{
				if(browser->isListMode ())
					browser->setTreeFocusNode (this);
				else
					browser->expandNode (this);
				return true;
			}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebVolumeNode::updateItemInBrowser (UrlRef webfsUrl)
{
	if(Browser* browser = getBrowser ())
	{
		FileNode* node = ccl_cast<FileNode> (browser->findNodeInSearchResults (webfsUrl));
		if(!node)
			node = FileNodeFinder (*browser).findFileNode (*this, webfsUrl);;
		if(node)
		{
			if(UnknownPtr<IWebNode> webNode = node->asUnknown ())
				webNode->updateCustomization ();
			else
			{
				// search result: only update thumbnail
				if(AutoPtr<IWebFileBrowserModel> model = customizer.openHandler<IWebFileBrowserModel> (*this))
					if(AutoPtr<IFileDescriptor> webfsItem = System::GetWebFileService ().openFileItem (node->getFilePath ()))
						if(IImage* image = model->getItemThumbnail (webfsItem))
							node->setThumbnail (image);
			}
			browser->updateThumbnail (node);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebVolumeNode::setItemUpdateInProgress (tbool state)
{
	if(Browser* browser = getBrowser ())
		browser->setActivityIndicatorState (state != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebVolumeNode::notify (ISubject* subject, MessageRef msg)
{
	if(subject == &builder)
	{		
		builder.updateNode (*this);

		if(builder.isCompleted ())
			if(AutoPtr<IWebFileBrowserModel> model = customizer.openHandler<IWebFileBrowserModel> (*this))
				model->onDirectoryExpanded (getFilePath ());
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* WebVolumeNode::createSearcher (ISearchDescription& description)
{
	return System::GetWebFileService ().createSearcher (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* WebVolumeNode::customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem)
{
	if(FileNode* fileNode = ccl_cast<FileNode> (&args.presentation))
	{
		customizer.customizeSearchResult (*fileNode);

		// add default folder icon
		if(fileNode->getFilePath ().isFolder () && !fileNode->getIcon ())
			fileNode->setIcon (FileIcons::instance ().getDefaultFolderIcon ());

		// prepare IDownloadable for dragging
		UrlRef path = fileNode->getFilePath ();
		if(FileMethods ().canDownload (path))
			return FileMethods ().createDownloadable (path);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebVolumeNode::updateCustomization ()
{
	customizer.customizeVolume (*this, true);
}

//************************************************************************************************
// WebDirectoryNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebDirectoryNode, DirectoryNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebDirectoryNode::WebDirectoryNode (Url* path, BrowserNode* parent, IUrlFilter* urlFilter)
: DirectoryNode (path, parent, urlFilter)
{
	builder.addObserver (this);

	customizer.customizeFolder (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebDirectoryNode::~WebDirectoryNode ()
{
	builder.removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebDirectoryNode::getUniqueName (MutableCString& name)
{
	// use file name instead of title
	String fileName;
	getFilePath ().getName (fileName);

	name.empty ();
	name.append (fileName, Text::kUTF8);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebDirectoryNode::isHiddenFile (UrlRef path) const
{
	UrlDisplayString displayString (path, Url::kStringDisplayName);
	return displayString.startsWith (".");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebDirectoryNode::getTargetLocation (Url& path) const
{
	return FileMethods ().getUploadFolder (path, getFilePath ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebDirectoryNode::shouldCopyByDefault (UrlRef sourcePath) const
{
	Url uploadFolder;
	if(getTargetLocation (uploadFolder) && uploadFolder == sourcePath)
		return true;

	return SuperClass::shouldCopyByDefault (sourcePath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebDirectoryNode::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	if(WebVolumeNode* volumeNode = getAncestorNode<WebVolumeNode> ())
		if(volumeNode->canInsertData (data, session, targetView, insertIndex))
			return true;
	
	ForEachUnknown (data, unk)
		if(UnknownPtr<IUrl> path = unk)
		{
			if(FileMethods ().canUploadFrom (*path) == false)
				return false;
			if(FileMethods ().canUploadToFolder (*getPath ()) == false)
				return false;
			return true;
		}
		else if(UnknownPtr<IDownloadable> downloadable = unk)
		{
			if(FileMethods ().isSameVolume (downloadable->getSourceUrl (), *getPath ()) == false)
				return false;
			if(FileMethods ().canUploadToFolder (*getPath ()) == false)
				return false;
			if(isAcceptedChildPath (*downloadable) == false)
				return false;
			return true;
		}
	EndFor

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebDirectoryNode::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	if(WebVolumeNode* volumeNode = getAncestorNode<WebVolumeNode> ())
		if(volumeNode->insertData (data, session, insertIndex))
			return true;
		
	ForEachUnknown (data, unk)
		UnknownPtr<IUrl> path (unk);
		UnknownPtr<IDownloadable> downloadable (unk);
		if(path)
			if(FileMethods ().canUploadFrom (*path))
				if(FileMethods ().canUploadToFolder (*getPath ()))
					FileMethods ().uploadObject (*getPath (), *path);
		if(downloadable)
			if(FileMethods ().isSameVolume (downloadable->getSourceUrl (), *getPath ()))
				if(FileMethods ().canUploadToFolder (*getPath ()))
					if(isAcceptedChildPath (*downloadable))
						FileMethods ().moveObjectToFolder (downloadable->getSourceUrl (), *getPath ());
		EndFor
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebDirectoryNode::isAcceptedChildPath (IDownloadable& webFile) const
{
	const WebDirectoryNode* restrictedRoot = nullptr;
	const WebDirectoryNode* node = this;
	while(node)
	{
		if(FileMethods ().acceptsChildrenOnly (*node->getPath ()))
			restrictedRoot = node;
		node = ccl_cast<WebDirectoryNode> (node->getParent ());
	}
	if(restrictedRoot)
	{
		String sourcePath = webFile.getSourceUrl ().getPath ();
		String restrictedRootPath = restrictedRoot->getPath ()->getPath ();
		if(sourcePath.contains (restrictedRootPath) == false)
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearchProvider* WebDirectoryNode::getSearchProvider ()
{
	return getAncestorNode<WebVolumeNode> (); // search is handled by web volume
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebDirectoryNode::getSubNodes (Container& children, NodeFlags flags)
{
	return builder.getSubNodes (*this, children, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID WebDirectoryNode::getCustomBackground () const
{
	return customizer.getBackgroundID (CSTR ("webfolder"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebDirectoryNode::drawIconOverlay (const IItemModel::DrawInfo& info)
{
	builder.drawStateOverlay (*this, info);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* WebDirectoryNode::createDragObject ()
{
	UrlRef path = getFilePath ();
	if(FileMethods ().canMoveFolder (path))
		return FileMethods ().createDownloadable (path); 
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebDirectoryNode::onRefresh ()
{
	builder.onRefresh (*this);
	customizer.refresh ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebDirectoryNode::notify (ISubject* subject, MessageRef msg)
{
	if(subject == &builder)
	{
		builder.updateNode (*this);

		if(builder.isCompleted ())
			if(AutoPtr<IWebFileBrowserModel> model = customizer.openHandler<IWebFileBrowserModel> (*this))
				model->onDirectoryExpanded (getFilePath ());
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebDirectoryNode::updateCustomization ()
{
	customizer.customizeFolder (*this);
}

//************************************************************************************************
// WebFileNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebFileNode, FileNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebFileNode::WebFileNode (Url* path, BrowserNode* parent)
: FileNode (path, parent)
{
	customizer.customizeFile (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebFileNode::getUniqueName (MutableCString& name)
{
	// use file name instead of title
	String fileName;
	getFilePath ().getName (fileName);

	name.empty ();
	name.append (fileName, Text::kUTF8);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID WebFileNode::getCustomBackground () const
{
	return customizer.getBackgroundID (CSTR ("webfile"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* WebFileNode::createDragObject ()
{
	UrlRef path = getFilePath ();
	if(FileMethods ().canDownload (path))
		return FileMethods ().createDownloadable (path);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebFileNode::updateCustomization ()
{
	customizer.customizeFile (*this);
}
