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
// Filename    : core/network/coreudpconnection.h
// Description : UDP Network Connection
//
//************************************************************************************************

#ifndef _coreudpconnection_h
#define _coreudpconnection_h

#include "core/public/coresocketaddress.h"

namespace Core {
namespace Sockets {

class Socket;

//************************************************************************************************
// IUDPPacketReceiver
//************************************************************************************************

struct IUDPPacketReceiver
{
	virtual void receiveUDPPacket (const IPAddress& srcIP, const uint8 buffer[], uint16 length) = 0;
};

//************************************************************************************************
// UDPNetworkConnection
//************************************************************************************************

class UDPNetworkConnection
{
public:
	UDPNetworkConnection ();
	~UDPNetworkConnection ();

	bool init ();
	void close ();

	bool sendPacket (const IPAddress& dstIP, const uint8 buffer[], uint16 length);
	bool sendBroadcastPacket (uint16 port, const uint8 buffer[], uint16 length);
	bool processPackets (IUDPPacketReceiver* receiver, int timeout);  ///< must not block longer than given timeout (ms)!

protected:
	Socket* udpSocket;
};

} // namespace Sockets
} // namespace Core

#endif // _coreudpconnection_h
