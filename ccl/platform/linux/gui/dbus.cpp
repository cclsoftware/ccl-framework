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
// Filename    : ccl/platform/linux/gui/dbus.cpp
// Description : D-Bus Support
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/linux/gui/dbus.h"
#include "ccl/gui/gui.h"

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// DBusSupport
//************************************************************************************************

DEFINE_SINGLETON (DBusSupport)

//////////////////////////////////////////////////////////////////////////////////////////////////

DBusSupport::DBusSupport ()
: sessionBusUseCount (0),
  systemBusUseCount (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

sdbus::IConnection& CCL_API DBusSupport::openSessionBusConnection ()
{
	if(!sessionBusConnection)
	{
		sessionBusConnection = NEW DBusConnection (std::move (sdbus::createSessionBusConnection ()));
		sessionBusConnection->startEventLoop ();
	}
	sessionBusUseCount++;
	return sessionBusConnection->getConnection ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DBusSupport::closeSessionBusConnection ()
{
	sessionBusUseCount--;
	if(sessionBusUseCount == 0)
	{
		sessionBusConnection->stopEventLoop ();
		sessionBusConnection.release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sdbus::IConnection& CCL_API DBusSupport::openSystemBusConnection ()
{
	if(!systemBusConnection)
	{
		systemBusConnection = NEW DBusConnection (std::move (sdbus::createSystemBusConnection ()));
		systemBusConnection->startEventLoop ();
	}
	systemBusUseCount++;
	return systemBusConnection->getConnection ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DBusSupport::closeSystemBusConnection ()
{
	systemBusUseCount--;
	if(systemBusUseCount == 0)
	{
		systemBusConnection->stopEventLoop ();
		systemBusConnection.release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DBusSupport::flushUpdates ()
{
	return GUI.flushUpdates ();
}

//************************************************************************************************
// DBusConnection
//************************************************************************************************

DBusConnection::DBusConnection (std::unique_ptr<sdbus::IConnection>&& connection)
: connection (std::move (connection))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DBusConnection::startEventLoop ()
{
	UnknownPtr<IEventLoop> eventLoop (GUI.asUnknown ());
	if(eventLoop.isValid ())
		eventLoop->addEventHandler (this, connection->getEventLoopPollData ().fd);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DBusConnection::stopEventLoop ()
{
	UnknownPtr<IEventLoop> eventLoop (GUI.asUnknown ());
	if(eventLoop.isValid ())
		eventLoop->removeEventHandler (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DBusConnection::onEvent (int eventFd)
{
	if(eventFd == connection->getEventLoopPollData ().fd)
		while (connection->processPendingRequest ());
}
