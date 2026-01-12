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
// Filename    : ccl/base/storage/configuration.h
// Description : Configuration
//
//************************************************************************************************

#ifndef _ccl_configuration_h
#define _ccl_configuration_h

#include "ccl/base/singleton.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/storage/iconfiguration.h"

namespace CCL {
class Settings;

namespace Configuration {

//************************************************************************************************
// Configuration::Registry
//************************************************************************************************

class Registry: public Object,
				public IRegistry,
				public Singleton<Registry>
{
public:
	DECLARE_CLASS (Registry, Object)
	DECLARE_METHOD_NAMES (Registry)

	Registry ();
	~Registry ();

	bool loadFromFile (UrlRef path);

	void initValue (StringID section, StringID key, int value);
	void initValue (StringID section, StringID key, bool value);
	void initValue (StringID section, StringID key, double value);
	void initValue (StringID section, StringID key, StringRef value);

	bool getValue (int& value, StringID section, StringID key) const;
	bool getValue (bool& value, StringID section, StringID key) const;
	bool getValue (double& value, StringID section, StringID key) const;
	bool getValue (String& value, StringID section, StringID key) const;

	// IRegistry
	void CCL_API setValue (StringID section, StringID key, VariantRef value) override;
	void CCL_API appendValue (StringID section, StringID key, VariantRef value) override;
	tbool CCL_API getValue (Variant& value, StringID section, StringID key) const override;

	CLASS_INTERFACE (IRegistry, Object)

protected:
	Settings& settings;

	void initValue (StringID section, StringID key, VariantRef value);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// Configuration::Value
//************************************************************************************************

class Value: public Object
{
public:
	Value (StringID section, StringID key);
	~Value ();

	CStringRef getSection () const;
	CStringRef getKey () const;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	CString section;
	CString key;
	mutable bool read;
};

//************************************************************************************************
// Configuration::TypeValue
//************************************************************************************************

template <typename T>
class TypeValue: public Value
{
public:
	TypeValue (StringID section, StringID key, T defaultValue = {});

	void setValue (T v);
	T getValue () const;
	operator T () const;
	operator T ();

protected:
	mutable T value;
};

/** Configuration integer value. */
typedef TypeValue<int> IntValue;

/** Configuration boolean value. */
typedef TypeValue<bool> BoolValue;

/** Configuration float value. */
typedef TypeValue<double> FloatValue;

/** Configuration string value. */
typedef TypeValue<String> StringValue;

//////////////////////////////////////////////////////////////////////////////////////////////////
// TypeValue implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
TypeValue<T>::TypeValue (StringID section, StringID key, T defaultValue)
: Value (section, key),
  value (defaultValue)
{
	Registry::instance ().initValue (section, key, defaultValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
void TypeValue<T>::setValue (T v)
{
	Registry::instance ().setValue (section, key, v);
	value = v;
	read = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T TypeValue<T>::getValue () const
{
	if(!read)
	{
		Registry::instance ().getValue (value, section, key);
		read = true;
	}
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
TypeValue<T>::operator T () const
{
	return getValue ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
TypeValue<T>::operator T ()
{
	return getValue ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Configuration
} // namespace CCL

#endif // _ccl_configuration_h
