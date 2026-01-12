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
// Filename    : ccl/public/network/inetdiscovery.h
// Description : Network DNSSD Interfaces
//
//************************************************************************************************

#ifndef _ccl_inetdiscovery_h
#define _ccl_inetdiscovery_h

#include "ccl/public/network/isocket.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {
interface IObserver;
interface ICStringDictionary;

namespace Net {

//************************************************************************************************
// ServiceBasicInformation
//************************************************************************************************

struct ServiceBasicInformation
{
	String name;				///< service name available via DNSSD
	MutableCString type;		///< service type (application-specific)
	ProtocolType protocol;		///< TCP or UDP
	String hostname;			///< host name
	PortNumber port;			///< port number

	ServiceBasicInformation ()
	: protocol (kTCP),
	  port (0)
	{}
};

//************************************************************************************************
// IServiceDescriptor
//************************************************************************************************

interface IServiceDescriptor: IUnknown
{
	/** Get basic service information. */
	virtual const ServiceBasicInformation& CCL_API getBasicInformation () const = 0;

	/** Get text record. */
	virtual const ICStringDictionary& CCL_API getTextRecord () const = 0;

	DECLARE_IID (IServiceDescriptor)
};

DEFINE_IID (IServiceDescriptor, 0xff8e73dd, 0x5a02, 0x45b0, 0xb7, 0xc3, 0x9e, 0xd3, 0xe1, 0x25, 0x20, 0x2d)

//************************************************************************************************
// IDiscoveryHandler
/** 
	Threading Policy: 
	Browsing/Registration methods must be called from main thread only, otherwise kResultWrongThread 
	is returned. All notification messages are deferred to the main thread.
*/
//************************************************************************************************

interface IDiscoveryHandler: IUnknown
{
	/** Check if DNSSD daemon is installed on client computer. */
	virtual tbool CCL_API isInstalled () = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Service Browsing
	//////////////////////////////////////////////////////////////////////////////////////////////

	DECLARE_STRINGID_MEMBER (kServiceResolved)	///< args[0]: IServiceDescriptor
	DECLARE_STRINGID_MEMBER (kServiceRemoved)	///< args[0]: IServiceDescriptor

	/** Start browsing for given service type, observer will receive notification messages. */
	virtual tresult CCL_API startBrowsing (IObserver* observer, StringID type, ProtocolType protocol) = 0;

	/** Stop browsing for given observer. */
	virtual tresult CCL_API stopBrowsing (IObserver* observer) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Service Registration
	//////////////////////////////////////////////////////////////////////////////////////////////

	DECLARE_STRINGID_MEMBER (kServiceRegistered)			///< args[0]: IServiceDescriptor
	DECLARE_STRINGID_MEMBER (kServiceRegistrationFailed)	///< args[0]: IServiceDescriptor

	/** Create service with given information and optional text record. */
	virtual IServiceDescriptor* CCL_API createService (const ServiceBasicInformation& info, const ICStringDictionary* textRecord = nullptr) = 0;

	/** Register service, observer will receive notification messages.  */
	virtual tresult CCL_API registerService (IServiceDescriptor* service, IObserver* observer) = 0;

	/** Unregister service. */
	virtual tresult CCL_API unregisterService (IServiceDescriptor* service) = 0;

	DECLARE_IID (IDiscoveryHandler)
};

DEFINE_IID (IDiscoveryHandler, 0x4fc8b560, 0x1fa6, 0x4fc8, 0xb0, 0x40, 0xd9, 0xca, 0x26, 0x9f, 0xc7, 0x23)

DEFINE_STRINGID_MEMBER (IDiscoveryHandler, kServiceResolved, "serviceResolved")
DEFINE_STRINGID_MEMBER (IDiscoveryHandler, kServiceRemoved, "serviceRemoved")
DEFINE_STRINGID_MEMBER (IDiscoveryHandler, kServiceRegistered, "serviceRegistered")
DEFINE_STRINGID_MEMBER (IDiscoveryHandler, kServiceRegistrationFailed, "serviceRegistrationFailed")

} // namespace Net
} // namespace CCL

#endif // _ccl_inetdiscovery_h
