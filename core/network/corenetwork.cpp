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
// Filename    : core/network/corenetwork.cpp
// Description : Network Functions
//
//************************************************************************************************

#include "core/network/corenetwork.h"

using namespace Core;
using namespace Sockets;

//************************************************************************************************
// Network Functions
//************************************************************************************************

void Network::getLocalIPAddressList (Vector<IPAddress>& addressList)
{
	Platform::AdapterIterator iter;
	const Platform::AdapterIterator::Entry* entry;
	while((entry = iter.next ()) != nullptr)
		if(iter.matches (entry))
		{
			IPAddress address;
			if(iter.getIPAddress (address, entry))
				addressList.add (address);
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Network::getLocalIPAddressList (IPAddress addressList[], int maxCount)
{
	int count = 0;
	Platform::AdapterIterator iter;
	const Platform::AdapterIterator::Entry* entry;
	while((entry = iter.next ()) != nullptr)
		if(iter.matches (entry))
		{
			IPAddress address;
			if(iter.getIPAddress (address, entry))
			{
				if(count >= maxCount)
					break;
				addressList[count++] = address;
			}
		}
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Network::getLocalIPAddressListUnique (IPAddress addressList[], int maxCount)
{
	int count = 0;
	Platform::AdapterIterator iter;
	const Platform::AdapterIterator::Entry* entry;
	while((entry = iter.next ()) != nullptr)
	{
		if(!iter.matches (entry))
			continue;

		IPAddress address, netmask;
		if(!iter.getIPAddress (address, entry) || !iter.getIPSubnetMask (netmask, entry))
			continue;

		bool append = true;
		for(int i = 0; i < count; i++)
		{
			if(addressList[i].isEqual (address, netmask))
			{
				append = false;
				break;
			}
		}

		if(append)
		{
			if(count >= maxCount)
				break;

			addressList[count++] = address;
		}
	}

	return count;
}
