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
// Filename    : ccl/network/web/webclient.cpp
// Description : Web Client
//
//************************************************************************************************

#include "ccl/base/storage/attributes.h"

// THE CODE IN THIS FILE HAS BEEN MOVED PARTIALLY!
#include "ccl/extras/web/webprotocol.cpp"

#include "ccl/network/web/webclient.h"

//************************************************************************************************
// WebCredentials
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebCredentials, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebCredentials::WebCredentials ()
: attributes (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebCredentials::~WebCredentials ()
{
	safe_release (attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebCredentials::assign (StringRef _userName, StringRef _password, StringRef _authType)
{
	userName = _userName;
	password = _password;
	authType = _authType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebCredentials::setAttributes (const IAttributeList& a)
{
	if(attributes == nullptr)
		attributes = NEW Attributes;
	attributes->copyFrom (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebCredentials::getAttributes (IAttributeList& a) const
{
	if(attributes)
		a.copyFrom (*attributes);
	else
		a.removeAll ();
}
