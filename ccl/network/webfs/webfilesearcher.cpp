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
// Filename    : ccl/network/webfs/webfilesearcher.cpp
// Description : WebFS Searcher
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/network/webfs/webfilesearcher.h"
#include "ccl/network/webfs/webfileservice.h"
#include "ccl/network/webfs/webfilesystem.h"

#include "ccl/base/collections/stringdictionary.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/network/web/iwebfiletask.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// FileSearcher
//************************************************************************************************

FileSearcher::FileSearcher (ISearchDescription& description)
: description (description)
{
	description.retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileSearcher::~FileSearcher ()
{
	description.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileSearcher::find (ISearchResultSink& resultSink, IProgressNotify* progress)
{
	ASSERT (!System::IsInMainThread ())
	if(System::IsInMainThread ())
		return kResultWrongThread;

	CCL_PRINT ("----> Web File Search for ")
	CCL_PRINTLN (description.getSearchTerms ())

	// open remote session
	Url webfsUrl (description.getStartPoint ());
	AutoPtr<IRemoteSession> session = WebFileService::instance ().openSession (webfsUrl);
	if(!session)
		return kResultFailed;

	String remotePath;
	session->getRemotePath (remotePath, webfsUrl);
	String volumeName (webfsUrl.getHostName ());

	// check if server supports customized search
	UnknownPtr<IWebFileSearchClient> searchClient (&session->getClient ());
	if(searchClient)
	{
		StringDictionary query;
		query.setEntry (String (IWebFileSearchClient::kSearchTerms), description.getSearchTerms ());
		query.setEntry (String (IWebFileSearchClient::kPaginationOffset), String () << description.getPaginationOffset ());

		AutoPtr<IWebFileSearchClient::IResultIterator> iter = searchClient->search (remotePath, query, progress);
		if(!iter)
		{
			if(progress && progress->isCanceled ())
				return kResultAborted;
			return kResultFailed;
		}

		// insert search result
		UnknownList outItems;
		VolumeHandler& handler = WebFileService::instance ().getVolumes ();
		AutoPtr<Volume> volume = handler.openVolume (volumeName);
		ASSERT (volume)
		if(volume)
			volume->getFs ()->insertSearchResult (*iter, &outItems);

		// feed sink
		if(!outItems.isEmpty ())
		{
			resultSink.addResults (outItems);

			// TODO: check via result iterator?
			resultSink.setPaginationNeeded (true);
		}

		return kResultOk;
	}
	else
	{
		// generic search via directory listing
		return findInFolder (session->getClient (), volumeName, remotePath, resultSink, progress);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult FileSearcher::findInFolder (IWebFileClient& client, StringRef volumeName, StringRef remotePath, ISearchResultSink& resultSink, IProgressNotify* progress)
{
	AutoPtr<IWebFileClient::IDirIterator> iter = client.openDirectory (remotePath, progress);
	if(iter)
	{
		const IWebFileClient::DirEntry* entry = nullptr;
		for(int index = 0; entry = iter->getEntry (index); index++)
		{
			if(progress && progress->isCanceled ())
				return kResultAborted;

			if(entry->directory)
			{
				String subPath (remotePath);
				subPath << entry->name << Url::strPathChar;

				tresult tr = findInFolder (client, volumeName, subPath, resultSink, progress);
				if(tr != kResultOk)
					return tr;
			}
			else
			{
				if(description.matchesName (entry->name))
				{
					// insert search result
					IUnknown* outItem = nullptr;
					VolumeHandler& handler = WebFileService::instance ().getVolumes ();
					AutoPtr<Volume> volume = handler.openVolume (volumeName);
					ASSERT (volume)
					if(volume)
					{						
						IWebFileClient::DirEntry resultEntry;
						resultEntry = *entry;
						resultEntry.name.prepend (remotePath);
						IUnknown* object = iter->getObject (index);

						volume->getFs ()->insertSearchResult (resultEntry, object, &outItem);
					}

					// feed sink
					if(outItem)
						resultSink.addResult (outItem);
				}
			}
		}
	}
	return kResultOk;
}
