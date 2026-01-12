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
// Filename    : ccl/extras/webfs/webfiledraghandler.cpp
// Description : Web File Drag Handler
//
//************************************************************************************************

#include "ccl/extras/webfs/webfiledraghandler.h"
#include "ccl/extras/webfs/webfilemethods.h"
#include "ccl/extras/webfs/webfilenodes.h"

#include "ccl/app/utilities/fileicons.h"

#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Web;
using namespace Browsable;

//************************************************************************************************
// WebFileDragHandler
//************************************************************************************************

WebFileDragHandler::WebFileDragHandler (IView* view, Browser* browser)
: DragHandlerBase (view, browser)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebFileDragHandler::setTargetNode (BrowserNode* node)
{
	if(WebDirectoryNode* dir = ccl_cast<WebDirectoryNode> (node))
	{
		if(const Url* path = dir->getPath ())
		{
			if(homeFolders.contains (*path))
				return false;
			if(childFolders.containsSubPath (*path))
				return false;
		}
	}
	
	UnknownPtr<IDataTarget> dataTarget (ccl_as_unknown (node));
	if(dataTarget)
		if(dataTarget->canInsertData (data))
		{
			targetNode = node;
			return true;
		}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* WebFileDragHandler::prepareDataItem (IUnknown& item, IUnknown* context)
{
	UnknownPtr<IDownloadable> sourceInfo (&item);
	UnknownPtr<IFileDescriptor> descriptor (&item);
	if(sourceInfo)
	{
		UrlRef url = sourceInfo->getSourceUrl ();
		Url parentFolder (url);
		if(parentFolder.ascend ())
			homeFolders.addPath (parentFolder);

		AutoPtr<IImage> icon;
		String elementName;
		if(descriptor)
		{
			FileType fileType;
			descriptor->getFileName (elementName);
			descriptor->getFileType (fileType);
			icon = FileIcons::instance ().createIcon (fileType);
		}
		else
		{
			if(url.isFolder ())
			{
				url.getName (elementName);
				icon = FileIcons::instance ().createIcon (url);
				homeFolders.addPath (url);
				childFolders.addPath (url);
			}
		}
		if(icon)
			spriteBuilder.addItem (icon, elementName);
		return sourceInfo.detach ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebFileDragHandler::dragOver (const DragEvent& event)
{
	SuperClass::dragOver (event);
	 
	event.session.setResult (targetNode ? IDragSession::kDropMove : IDragSession::kDropNone);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebFileDragHandler::afterDrop (const DragEvent& event)
{
	UnknownPtr<IDataTarget> dataTarget (ccl_as_unknown (targetNode));
	if(dataTarget)
		return dataTarget->insertData (data);

	return false;
}
