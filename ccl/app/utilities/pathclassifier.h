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
// Filename    : ccl/app/utilities/pathclassifier.h
// Description : Path Classifier
//
//************************************************************************************************

#ifndef _ccl_pathclassifier_h
#define _ccl_pathclassifier_h

#include "ccl/base/storage/url.h"

namespace CCL {

struct VolumeInfo;

//************************************************************************************************
// PathClassifier
//************************************************************************************************

class PathClassifier
{
public:
	enum PathClass
	{
		kNativeRoot,		///< root of all native volumes ("file:///")
		kPackageRoot,		///< root of all package volumes ("package:///")
		kNativeVolume,		///< native volume (e.g. "file:///c:/")
		kPackageVolume,		///< package volume (e.g. "package://package-id/")
		kFile,				///< regular file
		kFolder				///< regular folder
	};

	static PathClass classify (UrlRef path);

	static bool isRoot (UrlRef path);		///< kNativeRoot or kPackageRoot
	static bool isVolume (UrlRef path);		///< kNativeVolume or kPackageVolume
	static bool isRegular (UrlRef path);	///< kFile or kFolder

	static bool isSameVolume (UrlRef path1, UrlRef path2);
	static bool needsExtraction (UrlRef path);	///< file inside a package which needs to be extracted
	static bool isCompressedFile (UrlRef path);	///< file inside a package which is compressed

	static String getVolumeLabel (UrlRef path, const VolumeInfo& info);				 ///< for display, may contain translated elements
	static MutableCString getVolumeIdentifier (UrlRef path, const VolumeInfo& info); ///< for internal identification, language-indenpendent
};

} // namespace CCL

#endif // _ccl_pathclassifier_h
