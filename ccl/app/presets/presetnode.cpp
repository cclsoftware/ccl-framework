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
// Filename    : ccl/app/presets/presetnode.cpp
// Description : Preset Node
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/presets/presetnode.h"
#include "ccl/app/presets/presetfileprimitives.h"
#include "ccl/app/presets/presetfile.h"
#include "ccl/app/presets/presetsystem.h"
#include "ccl/app/presets/presetcomponent.h"

#include "ccl/app/browser/pluginnodes.h"
#include "ccl/app/browser/browser.h"

#include "ccl/app/controls/draghandler.h"
#include "ccl/app/controls/itemviewmodel.h"

#include "ccl/app/params.h"
#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/utilities/fileoperations.h"
#include "ccl/app/utilities/sortfolderlist.h"
#include "ccl/app/utilities/shellcommand.h"

#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/app/signals.h"

#include "ccl/base/signalsource.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/message.h"

#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/graphics/font.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/guiservices.h"

namespace CCL {
namespace Browsable {

//************************************************************************************************
// PresetNodeConstructor
//************************************************************************************************

class PresetNodeConstructor: public FileNodeConstructor
{
public:
	// FileNodeConstructor
	bool canCreateNode (UrlRef path) const override;
	BrowserNode* createNode (UrlRef path) const override;
};

} // namespace Browsable
} // namespace CCL

using namespace CCL;
using namespace Browsable;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (PresetNode, kFirstRun)
{
	FileNodeFactory::instance ().addConstructor (NEW PresetNodeConstructor);
	return true;
}

//************************************************************************************************
// PresetSearchProvider
//************************************************************************************************

