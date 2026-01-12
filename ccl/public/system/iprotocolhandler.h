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
// Filename    : ccl/public/system/iprotocolhandler.h
// Description : Protocol Handler
//
//************************************************************************************************

#ifndef _ccl_iprotocolhandler_h
#define _ccl_iprotocolhandler_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IFileSystem;

//************************************************************************************************
// IProtocolHandler
/** Protocol handler interface. 
	\ingroup ccl_system */
//************************************************************************************************

interface IProtocolHandler: IUnknown
{
	/** Get protocol string. */
	virtual StringRef CCL_API getProtocol () const = 0;

	/** Get file system root object by name. */
	virtual IFileSystem* CCL_API getMountPoint (StringRef name) = 0;

	DECLARE_IID (IProtocolHandler)
};

DEFINE_IID (IProtocolHandler, 0x538d3947, 0x4bc2, 0x4ca4, 0xa2, 0x96, 0x4, 0x20, 0xbb, 0x6, 0xf3, 0x5)

//************************************************************************************************
// IProtocolHandlerRegistry
/** Registration interface for protocol handlers. 
	\ingroup ccl_system */
//************************************************************************************************

interface IProtocolHandlerRegistry: IUnknown
{
	/** Register handler for given protocol. Handler object will be shared by registry. */
	virtual tresult CCL_API registerProtocol (IProtocolHandler* handler) = 0;

	/** Unregister protocol handler. */
	virtual tresult CCL_API unregisterProtocol (IProtocolHandler* handler) = 0;

	/** Get handler for given protocol. */
	virtual IProtocolHandler* CCL_API getHandler (StringRef protocol) const = 0;

	DECLARE_IID (IProtocolHandlerRegistry)
};

DEFINE_IID (IProtocolHandlerRegistry, 0x1a4a8ee1, 0xb464, 0x468f, 0x98, 0xcd, 0xcc, 0x30, 0x48, 0x8b, 0x18, 0xba)

} // namespace CCL

#endif // _ccl_iprotocolhandler_h
