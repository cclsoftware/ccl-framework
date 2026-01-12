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
// Filename    : ccl/base/storage/configuration.cpp
// Description : Configuration Registry
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/base/storage/configuration.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/storableobject.h"

#include "ccl/base/message.h"

namespace CCL {
namespace Configuration {

//************************************************************************************************
// Configuration::Loader
//************************************************************************************************

class Loader: public PersistentAttributes
{
public:
	DECLARE_CLASS (Loader, PersistentAttributes)

	// PersistentAttributes
	bool load (const Storage& storage) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
bool getRegistryValue (T& value, const Registry& registry, StringID section, StringID key)
{
	Variant var;
	if(registry.getValue (var, section, key))
	{
		value = var;
		return true;
	}
	return false;
}

} // namespace Configuration
} // namespace CCL

using namespace CCL;
using namespace Configuration;

//************************************************************************************************
// Configuration::Loader
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Loader, PersistentAttributes, "Configuration")
DEFINE_CLASS_NAMESPACE (Loader, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Loader::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Registry& registry = Registry::instance ();

	ForEachAttribute (*this, attrName, value)
		int index = attrName.lastIndex ('.');
		ASSERT (index != -1)
		if(index != -1)
		{
			MutableCString categoryPart = attrName.subString (0, index);
			MutableCString namePart = attrName.subString (index+1);
			registry.setValue (categoryPart, namePart, value);
		}
	EndFor
	return true;
}

//************************************************************************************************
// Configuration::Registry
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Registry, Object)
DEFINE_SINGLETON (Registry)

//////////////////////////////////////////////////////////////////////////////////////////////////

Registry::Registry ()
: settings (*NEW XmlSettings)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Registry::~Registry ()
{
	settings.release ();

	signal (Message (kDestroyed));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Registry::loadFromFile (UrlRef path)
{
	Loader loader;
	return StorableObject::loadFromFile (loader, path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Registry::initValue (StringID section, StringID key, int value)
{
	initValue (section, key, Variant (value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Registry::initValue (StringID section, StringID key, bool value)
{
	initValue (section, key, Variant (value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Registry::initValue (StringID section, StringID key, double value)
{
	initValue (section, key, Variant (value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Registry::initValue (StringID section, StringID key, StringRef value)
{
	initValue (section, key, Variant (value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Registry::initValue (StringID section, StringID key, VariantRef value)
{
	Attributes& a = settings.getAttributes (String (section));
	if(!a.contains (key))
		a.setAttribute (key, value, Attributes::kTemp);
	#if DEBUG_LOG
	else
		CCL_PRINTF ("Configuration variable %s %s already initialized!\n", section.str (), key.str ())
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Registry::setValue (StringID _section, StringID key, VariantRef value)
{
	String section (_section);
	settings.getAttributes (section).setAttribute (key, value, Attributes::kTemp);
	signal (Message (kChanged, section, String (key)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Registry::appendValue (StringID section, StringID key, VariantRef value)
{
	String list;
	getValue (list, section, key);
	if(list.isEmpty () == false)
		list.append (";");
	list.append (value.asString ());
	setValue (section, key, list);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Registry::getValue (Variant& value, StringID section, StringID key) const
{
	value.clear ();
	return settings.getAttributes (String (section)).getAttribute (value, key) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Registry::getValue (int& value, StringID section, StringID key) const
{
	return getRegistryValue<int> (value, *this, section, key);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Registry::getValue (bool& value, StringID section, StringID key) const
{
	return getRegistryValue<bool> (value, *this, section, key);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Registry::getValue (double& value, StringID section, StringID key) const
{
	return getRegistryValue<double> (value, *this, section, key);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Registry::getValue (String& value, StringID section, StringID key) const
{
	return getRegistryValue<String> (value, *this, section, key);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Registry::getProperty (Variant& var, MemberID propertyId) const
{
	// provide values as properties: value[section.key]
	MutableCString arrayKey;
	if(propertyId.getBetween (arrayKey, "value[", "]"))
	{
		int pointIdx = arrayKey.lastIndex ('.');
		if(pointIdx < 0)
			return false;

		MutableCString section (arrayKey.subString (0, pointIdx));
		MutableCString key (arrayKey.subString (pointIdx + 1));
		ASSERT (!section.isEmpty () && !key.isEmpty ())
		getValue (var, section, key);
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Registry)
	//DEFINE_METHOD_NAME ("setValue")
	DEFINE_METHOD_NAME ("getValue")
END_METHOD_NAMES (Registry)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Registry::invokeMethod (Variant& returnValue, MessageRef msg)
{
	/*if(msg == "setValue")
	{
		MutableCString section (msg[0].asString ());
		MutableCString key (msg[1].asString ());
		ASSERT (!section.isEmpty () && !key.isEmpty ())
		if(!section.isEmpty () && !key.isEmpty ())
			setValue (section, key, msg[2]);
		return true;
	}
	else*/
	if(msg == "getValue")
	{
		MutableCString section (msg[0].asString ());
		MutableCString key (msg[1].asString ());
		ASSERT (!section.isEmpty () && !key.isEmpty ())
		getValue (returnValue, section, key);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// Value
//************************************************************************************************

Value::Value (StringID section, StringID key)
: section (section),
  key (key),
  read (false)
{
	Registry::instance ().addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Value::~Value ()
{
	if(Registry* registry = Registry::peekInstance ())
		registry->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringRef Value::getSection () const
{
	return section;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringRef Value::getKey () const
{
	return key;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Value::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		MutableCString section (msg[0].asString ());
		MutableCString key (msg[1].asString ());
		if(section == this->section && key == this->key)
		{
			read = false;
			signal (Message (kChanged));
		}
	}
	else if(msg == kDestroyed)
	{
		Registry::instance ().removeObserver (this);
	}
}
