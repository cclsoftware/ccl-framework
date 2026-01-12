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
// Filename    : ccl/base/development.h
// Description : Determine locations in working copy
//
//************************************************************************************************

#ifndef _ccl_development_h
#define _ccl_development_h

#include "ccl/public/base/platform.h"

#include "core/public/coredevelopment.h"

namespace CCL {

class Url;

//************************************************************************************************
// Development macros
//************************************************************************************************

// CCL_FRAMEWORK_DIRECTORY is defined by CMake, see cclbase.cmake
#if DEBUG && !defined (CCL_FRAMEWORK_DIRECTORY)
#define CCL_FRAMEWORK_DIRECTORY ""
#endif

#if DEBUG && CCL_PLATFORM_DESKTOP

/** Get folder location in working copy. */
#define GET_DEVELOPMENT_FOLDER_LOCATION(url, rootProject, relativePath) \
CCL::Development::makeAbsolutePath (url, rootProject, relativePath, CCL::Url::kFolder);

/** Get file location in working copy. */
#define GET_DEVELOPMENT_FILE_LOCATION(url, rootProject, relativePath) \
CCL::Development::makeAbsolutePath (url, rootProject, relativePath, CCL::Url::kFile);

/** Get build output folder of working copy. */
#define GET_BUILD_FOLDER_LOCATION(url) \
CCL::Development::makeAbsolutePath (url, BUILD_FOLDER_NAME, RELATIVE_BUILD_PATH, CCL::Url::kFolder);

#else // !DEBUG
#define GET_DEVELOPMENT_FOLDER_LOCATION(url, rootProject, relativePath)
#define GET_DEVELOPMENT_FILE_LOCATION(url, rootProject, relativePath)
#define GET_BUILD_FOLDER_LOCATION(url)
#endif

//************************************************************************************************
// Development namespace
//************************************************************************************************

namespace Development
{
	/** Get root folder of working copy, searches upwards starting from given folder. */
	bool getRootFolder (Url& url, UrlRef startFolder);

	/** Get top level folder in working copy (one level under root; start folder is determined at build time). */
	Url& getTopLevelFolder (Url& url, CStringPtr folderName);

	/** Make absolute path in working copy. */
	Url& makeAbsolutePath (Url& url, CStringPtr rootProject, CStringPtr relativePath, int type);
}

} // namespace CCL

#endif // _ccl_development_h
