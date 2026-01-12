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
// Filename    : ccl/public/netservices.h
// Description : Networking Service APIs
//
//************************************************************************************************

#ifndef _ccl_netservices_h
#define _ccl_netservices_h

#include "ccl/public/cclexports.h"
#include "ccl/public/network/inetwork.h"

namespace CCL {

namespace Net {
interface IDiscoveryHandler; }

namespace Web {
interface IWebService; 
interface IWebFileService; 
interface ITransferManager; }

namespace System {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Network Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Returns the Network singleton. */
CCL_EXPORT Net::INetwork& CCL_API CCL_ISOLATED (GetNetwork) ();
inline Net::INetwork& GetNetwork () { return CCL_ISOLATED (GetNetwork) (); }

/** Returns the DNSSD handler singleton. */
CCL_EXPORT Net::IDiscoveryHandler& CCL_API CCL_ISOLATED (GetDiscoveryHandler) ();
inline Net::IDiscoveryHandler& GetDiscoveryHandler () { return CCL_ISOLATED (GetDiscoveryHandler) (); }

/** Returns the Web Service singleton. */
CCL_EXPORT Web::IWebService& CCL_API CCL_ISOLATED (GetWebService) ();
inline Web::IWebService& GetWebService () { return CCL_ISOLATED (GetWebService) (); }

/** Returns the Web File Service singleton. */
CCL_EXPORT Web::IWebFileService& CCL_API CCL_ISOLATED (GetWebFileService) ();
inline Web::IWebFileService& GetWebFileService () { return CCL_ISOLATED (GetWebFileService) (); }

/** Returns the Transfer Manager singleton. */
CCL_EXPORT Web::ITransferManager& CCL_API CCL_ISOLATED (GetTransferManager) ();
inline Web::ITransferManager& GetTransferManager () { return CCL_ISOLATED (GetTransferManager) (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

#if CCL_STATIC_LINKAGE
/** Network Framework Initialization. */
tbool CCL_API InitializeNetworkFramework (tbool state);
#endif

} // namespace System
} // namespace CCL

#endif // _ccl_netservices_h
