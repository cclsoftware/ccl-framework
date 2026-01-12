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
// Filename    : ccl/app/components/searchprovider.h
// Description : Search Provider
//
//************************************************************************************************

#ifndef _ccl_searchprovider_h
#define _ccl_searchprovider_h

#include "ccl/app/components/isearchprovider.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/gui/graphics/iimage.h"

namespace CCL {

//************************************************************************************************
// SearchProvider
//************************************************************************************************

class SearchProvider: public Object,
					  public ISearchProvider
{
public:
	DECLARE_CLASS (SearchProvider, Object)

	SearchProvider (UrlRef startPoint = Url ());

	void setTitle (StringRef _title) { title = _title; }
	void setSearchIcon (IImage* _searchIcon) { searchIcon = _searchIcon; }

	// ISearchProvider
	StringRef getTitle () const override { return title; }
	UrlRef getStartPoint () const override { return startPoint; }
	IImage* getSearchIcon () const override { return searchIcon; }
	ISearcher* createSearcher (ISearchDescription& description) override;
	IUrlFilter* getSearchResultFilter () const override;
	IUnknown* customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem) override;

	CLASS_INTERFACE (ISearchProvider, Object)

protected:
	String title;
	Url startPoint;
	SharedPtr<IImage> searchIcon;
};

//************************************************************************************************
// MultiSearchProvider
/** Combines multiple search providers. getStartPoint () has no meaning. */
//************************************************************************************************

class MultiSearchProvider: public SearchProvider
{
public:
	DECLARE_CLASS (MultiSearchProvider, SearchProvider)

	MultiSearchProvider ();

	void addSearchProvider (ISearchProvider* provider); ///< takes ownership

	PROPERTY_SHARED_AUTO (IUrlFilter, urlFilter, UrlFilter)

	// ISearchProvider
	ISearcher* createSearcher (ISearchDescription& description) override;
	IUrlFilter* getSearchResultFilter () const override;
	IUnknown* customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem) override;

private:
	UnknownList searchProviders;
};

//************************************************************************************************
// MultiSearcher
//************************************************************************************************

class MultiSearcher: public Object,
					 public ISearcher
{
public:
	void addSearcher (ISearcher* searcher);

	// ISearcher
	tresult CCL_API find (ISearchResultSink& resultSink, IProgressNotify* progress) override;

	CLASS_INTERFACE (ISearcher, Object)

private:
	UnknownList searchers;
};

} // namespace CCL

#endif // _ccl_searchprovider_h
