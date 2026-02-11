//************************************************************************************************
//
// Bin2C Tool
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
// Filename    : bin2c.h
// Description : Bin2C Tool
//
//************************************************************************************************

#include "core/public/corestream.h"
#include "core/public/corestringbuffer.h"

static Core::CStringPtr bin2c_commentline = "/////////////////////////////////////////////////////////////////////////////////////////////\n";

template <class String>
static void bin2c_size (String& str, Core::CStringPtr arrayName)
{
	str.appendFormat ("%s_size", arrayName);
}

template <class String>
static void bin2c_code (String& str, Core::CStringPtr arrayName)
{
	str.appendFormat ("%s_code", arrayName);
}

static void bin2c (Core::IO::Stream& writer, Core::IO::Stream& reader, Core::CStringPtr arrayName, int filesize)
{
	const int lineW = 32;

	Core::CString256 str;
	str.appendFormat ("const unsigned int %s_size = %i;\n", arrayName, filesize);
	str.appendFormat ("const unsigned char %s_code[%i] = {\n ", arrayName, filesize);
	writer.writeBytes ((const void*)str.getBuffer (), str.length ());
		
	for(int i = 0; i < filesize; i++)
	{
		str = "";
		unsigned char byte = 0;
		int result = reader.readBytes (&byte, 1);
		ASSERT (result == 1)
		
		str.appendFormat ("0x%02x", (int)byte);
		if(i < filesize-1)
		{
			str += ",";
		}
		if((i%lineW) ==(lineW-1))
		{
			str += "\n  ";
		}
		writer.writeBytes ((const void*)str.getBuffer (), str.length ());
	}

	str = "\n};\n";
	writer.writeBytes ((const void*)str.getBuffer (), str.length ());
}
