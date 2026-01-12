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
// Filename    : ccl/app/paramcontainer.cpp
// Description : Parameter Container
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/paramcontainer.h"
#include "ccl/app/params.h"
#include "ccl/app/paramalias.h"

#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/settings.h"

using namespace CCL;

//************************************************************************************************
// ParamContainer
//************************************************************************************************

DEFINE_CLASS (ParamContainer, Object)
DEFINE_CLASS_NAMESPACE (ParamContainer, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParamContainer::newParameter (UIDRef cid) const
{
	if(cid == ClassID::Parameter)
		return NEW Parameter;
	else
	if(cid == ClassID::AliasParam)
		return NEW AliasParam;
	else
	if(cid == ClassID::IntParam)
		return NEW IntParam;
	else
	if(cid == ClassID::FloatParam)
		return NEW FloatParam;
	else
	if(cid == ClassID::StringParam)
		return NEW StringParam;
	else
	if(cid == ClassID::ListParam)
		return NEW ListParam;
	else
	if(cid == ClassID::MenuParam)
		return NEW MenuParam;
	else
	if(cid == ClassID::PaletteParam)
		return NEW PaletteParam;
	else
	if(cid == ClassID::CommandParam)
		return NEW CommandParam;
	else
	if(cid == ClassID::ScrollParam)
		return NEW ScrollParam;
	else
	if(cid == ClassID::ColorParam)
		return NEW ColorParam;
	else
	if(cid == ClassID::ImageProvider)
		return NEW ImageProvider;
	else
	if(cid == ClassID::TextModelProvider)
		return NEW TextModelProvider;
	return ParamList::newParameter (cid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ParamContainer::countParameters () const
{
	return count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API ParamContainer::getParameterAt (int index) const
{
	return at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API ParamContainer::findParameter (StringID name) const
{
	return lookup (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API ParamContainer::getParameterByTag (int tag) const
{
	return byTag (tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamContainer::storeSettingsIncrementally (StringRef settingsID) const
{
	if(params.isEmpty ())
		return;

	Attributes& a = Settings::instance ().getAttributes (settingsID);
	if(PersistentAttributes* a2 = a.getObject<PersistentAttributes> ("params"))
		storeValues (*a2, true); // add / overwrite currently storable params, but don't discard other previously stored values
	else
		storeSettings (settingsID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamContainer::storeSettings (StringRef settingsID) const
{
	if(params.isEmpty ())
		return;

	Attributes& a = Settings::instance ().getAttributes (settingsID);
	a.removeAll ();
	PersistentAttributes* a2 = NEW PersistentAttributes;
	storeValues (*a2, true);
	a.set ("params", a2, Attributes::kOwns);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamContainer::restoreSettings (StringRef settingsID, bool update)
{
	if(Settings::instance ().isEmpty (settingsID) || params.isEmpty ())
		return;

	Attributes& a = Settings::instance ().getAttributes (settingsID);
	PersistentAttributes* a2 = a.getObject<PersistentAttributes> ("params");
	if(a2)
		restoreValues (*a2, true, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ParamContainer::load (const Storage& storage)
{
	restoreValues (storage.getAttributes (), true, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ParamContainer::save (const Storage& storage) const
{
	storeValues (storage.getAttributes (), true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamContainer::storeValues (Attributes& a, bool storable) const
{
	for(int i = 0; i < count (); i++)
	{
		IParameter* p = at (i);
		if(storable && !p->isStorable ())
			continue;

		storeValue (a, p);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamContainer::storeValue (Attributes& a, StringID name) const
{
	IParameter* p = lookup (name);
	ASSERT (p)
	if(p)
		storeValue (a, p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamContainer::storeValue (Attributes& a, IParameter* p) const
{
	CCL_PRINT ("Save Param: ")
	CCL_PRINTLN (p->getName ())

	MutableCString keyName (p->getName ());
	ASSERT (keyName.isEmpty () == false)
	a.makeValidKey (keyName);

	Variant value (p->getValue ());
	if(p->isStoreListValue ())
	{
		UnknownPtr<IListParameter> list (p);
		if(list)
		{
			value = list->getSelectedValue ();
			ASSERT (value.getType () != Variant::kObject)
		}
	}

	a.setAttribute (keyName, value, value.getType () == Variant::kString ? Attributes::kTemp : 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamContainer::restoreValues (const Attributes& a, bool storable, bool update)
{
	for(int i = 0; i < count (); i++)
	{
		IParameter* p = at (i);
		if(storable && !p->isStorable ())
			continue;

		restoreValue (a, p, update);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ParamContainer::restoreValue (const Attributes& a, StringID name, bool update)
{
	IParameter* p = lookup (name);
	ASSERT (p)
	if(p)
		return restoreValue (a, p, update);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ParamContainer::restoreValue (const Attributes& a, IParameter* p, bool update)
{
	MutableCString keyName (p->getName ());
	a.makeValidKey (keyName);
	
	Variant value;
	if(a.getAttribute (value, keyName))
	{
		CCL_PRINT ("Load Param: ")
		CCL_PRINTLN (p->getName ())

		Parameter::restoreValue (p, value, update);
		return true;
	}
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamContainer::setDefaultValues (bool storable, bool update)
{
	for(int i = 0; i < count (); i++)
	{
		IParameter* p = at (i);
		if(storable && !p->isStorable ())
			continue;

		p->setValue (p->getDefaultValue (), update);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamContainer::enableAll (bool state)
{
	for(int i = 0; i < count (); i++)
	{
		IParameter* p = at (i);	
		p->enable (state);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamContainer::addParametersFrom (const ParamContainer& container)
{
	VectorForEachFast (container.params, IParameter*, p)
		if(Object* obj = unknown_cast<Object> (p))
		{
			UnknownPtr<IParameter> newParam (static_cast<IObject*> (obj->clone ()));
			add (newParam, p->getTag ());
		}
	EndFor

	// todo: copy arrays?
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ParamContainer)
	DEFINE_METHOD_ARGR ("add",	"param: Parameter", "Parameter")
	DEFINE_METHOD_ARGR ("addParam",	"name: string", "Parameter")
	DEFINE_METHOD_ARGR ("addFloat", "min: float, max: float, name: string", "FloatParam")
	DEFINE_METHOD_ARGR ("addInteger", "min: int, max: int, name: string", "IntParam")
	DEFINE_METHOD_ARGR ("addString", "name: string", "StringParam")
	DEFINE_METHOD_ARGR ("addList", "name: string", "ListParam")
	DEFINE_METHOD_ARGR ("addMenu", "name: string", "MenuParam")
	DEFINE_METHOD_ARGR ("addCommand", "commandCategory: string, commandName: string, name: string", "CommandParam")
	DEFINE_METHOD_ARGR ("addColor", "name: string", "ColorParam")
	DEFINE_METHOD_ARGR ("addAlias", "name: string", "AliasParam")
	DEFINE_METHOD_ARGR ("addImage", "name: string", "ImageProvider")
	DEFINE_METHOD_ARGR ("remove", "name: string", "Parameter")
	DEFINE_METHOD_ARGR ("lookup", "name: string", "Parameter")
	DEFINE_METHOD_ARGR ("findParameter", "name: string", "Parameter")
END_METHOD_NAMES (ParamContainer)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ParamContainer::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg.getID ().startsWith ("add"))
	{
		IParameter* p = nullptr;
		bool result = true;
		int tag = 100 + count () + 1;
		bool storable = true;

		if(msg == "add")
		{
			p = UnknownPtr<IParameter> (msg[0]);
			ASSERT (p != nullptr)
			if(p)
				add (return_shared (p));
		}
		else if(msg == "addParam")
			p = addParam (MutableCString (msg[0].asString ()), tag);
		else if(msg == "addFloat")
			p = addFloat ((float)msg[0], (float)msg[1], MutableCString (msg[2].asString ()), tag);
		else if(msg == "addInteger")
			p = addInteger ((int)msg[0], (int)msg[1], MutableCString (msg[2].asString ()), tag);
		else if(msg == "addString")
			p = addString (MutableCString (msg[0].asString ()), tag);
		else if(msg == "addList")
			p = addList (MutableCString (msg[0].asString ()), tag);
		else if(msg == "addMenu")
			p = addMenu (MutableCString (msg[0].asString ()), tag);
		else if(msg == "addCommand")
			p = addCommand (MutableCString (msg[0].asString ()), MutableCString (msg[1].asString ()), MutableCString (msg[2].asString ()));
		else if(msg == "addColor")
			p = addColor (MutableCString (msg[0].asString ()), tag);
		else if(msg == "addAlias")
			p = UnknownPtr<IParameter> (addAlias (MutableCString (msg[0].asString ()), tag));
		else if(msg == "addImage")
		{
			p = UnknownPtr<IParameter> (addImage (MutableCString (msg[0].asString ()), tag));
			storable = false;
		}
		else
			result = false;

		if(p)
		{
			p->setStorable (storable);
			p->setPublic (true);
		}

		returnValue = p;
		return result;
	}
	else if(msg == "remove")
	{
		IParameter* p = lookup (MutableCString (msg[0].asString ()));
		if(p)
		{
			remove (p);
			p->release ();
			returnValue = true;
		}
		return true;
	}
	else if(msg == "lookup" || msg == "findParameter")
	{
		IParameter* p = lookup (MutableCString (msg[0].asString ()));
		returnValue.takeShared (p);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
