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
// Filename    : ccl/network/webfs/webfilesearcher.h
// Description : WebFS Searcher
//
//************************************************************************************************

#ifndef _ccl_webfilesearcher_h
#define _ccl_webfilesearcher_h

#include "ccl/base/storage/url.h"

#include "ccl/public/system/isearcher.h"

namespace CCL {
namespace Web {

interface IWebFileClient;

//************************************************************************************************
// FileSearcher
//************************************************************************************************

class FileSearcher: public Object,
					public ISearcher
{
public:
	FileSearcher (ISearchDescription& description);
	~FileSearcher ();

	// ISearcher
	tresult CCL_API find (ISearchResultSink& resultSink, IProgressNotify* progress) override;

	CLASS_INTERFACE (ISearcher, Object)

protected:
	ISearchDescription& description;

	tresult findInFolder (IWebFileClient& client, StringRef volumeName, StringRef remotePath,
						  ISearchResultSink& resultSink, IProgressNotify* progress);
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webfilesearcher_h
