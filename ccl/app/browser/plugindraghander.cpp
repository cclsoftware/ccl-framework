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
// Filename    : ccl/app/browser/plugindraghander.cpp
// Description : Drag handler for moving plugins to another sort folder
//
//************************************************************************************************

#include "ccl/app/browser/plugindraghander.h"
#include "ccl/app/browser/pluginnodes.h"
#include "ccl/app/browser/browser.h"

#include "ccl/app/presets/objectpreset.h"
#include "ccl/app/utilities/fileoperations.h"
#include "ccl/app/utilities/pluginclass.h"

#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/gui/framework/iview.h"

#include "ccl/public/plugservices.h"

using namespace CCL;
using namespace Browsable;

//************************************************************************************************
// Browsable::PluginDraghandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PluginDraghandler, DragHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

PluginDraghandler::PluginDraghandler (IView* view, Browser* browser)
: DragHandlerBase (view, browser)
{
	canTryParentFolders (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInCategoryNode* PluginDraghandler::findCategoryNode (StringRef category)
{
	return browser->findNode<PlugInCategoryNode> (AutoPtr<IRecognizer> (Recognizer::create ([&] (IUnknown* obj)
		{
			PlugInCategoryNode* categoryNode = unknown_cast<PlugInCategoryNode> (obj);
			return categoryNode && categoryNode->getCategory1 () == category;
		})));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* PluginDraghandler::prepareDataItem (IUnknown& item, IUnknown* context)
{
	const IClassDescription* description = nullptr;
	if(UnknownPtr<IClassDescription> desc = &item)
		description = desc;
	else
	{
		ObjectPreset* objectPreset = unknown_cast<ObjectPreset> (&item);
		if(objectPreset && objectPreset->isOnlyClass () && objectPreset->getMetaInfo ())
		{
			UID cid;
			PresetMetaAttributes (*objectPreset->getMetaInfo ()).getClassID (cid);
			description = System::GetPlugInManager ().getClassDescription (cid);
		}
	}

	if(description)
	{
		// find corresponding PlugInCategoryNode to check if presentation can be edited
		PlugInCategoryNode* categoryNode = findCategoryNode (description->getCategory ());
		if(categoryNode && !categoryNode->canEditPresentation ())
			return nullptr;

		PlugInClass* pluginClass = NEW PlugInClass (*description);

		String title;
		description->getLocalizedName (title);
		IImage* icon = pluginClass->getIcon ();
		spriteBuilder.addItem (icon, title);
		return pluginClass->asUnknown ();
	}
	else if(auto folderNode = unknown_cast<CustomSortFolderNode> (&item))
	{
		if(ccl_cast<PlugInSortFolderNode> (folderNode) || ccl_cast<FavoritesSortFolderNode> (folderNode))
		{
			// dragged sort folder or favorites folder node
			spriteBuilder.addItem (folderNode->getIcon (), folderNode->getTitle ());
			return return_shared (&item);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PluginDraghandler::finishPrepare ()
{
	spriteBuilder.addHeader (nullptr, nullptr, -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PluginDraghandler::setTargetNode (BrowserNode* node)
{
	if(PlugInSortFolderNode* folderNode = ccl_cast<PlugInSortFolderNode> (node))
	{
		targetNode = node;
		targetSortPath.empty ();
		folderNode->getSortPath (targetSortPath);
		return true;
	}
	else if(PlugInCategoryNode* categoryNode = ccl_cast<PlugInCategoryNode> (node))
	{
		if(categoryNode->isSortByUserFolder ())
		{
			targetNode = node;
			return true;
		}
	}
	else if(ccl_cast<PlugInFavoritesNode> (node) || ccl_cast<RecentPlugInsNode> (node))
	{
		targetNode = node;
		return true;
	}
	else if(auto folderNode = ccl_cast<FavoritesSortFolderNode> (node))
	{
		targetNode = node;
		targetSortPath.empty ();
		folderNode->getSortPath (targetSortPath);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PluginDraghandler::dragOver (const DragEvent& event)
{
	SuperClass::dragOver (event);

	int result = IDragSession::kDropNone;
	String header;

	PlugInClass* draggedPlugClass = nullptr;
	PlugInSortFolderNode* draggedPlugFolder = nullptr;
	FavoritesSortFolderNode* draggedFavoritesFolder = nullptr;

	if(targetNode)
	{
		bool isAlreadyFavorite = false;
		String oldSortPath;
		String oldFavoritePath;

		if(draggedPlugClass = unknown_cast<PlugInClass> (data.getFirst ()))
		{
			isAlreadyFavorite = System::GetPluginPresentation ().isFavorite (draggedPlugClass->getClassID ());
			oldSortPath = System::GetPluginPresentation ().getSortPath (draggedPlugClass->getClassID ());
			oldFavoritePath = System::GetPluginPresentation ().getFavoriteFolder (draggedPlugClass->getClassID ());
			if(!isAlreadyFavorite)
				oldFavoritePath = Url::strPathChar; // invalid path, to force a difference to any target folder
		}
		else if(draggedPlugFolder = unknown_cast<PlugInSortFolderNode> (data.getFirst ()))
			oldSortPath = draggedPlugFolder->getSortPath ();
		else if(draggedFavoritesFolder = unknown_cast<FavoritesSortFolderNode> (data.getFirst ()))
		{
			isAlreadyFavorite = true;
			oldFavoritePath = draggedFavoritesFolder->getSortPath ();
		}

		if(ccl_cast<PlugInFavoritesNode> (targetNode))
		{
			if(draggedPlugClass || draggedFavoritesFolder) // don't accept plug-in folders
				describeTransferToFavoriteFolder (header, result, oldFavoritePath, String::kEmpty, isAlreadyFavorite, draggedPlugClass != nullptr);
		}
		else if(auto favoritesFolder = ccl_cast<FavoritesSortFolderNode> (targetNode))
		{
			if(draggedPlugClass || draggedFavoritesFolder)
			{
				if(draggedFavoritesFolder && !favoritesFolder->acceptMovedFolder (draggedFavoritesFolder))
				{
					header = FileStrings::Move ();
					if(draggedFavoritesFolder == favoritesFolder)
						result = IDragSession::kDropMove;
				}
				else
					describeTransferToFavoriteFolder (header, result, oldFavoritePath, favoritesFolder->getSortPath (), isAlreadyFavorite, draggedPlugClass != nullptr);
			}
		}
		else if(ccl_cast<PlugInCategoryNode> (targetNode))
		{
			if(draggedPlugClass || draggedPlugFolder)
				describeTransferToFolder (header, result, oldSortPath, String::kEmpty, true, draggedPlugClass != nullptr);
		}
		else if(auto plugFolder = ccl_cast<PlugInSortFolderNode> (targetNode))
		{
			if(draggedPlugClass || draggedPlugFolder)
			{
				if(draggedPlugFolder && !plugFolder->acceptMovedFolder (draggedPlugFolder))
				{
					header = FileStrings::Move ();
					if(draggedPlugFolder == plugFolder)
						result = IDragSession::kDropMove;
				}
				else
					describeTransferToFolder (header, result, oldSortPath, plugFolder->getSortPath (), true, draggedPlugClass != nullptr);
			}
		}
	}

	// avoid empty header: use plugin category title
	if(header.isEmpty ())
		if(draggedPlugClass)
			if(PlugInCategoryNode* categoryNode = findCategoryNode (draggedPlugClass->getCategory ()))
				header = categoryNode->getTitle ();

	event.session.setResult (result);
	spriteBuilder.replaceItemText (*sprite, 0, header);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PluginDraghandler::afterDrop (const DragEvent& event)
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
