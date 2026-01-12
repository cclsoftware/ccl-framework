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
// Filename    : core/portable/corevalues.cpp
// Description : Value Controller
//
//************************************************************************************************

#include "core/portable/corevalues.h"

#include "core/public/corestringbuffer.h"
#include "core/text/coreattributehandler.h"

using namespace Core;
using namespace Portable;

#define ForEachParamInfo(info) \
	for(int plIndex = 0; plIndex < paramListCount; plIndex++) \
	{ for(int pIndex = 0; pIndex < paramLists[plIndex].length; pIndex++) \
		{ const ParamInfo& info = paramLists[plIndex].infos[pIndex];

//************************************************************************************************
// Value
//************************************************************************************************

bool Value::asBool () const
{
	switch(type)
	{
	case kBool : return data.boolValue;
	case kFloat : return data.floatValue != 0.0f;
	case kInt : return data.intValue != 0;
	default : return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Value::asInt () const
{
	switch(type)
	{
	case kBool : return data.boolValue;
	case kFloat : return static_cast<int>(data.floatValue);
	case kInt : return data.intValue;
	default : return 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float Value::asFloat () const
{
	switch(type)
	{
	case kBool : return data.boolValue;
	case kFloat : return data.floatValue;
	case kInt : return static_cast<float>(data.intValue);
	default : return 0.0f;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Value& Value::operator = (const Value& rhs)
{
	type = rhs.type;

	switch(type)
	{
	case kBool : data.boolValue = rhs.data.boolValue; break;
	case kInt : data.intValue = rhs.data.intValue; break;
	case kFloat : data.floatValue = rhs.data.floatValue; break;
	default : break;
	}

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Value& Value::operator = (bool value)
{
	type = kBool;
	data.boolValue = value;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Value& Value::operator = (int value)
{
	type = kInt;
	data.intValue = value;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Value& Value::operator = (float value)
{
	type = kFloat;
	data.floatValue = value;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Value::operator == (const Value& rhs) const
{
	switch(type)
	{
	case kUnspecified : return rhs.type == kUnspecified;
	case kBool : return data.boolValue == rhs.asBool ();
	case kInt : return data.intValue == rhs.asInt ();
	case kFloat : return data.floatValue == rhs.asFloat ();
	default : return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Value::operator != (const Value& rhs) const
{
	switch(type)
	{
	case kUnspecified : return rhs.type != kUnspecified;
	case kBool : return data.boolValue != rhs.asBool ();
	case kInt : return data.intValue != rhs.asInt ();
	case kFloat : return data.floatValue != rhs.asFloat ();
	default : return false;
	}
}

//************************************************************************************************
// ValueController
//************************************************************************************************

ValueController::ValueController (const ParamInfo infos[], int count)
:  parent (nullptr),
   paramListCount (infos ? 1 : 0)
{
	::memset (paramLists, 0, sizeof(paramLists));
	paramLists[0].infos = infos;
	paramLists[0].length = count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ValueController::ValueController (const ParamInfo infos1[], int count1, const ParamInfo infos2[], int count2)
: parent (nullptr),
  paramListCount (2)
{
	::memset (paramLists, 0, sizeof(paramLists));
	paramLists[0].infos = infos1;
	paramLists[0].length = count1;
	paramLists[1].infos = infos2;
	paramLists[1].length = count2;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RootValueController* ValueController::getRootController () const
{
	return parent ? parent->getRootController () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ValueController* ValueController::findChild (CStringPtr name) const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Value ValueController::getValue (int paramTag) const
{
	return getModelValue (paramTag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueController::setValue (int paramTag, Value value, int flags /* = 0 */)
{
	const ParamInfo* info = getInfo (paramTag);
	if(info != nullptr)
	{
		switch(value.getType ())
		{
		case Value::kInt :
			value = static_cast<int>(info->makeValid (static_cast<ParamValue>(value.asInt ())));
			break;

		case Value::kFloat :
			value = info->makeValid (value.asFloat ());
			break;

		default :
			break;
		}
	}

	if(value != getValue (paramTag))
	{
		if(setModelValue (paramTag, value, flags))
		{
			signalValueChange (paramTag, info);
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueController::resetValue (int paramTag, int flags)
{
	const ParamInfo* info = getInfo (paramTag);

	if(info != nullptr && getValue (paramTag) != Value (info->defaultValue))
	{
		if(setModelValue (paramTag, info->defaultValue, flags))
		{
			signalValueChange (paramTag, info);
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamValue ValueController::getNormalized (int paramTag) const
{
	const ParamInfo* info = getInfo (paramTag);
	if(info == nullptr)
		return 0;

	ParamValue range = info->maxValue - info->minValue;
	if(range == 0)
		return 0;

	ParamValue value = getFloatValue (paramTag);
	return (value - info->minValue) / range;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueController::getRange (Value& min, Value& max, int paramTag) const
{
	const ParamInfo* info = getInfo (paramTag);
	if(info == nullptr)
		return false;

	min = info->minValue;
	max = info->maxValue;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueController::getDelta (Value& delta, int paramTag) const
{
	const ParamInfo* info = getInfo (paramTag);
	if(info == nullptr)
		return false;

	delta = info->deltaValue;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueController::setValueRelative (int paramTag, int steps, int flags /* = 0 */)
{
	const ParamInfo* info = getInfo (paramTag);
	if(info == nullptr)
		return false;

	ParamValue newValue = getValue (paramTag).asFloat() + steps * info->deltaValue;
	return setValue (paramTag, newValue, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueController::getTagByName (int& paramTag, CStringPtr name) const
{
	const ParamInfo* info = lookup (name);
	if(info == nullptr)
		return false;
	
	paramTag = info->tag;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueController::toString (StringResult& string, int paramTag) const
{
	if(getModelString (string, paramTag))
		return true;

	const ParamInfo* info = getInfo (paramTag);
	if(!info)
		return false;

	switch(info->type)
	{
	case ParamInfo::kToggle :
		ConstString (getBoolValue (paramTag) ? "On" : "Off").copyTo (string.charBuffer, string.charBufferSize);
		break;

	case ParamInfo::kInt :
		snprintf (string.charBuffer, string.charBufferSize, "%d", getIntValue (paramTag));
		break;
			
	case ParamInfo::kFloat :
		snprintf (string.charBuffer, string.charBufferSize, "%.02f", getFloatValue (paramTag));
		break;

	case ParamInfo::kList :
		{
			int count = (int)info->maxValue + 1;
			int index = getIntValue (paramTag);
			CStringPtr listString = info->stringList && index >= 0 && index < count ? info->stringList[index] : "";
			ConstString (listString).copyTo (string.charBuffer, string.charBufferSize);
		}
		break;
			
	default:
		ConstString ("").copyTo (string.charBuffer, string.charBufferSize);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueController::storeValues (AttributeHandler& writer) const
{
	ForEachParamInfo (info)
		if(info.flags & ParamInfo::kStorable)
		{
			switch(info.type)
			{
			case ParamInfo::kFloat :
				writer.setValue (info.name, getValue (info.tag).asFloat());
				break;

			default :
				writer.setValue (info.name, getIntValue (info.tag));
				break;
			}
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITypedObject* ValueController::getObject (CStringPtr name) const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ParamInfo* ValueController::getInfo (int paramTag) const
{
	// TODO: optimize if sorted by tag in ascending order...
	ForEachParamInfo (info)
		if(info.tag == paramTag)
			return &info;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ParamInfo* ValueController::lookup (CStringPtr _name) const
{
	ConstString name (_name);
	ForEachParamInfo (info)
		if(name == info.name)
			return &info;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ValueController::getModelString (StringResult& string, int paramTag) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IValueObserver* ValueController::getObserver () const
{
	RootValueController* rootController = getRootController ();
	return rootController ? rootController->getObserver () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ValueController::signalValueChange (int paramTag, const ParamInfo* paramInfo)
{
	RootValueController* rootController = getRootController ();
	if(rootController != nullptr)
	{
		if(paramInfo && (paramInfo->flags & ParamInfo::kStorable) != 0)
			rootController->setNeedsSave (true);

		rootController->signalValueChange (this, paramTag);
	}
}

//************************************************************************************************
// RootValueController
//************************************************************************************************

RootValueController::RootValueController (const ParamInfo infos1[], int count1, const ParamInfo infos2[], int count2)
: ValueController (infos1, count1, infos2, count2),
  dirty (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

RootValueController* RootValueController::getRootController () const
{
	return const_cast<RootValueController*> (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootValueController::addObserver (IValueObserver* observer)
{
	return observerList.add (observer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootValueController::removeObserver (IValueObserver* observer)
{
	return observerList.remove (observer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootValueController::signalValueChange (ValueController* controller, int paramTag)
{
	VectorForEach (observerList, IValueObserver *, observer)
		observer->valueChanged (controller, paramTag);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootValueController::needsSave () const
{
	return dirty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootValueController::setNeedsSave (bool needsSave)
{
	dirty = needsSave;
}
