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
// Filename    : ccl/app/browser/pluginbrowser.cpp
// Description : Plug-in Browser
//
//************************************************************************************************

#include "ccl/app/browser/pluginbrowser.h"
#include "ccl/app/browser/pluginnodes.h"
#include "ccl/app/browser/plugindraghander.h"
#include "ccl/app/browser/filesystemnodes.h"

#include "ccl/app/presets/presetnode.h"
#include "ccl/app/presets/presetdrag.h"
#include "ccl/app/presets/presetsystem.h"
#include "ccl/app/presets/presettrader.h"
#include "ccl/app/presets/presetfileprimitives.h"

#include "ccl/app/fileinfo/filepreviewcomponent.h"
#include "ccl/app/utilities/fileoperations.h"
#include "ccl/app/utilities/pluginclass.h"
#include "ccl/app/utilities/fileicons.h"

#include "ccl/base/storage/file.h"
#include "ccl/base/objectconverter.h"

#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/plugservices.h"

using namespace CCL;
using namespace Browsable;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Presets")
	XSTRING (ImportPresets, "Import Presets")
	XSTRING (Visible, "Visible")
END_XSTRINGS

//************************************************************************************************
// PlugInBrowser::ImportPresetDragHandler
//************************************************************************************************

class PlugInBrowser::ImportPresetDragHandler: public PresetDragHandler,
											  public IItemDragVerifier
{
public:
	ImportPresetDragHandler (IView* view, PlugInBrowser* browser)
	: PresetDragHandler (view)
	{
		UnknownPtr<IItemView> itemView (view);
		if(itemView)
			setChildDragHandler (itemView->createDragHandler (IItemView::kCanDragOnItem, this));
	}

	// IItemDragVerifier
	tbool CCL_API verifyTargetItem (ItemIndex& item, int& relation) override
	{
		// no target folder selection, presets are sorted automatically
		item = ItemIndex ();
		relation = IItemViewDragHandler::kFullView;
		return true;
	}

	// DragHandler
	IUnknown* prepareDataItem (IUnknown& item, IUnknown* context) override
	{
		UnknownPtr<IUrl> url (&item);
		if(url && url->isNativePath ())
		{
			if(url->isFolder ())
			{
				AutoPtr<IImage> icon (FileIcons::instance ().createIcon (*url));
				String fileName;
				url->getName (fileName, true);
				spriteBuilder.addItem (icon, fileName);
			}
			else
			{
				AutoPtr<IPreset> preset = preparePreset (item); // adds sprite item
				if(!preset)
					return nullptr;
			}
			url->retain ();
			return url;
		}
		return nullptr;
	}

	tbool CCL_API afterDrop (const DragEvent& event) override
	{
		PresetTrader::importPresets (getData ());
		return DragHandler::afterDrop (event);
	}

	CLASS_INTERFACE (IItemDragVerifier, DragHandler)
};

//************************************************************************************************
// PresetSortDraghandler
//************************************************************************************************

class PresetSortDraghandler: public DragHandlerBase
{
public:
	DECLARE_CLASS_ABSTRACT (PresetSortDraghandler, DragHandlerBase)

	PresetSortDraghandler (IView* view = nullptr, Browser* browser = nullptr);

	// DragHandlerBase
	bool setTargetNode (BrowserNode* node) override;

protected:
	String targetSortPath;
	String classKey;
	bool canMove;	///< at least one preset can be moved (is in a writable location)

	// DragHandlerBase
	IUnknown* prepareDataItem (IUnknown& item, IUnknown* context) override;
	void finishPrepare () override;
	tbool CCL_API dragOver (const DragEvent& event) override;
	tbool CCL_API afterDrop (const DragEvent& event) override;
};

