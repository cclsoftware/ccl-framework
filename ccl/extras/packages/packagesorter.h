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
// Filename    : ccl/extras/packages/packagesorter.h
// Description : Package Sorter
//
//************************************************************************************************

#ifndef _ccl_packagesorter_h
#define _ccl_packagesorter_h

#include "ccl/base/object.h"

#include "ccl/public/text/cclstring.h"

namespace CCL {
class FileType;

namespace Packages {
class UnifiedPackage;

//************************************************************************************************
// PackageSorter
/** Sorter used in conjunction with a PackageManager. */
//************************************************************************************************

class PackageSorter: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (PackageSorter, Object)

	PackageSorter (StringRef title = nullptr);

	StringRef getTitle () const;

	virtual int compare (const UnifiedPackage* lhs, const UnifiedPackage* rhs) const = 0;

protected:
	String title;
};

//************************************************************************************************
// NamePackageSorter
//************************************************************************************************

class NamePackageSorter: public PackageSorter
{
public:
	NamePackageSorter ();

	// PackageSorter
	int compare (const UnifiedPackage* lhs, const UnifiedPackage* rhs) const override;

protected:
	NamePackageSorter (StringRef title);
};

//************************************************************************************************
// FileTypePackageSorter
//************************************************************************************************

class FileTypePackageSorter: public NamePackageSorter
{
public:
	FileTypePackageSorter ();

	// NamePackageSorter
	int compare (const UnifiedPackage* lhs, const UnifiedPackage* rhs) const override;

private:
	bool getFirstFileType (FileType& fileType, const UnifiedPackage& package) const;
};

//************************************************************************************************
// TypePackageSorter
//************************************************************************************************

class TypePackageSorter: public NamePackageSorter
{
public:
	TypePackageSorter ();

	// NamePackageSorter
	int compare (const UnifiedPackage* lhs, const UnifiedPackage* rhs) const override;

private:
	bool getFirstType (String& fileType, const UnifiedPackage& package) const;
};

} // namespace Packages
} // namespace CCL

#endif // _ccl_packagesorter_h
