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
// Filename    : ccl/app/components/searchprovider.cpp
// Description : Search Provider
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/components/searchprovider.h"

#include "ccl/base/storage/file.h"

#include "ccl/public/base/iprogress.h"

using namespace CCL;

//************************************************************************************************
// SearchProvider
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SearchProvider, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

SearchProvider::SearchProvider (UrlRef startPoint)
: startPoint (startPoint)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* SearchProvider::createSearcher (ISearchDescription& description)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrlFilter* SearchProvider::getSearchResultFilter () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* SearchProvider::customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem)
{
	return nullptr;
}

//************************************************************************************************
// MultiSearcher
//************************************************************************************************

void MultiSearcher::addSearcher (ISearcher* searcher)
{
	searchers.add (searcher);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MultiSearcher::find (ISearchResultSink& resultSink, IProgressNotify* progress)
{
	tresult result = kResultFalse;

	ForEachUnknown (searchers, unk)
		if(progress && progress->isCanceled ())
			return kResultAborted;

		UnknownPtr<ISearcher> searcher (unk);
		if(searcher)
			if(searcher->find (resultSink, progress) == kResultOk)
				result = kResultOk;
	EndFor
	return result;
}

//************************************************************************************************
// MultiSearchProvider
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MultiSearchProvider, SearchProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiSearchProvider::MultiSearchProvider ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiSearchProvider::addSearchProvider (ISearchProvider* provider)
{
	searchProviders.add (provider);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* MultiSearchProvider::createSearcher (ISearchDescription& description)
{
	MultiSearcher* multiSearcher = NEW MultiSearcher;

	ForEachUnknown (searchProviders, unk)
		UnknownPtr<ISearchProvider> provider (unk);
		ASSERT (provider)
		if(provider)
		{
			AutoPtr<SearchDescription> desc (SearchDescription::create (provider->getStartPoint (), description.getSearchTerms (), description.getOptions (), description.getTokenDelimiter ()));
			if(ISearcher* searcher = provider->createSearcher (*desc))
				multiSearcher->addSearcher (searcher);
		}
	EndFor

	return multiSearcher;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrlFilter* MultiSearchProvider::getSearchResultFilter () const
{
	return urlFilter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* MultiSearchProvider::customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem)
{
	AutoPtr<IUnknown> dragObject;
	ForEachUnknown (searchProviders, unk)
		UnknownPtr<ISearchProvider> provider (unk);
		if(provider)
			dragObject = provider->customizeSearchResult (args, resultItem);
	EndFor
	return dragObject.detach ();
}
