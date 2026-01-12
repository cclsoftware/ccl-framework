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
// Filename    : ccl/app/components/isearchprovider.h
// Description : Search Provider Interfaces
//
//************************************************************************************************

#ifndef _ccl_isearchprovider_h
#define _ccl_isearchprovider_h

#include "ccl/public/system/isearcher.h"

#include "ccl/public/gui/graphics/rect.h"

namespace CCL {

interface IView;
interface IUrlFilter;
interface IImage;
class ListViewItem;

//************************************************************************************************
// ISearchProvider
//************************************************************************************************

interface ISearchProvider: IUnknown
{
	virtual StringRef getTitle () const = 0;

	virtual UrlRef getStartPoint () const = 0;

	virtual IImage* getSearchIcon () const = 0;

	virtual ISearcher* createSearcher (ISearchDescription& description) = 0;

	virtual IUrlFilter* getSearchResultFilter () const = 0;

	struct CustomizeArgs
	{
		ListViewItem& presentation;	//< e.g. title, icon
		String& resultCategory;		//< e.g. displayed as folder in result list
		String& sortString;			//< customize order of results (inside category); results with same sortString are ordered by url

		CustomizeArgs (ListViewItem& presentation, String& resultCategory, String& sortString)
		: presentation (presentation), resultCategory (resultCategory), sortString (sortString)
		{}
	};

	virtual IUnknown* customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem) = 0;

	DECLARE_IID (ISearchProvider)
};

//************************************************************************************************
// ISearchResultViewer
//************************************************************************************************

interface ISearchResultViewer: IUnknown
{
	virtual bool isViewVisible () = 0;

	virtual IView* createView (const Rect& bounds) = 0;

	virtual void onSearchStart (ISearchDescription& description, ISearchProvider* searchProvider) = 0;
	
	virtual void onSearchEnd (bool canceled) = 0;
	
	virtual void onResultItemsAdded (const IUnknownList& items) = 0;

	DECLARE_STRINGID_MEMBER (kCloseViewer) ///< result viewer wants to be closed

	DECLARE_IID (ISearchResultViewer)
};

} // namespace CCL

#endif // _ccl_isearchprovider_h
