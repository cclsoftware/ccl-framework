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
// Filename    : ccl/gui/system/fontresource.cpp
// Description : Font Resource
//
//************************************************************************************************

#include "ccl/gui/system/fontresource.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// FontResource
//************************************************************************************************

FontResource* FontResource::install (UrlRef path, int fontStyle)
{
    String name;
    path.getName (name, true);
    
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path);
	if(stream)
		return install (*stream, name, fontStyle);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FontResource::beginInstallation (bool state)
{
	NativeGraphicsEngine::instance ().beginFontInstallation (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (FontResource, Object)
