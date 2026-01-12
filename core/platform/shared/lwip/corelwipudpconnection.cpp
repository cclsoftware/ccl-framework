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
// Filename    : core/platform/shared/lwip/corelwipudpconnection.cpp
// Description : LWIP UDP Network Connection
//
//************************************************************************************************

#include "core/platform/shared/lwip/corelwipudpconnection.h"

#include "core/network/coresocket.h"

#include "lwip/api.h"
#include "lwip/ip.h"
#undef select

using namespace Core;
using namespace Sockets;

//************************************************************************************************
// LWIPUDPNetworkConnection
//************************************************************************************************

LWIPUDPNetworkConnection::LWIPUDPNetworkConnection () 
: connection(0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LWIPUDPNetworkConnection::~LWIPUDPNetworkConnection ()
{
	ASSERT (connection == 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LWIPUDPNetworkConnection::init ()
{
	ASSERT (connection == 0)

	connection = netconn_new (NETCONN_UDP);
	if(connection == 0)
		return false;

	ip_set_option(connection->pcb.ip, SO_BROADCAST);
	netconn_set_nonblocking(connection, true);
	return connection != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LWIPUDPNetworkConnection::close ()
{
	if(connection != 0)
	{
		netconn_delete (connection);
		connection = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LWIPUDPNetworkConnection::sendPacket (const IPAddress& dstIP, const uint8 buffer[], uint16 length)
{
	ASSERT (connection != 0)

	if(connection == 0)
		return false;

	Platform::SocketAddressConverter temp (dstIP);
	if(!temp.valid)
		return false;

	netbuf localBuffer = {0};
	err_t result = netbuf_ref(&localBuffer, buffer, length);
	 
	if(result == ERR_OK)
	{
		in_addr *address1 = &reinterpret_cast<sockaddr_in *>(temp.as<sockaddr_in> ())->sin_addr;
		ip_addr_t address2;
		inet_addr_to_ipaddr(&address2, address1);

		result = netconn_sendto (connection, &localBuffer, &address2, dstIP.port);
	}

	netbuf_free(&localBuffer);
	return result != ERR_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LWIPUDPNetworkConnection::sendBroadcastPacket (uint16 port, const uint8 buffer[], uint16 length)
{
	IPAddress broadcastAddress;
	broadcastAddress.setIP (255, 255, 255, 255);
	broadcastAddress.port = port;
	return sendPacket (broadcastAddress, buffer, length);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LWIPUDPNetworkConnection::processPackets (IUDPPacketReceiver* receiver, int timeout)
{
	ASSERT (connection != 0)

	if(connection == 0)
		return false;

	netconn_set_recvtimeout(connection, timeout);

	netbuf *buffer;
	if(netconn_recv (connection, &buffer) != ERR_OK)
		return false;

	IPAddress sourceAddress;
	sourceAddress.setIP(netbuf_fromaddr (buffer)->addr, netbuf_fromport (buffer));

	void* pointer;
	u16_t length;
	if(netbuf_data (buffer, &pointer, &length) == ERR_OK)
		receiver->receiveUDPPacket (sourceAddress, reinterpret_cast<const uint8*>(pointer), length);

	netbuf_delete(buffer);
	return true;
}
