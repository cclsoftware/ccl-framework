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
// Filename    : ccl/public/base/iformatter.h
// Description : Value Formatter
//
//************************************************************************************************

#ifndef _ccl_iformatter_h
#define _ccl_iformatter_h

#include "ccl/public/base/unknown.h"

namespace Core {
class Formatter; }

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Formatter macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define DEFINE_FORMATTER_METHODS \
CCL::tbool CCL_API printString (CCL::String& string, CCL::VariantRef value) const override \
{ string = print (value); return true; } \
CCL::tbool CCL_API scanString (CCL::Variant& value, CCL::StringRef string) const override \
{ return scan (value, string); }

#define DEFINE_FORMATTER_METHODS_NAMED(NameString) \
DEFINE_FORMATTER_METHODS \
CCL::CStringPtr CCL_API getFactoryName () const override { return factoryName ? factoryName : NameString; }

#define DECLARE_FORMATTER_FACTORY(ClassName) \
typedef CCL::TFormatterFactory<ClassName> Factory; \
static Factory __factory;

#define DEFINE_FORMATTER_FACTORY(ClassName, NameString) \
ClassName::Factory ClassName::__factory (NameString);

//************************************************************************************************
// IFormatter
/** Formatter interface, converts values to strings and vice versa. 
	\ingroup gui_param */
//************************************************************************************************

interface IFormatter: IUnknown
{
	/** Formatter flags. */
	enum Flags 
	{ 
		kNormalized = 1<<0, ///< formatter uses normalized values [0..1]
		kStateful = 1<<1	///< formatter cannot be used for arbitrary values
	};

	/** Get formatter flags. */
	virtual int CCL_API getFlags () const = 0;

	/** Convert value to string. */
	virtual tbool CCL_API printString (String& string, VariantRef value) const = 0;

	/** Convert string to value. */
	virtual tbool CCL_API scanString (Variant& value, StringRef string) const = 0;

	/** Get name of factory which created this instance. */
	virtual CStringPtr CCL_API getFactoryName () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////

	bool isNormalized () const { return (getFlags () & kNormalized) != 0; }
	bool isStateful () const { return (getFlags () & kStateful) != 0; }

	DECLARE_IID (IFormatter)
};

DEFINE_IID (IFormatter, 0x8e5a60d, 0xb0ac, 0x4d1c, 0x8b, 0xf6, 0x6f, 0x9a, 0x27, 0xfe, 0x41, 0x6f)

//************************************************************************************************
// IFormatterRange
/**
	\ingroup gui_param */
//************************************************************************************************

interface IFormatterRange: IUnknown
{
	virtual void CCL_API setRange (VariantRef minValue, VariantRef maxValue) = 0;

	DECLARE_IID (IFormatterRange)
};

DEFINE_IID (IFormatterRange, 0xb273e100, 0xd284, 0x4e07, 0x8d, 0xc7, 0x57, 0x47, 0xbd, 0x46, 0x4d, 0x1a)

//************************************************************************************************
// Formatter
/** Base class for implementing formatter objects. 
	\ingroup gui_param */
//************************************************************************************************

class Formatter: public Unknown,
				 public IFormatter
{
public:
	Formatter (CStringPtr factoryName = nullptr)
	: factoryName (factoryName)
	{}

	void setFactoryName (CStringPtr _factoryName) { factoryName = _factoryName; }

	// IFormatter
	int CCL_API getFlags () const override { return 0; }
	tbool CCL_API printString (String& string, VariantRef value) const override { return false; }
	tbool CCL_API scanString (Variant& value, StringRef string) const override { return false; }
	CStringPtr CCL_API getFactoryName () const override { return factoryName; }

	CLASS_INTERFACE (IFormatter, Unknown)

protected:
	CStringPtr factoryName;
};

//************************************************************************************************
// FormatterFactory
/** Formatter class registration. 
	\ingroup gui_param */
//************************************************************************************************

class FormatterFactory
{
public:
	static void add (FormatterFactory* factory);
	static IFormatter* create (StringID name);
	static IFormatter* create (const Core::Formatter& formatter);
	static IFormatter* createInt ();
	static IFormatter* createFloat ();

protected:
	CStringPtr name;
	FormatterFactory (CStringPtr name);

	virtual IFormatter* create () = 0;
};

//************************************************************************************************
// TFormatterFactory
/** Template for formatter factory class. 
	\ingroup gui_param */
//************************************************************************************************

template <class FormatterClass>
class TFormatterFactory: public FormatterFactory
{
public:
	TFormatterFactory (CStringPtr name): FormatterFactory (name) {}
	IFormatter* create () override { Formatter* f = NEW FormatterClass; f->setFactoryName (name); return f; }
};

} // namespace CCL

#endif // _ccl_iformatter_h

