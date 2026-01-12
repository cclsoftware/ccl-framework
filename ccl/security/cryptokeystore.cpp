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
// Filename    : ccl/security/cryptokeystore.cpp
// Description : Cryptographical Key Store
//
//************************************************************************************************

#include "ccl/security/cryptokeystore.h"

#include "ccl/public/securityservices.h"

using namespace CCL;
using namespace Security;
using namespace Crypto;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Security Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ICryptoKeyStore& CCL_API System::CCL_ISOLATED (GetCryptoKeyStore) ()
{
	return CryptoKeyStore::instance ();
}

//************************************************************************************************
// CryptoKeyStore
//************************************************************************************************

CryptoKeyStore& CryptoKeyStore::instance ()
{
	static CryptoKeyStore theCryptoKeyStore;
	return theCryptoKeyStore;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (CryptoKeyStore, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

CryptoKeyStore::CryptoKeyStore ()
{
	entries.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CryptoKeyStore::Entry* CryptoKeyStore::lookup (StringID name, MaterialType type) const
{
	uint32 hashValue = name.getHashCode ();
	ForEach (entries, Entry, e)
		if(e->getHashValue () == hashValue && e->getType () == type)
			return e;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoKeyStore::addMaterial (StringID name, MaterialType type, IStream& data)
{
	const Entry* existing = lookup (name, type);
	ASSERT (existing == nullptr)
	if(existing)
		return kResultFailed;

	Entry* e = NEW Entry;
	e->setHashValue (name.getHashCode ());
	e->setType (type);
	e->copyFrom (data);
	entries.add (e);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoKeyStore::addMaterial (StringID name, MaterialType type, const void* data, uint32 length)
{
	return addMaterial (name, type, Material (Block (data, length)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoKeyStore::getMaterial (IStream& data, StringID name, MaterialType type) const
{
	const Entry* e = lookup (name, type);
	if(e == nullptr)
		return kResultFailed;

	e->copyTo (data);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoKeyStore::removeMaterial (StringID name, MaterialType type)
{
	Entry* existing = lookup (name, type);
	if(existing)
	{
		entries.remove (existing);
		existing->release ();
		return kResultOk;
	}
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CryptoKeyStore::removeMaterial (StringID name)
{
	tresult result = kResultFalse;
	uint32 hashValue = name.getHashCode ();
	for(int i = entries.count ()-1; i >= 0; i--)
	{
		Entry* e = static_cast<Entry*> (entries.at (i));
		if(e->getHashValue () == hashValue)
		{
			entries.removeAt (i);
			e->release ();
			result = kResultTrue;
		}
	}
	return result;
}
