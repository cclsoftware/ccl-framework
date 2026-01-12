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
// Filename    : ccl/gui/itemviews/namenavigator.h
// Description : Name Navigator
//
//************************************************************************************************

#ifndef _ccl_namenavigator_h
#define _ccl_namenavigator_h

#include "ccl/public/gui/framework/inamenavigator.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/base/unknown.h"

namespace CCL {

struct KeyEvent;

//************************************************************************************************
// NameNavigator
//************************************************************************************************

class NameNavigator: public Unknown,
					 public INameNavigator,
					 public IdleClient
{
public:
	NameNavigator (INamedItemIterator* iterator = nullptr);

	void reset ();

	// INameNavigator
	void CCL_API init (INamedItemIterator* iterator) override;
	tbool CCL_API onKey (Variant& resultItem, const KeyEvent& event) override;

	CLASS_INTERFACE2 (INameNavigator, IdleClient, Unknown)

private:
	INamedItemIterator* iterator;
	String typedString;
	String currentName;
	Variant currentItem;
	Variant startItem;
	bool sameChars;
	
	enum { kTimeOutMs = 1000 };

	bool getNextItem ();
	bool resetItem ();

	// IdleClient
	void onIdleTimer () override;
};

} // namespace CCL

#endif // _ccl_namenavigator_h
