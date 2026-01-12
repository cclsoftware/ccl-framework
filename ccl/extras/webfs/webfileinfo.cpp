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
// Filename    : ccl/extras/webfs/webfileinfo.cpp
// Description : Web File Info Component
//
//************************************************************************************************

#include "ccl/extras/webfs/webfileinfo.h"
#include "ccl/extras/webfs/webfilemethods.h"

#include "ccl/public/gui/iparameter.h"

#include "ccl/public/network/web/iwebfileservice.h"
#include "ccl/public/netservices.h"

using namespace CCL;
using namespace Web;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum WebFileInfoTags
	{
		kDownload = 'Down'
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT (WebFileInfoFactory)
{
	FileInfoRegistry::instance ().registerFileInfoFactory (NEW WebFileInfoFactory);
	return true;
}

//************************************************************************************************
// WebFileInfoFactory
//************************************************************************************************

void WebFileInfoFactory::forceLinkage ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebFileInfoFactory::WebFileInfoFactory ()
{
	setLocalFilesOnly (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileInfoComponent* CCL_API WebFileInfoFactory::createComponent (UrlRef path) const
{
	if(FileInfoComponent::isLocal (path))
		return nullptr;

	// check for customized info component
	IFileInfoComponent* component = nullptr;
	if(AutoPtr<IFileInfoFactory> factory = System::GetWebFileService ().openHandler<IFileInfoFactory> (path))
		component = factory->createComponent (path);

	// create default component for files
	if(!component && !path.isFolder ())
	{
		ASSERT (WebFileInfoComponent::canHandleFile (path))
		component = NEW WebFileInfoComponent;
	}

	if(component)
		component->setFile (path);
	return component;
}

//************************************************************************************************
// WebFileInfoComponent
//************************************************************************************************

bool WebFileInfoComponent::canHandleFile (UrlRef path)
{
	if(isLocal (path))
		return false;
	if(path.isFolder ())
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (WebFileInfoComponent, StandardFileInfo)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebFileInfoComponent::WebFileInfoComponent ()
: StandardFileInfo (CCLSTR ("WebFileInfo"), "WebFileInfo")
{
	paramList.addParam (CSTR ("download"), Tag::kDownload)->enable (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebFileInfoComponent::setFile (UrlRef path)
{
	if(!canHandleFile (path))
		return false;
	webfsUrl = path;
	paramList.byTag (Tag::kDownload)->enable (FileMethods ().canDownload (webfsUrl));
	return SuperClass::setFile (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebFileInfoComponent::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kDownload)
	{
		bool canDownload = FileMethods ().canDownload (webfsUrl);
		ASSERT (canDownload == true)
		if(canDownload)
			FileMethods ().downloadFile (webfsUrl);
		return true;
	}
	else
		return SuperClass::paramChanged (param);
}
