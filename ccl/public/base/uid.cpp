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
// Filename    : ccl/public/base/uid.cpp
// Description : UID classes
//
//************************************************************************************************

#include "ccl/public/base/uid.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// UID
//************************************************************************************************

UID::UID (int unused)
{
	ASSERT (unused == 0)
	data1 = 0;
	data2 = data3 = 0;
	::memset (data4, 0, 8);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UID::UID (UIDRef uid)
{
	assign (uid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UID::UID (uint32 _data1, uint16 _data2, uint16 _data3, uint8 a, uint8 b, uint8 c, uint8 d,
		  uint8 e, uint8 f, uint8 g, uint8 h)
{
	data1 = _data1;
	data2 = _data2;
	data3 = _data3;
	uint8 temp[8] = {a, b, c, d, e, f, g, h};
	::memcpy (data4, temp, 8);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UID::generate ()
{
	return System::CreateUID (*this) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 UID::hash () const
{
	return System::Hash (this, 16, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UID::toCString (MutableCString& cString, int format) const
{
	char temp[128] = {0};
	toCString (temp, sizeof(temp), format);
	cString.empty ();
	cString.append (temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UID::toString (String& string, int format) const
{
	char temp[128] = {0};
	toCString (temp, sizeof(temp), format);
	string.empty ();
	string.appendASCII (temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UID::fromString (StringRef string, int format)
{
	char temp[128] = {0};
	string.toASCII (temp, sizeof(temp));
	return fromCString (temp, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UID::toBuffer (UIDBuffer& buffer) const
{
	buffer[0] = (data1 >> 24) & 0xFF;
	buffer[1] = (data1 >> 16) & 0xFF;
	buffer[2] = (data1 >>  8) & 0xFF;
	buffer[3] = data1 & 0xFF;

	buffer[4] = data2 >> 8;
	buffer[5] = data2 & 0xFF;

	buffer[6] = data3 >> 8;
	buffer[7] = data3 & 0xFF;

	for(int i = 0; i < 8; ++i) buffer[i + 8] = data4[i];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UID::fromBuffer (const UIDBuffer& buffer)
{
	if(::memcmp (buffer, &kNullUID, sizeof(kNullUID)) == 0)
		return false;

	data1 = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
	data2 = uint16 (buffer[4] <<  8 | buffer[5]);
	data3 = uint16 (buffer[6] <<  8 | buffer[7]);

	for(int i = 0; i < 8; ++i) data4[i] = buffer[i + 8];

	return true;
}
