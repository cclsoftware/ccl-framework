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
// Filename    : ccl/system/localization/languagepack.h
// Description : Language Pack
//
//************************************************************************************************

#ifndef _ccl_languagepack_h
#define _ccl_languagepack_h

#include "ccl/system/localization/localeinfo.h"

#include "ccl/base/storage/fileresource.h"

#include "ccl/public/system/ilocalemanager.h"

namespace CCL {

interface IPackageFile;

//************************************************************************************************
// LanguagePack
//************************************************************************************************

class LanguagePack: public FileResource,
					public ILanguagePack
{
public:
	DECLARE_CLASS (LanguagePack, FileResource)

	LanguagePack (UrlRef path = Url ());
	~LanguagePack ();

	static const FileType& getFileType ();

	bool readMetaInfo ();

	const LocaleInfo& getLocaleInfo () const;
	bool getTableLocation (IUrl& path, StringID tableID) const;

	// ILanguagePack
	StringRef CCL_API getTitle () const override;
	StringID CCL_API getLanguage () const override;
	tbool CCL_API getResourceLocation (IUrl& path, StringRef resourceName) const override;
	int CCL_API getRevision () const override;

	CLASS_INTERFACE (ILanguagePack, FileResource)

protected:
	AutoPtr<LocaleInfo> localeInfo;
	StringDictionary tableMap;
	StringDictionary resourceMap;
	IPackageFile* packageFile;
	String packageID;
	int revision;

	bool getLocation (IUrl& path, StringRef subFolder, StringRef relativePath, int type) const;

	// FileResource
	bool openFile (int mode) override;
	bool createFile (int mode) override;
	bool closeFile () override;
	bool equals (const Object& obj) const override;
	int compare (const Object& obj) const override;
};

//************************************************************************************************
// LanguagePackHandler
//************************************************************************************************

class LanguagePackHandler
{
public:
	static int find (Container& packs, UrlRef folder);
};

} // namespace CCL

#endif // _ccl_languagepack_h
