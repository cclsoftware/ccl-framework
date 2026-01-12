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
// Filename    : ccl/public/gui/inavigator.h
// Description : Navigator Interface
//
//************************************************************************************************

#ifndef _ccl_inavigator_h
#define _ccl_inavigator_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface INavigationHistory;
interface INavigationHistoryEntry;

//************************************************************************************************
// INavigator
/** Basic navigator interface. 
	\ingroup gui */
//************************************************************************************************

interface INavigator: IUnknown
{
	/** Navigate to new location. */
	virtual tresult CCL_API navigate (UrlRef url) = 0;

	/** Navigate to new location deferred. */
	virtual tresult CCL_API navigateDeferred (UrlRef url) = 0;

	/** Refresh current location. */
	virtual tresult CCL_API refresh () = 0;

	/** Get URL of current location. */
	virtual UrlRef CCL_API getCurrentUrl () const = 0;

	/** Get title of current location. */
	virtual StringRef CCL_API getCurrentTitle () const = 0;

	/** Navigate to previous location. */
	virtual tresult CCL_API goBack () = 0;

	/** Navigate to next location. */
	virtual tresult CCL_API goForward () = 0;

	/** Check if backward navigation possible. */
	virtual tbool CCL_API canGoBack () const = 0;

	/** Check if forward navigation possible. */
	virtual tbool CCL_API canGoForward () const = 0;

	/** Navigate to home location. */
	virtual tresult CCL_API goHome () = 0;
	
	/** Get URL of home location. */
	virtual UrlRef CCL_API getHomeUrl () const = 0;

	DECLARE_IID (INavigator)
};

DEFINE_IID (INavigator, 0xa3640517, 0x1124, 0x478d, 0x98, 0xfc, 0xe5, 0x42, 0x48, 0x3, 0x5c, 0x16)

//************************************************************************************************
// INavigator2
/** Extension to INavigator interface. */
//************************************************************************************************

interface INavigator2: IUnknown
{
	/** Get backward history. */
	virtual INavigationHistory& CCL_API getBackwardHistory () const = 0;

	/** Get forward history. */
	virtual INavigationHistory& CCL_API getForwardHistory () const = 0;

	DECLARE_IID (INavigator2)
};

DEFINE_IID (INavigator2, 0xd27d6450, 0xdf07, 0x4a57, 0xb2, 0x49, 0x67, 0x8c, 0x4e, 0x92, 0x71, 0xd0)

//************************************************************************************************
// INavigationHistory
/** Navigation history interface. */
//************************************************************************************************

interface INavigationHistory: IUnknown
{
	/** Get number of entries. */
	virtual int CCL_API countEntries () const = 0;

	/** Get entry at given index. */
	virtual const INavigationHistoryEntry* CCL_API getEntry (int index) const = 0;

	/** Get top-most entry. */
	virtual const INavigationHistoryEntry* CCL_API peekEntry () const = 0;

	DECLARE_IID (INavigationHistory)
};

DEFINE_IID (INavigationHistory, 0xbdc6a4b3, 0x54fc, 0x42c2, 0xa2, 0xb4, 0xf7, 0xe7, 0x55, 0x9c, 0xe1, 0x37)

//************************************************************************************************
// INavigationHistoryEntry
/** Entry in navigation history. */
//************************************************************************************************

interface INavigationHistoryEntry: IUnknown
{
	/** Get URL. */
	virtual UrlRef CCL_API getUrl () const = 0;

	/** Get title. */
	virtual StringRef CCL_API getTitle () const = 0;

	DECLARE_IID (INavigationHistoryEntry)
};

DEFINE_IID (INavigationHistoryEntry, 0xf90c21b3, 0xe82a, 0x4c08, 0xbd, 0xdd, 0x8, 0xcc, 0xf7, 0xe0, 0xcc, 0xef)

} // namespace CCL

#endif // _ccl_inavigator_h
