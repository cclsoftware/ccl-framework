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
// Filename    : ccl/public/network/web/iwebprotocol.h
// Description : Web Protocol Interface
//
//************************************************************************************************

#ifndef _ccl_iwebprotocol_h
#define _ccl_iwebprotocol_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
namespace Web {

interface IWebClient;

//////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUG_CATEGORY_WEBCLIENTPROTOCOL "WebClientProtocol"

//************************************************************************************************
// IWebClientProtocol
/** Web protocol handler interface. */
//************************************************************************************************

interface IWebClientProtocol: IUnknown
{
	/** Get protocol identifier. */
	virtual StringID CCL_API getProtocol () const = 0;

	/** Create new client instance. */
	virtual IWebClient* CCL_API createClient () = 0;

	DECLARE_IID (IWebClientProtocol)
};

DEFINE_IID (IWebClientProtocol, 0x9ebefd7a, 0x2826, 0x40fb, 0xb2, 0x46, 0x22, 0xc1, 0xce, 0x8c, 0x83, 0x3b)

//************************************************************************************************
// IWebProtocolRegistrar
/** Web protocol registration. */
//************************************************************************************************

interface IWebProtocolRegistrar: IUnknown
{
	/** Register web protocol. */
	virtual tresult CCL_API registerProtocol (IWebClientProtocol* protocol) = 0;

	/** Unregister web protocol. */
	virtual tresult CCL_API unregisterProtocol (IWebClientProtocol* protocol) = 0;

	/** Instantiate and register all web protocols in PLUG_CATEGORY_WEBCLIENTPROTOCOL. */
	virtual tresult CCL_API registerProtocolPlugins () = 0;

	/** Uregister all web protocol plug-ins. */
	virtual tresult CCL_API unregisterProtocolPlugins () = 0;

	DECLARE_IID (IWebProtocolRegistrar)
};

DEFINE_IID (IWebProtocolRegistrar, 0x5f65bf56, 0xe061, 0x485c, 0x93, 0xf2, 0xf, 0xcd, 0xe, 0x7d, 0xab, 0x58)

} // namespace Web
} // namespace CCL

#endif // _ccl_iwebprotocol_h
