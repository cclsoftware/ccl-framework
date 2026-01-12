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
// Filename    : ccl/public/gui/framework/inamenavigator.h
// Description : Name Navigator Interface
//
//************************************************************************************************

#ifndef _ccl_inamenavigator_h
#define _ccl_inamenavigator_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

struct KeyEvent;
struct Variant;
class String;

//************************************************************************************************
// INamedItemIterator
/** Used by INameNavigator for iterating the items. 
	\ingroup gui */
//************************************************************************************************

interface INamedItemIterator: IUnknown
{
	/** Get the item to start searching with. This could be the first item in a list, or the item after the current focus item. */
	virtual tbool CCL_API getStartItem (Variant& item, String& name) = 0;

	/** Get the item after the given item. Should wrap-around at the end.*/
	virtual tbool CCL_API getNextItem (Variant& item, String& name) = 0;

	DECLARE_IID (INamedItemIterator)
};

DEFINE_IID (INamedItemIterator, 0xF5FD59AC, 0x102F, 0x432D, 0xB8, 0x28, 0xF8, 0x54, 0xA2, 0x86, 0xC8, 0x00)

//************************************************************************************************
// INameNavigator
/** 
	\ingroup gui */
//************************************************************************************************

interface INameNavigator: IUnknown
{
	/** Set item iterator. Iterator is not shared! */
	virtual void CCL_API init (INamedItemIterator* iterator) = 0;

	/** Feed a key. If it returns true, resultItem is the item the user has navigated to. */
	virtual tbool CCL_API onKey (Variant& resultItem, const KeyEvent& event) = 0;

	DECLARE_IID (INameNavigator)
};

DEFINE_IID (INameNavigator, 0x9FEECB54, 0xC2F5, 0x4D22, 0xA8, 0xDF, 0x0C, 0xBB, 0x80, 0x43, 0x93, 0x26)

} // namespace CCL

#endif // _ccl_inamenavigator_h
