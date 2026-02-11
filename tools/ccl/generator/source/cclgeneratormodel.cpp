//************************************************************************************************
//
// CCL Generator
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
// Filename    : cclgeneratormodel.cpp
// Description : Generator Tool model
//
//************************************************************************************************

#include "cclgeneratormodel.h"

using namespace CCL;

//************************************************************************************************
// LanguageConfig
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LanguageConfig, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

LanguageConfig::LanguageConfig ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID LanguageConfig::getLanguageID () const
{
	return languageId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String LanguageConfig::getBoolType () const
{
	String value = attrs.getString (LanguageConfigFormat::kAttrTypeBool);
	ASSERT (value.isEmpty () == false)
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String LanguageConfig::getIntType () const
{
	String value = attrs.getString (LanguageConfigFormat::kAttrTypeInt);
	ASSERT (value.isEmpty () == false)
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String LanguageConfig::getBigIntType () const
{
	String value = attrs.getString (LanguageConfigFormat::kAttrTypeBigInt);
	ASSERT (value.isEmpty () == false)
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String LanguageConfig::getFloatType () const
{
	String value = attrs.getString (LanguageConfigFormat::kAttrTypeFloat);
	ASSERT (value.isEmpty () == false)
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String LanguageConfig::getDoubleType () const
{
	String value = attrs.getString (LanguageConfigFormat::kAttrTypeDouble);
	ASSERT (value.isEmpty () == false)
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String LanguageConfig::getStringType () const
{
	String value = attrs.getString (LanguageConfigFormat::kAttrTypeString);
	ASSERT (value.isEmpty () == false)
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String LanguageConfig::getBoolValueTrue () const
{
	String value = attrs.getString (LanguageConfigFormat::kAttrBoolValueTrue);
	ASSERT (value.isEmpty () == false)
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String LanguageConfig::getBoolValueFalse () const
{
	String value = attrs.getString (LanguageConfigFormat::kAttrBoolValueFalse);
	ASSERT (value.isEmpty () == false)
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LanguageConfig* LanguageConfig::createFromAttributes (const Attributes& a)
{
	LanguageConfig* lang = NEW LanguageConfig;
	if(lang->load (a))
		return lang;

	safe_release (lang);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LanguageConfig::load (const Attributes& a)
{
	languageId = a.getCString (LanguageConfigFormat::kAttrId);
	attrs.copyFrom (a);
	return true;
}

//************************************************************************************************
// MetaModel::ModelObject
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MetaModel::ModelObject, Object)

//************************************************************************************************
// MetaModel::Root
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MetaModel::Root, MetaModel::ModelObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaModel::Root::Root ()
{
	constants.objectCleanup (true);
	definitions.objectCleanup (true);
	enums.objectCleanup (true);
	groups.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetaModel::Root::add (Constant* constant)
{
	constants.add (constant);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetaModel::Root::add (Definition* definition)
{
	definitions.add (definition);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetaModel::Root::add (Enumeration* enumeration)
{
	enums.add (enumeration);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetaModel::Root::add (MetaModel::Group* group)
{
	groups.add (group);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& MetaModel::Root::getConstants () const
{
	return constants;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& MetaModel::Root::getDefinitions () const
{
	return definitions;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& MetaModel::Root::getEnums () const
{
	return enums;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& MetaModel::Root::getGroups () const
{
	return groups;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::Root::hasData () const
{
	return !(
		definitions.isEmpty () &&
		enums.isEmpty () && 
		constants.isEmpty () &&
		groups.isEmpty ()
	);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::Root::load (const Attributes& a)
{
	description = a.getString (MetaFileFormat::kAttrDescription);

	// Constants
	if(auto* it = a.newQueueIterator (MetaFileFormat::kAttrConstants, ccl_typeid<Attributes> ()))
	{
		IterForEach (it, Attributes, attr)
			if(auto* constant = Constant::createFromAttributes (*attr))
				constants.add (constant);
		EndFor
	}

	// Definitions
	if(auto* it = a.newQueueIterator (MetaFileFormat::kAttrDefinitions, ccl_typeid<Attributes> ()))
	{
		IterForEach (it, Attributes, attr)
			if(auto* definition = Definition::createFromAttributes (*attr))
				definitions.add (definition);
		EndFor
	}

	// Enums
	if(auto* it = a.newQueueIterator (MetaFileFormat::kAttrEnumerations, ccl_typeid<Attributes> ()))
	{
		IterForEach (it, Attributes, attr)
			if(auto* constant = Enumeration::createFromAttributes (*attr))
				enums.add (constant);
		EndFor
	}

	// Groups
	if(auto* it = a.newQueueIterator (MetaFileFormat::kAttrGroups, ccl_typeid<Attributes> ()))
	{
		IterForEach (it, Attributes, attr)
			if(auto* group = Group::createFromAttributes (*attr))
				groups.add (group);
		EndFor
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::Root::save (Attributes& a) const
{
	if(!description.isEmpty ())
		a.setAttribute (MetaFileFormat::kAttrDescription, description);

	ForEach (constants, Constant, constant)
		Attributes* attr = NEW Attributes;
		constant->save (*attr);
		a.queue (MetaFileFormat::kAttrConstants, attr, Attributes::kOwns);
	EndFor

	ForEach (definitions, Definition, definition)
		Attributes* attr = NEW Attributes;
		definition->save (*attr);
		a.queue (MetaFileFormat::kAttrDefinitions, attr, Attributes::kOwns);
	EndFor

	ForEach (enums, Enumeration, enumeration)
		Attributes* attr = NEW Attributes;
		enumeration->save (*attr);
		a.queue (MetaFileFormat::kAttrEnumerations, attr, Attributes::kOwns);
	EndFor

	ForEach (groups, Group, group)
		Attributes* attr = NEW Attributes;
		group->save (*attr);
		a.queue (MetaFileFormat::kAttrGroups, attr, Attributes::kOwns);
	EndFor

	return true;
}

//************************************************************************************************
// MetaModel::Group
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MetaModel::Group, Root)

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaModel::Group* MetaModel::Group::createFromAttributes (const Attributes& a)
{
	Group* group = NEW Group;
	if(group->load (a))
		return group;

	safe_release (group);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::Group::load (const Attributes& a)
{
	if(!SuperClass::load (a))
		return false;

	name = a.getString (MetaFileFormat::kAttrName);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::Group::save (Attributes& a) const
{
	if(!name.isEmpty ())
		a.setAttribute (MetaFileFormat::kAttrName, String (name));

	return SuperClass::save (a);
}

//************************************************************************************************
// MetaModel::Documented
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MetaModel::Documented, ModelObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::Documented::load (const Attributes& a)
{
	brief = a.getString (MetaFileFormat::kAttrBrief);
	details = a.getString (MetaFileFormat::kAttrDetails);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::Documented::save (Attributes& a) const
{
	if(!brief.isEmpty ())
		a.setAttribute (MetaFileFormat::kAttrBrief, brief);

	if(!details.isEmpty ())
		a.setAttribute (MetaFileFormat::kAttrDetails, details);

	return true;
}

//************************************************************************************************
// MetaModel::ValueFunction
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MetaModel::ValueFunction, ModelObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaModel::ValueFunction* MetaModel::ValueFunction::createFromAttributes (const Attributes& a)
{
	ValueFunction* valueFunction = NEW ValueFunction;
	if(valueFunction->load (a))
		return valueFunction;

	safe_release (valueFunction);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<Variant>& MetaModel::ValueFunction::getArgs () const
{
	return args;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::ValueFunction::load (const Attributes& a)
{
	ASSERT (a.contains (MetaFileFormat::kAttrName))
	name = a.getString (MetaFileFormat::kAttrName);

	IterForEach (a.newQueueIterator (MetaFileFormat::kAttrArgs, ccl_typeid<Attribute> ()), Attribute, attr)
		Variant attrValue = attr->getValue ();
		args.add (attrValue);
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::ValueFunction::save (Attributes& a) const
{
	ASSERT (name.isEmpty () == false)
	a.setAttribute (MetaFileFormat::kAttrName, name);

	for(StringRef arg : args)
	{
		Variant argVariant;
		argVariant.fromString (arg);
		a.queueAttribute (MetaFileFormat::kAttrArgs, argVariant);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

//************************************************************************************************
// MetaModel::TypedValue
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MetaModel::TypedValue, Documented)

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaModel::TypedValue::TypedValue ()
: expression (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaModel::ValueFunction* MetaModel::TypedValue::getValueFunction () const
{
	return valueFunction;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::TypedValue::load (const Attributes& a)
{
	if(!SuperClass::load (a))
		return false;

	// Default to int.
	valueType = a.getString (MetaFileFormat::kAttrType);
	if(valueType.isEmpty ())
		valueType = MetaFileFormat::kValueTypeInt;

	expression = a.getBool (MetaFileFormat::kAttrExpression);

	// Value is denoted as a function. Expect value attribute
	// to not be set in this case. A model processing class
	// should resolve value from the function.

	Attributes* functionAttributes = a.getAttributes (MetaFileFormat::kAttrFunction);
	if(functionAttributes)
	{
		valueFunction = ValueFunction::createFromAttributes (*functionAttributes);
		ASSERT (a.contains (MetaFileFormat::kAttrValue) == false)
	}

	// Value is always stored as string to avoid floating
	// point number to string conversion, introducing
	// precision and rounding errors.

	Variant valueAttr;
	if(a.getAttribute (valueAttr, MetaFileFormat::kAttrValue))
	{
		ASSERT (valueAttr.isString ())
		value = valueAttr.toString ();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::TypedValue::save (Attributes& a) const
{
	a.setAttribute (MetaFileFormat::kAttrType, String (valueType));
	a.setAttribute (MetaFileFormat::kAttrValue, value);

	if(expression)
		a.setAttribute (MetaFileFormat::kAttrExpression, true);

	return SuperClass::save (a);
}

//************************************************************************************************
// MetaModel::Assignment
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MetaModel::Assignment, TypedValue)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::Assignment::load (const Attributes& a)
{
	if(!SuperClass::load (a))
		return false;

	name = a.getString (MetaFileFormat::kAttrName);
	ASSERT (!name.isEmpty ())

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::Assignment::save (Attributes& a) const
{
	ASSERT (!name.isEmpty ())
	a.setAttribute (MetaFileFormat::kAttrName, String (name));

	return SuperClass::save (a);
}

//************************************************************************************************
// MetaModel::Constant
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MetaModel::Constant, Assignment)

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaModel::Constant* MetaModel::Constant::createFromAttributes (const Attributes& a)
{
	Constant* constant = NEW Constant;
	if(constant->load (a))
		return constant;

	safe_release (constant);
	return nullptr;
}

//************************************************************************************************
// MetaModel::Definition
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MetaModel::Definition, Assignment)

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaModel::Definition* MetaModel::Definition::createFromAttributes (const Attributes& a)
{
	Definition* def = NEW Definition;
	if(def->load (a))
		return def;

	safe_release (def);
	return nullptr;
}

//************************************************************************************************
// MetaModel::Enumeration
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MetaModel::Enumeration, Documented)

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaModel::Enumeration* MetaModel::Enumeration::createFromAttributes (const Attributes& a)
{
	Enumeration* enumeration = NEW Enumeration;
	if(enumeration->load (a))
		return enumeration;

	safe_release (enumeration);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaModel::Enumeration::Enumeration ()
: autoValue (false)
{
	enumerators.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& MetaModel::Enumeration::getEnumerators () const
{
	return enumerators;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetaModel::Enumeration::addEnumerator (Enumerator* enumerator)
{
	// Takes ownership, see objectCleanup ().
	enumerators.add (enumerator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::Enumeration::load (const Attributes& a)
{
	if(!SuperClass::load (a))
		return false;

	name = a.getString (MetaFileFormat::kAttrName);
	ASSERT (!name.isEmpty ())

	autoValue = a.getBool (MetaFileFormat::kAttrAutoValue);

	// Query enumerators.
	if(auto* it = a.newQueueIterator (MetaFileFormat::kAttrEnumerators, ccl_typeid<Attributes> ()))
	{
		IterForEach (it, Attributes, attr)
			if(Enumerator* enumerator = Enumerator::createFromAttributes (*attr))
				enumerators.add (enumerator);
		EndFor
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaModel::Enumeration::save (Attributes& a) const
{
	ASSERT (!name.isEmpty ())
	a.setAttribute (MetaFileFormat::kAttrName, String (name));

	if(autoValue)
		a.setAttribute (MetaFileFormat::kAttrAutoValue, autoValue);

	// Enqueue enumerators.
	ForEach (enumerators, Enumerator, enumerator)
		Attributes* attr = NEW Attributes;
		enumerator->save (*attr);
		a.queue (MetaFileFormat::kAttrEnumerators, attr, Attributes::kOwns);
	EndFor

	return SuperClass::save (a);
}

//************************************************************************************************
// MetaModel::Enumerator
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MetaModel::Enumerator, Assignment)

////////////////////////////////////////////////////////////////////////////////////////////////////

MetaModel::Enumerator* MetaModel::Enumerator::createFromAttributes (const Attributes& a)
{
	Enumerator* enumerator = NEW Enumerator;
	if(enumerator->load (a))
		return enumerator;

	safe_release (enumerator);
	return nullptr;
}
