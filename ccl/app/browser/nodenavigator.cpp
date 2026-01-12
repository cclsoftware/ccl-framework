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
// Filename    : ccl/app/browser/nodenavigator.cpp
// Description : Navigator for Browser Nodes
//
//************************************************************************************************

#include "ccl/app/browser/nodenavigator.h"
#include "ccl/app/browser/browsernode.h"
#include "ccl/app/browser/browser.h"

using namespace CCL;

//************************************************************************************************
// BrowserNodeNavigator
//************************************************************************************************

BrowserNodeNavigator::BrowserNodeNavigator (Browser& browser)
: NavigatorBase2 ("NodeNavigator"),
	browser (browser)
{
	browser.addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNodeNavigator::~BrowserNodeNavigator ()
{
	browser.removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserNodeNavigator::isOpen () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BrowserNodeNavigator::refresh ()
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserNodeNavigator::onNavigated ()
{
	if(BrowserNode* node = browser.findNode (MutableCString (currentUrl.getPath ()), true))
		if(node != browser.getFocusNode ())
			browser.setFocusNode (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BrowserNodeNavigator::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Browser::kNodeFocused)
	{
		MutableCString path;
		BrowserNode* node = unknown_cast<BrowserNode> (msg[0].asUnknown ());
		if(node && browser.makePath (path, node))
			navigate (Url (nullptr, nullptr, String (path)));
	}
	else
		SuperClass::notify (subject, msg);
}
