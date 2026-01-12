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
// Filename    : coresscanf.cpp
// Description : C Runtime fallback sscanf implementation
//
//************************************************************************************************

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

//************************************************************************************************
// vsscanf
//************************************************************************************************

int vsscanf (const char* str, const char* format, va_list ap)
{
	enum Conversion
	{
		NO_CONVERSION,
		INT_CONVERSION,
		INT64_CONVERSION,
		FLOAT_CONVERSION,
		DOUBLE_CONVERSION,
		HEX_CONVERSION,
		HEX64_CONVERSION,
	};
	int conversion = NO_CONVERSION;

	int length = 1;
	int maxWidth = 0;

	while(*format != '%')
	{
		if(*format++ == '\0' || *str++ == '\0')
			return 0;
	}
	++format;

	while(isdigit (*format))
	{
		maxWidth = maxWidth * 10 + *format++ - '0';
	}

	while(conversion == NO_CONVERSION)
	{
		switch(*format++)
		{
		case 'l':
			++length;
			break;
		case 'd':
			conversion = length == 1 ? INT_CONVERSION : INT64_CONVERSION;
			break;
		case 'x':
			conversion = length == 1 ? HEX_CONVERSION : HEX64_CONVERSION;
			break;
		case 'f':
			conversion = length == 1 ? FLOAT_CONVERSION : DOUBLE_CONVERSION;
			break;
		case 'X':
			conversion = length == 1 ? HEX_CONVERSION : HEX64_CONVERSION;
			break;
		default:
			printf ("unimplemented sscanf conversion\n");
			printf (format);
			return 0;
		}
	}

	if(*str == '\0')
		return 0;

	char conversionSource[21];

	maxWidth = maxWidth ? maxWidth : 20;
	int width = 0;
	while(width < maxWidth)
	{
		if(conversion == HEX_CONVERSION || conversion == HEX64_CONVERSION)
		{
			if(!isxdigit (*str))
				break;
		}
		else if(!isdigit (*str) && !((conversion == FLOAT_CONVERSION || conversion == DOUBLE_CONVERSION) && *str == '.'))
		{
			break;
		}
		conversionSource[width++] = *str++;
	}
	conversionSource[width] = '\0';

	switch(conversion)
	{
	case INT_CONVERSION:
		*va_arg (ap, int*) = atoi (conversionSource);
		break;
	case INT64_CONVERSION:
		*va_arg (ap, long long int*) = atol (conversionSource);
		break;
	case FLOAT_CONVERSION:
		*va_arg (ap, float*) = (float)(strtod (conversionSource, nullptr));
		break;
	case DOUBLE_CONVERSION: 
		*va_arg (ap, double*) = strtod (conversionSource, nullptr);
		break;
	case HEX_CONVERSION:
		*va_arg (ap, unsigned int*) = (unsigned int)(strtol (conversionSource, nullptr, 16));
		break;
	case HEX64_CONVERSION:
		*va_arg (ap, long unsigned int*) = strtol (conversionSource, nullptr, 16);
		break;
	default:
		break;
	}

	if(*str != '\0' && *format != '\0')
		return 1 + vsscanf (str, format, ap);
	return 1;
}

//************************************************************************************************
// sscanf
//************************************************************************************************

int sscanf (const char* s, const char* format, ...)
{
	va_list args;
	int rv = 0;
	va_start (args, format);
	rv = vsscanf (s, format, args);
	va_end (args);
	return rv;
}
