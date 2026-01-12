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
// Filename    : core/public/coreenumdef.h
// Description : Enum Definition
//
//************************************************************************************************

#ifndef _coreenumdef_h
#define _coreenumdef_h

#include "core/public/coretypes.h"

namespace Core {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Enum Macros
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Declare enumeration. */
#define DECLARE_ENUMINFO(name)	static const Core::EnumInfo name[];

/** Begin enumeration. */
#define BEGIN_ENUMINFO(name)	const Core::EnumInfo name[] = {

/** End enumeration. */
#define END_ENUMINFO			{nullptr,0}};

/*
	Example:
	
	// Declaration
	DECLARE_ENUMINFO (Options)

	// Definition
	BEGIN_ENUMINFO (Options)
		{"option1", kOption1},
		{"option2", kOption2},
	END_ENUMINFO
*/

//************************************************************************************************
// EnumInfo
/** Enumerator value with name. */
//************************************************************************************************

struct EnumInfo
{
	CStringPtr name;
	int value;

	/** Parse single value from string (e.g. "option1"). */
	template <typename StringType> 
	static int parseOne (const StringType& string, const EnumInfo info[], int defValue);
	
	/** Parse multiple values from string (e.g. "option1 option2"). */
	template <typename StringType> 
	static int parseMultiple (const StringType& string, const EnumInfo info[], char separator = ' ');

	/** Print single value to string. */
	template <typename StringType>
	static void printOne (StringType& string, int value, const EnumInfo info[]);

	/** Print multiple values to string. */
	template <typename StringType>
	static void printMultiple (StringType& string, int value, const EnumInfo info[]);

	/** Get name by value */ 
	static CStringPtr nameByValue (int value, const EnumInfo info[]);

	/** Check if value is contained. */ 
	static bool containsValue (int value, const EnumInfo info[]);

	/** Count enumerators (null-terminated). */
	static int getCount (const EnumInfo info[]);

	// Methods used by CCL type library:
	CStringPtr getEnumName () const { return name; }
	int getEnumValue () const { return value; }
};
	
//////////////////////////////////////////////////////////////////////////////////////////////////
// EnumInfo inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename StringType>
int EnumInfo::parseOne (const StringType& string, const EnumInfo info[], int defValue)
{
	int result = defValue;
	for(; info->name; info++)
		if(string.compare (info->name) == 0)
		{
			result = info->value;
			break;
		}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename StringType>
int EnumInfo::parseMultiple (const StringType& string, const EnumInfo info[], char separator)
{
	int stringLength = 0;
	if(separator)
		stringLength = string.length ();

	int result = 0;
	for(; info->name; info++)
	{
		StringType infoName (info->name);
		int index = string.index (infoName);
		if(index > -1)
		{
			if(separator)
			{
				int nameLength = infoName.length ();
				if(index > 0 && string[index - 1] != separator) // check for leading separator
					continue;

				if(stringLength > index + nameLength && string[index + nameLength] != separator) // check for trailing separator
					continue;
			}

			result |= info->value;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename StringType>
void EnumInfo::printOne (StringType& string, int value, const EnumInfo info[])
{
	string = "";
	for(; info->name; info++)
		if(value == info->value)
		{
			string = info->name;
			break;
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename StringType>
void EnumInfo::printMultiple (StringType& string, int value, const EnumInfo info[])
{
	string = "";
	int doneMask = 0;
	for(; info->name; info++)
		if((value & info->value) == info->value)
		{
			// only if not already set (this allows us to specify multiple names for a single bit)
			if((doneMask & info->value) == 0) 
			{
				doneMask |= info->value;

				if(!string.isEmpty ())
					string += " ";
				string += info->name;
			}
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline CStringPtr EnumInfo::nameByValue (int value, const EnumInfo info[])
{
	for(; info->name; info++)
		if(value == info->value)
			return info->name;
		
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool EnumInfo::containsValue (int value, const EnumInfo info[])
{
	for(; info->name; info++)
		if(value == info->value)
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline int EnumInfo::getCount (const EnumInfo info[])
{
	int count = 0;
	for(; info->name; info++)
		count++;
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _coreenumdef_h
