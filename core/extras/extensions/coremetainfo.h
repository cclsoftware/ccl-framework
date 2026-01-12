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
// Filename    : core/extras/extensions/coremetainfo.h
// Description : Core Extension Meta Information
//
//************************************************************************************************

#ifndef _coremetainfo_h
#define _coremetainfo_h

#include "core/public/coretypes.h"

namespace Core {
namespace Meta {

//////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Examples:

	metainfo.json
	{
		"id": "{vendor.package.platform}",
		"name": "{Package Name}",
		"version": "{major.minor.revision.build}"
	}

	products.json
	[
		{
			"id": "{vendor.bundle.product}",
			"name": "{Product Name}",
		}
	]

*/
//////////////////////////////////////////////////////////////////////////////////////////////////

const CStringPtr kPackageInfoFile = "metainfo.json";
const CStringPtr kProductBundleFile = "products.json";

const CStringPtr kID = "id";
const CStringPtr kName = "name";
const CStringPtr kVersion = "version";
const CStringPtr kCID = "cid";

const CStringPtr kInclude = "include"; ///< relative path to other file
const CStringPtr kProductsArray = "products";

// product status
const CStringPtr kStatus = "status";
const CStringPtr kStatusActivated = "activated";
const CStringPtr kStatusDeleted = "deleted";

} // namespace Meta
} // namespace Core

#endif // _coremetainfo_h
