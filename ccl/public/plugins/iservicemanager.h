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
// Filename    : ccl/public/plugins/iservicemanager.h
// Description : Service Manager Interface
//
//************************************************************************************************

#ifndef _ccl_iservicemanager_h
#define _ccl_iservicemanager_h

#include "ccl/public/base/uid.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

interface IServiceNotification;
interface IProgressNotify;

//************************************************************************************************
// IServiceDescription
/** \ingroup base_plug */
//************************************************************************************************

interface IServiceDescription: IUnknown
{
	/** Get service name. */
	virtual StringRef CCL_API getServiceName () const = 0;

	/** Get service title (possibly localized). */
	virtual StringRef CCL_API getServiceTitle () const = 0;

	/** Get service description (possibly localized). */
	virtual StringRef CCL_API getServiceDescription () const = 0;

	/** Check if service can be enabled/disabled by user. */
	virtual tbool CCL_API isUserService () const = 0;

	/** Check if service is enabled by user. */
	virtual tbool CCL_API isUserEnabled () const = 0;
	
	/** Get service class identifier. */
	virtual UIDRef CCL_API getServiceID () const = 0;

	/** Get service instance (null if not started). */
	virtual IUnknown* CCL_API getServiceInstance () const = 0;

	DECLARE_IID (IServiceDescription)
};

DEFINE_IID (IServiceDescription, 0x5faf6e77, 0xc827, 0x4a53, 0x8a, 0xb6, 0x89, 0x84, 0xd7, 0x56, 0x4, 0x58)

//************************************************************************************************
// IServiceManager
/** \ingroup base_plug */
//************************************************************************************************

interface IServiceManager: IUnknown
{
	/** Start services. */
	virtual void CCL_API startup (IProgressNotify* progress = nullptr) = 0;

	/** Shutdown services. */
	virtual void CCL_API shutdown () = 0;

	/** Check if services can shutdown now. */
	virtual tbool CCL_API canShutdown () const = 0;

	/** Get number of installed services. */
	virtual int CCL_API countServices () const = 0;

	/** Get service description by index. */
	virtual const IServiceDescription* CCL_API getService (int index) const = 0;

	/** Get service instance by interface/class identifier. */
	virtual tresult CCL_API getInstance (UIDRef cid, UIDRef iid, void** object) const = 0;

	/** Get service instance by interface/class identifier. */
	template <class Interface> Interface* getInstance (UIDRef cid = kNullUID)
	{ Interface* iface = nullptr; getInstance (cid, ccl_iid<Interface> (), (void**)&iface); return iface; }

	/** Enable/disable service. */
	virtual tresult CCL_API enableService (const IServiceDescription& description, tbool state) = 0;

	/** Register service notification. */
	virtual void CCL_API registerNotification (IServiceNotification* notification) = 0;
	
	/** Unregister service notification. */
	virtual void CCL_API unregisterNotification (IServiceNotification* notification) = 0;

	DECLARE_IID (IServiceManager)
};

DEFINE_IID (IServiceManager, 0x6bfcf21f, 0x1d40, 0x4da5, 0x8f, 0x34, 0xb9, 0x2c, 0xf4, 0x32, 0xe6, 0x5e)

//************************************************************************************************
// IServiceNotification
/** \ingroup base_plug */
//************************************************************************************************

interface IServiceNotification: IUnknown
{
	enum EventCode
	{
		kServiceActivate,	///< check if service can be started
		kServiceStarted,	///< service was started
		kServiceStopped		///< service was stopped
	};

	/** Handle service notification. */
	virtual tresult CCL_API onServiceNotification (const IServiceDescription& description, int eventCode) = 0;

	DECLARE_IID (IServiceNotification)
};

DEFINE_IID (IServiceNotification, 0xc7424fb9, 0x89d1, 0x4dd6, 0x94, 0x96, 0x32, 0xfc, 0x78, 0xb6, 0x2a, 0x3d)

} // namespace CCL

#endif // _ccl_iservicemanager_h
