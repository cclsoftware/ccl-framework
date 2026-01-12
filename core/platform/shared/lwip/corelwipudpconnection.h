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
// Filename    : core/platform/shared/lwip/corelwipudpconnection.h
// Description : LWIP UDP Network Connection
//
//************************************************************************************************

#ifndef _corelwipudpconnection_h
#define _corelwipudpconnection_h

#include "core/network/coreudpconnection.h"

struct netconn;

namespace Core {
namespace Sockets {

//************************************************************************************************
// LWIPUDPNetworkConnection
//************************************************************************************************

class LWIPUDPNetworkConnection
{
public:
	LWIPUDPNetworkConnection ();
	~LWIPUDPNetworkConnection ();

	bool init ();
	void close ();

	bool sendPacket (const IPAddress& dstIP, const uint8 buffer[], uint16 length);
	bool sendBroadcastPacket (uint16 port, const uint8 buffer[], uint16 length);
	bool processPackets (IUDPPacketReceiver* receiver, int timeout);  ///< must not block longer than given timeout (ms)!

protected:
	netconn* connection;
};

} // namespace Sockets
} // namespace Core

#endif // _corelwipudpconnection_h
