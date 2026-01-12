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
// Filename    : ccl/gui/system/fontresource.h
// Description : Font Resource
//
//************************************************************************************************

#ifndef _ccl_fontresource_h
#define _ccl_fontresource_h

#include "ccl/base/object.h"

namespace CCL {

interface IStream;

//************************************************************************************************
// FontResource
/* Base class for platform specific implementations. */
//************************************************************************************************

class FontResource: public Object
{
public:
	DECLARE_CLASS (FontResource, Object)

	static FontResource* install (UrlRef path, int fontStyle);
	static FontResource* install (IStream& stream, StringRef name, int fontStyle);	 ///< platform-dependent
	static void beginInstallation (bool state);
	struct InstallationScope;
};

//************************************************************************************************
// FontResource::InstallationScope
//************************************************************************************************

struct FontResource::InstallationScope
{
	InstallationScope ()
	{
		beginInstallation (true);
	}
	~InstallationScope ()
	{
		beginInstallation (false);
	}
};

} // namespace CCL

#endif // _ccl_fontresource_h
