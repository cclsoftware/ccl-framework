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
// Filename    : ccl/public/system/ipackagemetainfo.h
// Description : Package Meta Information
//
//************************************************************************************************

#ifndef _ccl_ipackagemetainfo_h
#define _ccl_ipackagemetainfo_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IStorable;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Package Meta Information
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Meta
{
	/** Package meta information prefix. */
	DEFINE_STRINGID (kPackagePrefix, "Package:")

	/** Unique Package identifier [String] (e.g. "com.vendor.package")*/
	DEFINE_STRINGID (kPackageID, "Package:ID")

	/** Short Package identifier [String] (optional, e.g. "package")*/
	DEFINE_STRINGID (kPackageShortID, "Package:ShortID")

	/** Package name [String]. */
	DEFINE_STRINGID (kPackageName, "Package:Name")

	/** Localized package name [String]. */
	DEFINE_STRINGID (kPackageLocalizedName, "Package:LocalizedName")

	/** Package copyright information [String]. */
	DEFINE_STRINGID (kPackageCopyright, "Package:Copyright")

	/** Package vendor name [String]. */
	DEFINE_STRINGID (kPackageVendor, "Package:Vendor")

	/** Package description [String]. */
	DEFINE_STRINGID (kPackageDescription, "Package:Description")

	/** Localized package description [String]. */
	DEFINE_STRINGID (kPackageLocalizedDescription, "Package:LocalizedDescription")

	/** Package version [String]. */
	DEFINE_STRINGID (kPackageVersion, "Package:Version")

	/** Package vendor or product website [String]. */
	DEFINE_STRINGID (kPackageWebsite, "Package:Website")

	/** Package executable path [String]. */
	DEFINE_STRINGID (kPackageExecutable, "Package:Executable")

	/** Package icon (path to image file) [String]. */
	DEFINE_STRINGID (kPackageIcon, "Package:Icon")

	/** Package icon set (path to image file) [String]. */
	DEFINE_STRINGID (kPackageIconSet, "Package:IconSet")

	/** Package icon set file name. */
	DEFINE_STRINGID (kPackageIconSetFileName, "package.iconset")

	/** Package requires external encryption key [bool]. */
	DEFINE_STRINGID (kPackageExternalKeyRequired, "Package:ExternalKeyRequired")

	/** Package should be installed to shared location [bool]. */
	DEFINE_STRINGID (kPackageSharedLocation, "Package:SharedLocation")

	/** Package should be mounted hidden [bool]. */
	DEFINE_STRINGID (kPackageHidden, "Package:Hidden")
}

/*
	<MetaInformation>
		<Attribute id="Package:ID" value="com.vendor.package"/>
	</MetaInformation>
*/

//////////////////////////////////////////////////////////////////////////////////////////////////
// Class Meta Information
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Meta
{
	/** ClassID [UID]. */
	DEFINE_STRINGID (kClassID, "Class:ID")

	/** Class name [String]. */
	DEFINE_STRINGID (kClassName, "Class:Name")

	/** Class revision [int]. */
	DEFINE_STRINGID (kClassRevision, "Class:Revision")

	/** Category of the class [String]. */
	DEFINE_STRINGID (kClassCategory, "Class:Category")

	/** Subcategory of the class [String]. */
	DEFINE_STRINGID (kClassSubCategory, "Class:SubCategory")

	/** Vendor of the class [String]. */
	DEFINE_STRINGID (kClassVendor, "Class:Vendor")
		
	/** Version of the class [String]. */
	DEFINE_STRINGID (kClassVersion, "Class:Version")

	/** Class folder for sorting [String]. */
	DEFINE_STRINGID (kClassFolder, "Class:Folder")

	/** Localized class name [String]. */
	DEFINE_STRINGID (kClassLocalizedName, "Class:LocalizedName")

	/** Localized subcategory [String]. */
	DEFINE_STRINGID (kClassLocalizedSubCategory, "Class:LocalizedSubCategory")

	/** Localized class description [String]. */
	DEFINE_STRINGID (kClassLocalizedDescription, "Class:LocalizedDescription")
}

/*
	<MetaInformation>
		<Attribute id="Class:ID" value="{0C6E8453-4C58-478b-A218-3A6E0B226FF5}"/>
	</MetaInformation>
*/

//////////////////////////////////////////////////////////////////////////////////////////////////
// Translation Meta Information
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Meta
{
	/** Relative location of string table to load [String] */
	DEFINE_STRINGID (kTranslationStringTable, "Translation:StringTable")

	/** Table identifier to use when loading string table [String] */
	DEFINE_STRINGID (kTranslationTableID, "Translation:TableID")

	//** Table identifier of already loaded string table [String] */
	DEFINE_STRINGID (kTranslationSharedTableID, "Translation:SharedTableID")
}

//************************************************************************************************
// IPackageInfo
//************************************************************************************************

interface IPackageInfo: IUnknown
{
	virtual IStorable* CCL_API getResourceData (StringID id) const = 0;

	DECLARE_IID (IPackageInfo)
};

DEFINE_IID (IPackageInfo, 0x17e8c86d, 0x26ff, 0x40e6, 0x90, 0x8f, 0xac, 0x69, 0x73, 0xda, 0x2c, 0xc6)

} // namespace CCL

#endif // _ccl_ipackagemetainfo_h
