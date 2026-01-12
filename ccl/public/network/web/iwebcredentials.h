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
// Filename    : ccl/public/network/web/iwebcredentials.h
// Description : Web Credentials Interface
//
//************************************************************************************************

#ifndef _ccl_iwebcredentials_h
#define _ccl_iwebcredentials_h

#include "ccl/public/text/cclstring.h"

namespace CCL {

interface IAttributeList;

namespace Web {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Web Authentication Types
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Meta
{
	DEFINE_STRINGID (kBasic, "Basic")
	DEFINE_STRINGID (kBearer, "Bearer")
	DEFINE_STRINGID (kOAuth, "OAuth")
}

//************************************************************************************************
// IWebCredentials
//************************************************************************************************

interface IWebCredentials: IUnknown
{
	/** Assign credentials. */
	virtual void CCL_API assign (StringRef userName, StringRef password, StringRef authType = nullptr) = 0;

	/** Get authentication type. */
	virtual StringRef CCL_API getAuthType () const = 0;

	/** Get user name. */
	virtual StringRef CCL_API getUserName () const = 0;

	/** Get password. */
	virtual StringRef CCL_API getPassword () const = 0;

	/** Set additional attributes. */
	virtual void CCL_API setAttributes (const IAttributeList& attributes) = 0;

	/** Get additional attributes. */
	virtual void CCL_API getAttributes (IAttributeList& attributes) const = 0;

	DECLARE_IID (IWebCredentials)
};

DEFINE_IID (IWebCredentials, 0xdd0d1cbe, 0xcac7, 0x49f6, 0x9f, 0x4f, 0x4, 0x0, 0x58, 0x71, 0x7c, 0x91)

} // namespace Web
} // namespace CCL

#endif // _ccl_iwebcredentials_h
