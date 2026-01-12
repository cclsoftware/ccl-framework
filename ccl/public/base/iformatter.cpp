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
// Filename    : ccl/public/base/iformatter.cpp
// Description : Value Formatter
//
//************************************************************************************************

#include "ccl/public/base/iformatter.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/vector.h"

#include "core/public/coreformatter.h"

namespace CCL {

//************************************************************************************************
// CoreFormatter
/** Core formatter adapter class. */
//************************************************************************************************

class CoreFormatter: public Formatter,
					 public IFormatterRange
{
public:
	CoreFormatter (const Core::Formatter& formatter);
	
	// Formatter
	tbool CCL_API printString (String& string, VariantRef value) const override;
	tbool CCL_API scanString (Variant& value, StringRef string) const override;

	// IFormatterRange
	void CCL_API setRange (VariantRef minValue, VariantRef maxValue) override;

	CLASS_INTERFACE (IFormatterRange, Formatter)

protected:
	const Core::Formatter& formatter;
	Core::Formatter::Range range;
};

//************************************************************************************************
// FormatterFactoryList
//************************************************************************************************

typedef Vector<FormatterFactory*> FormatterFactoryList;

static FormatterFactoryList& getFormatterFactories ()
{
	static Deleter<FormatterFactoryList> theFactories (NEW FormatterFactoryList);
	return *theFactories._ptr;
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// FormatterFactory
//************************************************************************************************

void FormatterFactory::add (FormatterFactory* factory)
{
	getFormatterFactories ().add (factory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFormatter* FormatterFactory::create (StringID name)
{
	VectorForEach (getFormatterFactories (), FormatterFactory*, factory)
		if(name == factory->name)
			return factory->create ();
	EndFor

	// try core formatter registry
	if(const Core::Formatter* coreFormatter = Core::FormatterRegistry::find (name))
		return NEW CoreFormatter (*coreFormatter);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFormatter* FormatterFactory::create (const Core::Formatter& formatter)
{
	return NEW CoreFormatter (formatter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFormatter* FormatterFactory::createInt ()
{
	return create (Core::IntFormatter::instance ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFormatter* FormatterFactory::createFloat ()
{
	return create (Core::FloatFormatter::instance ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FormatterFactory::FormatterFactory (CStringPtr name)
: name (name)
{
	add (this);
}

//************************************************************************************************
// CoreFormatter
//************************************************************************************************

CoreFormatter::CoreFormatter (const Core::Formatter& formatter)
: Formatter (formatter.getName ()),
  formatter (formatter)
{
	range.minValue = 0.f;
	range.maxValue = 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreFormatter::setRange (VariantRef minValue, VariantRef maxValue)
{
	range.minValue = minValue.asFloat ();
	range.maxValue = maxValue.asFloat ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreFormatter::printString (String& string, VariantRef value) const
{
	char stringBuffer[STRING_STACK_SPACE_MAX] = {0};
	Core::Formatter::Data data = {nullptr, stringBuffer, sizeof(stringBuffer), value.asFloat (), &range};
	formatter.print (data);
	string.empty ();
	string.appendASCII (stringBuffer);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreFormatter::scanString (Variant& value, StringRef string) const
{
	MutableCString cString (string);
	Core::Formatter::Data data = {nullptr, const_cast<char*> (cString.str ()), -1, 0, &range};
	if(!formatter.scan (data))
		return false;
	value = data.value;
	return true;
}
