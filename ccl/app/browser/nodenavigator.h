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
// Filename    : ccl/app/browser/nodenavigator.h
// Description : Navigator for Browser Nodes
//
//************************************************************************************************

#ifndef _ccl_nodenavigator_h
#define _ccl_nodenavigator_h

#include "ccl/app/navigation/navigatorbase.h"

namespace CCL {

class Browser;

//************************************************************************************************
// BrowserNodeNavigator
/** Manages the focus node history of a browser. */
//************************************************************************************************

class BrowserNodeNavigator: public NavigatorBase2
{
public:
	BrowserNodeNavigator (Browser& browser);
	~BrowserNodeNavigator ();

	bool isOpen () const override;
	void onNavigated () override;
	tresult CCL_API refresh () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	Browser& browser;
};

} // namespace CCL

#endif // _ccl_nodenavigator_h