//************************************************************************************************
// PresetSortDraghandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PresetSortDraghandler, DragHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetSortDraghandler::PresetSortDraghandler (IView* view, Browser* browser)
: DragHandlerBase (view, browser),
  canMove (false)
{
	canTryParentFolders (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* PresetSortDraghandler::prepareDataItem (IUnknown& item, IUnknown* context)
{
	UnknownPtr<IPreset> preset (&item);
	if(preset)
	{
		IImage* icon = nullptr;
		PlugInClass plugClass;
		if(PresetDragHandler::extractClass (plugClass, *preset))
			icon = plugClass.getIcon ();

		// keep first preset class key (plug-in class id or category): will only move to folders of that class
		if(data.isEmpty ())
			if(auto metaInfo = preset->getMetaInfo ())
				classKey = PresetMetaAttributes (*metaInfo).getClassKey ();

		spriteBuilder.addItem (icon, preset->getPresetName ());
		return preset.detach ();
	}
	else if(auto folderNode = unknown_cast<CustomSortFolderNode> (&item))
	{
		if(ccl_cast<PresetSortFolderNode> (folderNode) || ccl_cast<PresetFavoritesSortFolderNode> (folderNode))
		{
			// dragged sort folder or favorites folder node
			if(data.isEmpty ())
				if(auto containerNode = folderNode->getAncestorNodeWithInterface<IPresetContainerNode> ())
					classKey = containerNode->getPresetClassKey ();

			spriteBuilder.addItem (folderNode->getIcon (), folderNode->getTitle ());
			return return_shared (&item);
		}
	}
	else if(auto folderNode = unknown_cast<PresetSortFolderNode> (&item))
	{
		if(data.isEmpty ())
			if(auto containerNode = folderNode->getAncestorNodeWithInterface<IPresetContainerNode> ())
				classKey = containerNode->getPresetClassKey ();

		spriteBuilder.addItem (folderNode->getIcon (), folderNode->getTitle ());
		return return_shared (&item);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetSortDraghandler::finishPrepare ()
{
	// check if at least one preset can be moved
	canMove = false;
	ForEachUnknown (data, obj)
		String sortPath;

		UnknownPtr<IPreset> preset (obj);
		if(preset)
		{
			if(!preset->isReadOnly ())
			{
				canMove = true;
				break;
			}
		}
		else if(auto folderNode = unknown_cast<PresetSortFolderNode> (obj))
		{
			if(folderNode->hasWritablePreset ())
			{
				canMove = true;
				break;
			}
		}
	EndFor

	spriteBuilder.addHeader (nullptr, nullptr, -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetSortDraghandler::setTargetNode (BrowserNode* node)
{
	auto matchesPlugInClass = [&] (BrowserNode* targetNode)
	{
		// can only move into folder of same plug-in
		UnknownPtr<IPresetContainerNode> containerNode (ccl_as_unknown (targetNode));
		if(!containerNode)
			containerNode = targetNode->getAncestorNodeWithInterface<IPresetContainerNode> ();

		return containerNode && containerNode->getPresetClassKey () == classKey;
	};

	if(auto folderNode = ccl_cast<PresetSortFolderNode> (node))
	{
		if(matchesPlugInClass (node))
		{
			targetNode = node;
			targetSortPath.empty ();
			folderNode->getSortPath (targetSortPath);
		}
		return true;
	}
	else if(UnknownPtr<IPresetContainerNode> (ccl_as_unknown (node)).isValid ())
	{
		if(matchesPlugInClass (node))
		{
			targetNode = node;
			targetSortPath.empty ();
		}
		return true;
	}
	else if(ccl_cast<PresetFavoritesNode> (node))
	{
		targetNode = node;
		return true;
	}
	else if(auto folderNode = ccl_cast<PresetFavoritesSortFolderNode> (node))
	{
		targetNode = node;
		targetSortPath.empty ();
		folderNode->getSortPath (targetSortPath);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetSortDraghandler::dragOver (const DragEvent& event)
{
	SuperClass::dragOver (event);

	int result = IDragSession::kDropNone;
	String header;

	UnknownPtr<IPreset> draggedPreset;
	PresetSortFolderNode* draggedFolder = nullptr;
	PresetFavoritesSortFolderNode* draggedFavoritesFolder = nullptr;

	if(targetNode)
	{
		bool isAlreadyFavorite = false;
		String oldSortPath;
		String oldFavoritePath;

		if(draggedPreset = data.getFirst ())
		{
			isAlreadyFavorite = System::GetPresetManager ().isFavorite (*draggedPreset);
			oldFavoritePath = System::GetPresetManager ().getFavoriteFolder (*draggedPreset);
			if(!isAlreadyFavorite)
				oldFavoritePath = Url::strPathChar; // invalid path, to force a difference to any target folder

			Url presetUrl;
			draggedPreset->getUrl (presetUrl);
			IAttributeList* metaInfo = draggedPreset->getMetaInfo ();
			IPresetFileHandler* handler = System::GetPresetFileRegistry ().getHandlerForFile (presetUrl);
			if(metaInfo && handler)
				oldSortPath = PresetFilePrimitives::determineRelativeSubFolder (*handler, *metaInfo, presetUrl);
		}
		else if(draggedFolder = unknown_cast<PresetSortFolderNode> (data.getFirst ()))
			oldSortPath = draggedFolder->getSortPath ();
		else if(draggedFavoritesFolder = unknown_cast<PresetFavoritesSortFolderNode> (data.getFirst ()))
		{
			isAlreadyFavorite = true;
			oldFavoritePath = draggedFavoritesFolder->getSortPath ();
		}

		if(UnknownPtr<IPresetContainerNode> (ccl_as_unknown (targetNode)).isValid ())
			describeTransferToFolder (header, result, oldSortPath, String::kEmpty, canMove, draggedPreset != nullptr);
		else if(auto folderNode = ccl_cast<PresetSortFolderNode> (targetNode))
		{
			if(draggedFolder&& !folderNode->acceptMovedFolder (draggedFolder))
			{
				header = canMove ? FileStrings::Move () : FileStrings::Copy ();
				if(draggedFolder == folderNode)
					result = canMove ? IDragSession::kDropMove : IDragSession::kDropCopyReal;
			}
			else
				describeTransferToFolder (header, result, oldSortPath, folderNode->getSortPath (), canMove, draggedPreset != nullptr);
		}
		else if(ccl_cast<PresetFavoritesNode> (targetNode))
		{
			if(draggedPreset|| draggedFavoritesFolder)
				describeTransferToFavoriteFolder (header, result, oldFavoritePath, String::kEmpty, isAlreadyFavorite, draggedPreset != nullptr);
		}
		else if(auto favoritesFolder = ccl_cast<PresetFavoritesSortFolderNode> (targetNode))
		{
			if(draggedPreset || draggedFavoritesFolder)
			{
				if(draggedFavoritesFolder && !favoritesFolder->acceptMovedFolder (draggedFavoritesFolder))
				{
					header = FileStrings::Move ();
					if(draggedFavoritesFolder == favoritesFolder)
						result = IDragSession::kDropMove;
				}
				else
					describeTransferToFavoriteFolder (header, result, oldFavoritePath, favoritesFolder->getSortPath (), isAlreadyFavorite, draggedPreset != nullptr);
			}
		}
	}

	// avoid empty header: use class name
	if(header.isEmpty ())
		if(draggedPreset && draggedPreset->getMetaInfo ())
			header = PresetMetaAttributes (*draggedPreset->getMetaInfo ()).getClassName ();

	event.session.setResult (result);
	spriteBuilder.replaceItemText (*sprite, 0, header);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetSortDraghandler::afterDrop (const DragEvent& event)
{
	SuperClass::afterDrop (event);

	if(event.session.getResult () != IDragSession::kDropNone && targetNode)
	{
		UnknownPtr<IDataTarget> dataTarget (targetNode->asUnknown ());
		if(dataTarget)
			dataTarget->insertData (event.session.getItems (), &event.session);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum PlugInBrowserTags
	{
		kEditMode = 200
	};
}

//************************************************************************************************
// PlugInBrowser
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PlugInBrowser, Browser)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInBrowser::PlugInBrowser (StringRef name, StringRef title, FilePreviewComponent* _preview)
: Browser (name, title),
  preview (_preview)
{
	setFormName ("CCL/PlugInBrowser");

	if(preview == nullptr)
		preview = NEW FilePreviewComponent (CCLSTR ("Preview"));

	addComponent (preview);
	addSearch ();

	paramList.addParam ("editMode", Tag::kEditMode);

	AutoPtr<IColumnHeaderList> columns (ccl_new<IColumnHeaderList> (ClassID::ColumnHeaderList));
	columns->addColumn (200, nullptr, nullptr, 0, 0);
	columns->addColumn (20, nullptr, PlugInClassNode::kVisible, 0, 0);
	columns->addColumn (20, nullptr, PlugInClassNode::kFavorite, 0, 0);
	columns->moveColumn (PlugInClassNode::kVisible, 0);
	columns->moveColumn (PlugInClassNode::kFavorite, 1);
	columns->hideColumn ( PlugInClassNode::kVisible, true);
	setDefaultColumns (columns);
	hideColumnHeaders (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInCategoryNode* PlugInBrowser::findCategoryNode (const IClassDescription& description) const
{
	for(auto node : iterate_as<BrowserNode> (ccl_const_cast (this)->getRootNode ()->getContent ()))
		if(auto plugCategoryNode = ccl_cast<PlugInCategoryNode> (node))
			if(plugCategoryNode->matches (description, false))
				return plugCategoryNode;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* PlugInBrowser::findNodeWithUrl (UrlRef url)
{
	// find class node
	if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (url))
	{
		if(BrowserNode* pluginNode = PlugInCategoryNode::findRegularPluginClassNode (description->getClassID (), *getRootNode ()))
			return pluginNode;

		if(PlugInCategoryNode* categoryNode = findCategoryNode (*description))
		{
			// search result list might query for a hidden plugin (not present in the browser tree): return a temporary node
			PlugInClassNode* tempNode = NEW PlugInClassNode (*description);
			tempNode->canEditPresentation (categoryNode->canEditPresentation ());

			Object::deferDestruction (tempNode);
			return tempNode;
		}
	}

	// find preset node
	AutoPtr<IPreset> preset (System::GetPresetManager ().openPreset (url));
	if(preset && preset->getMetaInfo ())
		return findPluginOrPresetNode (*preset->getMetaInfo (), url);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* PlugInBrowser::findPluginOrPresetNode (IAttributeList& metaInfo, UrlRef presetUrl)
{
	PresetMetaAttributes metaAttributes (metaInfo);
	UID pluginClass;
	metaAttributes.getClassID (pluginClass);

	// 1.) find plugin node by class ID
	BrowserNode* baseNode = PlugInCategoryNode::findRegularPluginClassNode (pluginClass, *getRootNode ());
	if(!baseNode)
	{
		// 2.) find a PresetContainerNode that handles the given metaInfo
		AutoPtr<IRecognizer> recognizer (PresetContainerNode::createRecognizer (metaInfo));
		baseNode = findNode<PresetContainerNode> (recognizer);
	}

	// find preset node in baseNode
	if(baseNode)
		if(PresetNode* presetNode = PresetNodeSorter::findPresetNode (*baseNode, presetUrl, &metaInfo, true))
			return presetNode;

	return baseNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInBrowser::selectPluginOrPreset (IAttributeList& metaInfo, UrlRef presetUrl)
{
	if(BrowserNode* toSelect = findPluginOrPresetNode (metaInfo, presetUrl))
	{
		expandNode (toSelect);
		setFocusNode (toSelect, true);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool PlugInBrowser::canInsertData (BrowserNode* node, const IUnknownList& data, IDragSession* session, IView* targetView)
{
	if(SuperClass::canInsertData (node, data, session, targetView))
		return true;

	AutoPtr<ImportPresetDragHandler> dragHandler (NEW ImportPresetDragHandler (targetView, this));
	if(dragHandler->prepare (data, nullptr, XSTR (ImportPresets)))
	{
		if(session)
			session->setDragHandler (dragHandler);
		return true;
	}

	AutoPtr<PluginDraghandler> plugDragHandler (NEW PluginDraghandler (targetView, this));
	if(plugDragHandler->prepare (data, session))
	{
		if(session)
			session->setDragHandler (plugDragHandler);
		return true;
	}

	AutoPtr<PresetSortDraghandler> presetSortDraghandler (NEW PresetSortDraghandler (targetView, this));
	if(presetSortDraghandler->prepare (data, session))
	{
		if(session)
			session->setDragHandler (presetSortDraghandler);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInBrowser::onNodeFocused (BrowserNode* node, bool inList)
{
	Url path;
	IImage* icon = nullptr;
	String title;

	if(node)
	{
		icon = node->getIcon ();
		title = node->getTitle ();
	}

	if(PlugInClassNode* plugInNode = ccl_cast<PlugInClassNode> (node))
	{
		plugInNode->getClassDescription ().getClassUrl (path);
	}
	else if(FileNode* fileNode = ccl_cast<FileNode> (node))
	{
		if(fileNode->getPath ())
			path = *fileNode->getPath ();
	}

	if(path.isEmpty ())
	{
		path.setProtocol (CCLSTR ("virtual"));

		// path of virtual folder
		String pathString;
		for(BrowserNode* n = node; n && n->getParent (); n = n->getParent ())
		{
			if(!pathString.isEmpty ())
				pathString.prepend (Url::strPathChar);
			pathString.prepend (n->getTitle ());
		}

		path.setPath (pathString, Url::kFolder);
	}

	if(path.isEqualUrl (preview->getFile ()) == false)
		preview->setFile (path, icon, title);

	SuperClass::onNodeFocused (node, inList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInBrowser::prepareRefresh ()
{
	System::GetPresetManager ().scanPresets (false);
	return false; // don't refresh in SuperClass, will be done via signal
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInBrowser::collectCategoryNodes (ObjectList& categoryNodes)
{
	// try first 2 levels
	if(PlugInCategoryNode* plugCategoryNode = ccl_cast<PlugInCategoryNode> (getRootNode ()))
		categoryNodes.add (plugCategoryNode);

	for(auto node : iterate_as<BrowserNode> (getRootNode ()->getContent ()))
		if(PlugInCategoryNode* plugCategoryNode = ccl_cast<PlugInCategoryNode> (node))
			categoryNodes.add (plugCategoryNode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInBrowser::onViewModeChanged ()
{
	// refresh category nodes (SeparatorNode should not appear in icon mode of ListView)
	ObjectList categoryNodes;
	collectCategoryNodes (categoryNodes);

	ScopedVar<bool> scope (getRestoringState (), true); // force getPresets immediately (not in background) to ensure finding a focus preset node in the new view

	for(auto plugCategoryNode : iterate_as<PlugInCategoryNode> (categoryNodes))
		refreshNode (plugCategoryNode, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInBrowser::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kEditMode && getRootNode ())
	{
		bool isEdit = param->getValue ().asBool ();

		// pass edit mode to PlugInCategoryNodes (try first 2 levels)
		ObjectList categoryNodes;
		collectCategoryNodes (categoryNodes);
		for(auto plugCategoryNode : iterate_as<PlugInCategoryNode> (categoryNodes))
		{
			plugCategoryNode->isEditMode (isEdit);
			refreshNode (plugCategoryNode, true);
		}

		if(defaultColumns)
		{
			// "visible" column shown only in edit mode
			defaultColumns->hideColumn (PlugInClassNode::kVisible, !isEdit);
			updateColumns ();
		}

		// when edit mode switched on in search mode, show result in browser (leave search mode) to allow editing
		if(isEdit && isSearchResultsVisible ())
			showSelectedSearchResultInContext ();

		if(!isEdit)
			System::GetPluginPresentation ().saveSettings ();
		return true;
	}
	return SuperClass::paramChanged (param);
}

