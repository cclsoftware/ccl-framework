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
// Filename    : ccl/extras/tools/repositoryinfo.h
// Description : Repository Info
//
//************************************************************************************************

#ifndef _repositoryinfo_h
#define _repositoryinfo_h

#include "ccl/base/collections/objectarray.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/text/cstring.h"

namespace CCL {
class Url;

//************************************************************************************************
// RepositoryInfo
//************************************************************************************************

class RepositoryInfo
{
public:
	RepositoryInfo ();
	~RepositoryInfo ();
	
	static const String kFileName;

	DECLARE_STRINGID_MEMBER (kSubmoduleDirectories)
	DECLARE_STRINGID_MEMBER (kTemplateDirectories)
	DECLARE_STRINGID_MEMBER (kIdentityDirectories)
	DECLARE_STRINGID_MEMBER (kClassModelDirectories)
	DECLARE_STRINGID_MEMBER (kDocumentationDirectories)
	DECLARE_STRINGID_MEMBER (kSkinDirectories)
	DECLARE_STRINGID_MEMBER (kSigningDirectories)
	DECLARE_STRINGID_MEMBER (kTranslationsDirectories)
	
	bool load (UrlRef startFolder, bool reload = false);
	bool getPaths (Container& paths, StringID category) const;
	bool findPath (Url& path, StringID category, StringRef innerPath) const;
	bool findAllPaths (Container& paths, StringID category, StringRef innerPath) const;

	UrlRef getRootDirectory () const;

private:	
	Url rootDirectory;

	struct Entry
	{
		CString category;
		ObjectArray paths;
	};
	Vector<Entry> entries;

	void removeEntries ();
};

} // namespace CCL

#endif // _repositoryinfo_h
