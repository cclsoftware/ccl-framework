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
// Filename    : ccl/platform/linux/gui/dbus.h
// Description : D-Bus Support
//
//************************************************************************************************

#ifndef _ccl_dbus_h
#define _ccl_dbus_h

#include "ccl/base/singleton.h"
#include "ccl/platform/linux/interfaces/idbussupport.h"
#include "ccl/public/gui/framework/ilinuxspecifics.h"

namespace CCL {

//************************************************************************************************
// DBusConnection
//************************************************************************************************

class DBusConnection: public Object,
					  public Linux::IEventHandler
{
public:
	DBusConnection (std::unique_ptr<sdbus::IConnection>&& connection);
	
	sdbus::IConnection& getConnection () const { return *connection; }

	void startEventLoop ();
	void stopEventLoop ();

	// IEventHandler
	void CCL_API onEvent (int eventFd) override;

	CLASS_INTERFACE (IEventHandler, Unknown)

private:
	std::unique_ptr<sdbus::IConnection> connection;
};

//************************************************************************************************
// DBusSupport
//************************************************************************************************

class DBusSupport: public Object,
				   public Singleton<DBusSupport>,
				   public IDBusSupport
{
public:
	DBusSupport ();

	// IDBusSupport
	sdbus::IConnection& CCL_API openSessionBusConnection () override;
	void CCL_API closeSessionBusConnection () override;
	sdbus::IConnection& CCL_API openSystemBusConnection () override;
	void CCL_API closeSystemBusConnection () override;
	tbool CCL_API flushUpdates () override;

	CLASS_INTERFACE (IDBusSupport, Object)

private:
	AutoPtr<DBusConnection> sessionBusConnection;
	AutoPtr<DBusConnection> systemBusConnection;
	int sessionBusUseCount;
	int systemBusUseCount;
};

} // namespace CCL

#endif // _ccl_dbus_h
