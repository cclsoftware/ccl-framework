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
// Filename    : ccl/extras//webfsbrowserextension.h
// Description : WebFS Browser Extension
//
//************************************************************************************************

#include "ccl/extras/webfs/webfsbrowserextension.h"

#include "ccl/extras/webfs/webfilenodes.h"
#include "ccl/extras/webfs/webfilemethods.h"
#include "ccl/public/network/web/iwebfileservice.h"
#include "ccl/public/netservices.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/plugservices.h"

using namespace CCL;
using namespace Web;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("WebFS")
	XSTRING (UploadTo, "Upload To")
END_XSTRINGS

//************************************************************************************************
// BrowserExtension
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (BrowserExtension, Object, "WebFSBrowserExtension")
DEFINE_CLASS_NAMESPACE (BrowserExtension, NAMESPACE_CCL)
DEFINE_CLASS_CATEGORY (BrowserExtension, MAKE_BROWSEREXTENSION_CATEGORY ("FileBrowser"))
DEFINE_CLASS_UID (BrowserExtension, 0xda017a6b, 0x1d35, 0x4af2, 0x98, 0xcd, 0x78, 0x94, 0xd5, 0x27, 0xc0, 0x8e)

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserExtension::BrowserExtension ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BrowserExtension::extendBrowserNodeMenu (IBrowserNode* node, IContextMenu& contextMenu, IUnknownList* selectedNodes)
{
	// Upload
	auto focusNode = unknown_cast<Browsable::FileNode> (node);
	UnknownPtr<IMenu> menu (&contextMenu);
	if(menu && focusNode && FileMethods ().canUploadFrom (focusNode->getFilePath ()))
	{
		AutoPtr<ObjectArray> sourceFiles = NEW ObjectArray;
		sourceFiles->objectCleanup (true);
		
		if(selectedNodes)
		{
			ForEachUnknown (*selectedNodes, unk)
				if(auto fileNode = unknown_cast<Browsable::FileNode> (node))
					if(FileMethods ().canUploadFrom (fileNode->getFilePath ()))
						sourceFiles->add (NEW Url (fileNode->getFilePath ()));
			EndFor
		}
		else
			sourceFiles->add (NEW Url (focusNode->getFilePath ()));

		ObjectArray volumesToCheck, uploadTargets;
		volumesToCheck.objectCleanup (true);
		uploadTargets.objectCleanup (true);
		FileMethods ().collectVolumes (volumesToCheck);

		ForEach (volumesToCheck, UrlWithTitle, volume)
			if(FileMethods ().canUploadToVolume (*volume))
				uploadTargets.add (return_shared (volume));
			else if(FileMethods ().canModifySpecificFolders (*volume))
				FileMethods ().collectUploadTargets (uploadTargets, *volume);
		EndFor

		if(!sourceFiles->isEmpty () && !uploadTargets.isEmpty ())
		{
			IMenu* uploadMenu = menu->createMenu ();
			uploadMenu->setMenuAttribute (IMenu::kMenuTitle, XSTR (UploadTo));
			menu->addMenu (uploadMenu);

			ForEach (uploadTargets, UrlWithTitle, targetFolder)
				AutoPtr<ObjectArray> params = NEW ObjectArray ();
				params->objectCleanup (true);
				params->add (NEW Url (*targetFolder));
				params->add (return_shared<ObjectArray> (sourceFiles));
						
				IMenuItem* menuItem = uploadMenu->addCommandItem (targetFolder->getTitle (), "File", "Upload To",
					CommandDelegate<BrowserExtension>::make (this, &BrowserExtension::onUpload, params->asUnknown ()));

				// customize menu icon
				if(AutoPtr<IWebFileBrowserModel> model = System::GetWebFileService ().openHandler<IWebFileBrowserModel> (*targetFolder))
					if(IImage* icon = model->getVolumeIcon (*targetFolder))
						menuItem->setItemAttribute (IMenuItem::kItemIcon, static_cast<IUnknown*> (icon));
			EndFor
		}
	}
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserExtension::onUpload (CmdArgs args, VariantRef data)
{
	if(!args.checkOnly ())
	{
		ObjectArray* params = unknown_cast<ObjectArray> (data);
		Url* targetFolder = static_cast<Url*> (params->at (0));
		ObjectArray* sourceFiles = static_cast<ObjectArray*> (params->at (1));
		
		ForEach (*sourceFiles, Url, path)
			FileMethods ().uploadObject (*targetFolder, *path);
		EndFor
	}
	return true;
}
