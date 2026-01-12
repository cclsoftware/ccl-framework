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
// Filename    : core/platform/cocoa/corenetwork.cocoa.cpp
// Description : Cocoa Network Functions
//
//************************************************************************************************

#include "core/platform/corefeatures.h"

#include "core/platform/cocoa/corenetwork.cocoa.h"

using namespace Core;
using namespace Sockets;
using namespace Platform;

//************************************************************************************************
// Network
//************************************************************************************************

INetwork& Network::instance () 
{ 
	static CocoaNetwork theNetwork;
	return theNetwork; 
}

//************************************************************************************************
// CocoaNetwork
//************************************************************************************************

bool CocoaNetwork::getLocalMACAddress (uint8 _mac[6])
{
	AdapterIterator iter;
	const AdapterIterator::Entry* entry;
	while((entry = iter.next ()) != nullptr)
	{
		const uint8* mac = nullptr;
		
		if(entry->ifa_addr && entry->ifa_addr->sa_family == AF_LINK)
		{
			sockaddr_dl* sdl = (struct sockaddr_dl *)entry->ifa_addr;
			if(sdl->sdl_alen > 0)
				mac = reinterpret_cast<uint8*> (LLADDR (sdl));
		}

		if(mac != nullptr)
		{
			::memcpy (_mac, mac, 6);
			return true;
		}
	}

	::memset (_mac, 0, 6);
	return false;
}
