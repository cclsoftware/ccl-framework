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
// Filename    : core/public/coreuid.cpp
// Description : 16-Byte GUID
//
//************************************************************************************************

#include "core/public/coreuid.h"

using namespace Core;

//************************************************************************************************
// UIDBytes
//************************************************************************************************

const UIDBytes Core::kNullUID = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0}};

CStringPtr UIDBytes::getFormatString (int format)
{
	static CStringPtr standardFormat = "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}";
	static CStringPtr standardNoBracesFormat = "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X";
	static CStringPtr compactFormat = "%08lX%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X";

	switch(format)
	{
	case kStandardNoBraces :
		return standardNoBracesFormat;
	case kCompact :
		return compactFormat;
	default :
		return standardFormat;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UIDBytes::toCString (char* cString, int cStringSize, int format) const
{
	int v[11];
	v[0] = data1;
	v[1] = data2;
	v[2] = data3;

	for(int i = 0; i < 8; i++)
		v[3 + i] = data4[i];

	snprintf (cString, cStringSize, getFormatString (format), v[0], v[1], v[2], v[3],
					v[4], v[5], v[6], v[7], v[8], v[9], v[10]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UIDBytes::fromCString (CStringPtr cString, int format)
{
	if(!cString || !cString[0])
		return false;

	int v[11] = {0};
	int result = ::sscanf (cString, getFormatString (format), &v[0], &v[1], &v[2], &v[3],
							&v[4], &v[5], &v[6], &v[7], &v[8], &v[9], &v[10]);
	if(result != 11)
		return false;

	data1 = (uint32)v[0];
	data2 = (uint16)v[1];
	data3 = (uint16)v[2];

	for(int i = 0; i < 8; i++)
		data4[i] = (uint8)v[3 + i];
	return true;
}
