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
// Filename    : ccl/system/localization/languagepack.cpp
// Description : Language Pack
//
//************************************************************************************************

#include "ccl/system/localization/languagepack.h"

#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/storage/propertyfile.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Type
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (LanguagePack, "Language Pack")
END_XSTRINGS

static FileType languagePackFileType ("Language Pack", "langpack", CCL_MIME_TYPE "-languagepack");

CCL_KERNEL_INIT_LEVEL (LanguagePack1, kFrameworkLevelFirst)
{
	System::GetFileTypeRegistry ().registerFileType (languagePackFileType);
	return true;
}

CCL_KERNEL_INIT_LEVEL (LanguagePack2, kFrameworkLevelLast)
{
	languagePackFileType.setDescription (XSTR (LanguagePack));
	System::GetFileTypeRegistry ().updateFileType (languagePackFileType);
	return true;
}

//************************************************************************************************
// LanguagePack
//************************************************************************************************

const FileType& LanguagePack::getFileType ()
{
	return languagePackFileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (LanguagePack, FileResource)

//////////////////////////////////////////////////////////////////////////////////////////////////

LanguagePack::LanguagePack (UrlRef path)
: FileResource (path),
  localeInfo (NEW LocaleInfo),
  packageFile (nullptr),
  revision (0)
{
	// case-insensitivity makes it easier to compare UIDs as string, etc.
	tableMap.setCaseSensitive (false);
	resourceMap.setCaseSensitive (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LanguagePack::~LanguagePack ()
{
	ASSERT (packageFile == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LanguagePack::equals (const Object& obj) const
{
	if(const LanguagePack* other = ccl_cast<LanguagePack> (&obj))
		return getPath ().isEqualUrl (other->getPath ()) != 0;
	else
		return SuperClass::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int LanguagePack::compare (const Object& obj) const
{
	if(const LanguagePack* other = ccl_cast<LanguagePack> (&obj))
		return getTitle ().compare (other->getTitle ());
	else
		return SuperClass::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LanguagePack::readMetaInfo ()
{
	AutoPtr<IPackageFile> pf = System::GetPackageHandler ().openPackage (path);
	if(pf == nullptr)
		return false;

	AutoPtr<PackageInfo> packageInfo = NEW PackageInfo;
	packageInfo->addResource ("LanguagePack:LocaleInfo", CCLSTR ("localeinfo.xml"), localeInfo);

	if(!packageInfo->loadFromPackage (*pf))
		return false;
	if(packageInfo->getPackageID ().isEmpty ())
		return false;
	if(!localeInfo->isValid ())
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LanguagePack::openFile (int mode)
{
	AutoPtr<IPackageFile> pf = System::GetPackageHandler ().openPackage (path);
	if(pf == nullptr)
		return false;

	AutoPtr<PackageInfo> packageInfo = NEW PackageInfo;
	packageInfo->addResource ("LanguagePack:LocaleInfo", CCLSTR ("localeinfo.xml"), localeInfo);
	AutoPtr<Java::PropertyFile> tableFile = NEW Java::PropertyFile;
	packageInfo->addResource ("LanguagePack:TableMap", CCLSTR ("tables.properties"), tableFile);
	AutoPtr<Java::PropertyFile> resourceFile = NEW Java::PropertyFile;
	packageInfo->addResource ("LanguagePack:ResourceMap", CCLSTR ("resources.properties"), resourceFile);

	if(!packageInfo->loadFromPackage (*pf))
		return false;

	packageID = packageInfo->getPackageID ();
	if(packageID.isEmpty ())
		return false;

	revision = packageInfo->getInt ("LanguagePack:Revision");

	if(!localeInfo->isValid ())
		return false;

	// mount package
	if(System::GetPackageHandler ().mountPackageVolume (pf, packageID, IPackageVolume::kHidden) != kResultOk)
		return false;

	tableMap.copyFrom (tableFile->getProperties ());
	resourceMap.copyFrom (resourceFile->getProperties ());

	packageFile = pf.detach ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LanguagePack::createFile (int mode)
{
	CCL_DEBUGGER ("Can't create language pack!\n")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LanguagePack::closeFile ()
{
	// unmount package
	if(packageFile)
	{
		System::GetPackageHandler ().unmountPackageVolume (packageFile);
		safe_release (packageFile);
	}

	packageID.empty ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API LanguagePack::getTitle () const
{
	return localeInfo->getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API LanguagePack::getLanguage () const
{
	return localeInfo->getLanguage ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API LanguagePack::getRevision () const
{
	return revision;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const LocaleInfo& LanguagePack::getLocaleInfo () const
{
	return *localeInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LanguagePack::getLocation (IUrl& path, StringRef subFolder, StringRef relativePath, int type) const
{
	if(packageFile == nullptr)
		return false;

	PackageUrl p (packageID, subFolder, Url::kFolder);
	p.descend (relativePath, type);

	path.assign (p);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LanguagePack::getTableLocation (IUrl& path, StringID _tableID) const
{
	String tableID (_tableID);
	ASSERT (!tableID.isEmpty ())
	String relativePath = tableMap.lookupValue (tableID);

	if(relativePath.isEmpty ()) // fall back to table identifier
		relativePath = tableID;

	if(getLocation (path, CCLSTR ("tables"), relativePath, Url::kFolder))
		if(System::GetFileSystem ().fileExists (path)) // check if folder exists inside package
			return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LanguagePack::getResourceLocation (IUrl& path, StringRef resourceName) const
{
	ASSERT (!resourceName.isEmpty ())
	String relativePath = resourceMap.lookupValue (resourceName);
	if(!relativePath.isEmpty ())
	{
		if(getLocation (path, CCLSTR ("resources"), relativePath, Url::kFile))
			if(System::GetFileSystem ().fileExists (path))
				return true;
	}
	return false;
}

//************************************************************************************************
// LanguagePackHandler
//************************************************************************************************

int LanguagePackHandler::find (Container& packs, UrlRef folder)
{
	int count = 0;
	ForEachFile (System::GetFileSystem ().newIterator (folder), path)
		if(path->getFileType () == LanguagePack::getFileType () || System::GetPackageHandler ().isPackage (*path))
		{
			AutoPtr<LanguagePack> pack = NEW LanguagePack (*path);
			if(packs.contains (*pack)) // check if already in list
			{
				count++; // outer instance needs to know that we found something
				continue;
			}

			if(pack->readMetaInfo ())
			{
				count++;
				packs.add (pack.detach ());
			}
		}
	EndFor
	return count;
}
