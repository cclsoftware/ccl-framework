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
// Filename    : ccl/platform/linux/interfaces/idbussupport.h
// Description : DBus Support Interfaces
//
//************************************************************************************************

#ifndef _ccl_idbussupport_h
#define _ccl_idbussupport_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/base/debug.h"

#include <algorithm>
#include <sdbus-c++/sdbus-c++.h>

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// D-Bus Macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define PRINT_DBUS_ERROR(error, context) CCL_WARN ("%s%s: %s\n", context, (error).getName ().c_str (), (error).getMessage ().c_str ())

#define CATCH_DBUS_ERROR catch(const sdbus::Error& e) { PRINT_DBUS_ERROR (e, "") }

//************************************************************************************************
// IDBusSupport
//************************************************************************************************

interface IDBusSupport: IUnknown
{
	virtual sdbus::IConnection& CCL_API openSessionBusConnection () = 0;

	virtual void CCL_API closeSessionBusConnection () = 0;

	virtual sdbus::IConnection& CCL_API openSystemBusConnection () = 0;

	virtual void CCL_API closeSystemBusConnection () = 0;

	virtual tbool CCL_API flushUpdates () = 0;

	DECLARE_IID (IDBusSupport)
};

DEFINE_IID (IDBusSupport, 0xf7d86a40, 0xd661, 0x425f, 0xb1, 0x52, 0x25, 0x29, 0x2e, 0x60, 0xe4, 0xe3)

//************************************************************************************************
// DBusProxy
/** Base class for D-Bus proxy objects which want to receive signals. */
//************************************************************************************************

template<typename DBusInterface>
class DBusProxy: public sdbus::ProxyInterfaces<DBusInterface>
{
public:
	DBusProxy (IDBusSupport& dbusSupport, std::string destination, std::string objectPath, bool useSystemBus = false)
	: sdbus::ProxyInterfaces<DBusInterface> (useSystemBus ? dbusSupport.openSystemBusConnection () : dbusSupport.openSessionBusConnection (),
	  										 std::move (destination), std::move (objectPath)),
	  dbusSupport (dbusSupport),
	  useSystemBus (useSystemBus)
	{
		this->registerProxy ();
	}

	virtual ~DBusProxy ()
	{
		this->unregisterProxy ();
		if(useSystemBus)
			dbusSupport.closeSystemBusConnection ();
		else
			dbusSupport.closeSessionBusConnection ();
	}

protected:
	bool useSystemBus;
	IDBusSupport& dbusSupport;
};

//************************************************************************************************
// DBusAdapter
/** Base class for server-side D-Bus adapter objects. */
//************************************************************************************************

template<typename DBusInterface>
class DBusAdapter: public sdbus::AdaptorInterfaces<DBusInterface>
{
public:
	DBusAdapter (IDBusSupport& dbusSupport, std::string objectPath)
	: sdbus::AdaptorInterfaces<DBusInterface> (dbusSupport.openSessionBusConnection (), std::move (objectPath)),
	  dbusSupport (dbusSupport)
	{
		this->registerAdaptor ();
	}

	virtual ~DBusAdapter ()
	{
		this->unregisterAdaptor ();
		dbusSupport.closeSessionBusConnection ();
	}

protected:
	IDBusSupport& dbusSupport;
};

} // namespace CCL

#endif // _ccl_idbussupport_h
