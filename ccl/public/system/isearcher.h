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
// Filename    : ccl/public/system/isearcher.h
// Description : Search Interfaces
//
//************************************************************************************************

#ifndef _ccl_isearcher_h
#define _ccl_isearcher_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IUnknownList;
interface IProgressNotify;
interface ISearchResultSink;

//************************************************************************************************
// ISearchDescription
//************************************************************************************************

interface ISearchDescription: IUnknown
{
	enum Options
	{
		kMatchCase = 1<<0,
		kMatchWholeWord = 1<<1,
		kIgnoreDelimiters = 1<<2,		///< certain delimiter characters like '-' should be ignored when matching strings
		kAllowTokenGrouping = 1<<3,		///< when a delimiter is used to tokenize the search terms, the character '"' can be used to suspend the tokenizing
		kMatchAllTokens = 1<<4			///< all tokens must match when a delimiter is used to tokenize the search terms
	};

	virtual UrlRef CCL_API getStartPoint () const = 0;

	virtual StringRef CCL_API getSearchTerms () const = 0;

	virtual tbool CCL_API matchesName (StringRef name) const = 0;

	virtual int CCL_API getPaginationOffset () const = 0;

	virtual int CCL_API getOptions () const = 0;
	
	virtual int CCL_API getSearchTokenCount () const = 0;
	
	virtual StringRef CCL_API getSearchToken (int index) const = 0;
	
	virtual StringRef CCL_API getTokenDelimiter () const = 0;
	
	DECLARE_IID (ISearchDescription)
};

DEFINE_IID (ISearchDescription, 0xcc450ad, 0x3c9d, 0x4f78, 0xb2, 0x32, 0xde, 0x38, 0xd, 0xe1, 0xc1, 0x7e)

//************************************************************************************************
// ISearcher
//************************************************************************************************

interface ISearcher: IUnknown
{
	virtual tresult CCL_API find (ISearchResultSink& resultSink, IProgressNotify* progress) = 0;

	DECLARE_IID (ISearcher)
};

DEFINE_IID (ISearcher, 0x84421a01, 0x422, 0x46fe, 0xb4, 0x64, 0x4, 0xf4, 0xec, 0xff, 0x7d, 0xab)

//************************************************************************************************
// ISearchResultSink
//************************************************************************************************

interface ISearchResultSink: IUnknown
{
	/** Add one result item. */
	virtual tresult CCL_API addResult (IUnknown* item) = 0;

	/** Add multiple result items at once. */
	virtual tresult CCL_API addResults (const IUnknownList& items) = 0;

	/** Enable search result pagination. */
	virtual void CCL_API setPaginationNeeded (tbool state) = 0;

	DECLARE_IID (ISearchResultSink)
};

DEFINE_IID (ISearchResultSink, 0xC6473D6A, 0x35F4, 0x44E8, 0xBF, 0x7F, 0x3A, 0xBA, 0x84, 0xE2, 0x73, 0xD2)

//************************************************************************************************
// AbstractSearcher
//************************************************************************************************

class AbstractSearcher: public ISearcher
{
public:
	AbstractSearcher (ISearchDescription& searchDescription);
	~AbstractSearcher ();

protected:
	ISearchDescription& searchDescription;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline AbstractSearcher::AbstractSearcher (ISearchDescription& searchDescription)
: searchDescription (searchDescription) { searchDescription.retain (); }

inline AbstractSearcher::~AbstractSearcher ()
{ searchDescription.release (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_isearcher_h
