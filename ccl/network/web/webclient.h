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
// Filename    : ccl/network/web/webclient.h
// Description : Web Client
//
//************************************************************************************************

// THE CODE IN THIS FILE HAS BEEN MOVED PARTIALLY!
#include "ccl/extras/web/webprotocol.h"

#ifndef _ccl_webclient_h
#define _ccl_webclient_h

namespace CCL {
class Attributes;

namespace Web {

//************************************************************************************************
// WebCredentials
//************************************************************************************************

class WebCredentials: public Object,
					  public IWebCredentials
{
public:
	DECLARE_CLASS (WebCredentials, Object)

	WebCredentials ();
	~WebCredentials ();

	// IWebCredentials
	void CCL_API assign (StringRef userName, StringRef password, StringRef authType = nullptr) override;
	StringRef CCL_API getAuthType () const override { return authType; }
	StringRef CCL_API getUserName () const override { return userName; }
	StringRef CCL_API getPassword () const override { return password; }
	void CCL_API setAttributes (const IAttributeList& attributes) override;
	void CCL_API getAttributes (IAttributeList& attributes) const override;

	CLASS_INTERFACE (IWebCredentials, Object)

protected:
	String userName, password, authType;
	Attributes* attributes;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webclient_h
