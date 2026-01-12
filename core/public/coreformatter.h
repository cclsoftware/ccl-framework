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
// Filename    : core/public/coreformatter.h
// Description : Value formatter
//
//************************************************************************************************

#ifndef _coreformatter_h
#define _coreformatter_h

#include "core/public/coretypes.h"

namespace Core {

class Formatter;

//************************************************************************************************
// FormatterRegistry
/**	Formatter class registration.
	Built-in classes are "int", "float", "percent", etc.
	Some formatters have variations like "percent.1" and "percent.2".
	
	In addition, registry can create formatters with customized strings for min/max on the fly.
	Usage: "{formatter}/{min-string}/{max-string}", use "~" for no change.
	
	\ingroup core_format */
//************************************************************************************************

class FormatterRegistry
{
public:
	/** Register formatter (by instance). */
	static void add (Formatter* formatter);
	
	/** Register formatter (by class). */
	template <class Type> static void add ()
	{
		static Type theFormatter;
		add (&theFormatter);
	}

	/** Optimize for finding formatters given by name faster. */
	static void optimize (CStringPtr formatters[]);

	/**	Find registered formatter by name. */
	static const Formatter* find (CStringPtr name);
};

//************************************************************************************************
// Formatter
/**	A formatter converts values to strings and vice versa.
	\ingroup core_format */
//************************************************************************************************

class Formatter
{
public:
	Formatter (CStringPtr name = "")
	: name (name)
	{}

	virtual ~Formatter ()
	{}

	/** Value range defintion. */
	struct Range
	{
		float minValue;	///< minimum value
		float maxValue;	///< maximum value
	};

	/** Data passed to print/scan methods. */
	struct Data
	{
		const void* object;	///< application-specific (optional)
		char* string;		///< string (input or output)
		int size;			///< string size in bytes
		float value;		///< numeric value (input or output)
		const Range* range;	///< conversion range
	};

	/** Get class name. */
	CStringPtr getName () const { return name; }

	/** Print value to string. */
	virtual void print (Data& d) const = 0;
	
	/** Scan value from string. */
	virtual bool scan (Data& d) const { return false; }

protected:
	CStringPtr name;
};

//************************************************************************************************
// IntFormatter 
/**	Format integer value.
	\ingroup core_format */
//************************************************************************************************

class IntFormatter: public Core::Formatter
{
public:
	IntFormatter (CStringPtr name = "int", CStringPtr label = nullptr);

	static const Formatter& instance ();

	// Formatter
	void print (Data& d) const override;
	bool scan (Data& d) const override;

protected:
	CStringPtr label;
};

//************************************************************************************************
// FloatFormatter
/**	Format float value.
	\ingroup core_format */
//************************************************************************************************

class FloatFormatter: public Core::Formatter
{
public:
	FloatFormatter (CStringPtr name = "float", CStringPtr label = nullptr);

	static const Formatter& instance ();

	/** Helper to scan float value with different decimal marks. */
	static bool getFloatValue (float& value, CStringPtr string); 

	// Formatter
	void print (Data& d) const override;
	bool scan (Data& d) const override;

protected:
	CStringPtr label;
};

//************************************************************************************************
// PercentFormatter
/**	Format normalized value [0..1] to 0 to 100% with given number of decimal digits.
	\ingroup core_format */
//************************************************************************************************

class PercentFormatter: public Core::Formatter
{
public:
	PercentFormatter (CStringPtr name = "percent", int numDecimalDigits = 0);

	static const Formatter& instance (int numDecimalDigits = 0);

	// Formatter
	void print (Data& d) const override;
	bool scan (Data& d) const override;

protected:
	char formatString[16];
};

//************************************************************************************************
// PercentRangeFormatter
/**	Format range value [0..100] to 0 to 100%.
	\ingroup core_format */
//************************************************************************************************

class PercentRangeFormatter: public Core::Formatter
{
public:
	PercentRangeFormatter (CStringPtr name = "percentrange");

	static const Formatter& instance ();

	// Formatter
	void print (Data& d) const override;
	bool scan (Data& d) const override;
};

//************************************************************************************************
// LeadingZeroFormatter
/**	Formatter to add a leading zero to integer numbers.
	\ingroup core_format */
//************************************************************************************************

class LeadingZeroFormatter: public Core::Formatter
{
public:
	LeadingZeroFormatter (CStringPtr name="leadingzero");

	static const Formatter& instance ();

	// Formatter
	void print (Data& d) const override;
	bool scan (Data& d) const override;
};

//************************************************************************************************
// StringListFormatter
/**	Formatter using a constant C-String array for conversion.
	\ingroup core_format */
//************************************************************************************************

class StringListFormatter: public Formatter
{
public:
	StringListFormatter (CStringPtr name, CStringPtr* strings, int count);

	// Formatter
	void print (Data& d) const override;
	bool scan (Data& d) const override;

protected:
	CStringPtr* strings;
	int count;
};

} // namespace Core

#endif // _coreformatter_h

