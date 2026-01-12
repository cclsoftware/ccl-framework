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
// Filename    : ccl/extras/packages/packagesorter.cpp
// Description : Package Sorter
//
//************************************************************************************************

#include "ccl/extras/packages/packagesorter.h"
#include "ccl/extras/packages/unifiedpackage.h"

#include "ccl/public/text/translation.h"

using namespace CCL;
using namespace Packages;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("PackageSorter")
	XSTRING (PackageName, "Name")
	XSTRING (PackageFileType, "File Type")
	XSTRING (PackageType, "Type")
END_XSTRINGS

//************************************************************************************************
// PackageSorter
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PackageSorter, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageSorter::PackageSorter (StringRef title)
: title (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef PackageSorter::getTitle () const
{
	return title;
}

//************************************************************************************************
// NamePackageSorter
//************************************************************************************************

NamePackageSorter::NamePackageSorter ()
: PackageSorter (XSTR (PackageName))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

NamePackageSorter::NamePackageSorter (StringRef title)
: PackageSorter (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int NamePackageSorter::compare (const UnifiedPackage* lhs, const UnifiedPackage* rhs) const
{
	StringRef leftTitle (lhs->getTitle ().isEmpty () && lhs->getChildren ().count () == 1 ? lhs->getChildren ().first ()->getTitle () : lhs->getTitle ());
	StringRef rightTitle (rhs->getTitle ().isEmpty () && rhs->getChildren ().count () == 1 ? rhs->getChildren ().first ()->getTitle () : rhs->getTitle ());
	return leftTitle.compare (rightTitle);
}

//************************************************************************************************
// FileTypePackageSorter
//************************************************************************************************

FileTypePackageSorter::FileTypePackageSorter ()
: NamePackageSorter (XSTR (PackageFileType))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileTypePackageSorter::getFirstFileType (FileType& fileType, const UnifiedPackage& package) const
{
	fileType = package.getFileType ();
	if(fileType.isValid ())
		return true;
	for(UnifiedPackage* child : package.getChildren ())
		if(getFirstFileType (fileType, *child))
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int FileTypePackageSorter::compare (const UnifiedPackage* lhs, const UnifiedPackage* rhs) const
{
	FileType lFileType;
	FileType rFileType;

	if(!getFirstFileType (lFileType, *lhs))
		return 1;
	if(!getFirstFileType (rFileType, *rhs))
		return -1;

	int result = lFileType.getExtension ().compare (rFileType.getExtension ());

	if(result == 0)
		result = NamePackageSorter::compare (lhs, rhs);

	return result;
}

//************************************************************************************************
// TypePackageSorter
//************************************************************************************************

TypePackageSorter::TypePackageSorter ()
: NamePackageSorter (XSTR (PackageType))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypePackageSorter::getFirstType (String& type, const UnifiedPackage& package) const
{
	type = package.getType ();
	if(type.isEmpty () == false)
		return true;
	for(UnifiedPackage* child : package.getChildren ())
		if(getFirstType (type, *child))
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TypePackageSorter::compare (const UnifiedPackage* lhs, const UnifiedPackage* rhs) const
{
	String lType;
	String rType;

	getFirstType (lType, *lhs);
	getFirstType (rType, *rhs);

	int result = lType.compare (rType);

	if(result == 0)
		result = NamePackageSorter::compare (lhs, rhs);

	return result;
}
