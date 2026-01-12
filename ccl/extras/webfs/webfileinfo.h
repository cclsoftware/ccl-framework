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
// Filename    : ccl/extras/webfs/webfileinfo.h
// Description : Web File Info Component
//
//************************************************************************************************

#ifndef _ccl_webfileinfo_h
#define _ccl_webfileinfo_h

#include "ccl/base/storage/url.h"

#include "ccl/app/fileinfo/fileinfocomponent.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// WebFileInfoComponent
//************************************************************************************************

class WebFileInfoComponent: public StandardFileInfo
{
public:
	DECLARE_CLASS (WebFileInfoComponent, StandardFileInfo)

	WebFileInfoComponent ();

	static bool canHandleFile (UrlRef path);

	// StandardFileInfo
	tbool CCL_API setFile (UrlRef path) override;
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	Url webfsUrl;
};

//************************************************************************************************
// WebFileInfoFactory
//************************************************************************************************

class WebFileInfoFactory: public FileInfoFactory
{
public:
	WebFileInfoFactory ();
	
	static void forceLinkage ();

	// FileInfoFactory
	IFileInfoComponent* CCL_API createComponent (UrlRef path) const override;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webfileinfo_h
