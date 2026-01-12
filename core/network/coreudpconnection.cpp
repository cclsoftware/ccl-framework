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
// Filename    : core/network/coreudpconnection.cpp
// Description : UDP Network Connection
//
//************************************************************************************************

#include "core/network/coreudpconnection.h"

#include "core/network/coresocket.h"

using namespace Core;
using namespace Sockets;

//************************************************************************************************
// UDPNetworkConnection
//************************************************************************************************

UDPNetworkConnection::UDPNetworkConnection ()
: udpSocket (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UDPNetworkConnection::~UDPNetworkConnection ()
{
	ASSERT (udpSocket == nullptr) // close must be called!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UDPNetworkConnection::init ()
{
	ASSERT (udpSocket == nullptr)
	if(udpSocket != nullptr)
		return false;

	udpSocket = NEW Socket (kInternet, kDatagram, kUDP);
	udpSocket->setOption (SocketOption::kBroadcast, 1);
	udpSocket->setOption (SocketOption::kNonBlocking, 1);

	IPAddress anyAddress;
	bool success = udpSocket->bind (anyAddress);
	ASSERT (success == true)
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UDPNetworkConnection::close ()
{
	delete udpSocket;
	udpSocket = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UDPNetworkConnection::processPackets (IUDPPacketReceiver* receiver, int timeout)
{
	ASSERT (udpSocket != nullptr)
	if(udpSocket == nullptr)
		return false;

	static const int kMaxPacketSize = 4096; // max. size we support (UDP limit is 64KB)	
	if(udpSocket->isReadable (timeout))
	{
		IPAddress sourceAddress;
		uint8 buffer[kMaxPacketSize];
		uint16 bytesAvailable = static_cast<uint16> (udpSocket->receiveFrom (buffer, kMaxPacketSize, sourceAddress));
		if(bytesAvailable > 0)
			receiver->receiveUDPPacket (sourceAddress, buffer, bytesAvailable);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UDPNetworkConnection::sendPacket (const IPAddress& dstIP, const uint8 buffer[], uint16 length)
{
	int result = -1;
	ASSERT (udpSocket != nullptr)
	if(udpSocket)
	{
		result = udpSocket->sendTo (buffer, length, dstIP);
		if(result == -1) // socket error, try to repair
		{
			close ();
			init ();
		}
	}
	return result == length;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UDPNetworkConnection::sendBroadcastPacket (uint16 port, const uint8 buffer[], uint16 length)
{
	IPAddress broadcastAddress;
	broadcastAddress.setIP (255, 255, 255, 255);
	broadcastAddress.port = port;
	return sendPacket (broadcastAddress, buffer, length);
}