DEFINE_CLASS (PresetSearchProvider, SearchProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetSearchProvider::PresetSearchProvider (StringRef category)
{
	startPoint.setProtocol (CCLSTR ("category"));
	startPoint.setPath (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetSearchProvider::PresetSearchProvider (UIDRef classId)
{
	startPoint.setProtocol (CCLSTR ("class"));
	String classIdString;
	UID (classId).toString (classIdString);
	startPoint.setPath (classIdString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* PresetSearchProvider::createSearcher (ISearchDescription& description)
{
	return System::GetPresetManager ().createSearcher (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* PresetSearchProvider::customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrlFilter* PresetSearchProvider::getSearchResultFilter () const
{
	return getUrlFilter ();
}

//************************************************************************************************
// PresetNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PresetNode, FileNode)

DEFINE_STRINGID_MEMBER_ (PresetNode, kFavorite, "favorite")
DEFINE_STRINGID_MEMBER_ (PresetNode, kEditSelection, "editSelection")

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetNode::PresetNode (IPreset* _preset, BrowserNode* parent, bool isSubPreset)
: FileNode (nullptr, parent),
  isSubPreset (isSubPreset)
{
	canRenameFile (false);
	canDeleteFile (false);

	if(_preset)
	{
		setPreset (_preset);

		AutoPtr<Url> path = NEW Url;
		preset->getUrl (*path);
		setPath (path);

		title = preset->getPresetName ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNode::isDefaultPreset () const
{
	return title == PresetFilePrimitives::kDefaultPresetFileName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNode::supportsFavorites () const
{
	auto containerNode = getAncestorNodeWithInterface<IPresetContainerNode> ();
	return containerNode ? containerNode->supportsFavorites () : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* PresetNode::getIcon ()
{
	if(!icon && getPath ())
	{
		if(isSubPreset) // different icon for subpresets inside a collection
		{
			MutableCString extension (getPath ()->getFileType ().getExtension ());
			extension.toLowercase ();
			MutableCString iconName ("PresetIcon:");
			iconName += extension;
			setIcon (getThemeIcon (iconName));
		}
		else
		{
			AutoPtr<IImage> fileIcon (FileIcons::instance ().createIcon (*getPath ()));
			setIcon (fileIcon);
		}
	}

	if(!icon)
		setIcon (getThemeIcon ("FileIcon:preset"));
	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID PresetNode::getCustomBackground () const
{
	return CSTR ("preset");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PresetNode::compare (const Object& obj) const
{
	if(ccl_cast<PresetFavoritesNode> (&obj))
		return NumericLimits::kMaxInt;

	// sort default preset before others
	if(isDefaultPreset ())
		return -1;

	if(const BrowserNode* node = ccl_cast<BrowserNode> (&obj))
	{
		if(const PresetNode* otherPreset = ccl_cast<PresetNode> (&obj))
		{
			if(otherPreset->isDefaultPreset ())
				return 1;
		}
		else if(ccl_cast<FolderNode> (&obj))
			return 1;	// presets after folders

		return compareTitle (*node);
	}
	return Object::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNode::drawDetail (const IItemModel::DrawInfo& info, StringID id, AlignmentRef alignment)
{
	if(id == nullptr && isDefaultPreset ())
	{
		Font font (info.style.font);
		font.isBold (true);
		info.graphics.drawString (info.rect, getTitle (), font, info.style.textBrush);
		return true;
	}
	else if(id == kFavorite)
	{
		bool isFavorite = preset && System::GetPresetManager ().isFavorite (*preset);
		if(isFavorite)
			if(IImage* icon = info.view->getVisualStyle ().getImage ("FavoriteIcon"))
				ItemModelPainter ().drawButtonImage (info, icon, true);
		
		return true;
	}
	else if(id == kEditSelection)
	{
		const IVisualStyle& vs = ViewBox (info.view).getVisualStyle ();
		if(IImage* icon = vs.getImage ("itemSelectIcon"))
		{
			IImage::Selector (icon, isChecked () ? ThemeNames::kNormalOn : ThemeNames::kNormal);
			Rect src (0, 0, icon->getWidth (), icon->getHeight ());
			Rect iconRect (src);
			iconRect.center (info.rect);
			info.graphics.drawImage (icon, src, iconRect);
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* PresetNode::createDragObject ()
{
	ASSERT (preset)
	if(preset)
	{
		AutoPtr<IUrl> mountedPath (path ? System::GetFileUtilities ().translatePathInMountedFolder (*path) : nullptr);
		if(mountedPath)
		{
			IPreset* mountedPreset = System::GetPresetManager ().openPreset (*mountedPath);
			if(mountedPreset)
				return mountedPreset;
		}

		preset->retain ();
	}
	return preset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNode::performRemoval (NodeRemover& remover)
{
	bool checkOnly = remover.isCheckOnly ();
	bool result = false;

	// collect preset nodes that can be removed
	ObjectList presetNodes;

	ForEach (remover, BrowserNode, node)
		if(PresetNode* presetNode = ccl_cast<PresetNode> (node))
			if(IPreset* preset = presetNode->getPreset ())
			{
				if(preset->isReadOnly ())
					continue;

				if(checkOnly)
					return true;
				else
					presetNodes.add (presetNode);
			}
	EndFor

	if(!checkOnly && !presetNodes.isEmpty ())
	{
		result = true;

		// ask user if presets should be removed...
		String presetNames;
		StringBuilder listWriter (presetNames);

		ForEach (presetNodes, PresetNode, presetNode)
			listWriter.addItem (presetNode->getTitle ());
			if(listWriter.isLimitReached ())
				break;
		EndFor

		if(PresetComponent::askRemovePreset (presetNodes.count () == 1, presetNames))
		{
			// remove them
			ForEach (presetNodes, PresetNode, presetNode)
				if(IPreset* preset = presetNode->getPreset ())
				{
					if(System::GetPresetManager ().removePreset (*preset))
						remover.removeNode (presetNode);
					else
						remover.keepNode (presetNode);
				}
			EndFor
		}
		else
		{
			// keep them
			ForEach (presetNodes, PresetNode, presetNode)
				remover.keepNode (presetNode);
			EndFor
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNode::onOpen (bool deferred)
{
	if(preset)
	{
		SignalSource (Signals::kPresetManager).signal (Message (Signals::kOpenPreset, (IPreset*)preset));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNode::isFolder () const
{
	return UnknownPtr<IPresetCollection> (preset).isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNode::hasSubNodes () const
{
	return UnknownPtr<IPresetCollection> (preset).isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNode::canAutoExpand () const
{
	// preset collections do not auto-expand
	return UnknownPtr<IPresetCollection> (preset).isValid () == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNode::getSubNodes (Container& children, NodeFlags flags)
{
	UnknownPtr<IPresetCollection> collection (preset);
	if(collection && flags.wantLeafs ())
	{
		int count = collection->countPresets ();
		for(int i = 0; i < count; i++)
		{
			AutoPtr<IPreset> preset = collection->openPreset (i);
			ASSERT (preset != nullptr)
			if(preset)
				children.add (NEW PresetNode (preset, this, true));
		}
		return true;
	}
	else
		return SuperClass::getSubNodes (children, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNode::onRefresh ()
{
	UnknownPtr<IPresetCollection> collection (preset);
	if(collection)
	{
		AutoPtr<IPreset> newPreset (System::GetPresetManager ().openPreset (*getPath ()));
		setPreset (newPreset);
	}

	return SuperClass::onRefresh ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PresetNode::appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes)
{
	if(supportsFavorites ())
		contextMenu.addCommandItem (BrowserStrings::strFavorite (), CSTR ("Browser"), CSTR ("Set Favorite"), nullptr);

	if(preset && !preset->isReadOnly ())
	{
		contextMenu.addCommandItem (PresetComponent::getRenamePresetTitle (), CSTR ("Presets"), CSTR ("Rename"), nullptr);
		contextMenu.addCommandItem (PresetComponent::getDeletePresetTitle (), CSTR ("Edit"), CSTR ("Delete"), nullptr);
	}
	contextMenu.addSeparatorItem ();
	#if 0 // superseeded by "Move to New Folder"
	contextMenu.addCommandItem (CommandWithTitle (CSTR ("Browser"), CSTR ("New Folder"), FileStrings::NewFolder ()), nullptr, true);
	#endif

	UnknownPtr<IMenu> menu (&contextMenu);
	if(menu)
	{
		// "Move to Folder" / "Move to New Folder"
		if(auto favoritesNode = getAncestorNode<PresetFavoritesNode> ())
			favoritesNode->appendMoveToFolderMenu (*menu, *this);
		else if(auto containerNode = getAncestorNodeWithInterface<IPresetContainerNode> ())
			containerNode->getPresetNodesBuilder ().appendMoveToFolderMenu (*menu, *containerNode, *this);
	}

	return SuperClass::appendContextMenu (contextMenu, selectedNodes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNode::interpretCommand (const CommandMsg& msg, const Container* selectedNodes)
{
	if((msg.category == "Presets" && msg.name == "Rename")
		|| (msg.category == "Browser" && msg.name == "Rename File"))
		return onRenamePreset (msg, selectedNodes);

	else if(msg.category == "Browser")
	{
		if(msg.name == "Set Favorite")
		{
			if(supportsFavorites ())
			{
				tbool isFavorite = preset && System::GetPresetManager ().isFavorite (*preset);
				if(msg.checkOnly ())
				{
					UnknownPtr<IMenuItem> menuItem (msg.invoker);
					if(menuItem)
						menuItem->setItemAttribute (IMenuItem::kItemChecked, isFavorite);
				}
				else
				{
					Browser::visitEditNodes<PresetNode> (this, selectedNodes, [&] (PresetNode& presetNode)
					{
						if(presetNode.getPreset ())
							System::GetPresetManager ().setFavorite (*presetNode.getPreset (), !isFavorite);
					});
				}
				return true;
			}
		}
		else if(msg.name == "New Folder")
		{
			// depending on the context (parent), we must create a favorites folder or sort folder for this preset
			if(auto favoritesNode = getAncestorNode<PresetFavoritesNode> ())
				return favoritesNode->onNewFolder (this, msg.checkOnly ());
			else if(auto containerNode = getAncestorNodeWithInterface<IPresetContainerNode> ())
				return containerNode->getPresetNodesBuilder ().onNewPresetFolder (*containerNode, this, msg.checkOnly ());
		}
	}
	return SuperClass::interpretCommand (msg, selectedNodes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNode::onRenamePreset (CommandMsg args, const Container* selectedNodes)
{
	if(!preset || preset->isReadOnly ())
		return false;

	if(!args.checkOnly ())
	{
		if(System::GetDesktop ().closePopupAndDeferCommand (this, args))
			return true;

		auto containerNode = unknown_cast<BrowserNode> (getAncestorNodeWithInterface<IPresetContainerNode> ());

		SharedPtr<PresetNode> holder (this); // node might be removed while updating browser

		PresetRenamer renamer (*preset);
		if(renamer.runDialog (PresetComponent::getRenamePresetTitle ()))
		{
			if(containerNode)
			{
				// find (new) preset node with new url (assuming it has already been added to the browser via kPresetCreated signal)
				// (this node (old preset) might have already been removed!)
				Url url;
				preset->getUrl (url);
				FileType fileType (url.getFileType ());

				url.setName (renamer.getNewName ());
				url.setFileType (fileType);

				if(PresetNode* newPresetNode = PresetNodeSorter::findPresetNode (*containerNode, url, preset->getMetaInfo ()))
					if(Browser* browser = newPresetNode->getBrowser ())
						browser->setFocusNode (newPresetNode);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNode::getSelectedPresets (UnknownList& presets, Browser* browser)
{
	if(browser)
	{
		ObjectList nodes;
		browser->getEditNodes (nodes, nullptr);

		for(auto n : nodes)
			if(auto presetNode = ccl_cast<PresetNode> (n))
				presets.add (presetNode->getPreset (), true);
	}
	return !presets.isEmpty ();
}

//************************************************************************************************
// PresetNodeConstructor
//************************************************************************************************

bool PresetNodeConstructor::canCreateNode (UrlRef path) const
{
	return System::GetPresetManager ().supportsFileType (path.getFileType ()) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* PresetNodeConstructor::createNode (UrlRef path) const
{
	AutoPtr<IPreset> preset = System::GetPresetManager ().openPreset (path);
	//ASSERT (preset != 0)
	if(preset)
		return NEW PresetNode (preset);
	else
		return nullptr;
}

//************************************************************************************************
// PresetNodeSorter
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PresetNodeSorter, NodeSorter)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNodeSorter::getSortPath (String& path, const BrowserNode* node)
{
	if(const PresetNode* presetNode = ccl_cast<PresetNode> (node))
	{
		// always sort presets by subfolder
		if(IPreset* preset = presetNode->getPreset ())
		{
			if(IAttributeList* metaInfo = preset->getMetaInfo ())
				path = PresetMetaAttributes (*metaInfo).getSubFolder ();
		
			// subfolder might need an additional prefix
			if(path.isEmpty ())
			{
				Url presetUrl;
				if(preset->getUrl (presetUrl))
					path = System::GetPresetFileRegistry ().getSubFolderPrefix (presetUrl);
			}
		}

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetNode* PresetNodeSorter::findPresetNode (BrowserNode& baseNode, UrlRef presetUrl, IAttributeList* metaInfo, bool createNodes)
{
	Browser* browser = baseNode.getBrowser ();
	if(!browser || presetUrl.isEmpty ())
		return nullptr;

	PresetNode* presetNode = nullptr;

	// make preset url relative to it's base folder
	Url relativePresetUrl (presetUrl);
	if(PresetFilePrimitives::makeRelativePresetUrl (relativePresetUrl, metaInfo))
	{
		// up to parent folder
		tbool result = relativePresetUrl.ascend ();
		ASSERT (result)

		// make browser path of plugin node as Url
		MutableCString baseNodePath;
		browser->makePath (baseNodePath, &baseNode);
		Url baseNodeUrl;
		baseNodeUrl.setPath (String (baseNodePath), IUrl::kFolder);

		// subfolder might need an additional prefix
		StringRef prefix = System::GetPresetFileRegistry ().getSubFolderPrefix (presetUrl);
		if(!prefix.isEmpty ())
			baseNodeUrl.descend (prefix, Url::kFolder);

		// make browser path of parent node of preset node
		Url parentNodeUrl (relativePresetUrl);
		parentNodeUrl.makeAbsolute (baseNodeUrl);

		ScopedVar<bool> scope (browser->getRestoringState (), true); // force getPresets immediately (not in background) to ensure finding the preset node

		// find parent node
		if(BrowserNode* parentNode = browser->findNode (MutableCString (parentNodeUrl.getPath (), Text::kUTF8), true))
		{
			if(FolderNode* parentFolderNode = ccl_cast<FolderNode> (parentNode))
			{
				if(createNodes)
					browser->createChildNodes (*parentFolderNode);

				// find the preset node with the given url
				ArrayForEach (parentFolderNode->getContent (), BrowserNode, node)
					if(PresetNode* candidate = ccl_cast<PresetNode> (node))
						if(const Url* path = candidate->getPath ())
						{
							// compare url without parameters (for subPrestes, handled below)
							if(path->getPath () == presetUrl.getPath ()
								&& path->getHostName () == presetUrl.getHostName ()
								&& path->getProtocol () == presetUrl.getProtocol ())
							{
								presetNode = candidate;
								break;
							}
						}
				EndFor
			}
			else
			{
				ASSERT (0) // todo
			}

			if(presetNode)
			{
				// check if it's a preset collection, and the url has a preset index parameter
				int presetIndex = PresetUrl::getSubPresetIndex (presetUrl);
				if(presetIndex >= 0)
				{
					UnknownPtr<IPresetCollection> collection (presetNode->getPreset ());
					if(collection)
					{
						AutoPtr<IPreset> subPreset = collection->openPreset ((int)presetIndex);
						if(subPreset)
						{
							// find sub preset node
							MutableCString nodePath;
							browser->makePath (nodePath, presetNode);
							nodePath.append ("/");
							nodePath.append (subPreset->getPresetName (), Text::kUTF8);
							if(PresetNode* subPresetNode = ccl_cast<PresetNode> (browser->findNode (nodePath, true)))
								presetNode = subPresetNode;
						}
					}
				}
			}
		}
	}
	return presetNode;
}

//************************************************************************************************
// Browsable::PresetSortFolderNode::FolderRenamer
//************************************************************************************************

class PresetSortFolderNode::FolderRenamer: public SortFolderRenamerBase
{
public:
	FolderRenamer (PresetSortFolderNode& node)
	: SortFolderRenamerBase (node)
	{
		if(auto containerNode = node.getAncestorNodeWithInterface<IPresetContainerNode> ())
			metaInfo = containerNode->getPresetMetaInfo ();
		ASSERT (metaInfo)
	}

	// SortFolderRenamerBase
	bool renameFolderInternal (String oldPath, StringRef newName) const override
	{
		if(!metaInfo)
			return false;

		System::GetPresetManager ().renameSortFolder (*metaInfo, oldPath, newName);
		return true;
	}

	bool hasSortFolderInternal (StringRef newPath) const override
	{
		return metaInfo && System::GetPresetManager ().hasSortFolder (*metaInfo, newPath) != 0;
	}

private:
	SharedPtr<IAttributeList> metaInfo;
};

//************************************************************************************************
// PresetSortFolderNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PresetSortFolderNode, CustomSortFolderNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetSortFolderNode::PresetSortFolderNode (StringRef title)
: CustomSortFolderNode (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PresetSortFolderNode::compare (const Object& obj) const
{
	if(ccl_cast<PresetFavoritesNode> (&obj))
		return NumericLimits::kMaxInt;

	if(auto presetNode = ccl_cast<PresetNode> (&obj))
		return -presetNode->compare (*this);

	return SuperClass::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* PresetSortFolderNode::getPresetMetaInfo () const
{
	auto containerNode = getAncestorNodeWithInterface<IPresetContainerNode> ();
	return containerNode ? containerNode->getPresetMetaInfo () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetSortFolderNode::determineFileSystemUrl (Url& folder) const
{
	// check for a child file node
	for(auto node : getContent ())
		if(auto fileNode = ccl_cast<FileNode> (node))
			if(const Url* filePath = fileNode->getPath ())
			{
				folder = *filePath;
				folder.ascend ();
				return true;
			}

	// check for a child sort folder node (recursion)
	for(auto node : getContent ())
		if(auto sortFolderNode = ccl_cast<PresetSortFolderNode> (node))
			if(sortFolderNode->determineFileSystemUrl (folder))
			{
				folder.ascend ();
				return true;
			}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetSortFolderNode::hasWritablePreset () const
{
	// check for a child preset nodes
	for(auto node : getContent ())
		if(auto presetNode = ccl_cast<PresetNode> (node))
			if(IPreset* preset = presetNode->getPreset ())
				if(!preset->isReadOnly ())
					return true;

	// check for a child sort folder node (recursion)
	for(auto node : getContent ())
		if(auto sortFolderNode = ccl_cast<PresetSortFolderNode> (node))
			if(sortFolderNode->hasWritablePreset ())
				return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Renamer* PresetSortFolderNode::createFolderRenamer ()
{
	return NEW FolderRenamer (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetSortFolderNode::createNewFolder (bool checkOnly)
{
	auto containerNode = getAncestorNodeWithInterface<IPresetContainerNode> ();
	return containerNode ? containerNode->getPresetNodesBuilder ().onNewPresetFolder (*containerNode, this, checkOnly) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetSortFolderNode::removeFolders (NodeRemover& remover, Container& folderNodes)
{
	if(IAttributeList* metaInfo = getPresetMetaInfo ())
 	{
		for(auto obj : folderNodes)
			if(auto node = ccl_cast<CustomSortFolderNode> (obj))
			{
				String path;
				node->getSortPath (path);
				System::GetPresetManager ().removeSortFolder (*metaInfo, path);
					
				remover.removeNode (node);
			}

		return true;
	}	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetSortFolderNode::sortNodesIntoFolder (const IUnknownList& items, StringRef sortPath, IPresetContainerNode* containerNode)
{
	String firstNewPath;
	SharedPtr<PresetSortFolderNode> firstFolderNode;
	Browser::ExpandState expandState;
	Browser* browser = nullptr;

	ForEachUnknown (items, unk)
		UnknownPtr<IPreset> preset (unk);
		if(preset)
		{
			System::GetPresetManager ().movePreset (*preset, sortPath);
			if(firstNewPath.isEmpty ())
			{
				if(!sortPath.isEmpty ())
					firstNewPath << sortPath << Url::strPathChar;
				firstNewPath << preset->getPresetName ();
			}
		}
		else if(auto folderNode = unknown_cast<PresetSortFolderNode> (unk))
		{
			String oldPath, newPath;
			if(folderNode->prepareMoveIntoFolder (oldPath, newPath, sortPath))
				if(IAttributeList* metaInfo = folderNode->getPresetMetaInfo ())
				{
					if(!firstFolderNode)
					{
						firstFolderNode = folderNode;
						if(browser = folderNode->getBrowser ())
							expandState.store (*browser, *firstFolderNode);
					}

					System::GetPresetManager ().moveSortFolder (*metaInfo, oldPath, newPath);

					if(firstNewPath.isEmpty ())
						firstNewPath = newPath;
				}
		}
	EndFor

	// focus first moved preset / folder
	if(!firstNewPath.isEmpty () && containerNode)
	{
		if(auto baseNode = unknown_cast<BrowserNode> (containerNode))
		{
			BrowserNode* newFolderNode = CustomSortFolderNode::setFocusNode (*baseNode, firstNewPath);

			if(firstFolderNode && newFolderNode && browser)
				expandState.restore (*browser, *newFolderNode);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PresetSortFolderNode::appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes)
{
	SuperClass::appendContextMenu (contextMenu, selectedNodes);
	contextMenu.addSeparatorItem ();
	contextMenu.addCommandItem (ShellCommand::getShowFileInSystemTitle (), CSTR ("Browser"), CSTR ("Show in Explorer/Finder"), this);

	return kResultFalse; // (continue)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetSortFolderNode::interpretCommand (const CommandMsg& msg, const Container* selectedNodes)
{
	if(msg.category == "Browser" && msg.name == "Show in Explorer/Finder")
	{
		Url url;
		if(determineFileSystemUrl (url))
			return ShellCommand::showFileInSystem (url, msg.checkOnly ());

		return false;
	}
	return SuperClass::interpretCommand (msg, selectedNodes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetSortFolderNode::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	String sortPath;
	getSortPath (sortPath);
	return sortNodesIntoFolder (data, sortPath, getAncestorNodeWithInterface<IPresetContainerNode> ());
}

//************************************************************************************************
// PresetFavoritesNode::Sorter
//************************************************************************************************

class PresetFavoritesNode::Sorter: public NodeSorter
{
public:
	// NodeSorter
	bool getSortPath (String& path, const BrowserNode* node) override;
};

//************************************************************************************************
// Browsable::PresetFavoritesNode::Sorter
//************************************************************************************************

bool PresetFavoritesNode::Sorter::getSortPath (String& path, const BrowserNode* node)
{
	auto presetNode = ccl_cast<PresetNode> (node);
	if(IPreset* preset = presetNode ? presetNode->getPreset () : nullptr)
	{
		path = System::GetPresetManager ().getFavoriteFolder (*preset);
		return true;
	}
	return false;
}

//************************************************************************************************
// PresetFavoritesNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PresetFavoritesNode, SortedNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetFavoritesNode::PresetFavoritesNode (IAttributeList* metaInfo)
: SortedNode (BrowserStrings::strFavorites ()),
  metaInfo (metaInfo)
{
	setIcon (RootComponent::instance ().getTheme ()->getImage ("FolderIcon:FavoritesFolder"));
	setSorter (AutoPtr<Sorter> (NEW Sorter));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PresetFavoritesNode::compare (const Object& obj) const
{
	// sort before folders & presets
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetFavoritesNode::build ()
{
	if(metaInfo)
	{
		// favorite presets
		IterForEachUnknown (System::GetPresetManager ().getFavoritePresets (*metaInfo), unk)
			UnknownPtr<IPreset> preset (unk);
			if(preset)
				addSorted (NEW PresetNode (preset));
		EndFor

		// get additional (empty) folders
		AutoPtr<IUnknownIterator> iterator (System::GetPresetManager ().getFavoriteFolders (*metaInfo));
		if(iterator)
			addSubFolders (*iterator);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SortFolderNode* PresetFavoritesNode::newFolder (StringRef title)
{
	return NEW PresetFavoritesSortFolderNode (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFavoritesNode::canRemoveParentFolder (FolderNode* parentFolder) const
{
	auto sortFolder = ccl_cast<PresetFavoritesSortFolderNode> (parentFolder);
	return metaInfo && sortFolder ? System::GetPresetManager ().hasFavoriteFolder (*metaInfo, sortFolder->getSortPath ()) : true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFavoritesNode::onNewFolder (BrowserNode* focusNode, bool checkOnly)
{
	if(metaInfo)
	{
		if(!checkOnly)
			createNewFolder (focusNode);

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetFavoritesNode::createNewFolder (BrowserNode* focusNode)
{
	String newPath;
	if(metaInfo)
		if(CustomSortFolderNode::askNewFolder (newPath, focusNode, ccl_typeid<PresetFavoritesSortFolderNode> ()))
			System::GetPresetManager ().addFavoriteFolder (*metaInfo, newPath);

	return newPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFavoritesNode::onMoveToFolder (CmdArgs args, VariantRef data)
{
	if(auto node = unknown_cast<BrowserNode> (data))
	{
		Browser* browser = node->getBrowser ();
		UnknownPtr<IDataTarget> dataTarget (node->asUnknown ());
		if(browser && dataTarget)
		{
			if(!args.checkOnly ())
			{
				UnknownList presets;
				if(PresetNode::getSelectedPresets (presets, browser))
					dataTarget->insertData (presets, nullptr); // handled by PresetFavoritesNode, PresetFavoritesSortFolderNode
			}
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFavoritesNode::onMoveToNewFolder (CmdArgs args, VariantRef data)
{
	if(!args.checkOnly ())
	{
		if(auto focusNode = unknown_cast<BrowserNode> (data))
		{
			IPresetContainerNode* containerNode = UnknownPtr<IPresetContainerNode> (focusNode->asUnknown ());
			if(!containerNode)
				containerNode = focusNode->getAncestorNodeWithInterface<IPresetContainerNode> ();
			if(containerNode)
			{
				UnknownList presets;
				if(PresetNode::getSelectedPresets (presets, focusNode->getBrowser ()))
				{

					String newFolder = createNewFolder (focusNode);
					if(!newFolder.isEmpty ())
						sortNodesIntoFolder (presets, nullptr, newFolder);
				}
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool PresetFavoritesNode::sortNodesIntoFolder (const IUnknownList& data, IDragSession* session, StringRef sortPath)
{
	if(metaInfo)
	{
		ForEachUnknown (data, unk)
			UnknownPtr<IPreset> preset (unk);
			if(preset)
				System::GetPresetManager ().setFavorite (*preset, true, sortPath);

			else if(auto folderNode = unknown_cast<PresetFavoritesSortFolderNode> (unk))
			{
				String oldPath, newPath;
				if(folderNode->prepareMoveIntoFolder (oldPath, newPath, sortPath))
					System::GetPresetManager ().moveFavoriteFolder (*metaInfo, oldPath, newPath);
			}
		EndFor
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetFavoritesNode::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	// TODO
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetFavoritesNode::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	return sortNodesIntoFolder (data, session, String::kEmpty);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PresetFavoritesNode::appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes)
{
	contextMenu.addCommandItem (CommandWithTitle (CSTR ("Browser"), CSTR ("New Folder"), FileStrings::NewFolder ()), nullptr, true);
	return kResultFalse; // (continue)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetFavoritesNode::appendMoveToFolderMenu (IMenu& menu, PresetNode& presetNode)
{
	struct MenuBuilder: public MoveToFolderMenuBuilder
	{
		PresetFavoritesNode& favoritesNode;

		MenuBuilder (PresetFavoritesNode& favoritesNode, BrowserNode* nodeToMove)
		: MoveToFolderMenuBuilder (nodeToMove),
			favoritesNode (favoritesNode)
		{}

		// MoveToFolderMenuBuilder
		ICommandHandler* createCommandHandler (FolderNode& targetFolderNode) override
		{
			return makeCommandDelegate (&favoritesNode, &PresetFavoritesNode::onMoveToFolder, Variant (targetFolderNode.asUnknown (), true)).detach ();
		}
	};

	menu.addCommandItem (CommandWithTitle (CSTR ("File"), CSTR ("New Folder"), FileStrings::MoveToNewFolder ()),
		makeCommandDelegate (this, &PresetFavoritesNode::onMoveToNewFolder, Variant (this->asUnknown (), true)), true);

	MenuBuilder (*this, &presetNode).appendSubMenu (menu, *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFavoritesNode::interpretCommand (const CommandMsg& msg, const Container* selectedNodes)
{
	if(msg.category == "Browser" && msg.name == "New Folder")
		return onNewFolder (this, msg.checkOnly ());

	return false;
}

//************************************************************************************************
// Browsable::PresetFavoritesSortFolderNode::FolderRenamer
//************************************************************************************************

class PresetFavoritesSortFolderNode::FolderRenamer: public SortFolderRenamerBase
{
public:
	FolderRenamer (PresetFavoritesSortFolderNode& node)
	: SortFolderRenamerBase (node)
	{
		if(auto favoritesNode = node.getAncestorNode<PresetFavoritesNode> ())
			metaInfo = ccl_const_cast (favoritesNode->getMetaInfo ());
		ASSERT (metaInfo)
	}

	// SortFolderRenamerBase
	bool renameFolderInternal (String oldPath, StringRef newName) const override
	{
		if(!metaInfo)
			return false;

		System::GetPresetManager ().renameFavoriteFolder (*metaInfo, oldPath, newName);
		return true;
	}

	bool hasSortFolderInternal (StringRef newPath) const override
	{
		return metaInfo && System::GetPresetManager ().hasFavoriteFolder (*metaInfo, newPath) != 0;
	}

private:
	SharedPtr<IAttributeList> metaInfo;
};

//************************************************************************************************
// PresetFavoritesSortFolderNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PresetFavoritesSortFolderNode, CustomSortFolderNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetFavoritesSortFolderNode::PresetFavoritesSortFolderNode (StringRef title)
: CustomSortFolderNode (title)
{
	if(IImage* icon = RootComponent::instance ().getTheme ()->getImage ("FolderIcon:FavoritesSortFolder"))
		setIcon (icon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Renamer* PresetFavoritesSortFolderNode::createFolderRenamer ()
{
	return NEW FolderRenamer (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFavoritesSortFolderNode::createNewFolder (bool checkOnly)
{
	auto favoritesNode = getAncestorNode<PresetFavoritesNode> ();
	return favoritesNode ? favoritesNode->onNewFolder (this, checkOnly) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFavoritesSortFolderNode::removeFolders (NodeRemover& remover, Container& folderNodes)
{
	auto favoritesNode = getAncestorNode<PresetFavoritesNode> ();
	if(favoritesNode && favoritesNode->getMetaInfo ())
	{
		for(auto obj : folderNodes)
			if(auto node = ccl_cast<PresetFavoritesSortFolderNode> (obj))
			{
				String path;
				node->getSortPath (path);
				System::GetPresetManager ().removeFavoriteFolder (*favoritesNode->getMetaInfo (), path);
					
				remover.keepNode (node); // (will be removed from browser via PresetMananger signal)
			}

		return true;
	}	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetFavoritesSortFolderNode::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	if(auto favoritesNode = getAncestorNode<PresetFavoritesNode> ())
	{
		String sortPath;
		getSortPath (sortPath);
		return favoritesNode->sortNodesIntoFolder (data, session, sortPath);
	}
	return false;
}

//************************************************************************************************
// PresetNodesBuilder
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (PresetNodesBuilder, kPresetsChanged, "presetsChanged")

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetNodesBuilder::PresetNodesBuilder (IAttributeList* metaInfo)
: metaInfo (metaInfo),
  forceAlways (false),
  presetsPending (false),
  resetSuspended (false),
  flags (0)
{
	metaInfo->retain ();

	// also query for presets of alternative class
	PresetMetaAttributes metaAttributes (*metaInfo);
	UID classID;
	if(metaAttributes.getClassID (classID))
		if(const IClassDescription* alternativeClass = System::GetPlugInManager ().getAlternativeClass (classID))
			metaAttributes.setAlternativeClassID (alternativeClass->getClassID ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetNodesBuilder::PresetNodesBuilder (const IClassDescription& description)
: metaInfo (NEW Attributes),
  forceAlways (false),
  presetsPending (false),
  resetSuspended (false),
  flags (0)
{
	PresetMetaAttributes metaAttributes (*metaInfo);
	metaAttributes.assign (description);

	// also query for presets of alternative class
	if(const IClassDescription* alternativeClass = System::GetPlugInManager ().getAlternativeClass (description.getClassID ()))
		metaAttributes.setAlternativeClassID (alternativeClass->getClassID ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetNodesBuilder::~PresetNodesBuilder ()
{
	cancelPresets (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetNodesBuilder::getClassKey () const
{
	return metaInfo ? PresetMetaAttributes (*metaInfo).getClassKey () : String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNodesBuilder::shouldForcePresets (const BrowserNode& node) const
{
	if(forceAlways)
		return true;

	bool restoring = false;
	if(Browser* browser = node.getBrowser ())
		restoring = browser->isRestoringState ();
	return restoring;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetNodesBuilder::cancelPresets (bool destructing)
{
	bool needsChanged = false;
	if(presetsPending)
	{
		System::GetPresetManager ().cancelGetPresets (this);
		presetsPending = false;
		needsChanged = true;
	}

	cancelSignals ();

	if(needsChanged && !destructing)
		deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNodesBuilder::hasPresets (const BrowserNode& node) const
{
	if(!metaInfo)
		return false;
	if(presets)
		return !presets->isEmpty ();

	bool force = shouldForcePresets (node);
	if(!force)
	{
		int result = System::GetPresetManager().hasPresets (metaInfo);
		if(result >= 0)
			return result > 0;
	}
	const_cast<PresetNodesBuilder*> (this)->getPresets (force);
	return presets && !presets->isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetNodesBuilder::getPresets (bool force)
{
	if(!presets && metaInfo)
	{
		if(force)
		{
			cancelPresets ();
			presets = System::GetPresetManager ().getPresets (metaInfo);
			filterPresets ();
		}
		else
		{
			if(!presetsPending)
			{
				presetsPending = true;
				System::GetPresetManager ().getPresetsInBackground (this, metaInfo);
				deferChanged ();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetNodesBuilder::resetPresets ()
{
	if(resetSuspended)
		return;

	cancelPresets ();
	presets.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetNodesBuilder::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kGetPresetsCompleted)
	{
		UnknownPtr<IUnknownList> list (msg[0]);
		ASSERT (list != nullptr)

		ASSERT (presets == nullptr)
		presets.share (list);
		presetsPending = false;
		filterPresets ();

		ScopedVar<bool> scope (resetSuspended, true);
		signal (Message (kPresetsChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetNodesBuilder::buildNodes (SortedNode& parentFolder)
{
	bool force = shouldForcePresets (parentFolder);
	getPresets (force);

	if(hasFavoritesFolder () && System::GetPresetManager ().hasFavoriteFolder (*metaInfo, String::kEmpty))
		parentFolder.addSorted (NEW PresetFavoritesNode (metaInfo));

	if(presets)
	{
		ForEachUnknown (*presets, p)
			UnknownPtr<IPreset> preset (p);
			parentFolder.addSorted (NEW PresetNode (preset));
		EndFor
	}

	// get additional (empty) folders
	if(metaInfo)
	{
		AutoPtr<IUnknownIterator> iterator (System::GetPresetManager ().getSortFolders (*metaInfo));
		if(iterator)
			parentFolder.addSubFolders (*iterator);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetNodesBuilder::drawPresetsPending (const IItemModel::DrawInfo& info)
{
	static SharedPtr<IImage> pendingIcon;

	static bool iconChecked = false;
	if(!iconChecked)
	{
		iconChecked = true;
		if(ITheme* theme = RootComponent::instance ().getTheme ())
			pendingIcon = theme->getImage ("OverlayIcon:PresetPending");
	}

	if(pendingIcon)
		info.graphics.drawImage (pendingIcon, info.rect.getLeftTop ());
	else
	{
		SolidBrush brush (Color (Colors::kBlue).setAlphaF (.1f));
		info.graphics.fillRect (info.rect, brush);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetNode* PresetNodesBuilder::findPresetNode (IPreset& preset, SortedNode& parentFolder)
{
	Url url;
	preset.getUrl (url);

	if(ccl_cast<PresetNodeSorter> (parentFolder.getSorter ()))
		return PresetNodeSorter::findPresetNode (parentFolder, url, preset.getMetaInfo ());

	return findPresetNodeDeep (url, parentFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetNode* PresetNodesBuilder::findPresetNodeDeep (UrlRef presetUrl, FolderNode& parentFolder)
{
	ForEach (parentFolder.getContent (), BrowserNode, node)
		if(PresetNode* presetNode = ccl_cast<PresetNode> (node))
		{
			const Url* nodeUrl = presetNode->getPath ();
			if(nodeUrl && *nodeUrl == presetUrl)
				return presetNode;
		}
		else if(FolderNode* folder = ccl_cast<FolderNode> (node))
		{
			// recursion
			if(PresetNode* presetNode = findPresetNodeDeep (presetUrl, *folder))
				return presetNode;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetNodesBuilder::getPresetsHiddenBy (IUnknownList& hiddenPresets, IPreset& preset)
{
	if(IAttributeList* metaInfo = preset.getMetaInfo ())
	{
		AutoPtr<IUnknownList> presets (System::GetPresetManager ().getPresets (metaInfo));

		StringRef name = preset.getPresetName ();
		String subFolder = PresetMetaAttributes (*metaInfo).getSubFolder ();
		Url url;
		preset.getUrl (url);

		ForEachUnknown (*presets, unk)
			UnknownPtr<IPreset> p (unk);
			if(p->getPresetName () == name)
				if(IAttributeList* mi = p->getMetaInfo ())
					if(PresetMetaAttributes (*mi).getSubFolder () == subFolder)
					{
						Url u;
						p->getUrl (u);
						if(u != url && u.getFileType () == url.getFileType ())
							hiddenPresets.add (p, true);
					}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNodesBuilder::isFiltered (IPreset& preset) const
{
	if(subCategoryFilter.isEmpty ())
		return false;

	if(auto presetMetaInfo = preset.getMetaInfo ())
	{
		String presetSubCategories = PresetMetaAttributes (*presetMetaInfo).getSubCategory ();			 
		ForEachStringToken (subCategoryFilter, CCLSTR (" "), subCategory)
			if(presetSubCategories.contains (subCategory, false))
				return false;
		EndFor
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetNodesBuilder::filterPresets ()
{
	if(subCategoryFilter.isEmpty () == false && presets != nullptr)
	{
		Vector<IUnknown*> toRemove;
		ForEachUnknown (*presets, p)
			UnknownPtr<IPreset> preset (p);
			if(isFiltered (*preset))
				toRemove.add (p);
		EndFor
		for(IUnknown* p : toRemove)
		{
			presets->remove (p);
			p->release ();
		}
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetNodesBuilder::checkAddPreset (IPreset& preset)
{
	if(!presetsPending)
	{
		getPresets (true);
		ASSERT (presets)
		if(!presets)
			return;

		Url presetUrl;
		preset.getUrl (presetUrl);

		ForEachUnknown (*presets, unk)
			UnknownPtr<IPreset> p (unk);
			if(p == &preset)
				return; // same preset object exists

			Url url;
			if(p->getUrl (url) && url == presetUrl)
				return; // other with same url exists
		EndFor
		
		if(isFiltered (preset) == false)
			presets->add (&preset, true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNodesBuilder::hasSortFolder (StringRef sortPath) const
{
	return metaInfo && System::GetPresetManager ().hasSortFolder (*metaInfo, sortPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetNodesBuilder::createNewPresetFolder (IPresetContainerNode& containerNode, BrowserNode* focusNode)
{
	String newPath;
	if(metaInfo && CustomSortFolderNode::askNewFolder (newPath, focusNode, ccl_typeid<PresetSortFolderNode> ()))
	{
		System::GetPresetManager ().addSortFolder (*metaInfo, newPath);

		if(auto baseNode = unknown_cast<BrowserNode> (&containerNode))
			CustomSortFolderNode::setFocusNode (*baseNode, newPath);
	}
	return newPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNodesBuilder::onNewPresetFolder (IPresetContainerNode& containerNode, BrowserNode* focusNode, bool checkOnly)
{
	if(!metaInfo)
		return false;

	if(!checkOnly)
		createNewPresetFolder (containerNode, focusNode);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNodesBuilder::onMoveToFolder (CmdArgs args, VariantRef data)
{
	if(auto node = unknown_cast<BrowserNode> (data))
	{
		Browser* browser = node->getBrowser ();
		UnknownPtr<IDataTarget> dataTarget (node->asUnknown ());
		if(browser && dataTarget)
		{
			if(!args.checkOnly ())
			{
				UnknownList presets;
				if(PresetNode::getSelectedPresets (presets, browser))
					dataTarget->insertData (presets, nullptr); // e.g. handled by PresetSortFolderNode, PresetContainerNode, ..
			}
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetNodesBuilder::onMoveToNewFolder (CmdArgs args, VariantRef data)
{
	if(!args.checkOnly ())
	{
		if(auto focusNode = unknown_cast<BrowserNode> (data))
		{
			IPresetContainerNode* containerNode = UnknownPtr<IPresetContainerNode> (focusNode->asUnknown ());
			if(!containerNode)
				containerNode = focusNode->getAncestorNodeWithInterface<IPresetContainerNode> ();
			if(containerNode)
			{
				UnknownList presets;
				if(PresetNode::getSelectedPresets (presets, focusNode->getBrowser ()))
				{
					String newFolder = createNewPresetFolder (*containerNode, focusNode);
					if(!newFolder.isEmpty ())
						PresetSortFolderNode::sortNodesIntoFolder (presets, newFolder, containerNode);
				}
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetNodesBuilder::appendMoveToFolderMenu (IMenu& menu, IPresetContainerNode& containerNode, PresetNode& presetNode)
{
	struct MenuBuilder: public MoveToFolderMenuBuilder
	{
		PresetNodesBuilder& builder;

		MenuBuilder (PresetNodesBuilder& builder, BrowserNode* nodeToMove)
		: MoveToFolderMenuBuilder (nodeToMove),
		  builder (builder)
		{}

		// MoveToFolderMenuBuilder
		bool handlesFolder (FolderNode* folderNode) const override
		{
			// e.g. exclude favorite folders
			return ccl_cast<PresetSortFolderNode> (folderNode) || UnknownPtr<IPresetContainerNode> (ccl_as_unknown (folderNode)).isValid ();
		}

		ICommandHandler* createCommandHandler (FolderNode& targetFolderNode) override
		{
			return makeCommandDelegate (&builder, &PresetNodesBuilder::onMoveToFolder, Variant (targetFolderNode.asUnknown (), true)).detach ();
		}
	};

	if(auto baseNode = unknown_cast<SortedNode> (&containerNode))
	{
		menu.addCommandItem (CommandWithTitle (CSTR ("File"), CSTR ("New Folder"), FileStrings::MoveToNewFolder ()),
			makeCommandDelegate (this, &PresetNodesBuilder::onMoveToNewFolder, Variant (presetNode.asUnknown (), true)), true);

		MenuBuilder (*this, &presetNode).appendSubMenu (menu, *baseNode);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetNodesBuilder::onPresetCreated (IPreset& preset, SortedNode& parentFolder)
{
	if(isFiltered (preset))
		return;

	PresetNode* presetNode = findPresetNode (preset, parentFolder);
	if(presetNode)
	{
		// already exists: refresh (content might have changed)
		if(Browser* browser = parentFolder.getBrowser ())
			browser->refreshNode (presetNode);
	}
	else
	{
		// check if preset must be added to our list of IPresets
		checkAddPreset (preset);

		presetNode = NEW PresetNode (&preset);
		if(SortedNode::insertNode (&parentFolder, presetNode, parentFolder.getBrowser ()))
		{
			// remove another node with the same name in the same folder
			if(SortedNode* parent = ccl_cast<SortedNode> (presetNode->getParent ()))
			{
				const FileType& fileType = presetNode->getFilePath ().getFileType ();
				auto isSameFileType = [&] (const BrowserNode* node)
				{
					auto fileNode = ccl_cast<FileNode> (node);
					return fileNode && fileNode->getFilePath ().getFileType () == fileType;
				};

				ForEach (parent->getContent (), BrowserNode, node)
					if(node != presetNode && node->getTitle () == preset.getPresetName () && isSameFileType (node))
					{
						SortedNode::removeNode (&parentFolder, node, parentFolder.getBrowser ());
						break;
					}
				EndFor
			}
		}

		// redraw parent node (expand sign might appear)
		if(Browser* browser = parentFolder.getBrowser ())
			browser->redrawNode (&parentFolder);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetNodesBuilder::onPresetRemoved (IPreset& preset, SortedNode& parentFolder)
{
	if(PresetNode* presetNode = findPresetNode (preset, parentFolder))
	{
		SortedNode::removeNode (&parentFolder, presetNode, parentFolder.getBrowser ());

		// check if another preset gets unhidden now
		UnknownList hiddenPresets;
		getPresetsHiddenBy (hiddenPresets, preset);

		ForEachUnknown (hiddenPresets, unk)
			UnknownPtr<IPreset> p (unk);
			PresetNode* node = NEW PresetNode (p);
			SortedNode::insertNode (&parentFolder, node, parentFolder.getBrowser ());
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetNodesBuilder::onPresetSubFoldersChanged (MessageRef msg, SortedNode& baseNode)
{
	String classKey (msg[0].asString ());
	String path (msg[1].asString ());

	if(getMetaInfo () && classKey == PresetMetaAttributes (*getMetaInfo ()).getClassKey ())
		if(Browser* browser = baseNode.getBrowser ())
		{
			auto folderNode = ccl_cast<PresetSortFolderNode> (baseNode.findSortFolderNode (path));

			if(msg == Signals::kPresetSubFolderAdded)
			{
				if(folderNode)
				{
					// already exists: refresh (content might have changed)
					browser->refreshNode (folderNode);
				}
				else
				{
					// add folder, insert into Browser
					if(FolderNode* newFolder = baseNode.addSubFolders (path))
						if(auto parentFolder = ccl_cast<FolderNode> (newFolder->getParent ()))
						{
							int index = parentFolder->getNodeIndex (newFolder);
							browser->insertNode (parentFolder, return_shared (newFolder), index);
						}
				}
			}
			else if(msg == Signals::kPresetSubFolderRemoved)
			{
				if(folderNode)
					SortedNode::removeNode (&baseNode, folderNode, browser);
			}
			else
			{
				ASSERT (0)
				browser->refreshNode (&baseNode, true);
			}
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetNodesBuilder::onPresetFavoritesChanged (StringRef classKey, SortedNode& baseNode, StringRef folderPath)
{
	if(hasFavoritesFolder () && getMetaInfo () && classKey == PresetMetaAttributes (*getMetaInfo ()).getClassKey ())
	{
		if(Browser* browser = baseNode.getBrowser ())
		{
			bool hasFavorites = System::GetPresetManager ().hasFavoriteFolder (*metaInfo, String::kEmpty);
			auto favoritesNode = baseNode.findNode<PresetFavoritesNode> (AutoPtr<IRecognizer> (Recognizer::create ([] (IUnknown* obj) { return unknown_cast<PresetFavoritesNode> (obj) != nullptr; })));

			if(hasFavorites != (favoritesNode != nullptr))
			{
				// add or remove favorites folder
				if(favoritesNode)
				{
					SortedNode::removeNode (&baseNode, favoritesNode, browser);
				}
				else
				{
					favoritesNode = NEW PresetFavoritesNode (metaInfo);
					SortedNode::insertNode (&baseNode, favoritesNode, browser);
				}
			}
			else if(favoritesNode)
			{
				browser->refreshNode (favoritesNode, true);

				if(!folderPath.isEmpty ())
				{
					// select folder of interest
					MutableCString path (browser->makePath (favoritesNode));
					(path += Url::strPathChar) += folderPath;

					if(BrowserNode* node = browser->findNode (path, true, true))
						browser->setFocusNode (node);
				}
			}
		}
	}
}

//************************************************************************************************
// PresetContainerNode::Recognizer
//************************************************************************************************

class PresetContainerNode::Recognizer: public CCL::Recognizer
{
public:
	Recognizer (IAttributeList& metaInfo) : metaInfo (&metaInfo), metaAttributes (metaInfo) {}

	tbool CCL_API recognize (IUnknown* object) const override
	{
		PresetContainerNode* containerNode = unknown_cast<PresetContainerNode> (object);
		return containerNode && containerNode->handlesPreset (metaAttributes);
	}
private:
	SharedPtr<IAttributeList> metaInfo;
	PresetMetaAttributes metaAttributes;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

IRecognizer* PresetContainerNode::createRecognizer (IAttributeList& metaInfo)
{
	return NEW PresetContainerNode::Recognizer (metaInfo);
}

//************************************************************************************************
// PresetContainerNode
//************************************************************************************************

DEFINE_IID_ (IPresetContainerNode, 0xd9c44a17, 0x5cd, 0x44fc, 0x94, 0x95, 0x6e, 0xc5, 0xa4, 0x79, 0xe2, 0xc6)

DEFINE_CLASS_ABSTRACT_HIDDEN (PresetContainerNode, SortedNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetContainerNode::PresetContainerNode (IAttributeList* metaInfo, StringRef title, BrowserNode* parent)
: SortedNode (title, parent),
  builder (metaInfo)
{
	AutoPtr<Browsable::PresetNodeSorter> sorter (NEW Browsable::PresetNodeSorter);
	setSorter (sorter);

	SignalSource::addObserver (Signals::kPresetManager, this);
	builder.addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetContainerNode::~PresetContainerNode ()
{
	builder.removeObserver (this);
	SignalSource::removeObserver (Signals::kPresetManager, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetContainerNode::handlesPreset (const PresetMetaAttributes& presetAttribs) const
{
	// this default criterion (preset category) can be overridden
	PresetMetaAttributes metaAttribs (*builder.getMetaInfo ());
	return metaAttribs.getCategory () == presetAttribs.getCategory ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetContainerNode::hasFavoritesFolder (bool state)
{
	builder.hasFavoritesFolder (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetContainerNode::supportsFavorites () const
{
	return builder.hasFavoritesFolder ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* PresetContainerNode::getPresetMetaInfo () const
{
	return builder.getMetaInfo ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetContainerNode::getPresetClassKey () const
{
	return builder.getClassKey ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SortFolderNode* PresetContainerNode::newFolder (StringRef title)
{
	return NEW PresetSortFolderNode (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetContainerNode::onRefresh ()
{
	builder.resetPresets ();
	return SuperClass::onRefresh ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetContainerNode::hasSubNodes () const
{
	return builder.hasPresets (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetContainerNode::build ()
{
	builder.buildNodes (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetContainerNode::drawIconOverlay (const IItemModel::DrawInfo& info)
{
	if(builder.hasPresetsPending ())
		builder.drawPresetsPending (info);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetContainerNode::onPresetCreated (IPreset& preset)
{
	builder.onPresetCreated (preset, *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetContainerNode::onPresetRemoved (IPreset& preset)
{
	builder.onPresetRemoved (preset, *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetContainerNode::canRemoveParentFolder (FolderNode* parentFolder) const
{
	auto sortFolder = ccl_cast<PresetSortFolderNode> (parentFolder);
	return !(sortFolder && builder.hasSortFolder (sortFolder->getSortPath ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetContainerNode::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetContainerNode::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	return PresetSortFolderNode::sortNodesIntoFolder (data, String::kEmpty, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PresetContainerNode::appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes)
{
	contextMenu.addCommandItem (CommandWithTitle (CSTR ("Browser"), CSTR ("New Folder"), FileStrings::NewFolder ()), nullptr, true);
	return kResultFalse; // (continue)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetContainerNode::interpretCommand (const CommandMsg& msg, const Container* selectedNodes)
{
	if(msg.category == "Browser")
	{
		if(msg.name == "New Folder")
			return getPresetNodesBuilder ().onNewPresetFolder (*this, this, msg.checkOnly ());
	}
	return SuperClass::interpretCommand (msg, selectedNodes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetContainerNode::notify (ISubject* s, MessageRef msg)
{
	bool presetCreated = (msg == Signals::kPresetCreated);
	if(presetCreated || msg == Signals::kPresetRemoved)
	{
		UnknownPtr<IPreset> preset (msg[0]);
		if(preset)
		{
			if(IAttributeList* presetInfo = preset->getMetaInfo ())
				if(handlesPreset (PresetMetaAttributes (*presetInfo)))
				{
					if(presetCreated)
						onPresetCreated (*preset);
					else
						onPresetRemoved (*preset);
				}
		}
	}
	else if(msg == Signals::kPresetsRefreshed)
	{
		if(Browser* browser = getBrowser ())
			browser->refreshAll (true);
	}
	else if(msg == Signals::kPresetSubFolderAdded || msg == Signals::kPresetSubFolderRemoved)
	{
		builder.onPresetSubFoldersChanged (msg, *this);
	}
	else if(msg == Signals::kPresetFavoritesChanged && supportsFavorites ())
	{
		String classId (msg[0].asString ());
		String folderPath (msg.getArgCount () > 1 ? msg[1].asString () : String::kEmpty);
		builder.onPresetFavoritesChanged (classId, *this, folderPath);
	}
	else if(s == &builder)
	{
		if(msg == PresetNodesBuilder::kPresetsChanged)
		{
			if(Browser* browser = getBrowser ())
				browser->refreshNode (this);
		}
		else if(msg == kChanged)
		{
			if(Browser* browser = getBrowser ())
				browser->redrawNode (this);
		}
	}
	else
		SuperClass::notify (s, msg);
}
