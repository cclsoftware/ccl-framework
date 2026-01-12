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
// Filename    : core/public/coreformatter.cpp
// Description : Value formatter
//
//************************************************************************************************

#include "coreformatter.h"

#include "core/public/corevector.h"
#include "core/public/corestringbuffer.h"
#include "core/public/coreprimitives.h"
#include "core/public/coremacros.h"

namespace Core {

//************************************************************************************************
// RangeFormatter
/** Replace min/max values with special strings, see FormatterRegistry::find(). */
//************************************************************************************************

class RangeFormatter: public Formatter
{
public:
	RangeFormatter (CStringPtr name, const Formatter& baseFormatter, 
					CStringPtr minString, CStringPtr maxString);

	// Formatter
	void print (Data& d) const override;
	bool scan (Data& d) const override;

protected:
	CString32 nameBuffer;
	const Formatter& baseFormatter;
	CString32 minString;
	CString32 maxString;
};

//************************************************************************************************
// FormatterRegistryList
//************************************************************************************************

class FormatterRegistryList: public Vector<Formatter*>
{
public:
	static FormatterRegistryList& instance ()
	{
		static FormatterRegistryList theInstance;
		return theInstance;
	}

	void addOwned (Formatter* formatter)
	{
		add (formatter);
		deleteList.add (formatter);
	}

	~FormatterRegistryList ()
	{
		VectorForEach (deleteList, Formatter*, formatter)
			delete formatter;
		EndFor
	}

protected:
	Vector<Formatter*> deleteList;
};

} // namespace Core

using namespace Core;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Formatter Registration
//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_INITIALIZER (CoreFormatter)
{
	FormatterRegistry::add (const_cast<Formatter*> (&IntFormatter::instance ()));
	FormatterRegistry::add (const_cast<Formatter*> (&FloatFormatter::instance ()));	
	FormatterRegistry::add (const_cast<Formatter*> (&PercentFormatter::instance ()));
	FormatterRegistry::add (const_cast<Formatter*> (&PercentFormatter::instance (1)));
	FormatterRegistry::add (const_cast<Formatter*> (&PercentFormatter::instance (2)));
	FormatterRegistry::add (const_cast<Formatter*> (&PercentRangeFormatter::instance ()));
	FormatterRegistry::add (const_cast<Formatter*> (&LeadingZeroFormatter::instance ()));
}

//************************************************************************************************
// FormatterRegistry
//************************************************************************************************

const Formatter* FormatterRegistry::find (CStringPtr _name)
{
	FormatterRegistryList& list = FormatterRegistryList::instance ();

	ConstString name (_name);
	if(!name.isEmpty ())
		VectorForEach (list, Formatter*, formatter)
			if(name == formatter->getName ())
				return formatter;
		EndFor
	
	// check if formatter with fixed strings for min/max is requested...
	int index = name.index ('/');
	if(index != -1)
	{
		CString32 baseName;
		baseName.append (name, index);
		if(const Formatter* baseFormatter = find (baseName))
		{
			CString32 strings[2]; // min/max	
			CStringPtr arguments = name.str () + index + 1;
			int i = 0;
			ForEachCStringToken (arguments, "/", token)
				if(i >= 2)
					break;
				if(ConstString (token) != "~")
					strings[i] = token;
				i++;
			EndFor

			RangeFormatter* rangeFormatter = NEW RangeFormatter (name, *baseFormatter, strings[0], strings[1]);
			list.addOwned (rangeFormatter);
			return rangeFormatter;
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FormatterRegistry::add (Formatter* formatter)
{
	FormatterRegistryList::instance ().add (formatter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FormatterRegistry::optimize (CStringPtr formatters[])
{
	FormatterRegistryList& list = FormatterRegistryList::instance ();

	int i = 0;
	while(CStringPtr name = formatters[i])
	{
		Formatter* formatter = const_cast<Formatter*> (find (name));
		Formatter* otherFormatter = const_cast<Formatter*> (list.at (i));
		if(formatter != otherFormatter && formatter && otherFormatter)
			list.swap (formatter, otherFormatter);
		i++;
	}
}

//************************************************************************************************
// RangeFormatter
//************************************************************************************************

RangeFormatter::RangeFormatter (CStringPtr name, const Formatter& baseFormatter, CStringPtr minString, CStringPtr maxString)
: nameBuffer (name),
  baseFormatter (baseFormatter),
  minString (minString),
  maxString (maxString)
{
	this->name = nameBuffer; // name has to be copied!  
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RangeFormatter::print (Data& d) const
{
	if(d.range)
	{
		static const float kTolerance = 0.01f;

		if(!minString.isEmpty ())
			if(d.value <= d.range->minValue + kTolerance)
			{
				minString.copyTo (d.string, d.size);
				return;
			}

		if(!maxString.isEmpty ())
			if(d.value >= d.range->maxValue - kTolerance)
			{
				maxString.copyTo (d.string, d.size);
				return;
			}
	}

	baseFormatter.print (d);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RangeFormatter::scan (Data& d) const
{
	return baseFormatter.scan (d);
}

//************************************************************************************************
// IntFormatter
//************************************************************************************************

const Formatter& IntFormatter::instance ()
{
	static IntFormatter theFormatter;
	return theFormatter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IntFormatter::IntFormatter (CStringPtr name, CStringPtr label)
: Formatter (name),
  label (label)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IntFormatter::print (Data& d) const
{
	CString128 buffer;
	if(label != nullptr)
		buffer.appendFormat ("%d %s", (int)d.value, label);
	else
		buffer.appendFormat ("%d", (int)d.value);
	buffer.copyTo (d.string, d.size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IntFormatter::scan (Data& d) const
{
	ConstString theString (d.string);
	int64 intVal = 0;
	if(!theString.getIntValue (intVal))
		return false;
	d.value = (float)intVal;
	return true;
}

//************************************************************************************************
// FloatFormatter
//************************************************************************************************

bool FloatFormatter::getFloatValue (float& value, CStringPtr _string)
{
	CString128 string (_string);
	string.replace (',', '.');
	return string.getFloatValue (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Formatter& FloatFormatter::instance ()
{
	static FloatFormatter theFormatter;
	return theFormatter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FloatFormatter::FloatFormatter (CStringPtr name, CStringPtr label)
: Formatter (name),
  label (label)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FloatFormatter::print (Data& d) const
{
	CString128 buffer;
	if(label != nullptr)
		buffer.appendFormat ("%.2f %s", d.value, label);
	else
		buffer.appendFormat ("%.2f", d.value);
	buffer.copyTo (d.string, d.size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FloatFormatter::scan (Data& d) const
{
	return getFloatValue (d.value, d.string);
}

//************************************************************************************************
// PercentFormatter
//************************************************************************************************

const Formatter& PercentFormatter::instance (int numDecimalDigits)
{
	static const PercentFormatter theFormatter;
	static const PercentFormatter theFormatter1 ("percent.1", 1);
	static const PercentFormatter theFormatter2 ("percent.2", 2);

	switch(numDecimalDigits)
	{
	case 1 : return theFormatter1;
	case 2 : return theFormatter2;
	default :
		ASSERT (numDecimalDigits == 0)
		return theFormatter;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PercentFormatter::PercentFormatter (CStringPtr name, int numDecimalDigits)
: Formatter (name)
{
	::strcpy (formatString, "%.0f %%");

	if(numDecimalDigits > 0)
		formatString[2] = '0' + (char)numDecimalDigits;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PercentFormatter::print (Data& d) const
{
	::snprintf (d.string, d.size, formatString, int (1e3f * d.value + ((d.value < 0) ? -0.5f : 0.5f)) * 0.1f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PercentFormatter::scan (Data& d) const
{
	float value = 0.;
	if(!FloatFormatter::getFloatValue (value, d.string))
		return false;
		
	d.value = 1e-2f * value;
	return true;
}

//************************************************************************************************
// PercentRangeFormatter
//************************************************************************************************

const Formatter& PercentRangeFormatter::instance ()
{
	static PercentRangeFormatter theFormatter;
	return theFormatter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PercentRangeFormatter::PercentRangeFormatter (CStringPtr name)
: Formatter (name)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PercentRangeFormatter::print (Data& d) const
{
	CString128 buffer;
	buffer.appendFormat ("%d%%", (int)round (d.value));
	buffer.copyTo (d.string, d.size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PercentRangeFormatter::scan (Data& d) const
{
	ConstString string (d.string);
	int64 val = 0;
	if(!string.getIntValue (val))
		return false;
	d.value = (float)val;
	return true;
}

//************************************************************************************************
// LeadingZeroFormatter
//************************************************************************************************

const Formatter& LeadingZeroFormatter::instance ()
{
	static LeadingZeroFormatter theFormatter;
	return theFormatter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LeadingZeroFormatter::LeadingZeroFormatter (CStringPtr name)
: Formatter (name)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LeadingZeroFormatter::print (Data& d) const
{
	CString128 buffer;
	int val = (int)round (d.value);
	if(val < 10)
		buffer.appendInteger (0);
	buffer.appendInteger (val);
	buffer.copyTo (d.string, d.size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LeadingZeroFormatter::scan (Data& d) const
{
	ConstString string (d.string);
	int64 val = 0;
	if(!string.getIntValue (val))
		return false;
	d.value = (float)val;
	return true;
}

//************************************************************************************************
// StringListFormatter
//************************************************************************************************

StringListFormatter::StringListFormatter (CStringPtr name, CStringPtr* strings, int count)
: Formatter (name),
  strings (strings),
  count (count)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringListFormatter::print (Data& d) const
{
	int index = (int)(d.value + 0.5);
	if(index >= 0 && index < count)
		ConstString (strings[index]).copyTo (d.string, d.size);
	else
		d.string[0] = '\0';
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringListFormatter::scan (Data& d) const
{
	for(int i = 0; i < count; i++)
		if(ConstString (strings[i]).compare (d.string, false) == 0)
		{
			d.value = (float)i;
			return true;
		}

	int64 value = 0;
	if(ConstString (d.string).getIntValue (value))
		if(value >= 0 && value < count)
		{
			d.value = (float)value;
			return true;
		}
	return false;
}
