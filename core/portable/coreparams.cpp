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
// Filename    : core/portable/coreparams.cpp
// Description : Parameter class
//
//************************************************************************************************

#include "coreparams.h"
#include "corestorage.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// Parameter
//************************************************************************************************

Parameter::Parameter (const ParamInfo& info, bool ownsInfo)
: flags (0),
  visualState (0),
  hashCode (hashName (info.name)),
  info (&info),
  formatter (nullptr),
  controller (nullptr)
{
	if(ownsInfo)
		getMutableInfo ();

	// assign formatter
	if(!ConstString (info.unitName).isEmpty ())
		setFormatter (FormatterRegistry::find (info.unitName));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter::~Parameter ()
{
	signal (kDestroyed);
	ASSERT (observerList.isEmpty ())

	if(flags & kOwnsInfo)
		delete info;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamInfo& Parameter::getMutableInfo ()
{
	ASSERT (info != nullptr)
	if((flags & kOwnsInfo) == 0)
	{
		info = NEW ParamInfo (*info);
		flags |= kOwnsInfo;
	}

	return *const_cast<ParamInfo*> (info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr Parameter::getTitle (bool shortVersion) const
{
	if(shortVersion && !ConstString (info->shortTitle).isEmpty ())
		return info->shortTitle;
	return info->title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::setPublic (bool state)
{
	if(state != isPublic ())
	{
		ParamInfo& mutableInfo = getMutableInfo ();
		if(state)
			mutableInfo.flags &= ~ParamInfo::kPrivate;
		else
			mutableInfo.flags |= ParamInfo::kPrivate;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::setStorable (bool state)
{
	if(state != isStorable ())
	{
		ParamInfo& mutableInfo = getMutableInfo ();
		if(state)
			mutableInfo.flags |= ParamInfo::kStorable;
		else
			mutableInfo.flags &= ~ParamInfo::kStorable;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::setLinkable (bool state)
{
	if(state != isLinkable ())
	{
		ParamInfo& mutableInfo = getMutableInfo ();
		if(state)
			mutableInfo.flags |= ParamInfo::kLinkable;
		else
			mutableInfo.flags &= ~ParamInfo::kLinkable;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* Parameter::getOriginal ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::setController (IParamObserver* c)
{
	controller = c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParamObserver* Parameter::getController () const
{
	return controller;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::setFeedbackNeeded (bool state)
{
	if(state)
		flags |= kFeedback;
	else
		flags &=~kFeedback;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::beginEdit ()
{
	flags |= kIsEditing;
	if(controller)
		controller->paramChanged (this, kBeginEdit);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::endEdit ()
{
	flags &= ~kIsEditing;
	if(controller)
		controller->paramChanged (this, kEndEdit);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parameter::isEditing () const
{
	return (flags & kIsEditing) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION
void Parameter::performEdit ()
{
	if(controller)
		controller->paramChanged (this, kEdit);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::addObserver (IParamObserver* observer)
{
	observerList.add (observer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::removeObserver (IParamObserver* observer)
{
	ASSERT (observer /*&& observerList.contains (observer)*/)
	observerList.remove (observer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION
void Parameter::changed ()
{
	if(flags & kFeedback)
		if(controller)
			controller->paramChanged (this, kChanged);

	signal (kChanged);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::rangeChanged ()
{
	if(flags & kFeedback)
		if(controller)
			controller->paramChanged (this, kRangeChanged);

	signal (kRangeChanged);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION
void Parameter::signal (int msg)
{
	if(!observerList.isEmpty ())
	{
		// single observer, no copy needed
		if(observerList.count () == 1)
		{
			observerList[0]->paramChanged (this, msg);
		}
		// Notify off a copy of the list.
		// This protects us so that observers can safely remove themselves.
		else if(observerList.count () <= kMaxObserverCount)
		{
			FixedObserverList list2;
			list2.addAll (observerList);
			VectorForEachFast (list2, IParamObserver*, observer)
				observer->paramChanged (this, msg);
			EndFor
		}
		else
		{
			// worst case with dynamic memory allocation
			ObserverList list2;
			list2.addAll (observerList);
			VectorForEachFast (list2, IParamObserver*, observer)
				observer->paramChanged (this, msg);
			EndFor
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parameter::isEnabled () const
{
	return (flags & kDisabled) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::enable (bool state)
{
	if(state != isEnabled ())
	{
		if(state)
			flags &= ~kDisabled;
		else
			flags |= kDisabled;

		changed ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Parameter::getVisualState () const
{
	return visualState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::setVisualState (int state)
{
	if(state != visualState)
	{
		visualState = state;
		changed ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Parameter::isBipolar () const
{
	return (info->flags & ParamInfo::kBipolar) != 0;
}

//************************************************************************************************
// NumericParam
//************************************************************************************************

NumericParam::NumericParam (const ParamInfo& info, bool ownsInfo)
: Parameter (info, ownsInfo),
  value (info.defaultValue),
  interpolator (nullptr)
{
	// assign interpolator
	if(!ConstString (info.curveName).isEmpty ())
	{
		Interpolator* interpolator = InterpolatorFactory::create (info.curveName);
		if(interpolator)
			interpolator->setRange (info.minValue, info.maxValue, info.midValue);
		setInterpolator (interpolator);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NumericParam::~NumericParam ()
{
	if(interpolator)
		delete interpolator;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NumericParam::setInterpolator (Interpolator* _interpolator)
{
	if(interpolator)
		delete interpolator;

	interpolator = _interpolator;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Interpolator* NumericParam::getInterpolator () const
{
	return interpolator;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NumericParam::isNumeric () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamValue NumericParam::getMin () const
{
	return info->minValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamValue NumericParam::getMax () const
{
	return info->maxValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamValue NumericParam::getValue () const
{
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamValue NumericParam::getDefault () const
{
	return info->defaultValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int NumericParam::getPrecision () const
{
	return info->deltaValue != 0.0f ? (int)(1.f / info->deltaValue) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NumericParam::setMin (ParamValue min)
{
	if(min == info->minValue)
		return;

	ParamInfo& mutableInfo = getMutableInfo ();
	mutableInfo.minValue = min;

	if(value < min)
		setValue (min);
	else
		signal (kChanged);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NumericParam::setMax (ParamValue max)
{
	if(max == info->maxValue)
		return;

	ParamInfo& mutableInfo = getMutableInfo ();
	mutableInfo.maxValue = max;

	if(value > max)
		setValue (max);
	else
		signal (kChanged);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NumericParam::setDefault (ParamValue _value)
{
	if(_value == info->defaultValue)
		return;

	ParamInfo& mutableInfo = getMutableInfo ();
	mutableInfo.defaultValue = _value;

	signal (kChanged);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamValue NumericParam::normalizedToRange (ParamValue normalized) const
{
	if(interpolator)
		return interpolator->normalizedToRange (normalized);
	else
	{
		ParamValue range = info->maxValue - info->minValue;
		return normalized * range + info->minValue;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamValue NumericParam::rangeToNormalized (ParamValue value) const
{
	if(interpolator)
		return interpolator->rangeToNormalized (value);
	else
	{
		ParamValue range = info->maxValue - info->minValue;
		if(range == 0)
			return 0;
		return (value - info->minValue) / range;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION
void NumericParam::setValue (ParamValue v, bool edit)
{
	v = info->makeValid (v);
	if(value != v)
	{
		value = v;

		if(edit)
			performEdit ();

		changed ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NumericParam::resetValue (bool edit)
{
	setValue (getDefault (), edit);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamValue NumericParam::getNormalized () const
{
	return rangeToNormalized (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NumericParam::setNormalized (ParamValue v, bool edit)
{
	setValue (normalizedToRange (v), edit);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NumericParam::increment (int steps)
{
	if(getType () == ParamInfo::kFloat)
		incDecNormalized (info->deltaValue, steps);
	else
		setValue (getValue () + (steps * info->deltaValue), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NumericParam::decrement (int steps)
{
	if(getType () == ParamInfo::kFloat)
		incDecNormalized (-info->deltaValue, steps);
	else
		setValue (getValue () - (steps * info->deltaValue), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NumericParam::incDecNormalized (ParamValue delta, int stepCount)
{
	ParamValue oldValue = getNormalized ();

 	// check if min/max already reached
	if((delta > 0.f && oldValue >= 1.f) || (delta < 0.f && oldValue <= 0.f))
		return;

	bool valueChanged = false;
	for(int stepIndex = 0; stepIndex < stepCount; stepIndex++)
	{
		// try multiple times in case interpolator snaps to min/max
		for(int i = 1; i <= 10; i++)
		{
			ParamValue newValue = oldValue + i * delta;
			if(newValue > 1.f)
				newValue = 1.f;
			else if(newValue < 0.f)
				newValue = 0.f;

			// don't notify anyone at all.. no controllers, no signaling
			this->value = info->makeValid (normalizedToRange (newValue));

			ParamValue testValue = getNormalized ();
			if(testValue != oldValue)
			{
				oldValue = testValue;
				valueChanged = true;
				break;
			}
		}
	}

	if(valueChanged)
	{
		performEdit ();
		changed ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NumericParam::toString (char* string, int size) const
{
	if(formatter)
	{
		Formatter::Range range = {info->minValue, info->maxValue};
		Formatter::Data data = {this, string, size, value, &range};
		formatter->print (data);
	}
	else switch(getType())
	{
	case ParamInfo::kToggle:
		::strcpy (string, getBoolValue () ? "On" : "Off");
		break;

	case ParamInfo::kInt:
		::snprintf (string, size, "%d", getIntValue ());
		break;

	case ParamInfo::kFloat:
		::snprintf (string, size, "%.02f", getValue ());
		break;

	default:
		string[0] = '\0';
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NumericParam::fromString (CStringPtr string, bool edit)
{
	if(formatter)
	{
		Formatter::Range range = {info->minValue, info->maxValue};
		Formatter::Data data = {this, const_cast<char*> (string), -1, 0, &range};
		if(formatter->scan (data))
			setValue (data.value, edit);
	}
}

//************************************************************************************************
// ListParam
//************************************************************************************************

ListParam::ListParam (const ParamInfo& info, bool ownsInfo)
: NumericParam (info, ownsInfo),
  sharedList (info.stringList, (int)info.maxValue + 1),
  mutableList (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListParam::~ListParam ()
{
	if(mutableList)
		delete mutableList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListParam::hasModifiedRange () const
{
	return getMin () > 0 || getMax () < getStringCount () - 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ListParam::StringList& ListParam::getStringList () const
{
	if(mutableList)
		return *mutableList;
	return sharedList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListParam::MutableList& ListParam::getMutableList ()
{
	if(mutableList == nullptr)
		mutableList = NEW MutableList;
	return *mutableList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListParam::setSharedStrings (CStringPtr* strings, int count)
{
	ASSERT (mutableList == nullptr)
	if(mutableList)
	{
		delete mutableList;
		mutableList = nullptr;
	}

	sharedList = SharedList (strings, count);
	setMax (ParamValue (count-1));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListParam::removeAll ()
{
	if(getStringList ().getCount () == 0) // list already empty
		return;

	MutableList& stringList = getMutableList ();
	stringList.strings.removeAll ();
	setMax (-1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListParam::appendString (CStringPtr string)
{
	MutableList& stringList = getMutableList ();
	stringList.strings.add (string);
	setMax (ParamValue (stringList.strings.count ()-1));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListParam::isEmpty () const
{
	return getStringList ().getCount () == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr ListParam::getStringAt (int index) const
{
	ASSERT (formatter == nullptr) // doesn't work with formatter
	const StringList& stringList = getStringList ();

	int count = stringList.getCount ();
	if(index >= 0 && index < count)
		return stringList.getStringAt (index);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr ListParam::getSelectedString () const
{
	ASSERT (formatter == nullptr) // doesn't work with formatter
	return getStringAt (getIntValue ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListParam::getStringIndex (CStringPtr _string) const
{
	ASSERT (formatter == nullptr) // doesn't work with formatter
	ConstString string (_string);
	const StringList& stringList = getStringList ();

	int count = stringList.getCount ();
	for(int i = 0; i < count; i++)
		if(string.compare (stringList.getStringAt (i), false) == 0)
			return i;

	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListParam::getStringForValue (char* string, int size, int value) const
{
	if(formatter)
	{
		Formatter::Range range = {info->minValue, info->maxValue};
		Formatter::Data data = {this, string, size, (float)value, &range};
		formatter->print (data);
	}
	else
	{
		CStringPtr str = getStringAt (value);
		ASSERT (str)
		if(str)
			::strcpy (string, str);
		else
			::strcpy (string, "");
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListParam::toString (char* string, int size) const
{
	getStringForValue (string, size, getIntValue ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListParam::fromString (CStringPtr _string, bool edit)
{
	if(formatter)
	{
		Formatter::Range range = {info->minValue, info->maxValue};
		Formatter::Data data = {this, const_cast<char*> (_string), -1, 0, &range};
		if(formatter->scan (data))
			setValue (data.value, edit);
	}
	else
	{
		int idx = getStringIndex (_string);
		if(idx >= 0)
		{
			setIntValue (idx, edit);
			return;
		}

		ConstString string (_string);
		int64 index = -1;
		if(string.getIntValue (index))
			setIntValue ((int)index, edit);
	}
}

//************************************************************************************************
// StringParam
//************************************************************************************************

StringParam::StringParam (const ParamInfo& info, bool ownsInfo)
: Parameter (info, ownsInfo),
  defaultText (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringParam::~StringParam ()
{
	delete defaultText;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const StringParam::TextValue& StringParam::getDefaultText () const
{
	if(defaultText == nullptr)
	{
		static const TextValue emptyString;
		return emptyString;
	}
	return *defaultText;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringParam::resetValue (bool edit)
{
	fromString (getDefaultText (), edit);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringParam::setDefaultText (CStringPtr other)
{
	if(defaultText == nullptr)
		defaultText = NEW TextValue;
	*defaultText = other;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringParam::toString (char* string, int size) const
{
	text.copyTo (string, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringParam::fromString (CStringPtr string, bool edit)
{
	if(text != string)
	{
		text = string;

		if(edit)
			performEdit ();

		changed ();
	}
}

//************************************************************************************************
// ColorParam
//************************************************************************************************

ColorParam::ColorParam (const ParamInfo& info, bool ownsInfo)
: Parameter (info, ownsInfo),
  color (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorParam::ColorValue ColorParam::getColor () const
{
	return color;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorParam::ColorPart ColorParam::getRed () const
{
	return ColorPart (color & 0x000000FF);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorParam::ColorPart ColorParam::getGreen () const
{
	return ColorPart ((color >> 8) & 0x000000FF);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorParam::ColorPart ColorParam::getBlue () const
{
	return ColorPart ((color >> 16) & 0x000000FF);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorParam::ColorPart ColorParam::getAlpha () const
{
	return ColorPart ((color >> 24) & 0x000000FF);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorParam::setColor (ColorValue c, bool edit)
{
	if(c != color)
	{
		color = c;

		if(edit)
			performEdit ();

		changed ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorParam::setValue (ParamValue value, bool edit)
{
	ASSERT (false)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamValue ColorParam::getValue () const
{
	ASSERT (false)
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorParam::resetValue (bool edit)
{
	setColor (0, edit);
}

//************************************************************************************************
// AliasParam
//************************************************************************************************

AliasParam::AliasParam (const ParamInfo& info, bool ownsInfo)
: Parameter (info, ownsInfo),
  original (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AliasParam::~AliasParam ()
{
	setOriginal (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::setOriginal (Parameter* p)
{
	if(original != p)
	{
		if(original)
			original->removeObserver (this);
		original = p;
		if(original)
			original->addObserver (this);

		changed ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* AliasParam::getOriginal ()
{
	return original ? original->getOriginal () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::paramChanged (Parameter* p, int msg)
{
	if(p == original)
	{
		if(msg == kDestroyed)
			setOriginal (nullptr);
		else if(msg == kRangeChanged)
			rangeChanged ();
		else
		{
			ASSERT (msg == kChanged)
			changed ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::beginEdit ()
{
	if(original)
		original->beginEdit ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::endEdit ()
{
	if(original)
		original->endEdit ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AliasParam::isEditing () const
{
	return original && original->isEditing ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::performEdit ()
{
	if(original)
		original->performEdit ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AliasParam::isEnabled () const
{
	return original && original->isEnabled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::enable (bool state)
{
	if(original)
		original->enable (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AliasParam::getVisualState () const
{
	return original ? original->getVisualState () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::setVisualState (int state)
{
	if(original)
		original->setVisualState (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AliasParam::isBipolar () const
{
	return original ? original->isBipolar () : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AliasParam::isNumeric () const
{
	return original ? original->isNumeric () : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::setMin (ParamValue min)
{
	if(original)
		original->setMin (min);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::setMax (ParamValue max)
{
	if(original)
		original->setMax (max);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::setDefault (ParamValue value)
{
	if(original)
		original->setDefault (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamValue AliasParam::getMin () const
{
	return original ? original->getMin () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamValue AliasParam::getMax () const
{
	return original ? original->getMax () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamValue AliasParam::getValue () const
{
	return original ? original->getValue () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AliasParam::getPrecision () const
{
	return original ? original->getPrecision() : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamValue AliasParam::getDefault () const
{
	return original ? original->getDefault () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::setValue (ParamValue v, bool update)
{
	if(original)
		original->setValue (v, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::resetValue (bool edit)
{
	if(original)
		original->resetValue (edit);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamValue AliasParam::getNormalized () const
{
	return original ? original->getNormalized () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::setNormalized (ParamValue v, bool update)
{
	if(original)
		original->setNormalized (v, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::increment (int steps)
{
	if(original)
		original->increment (steps);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::decrement (int steps)
{
	if(original)
		original->decrement (steps);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::toString (char* string, int size) const
{
	if(original)
		original->toString (string, size);
	else
		string[0] = '\0';
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::fromString (CStringPtr string, bool edit)
{
	if(original)
		original->fromString (string, edit);
}

//************************************************************************************************
// ParamList
//************************************************************************************************

ParamList::ParamList ()
: feedbackNeeded (false),
  controller (nullptr),
  storableParamCount (0),
  publicParamCount (0),
  sortedByTag (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamList::~ParamList ()
{
	unsigned int i = 0;
	VectorForEach (params, Parameter*, p)
		delete p;
		++i;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamList::setController (IParamObserver* c)
{
	controller = c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamList::add (Parameter* p)
{
	// check if parameters are still sorted by tag in ascending order
	if(sortedByTag && !params.isEmpty ())
		if(p->getTag () < params[params.count ()-1]->getTag ())
			sortedByTag = false;

	ASSERT (find (p->getName ()) == nullptr) // check for name/hash conflict

	p->setController (controller);
	p->setFeedbackNeeded (feedbackNeeded);
	params.add (p);

	if(p->isStorable ())
		storableParamCount++;
	if(p->isPublic ())
		publicParamCount++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* ParamList::add (const ParamInfo& info, bool ownsInfo)
{
	Parameter* p = nullptr;
	switch(info.type)
	{
	case ParamInfo::kAlias : p = NEW AliasParam (info, ownsInfo); break;
	case ParamInfo::kString : p = NEW StringParam (info, ownsInfo); break;
	case ParamInfo::kList : p = NEW ListParam (info, ownsInfo); break;
	case ParamInfo::kColor : p = NEW ColorParam (info, ownsInfo); break;
	default :
		p = NEW NumericParam (info, ownsInfo);
	}

	add (p);
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamList::add (const ParamInfo infos[], int count, bool ownsInfo)
{
	if(count > params.getDelta ()) // avoid multiple reallocations
		params.resize (params.count () + count);

	if(infos != nullptr) // call with null can be used to reserve memory
		for(int i = 0; i < count; i++)
			add (infos[i], ownsInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamList::remove (Parameter* p)
{
	if(params.remove (p))
	{
		if(p->isStorable ())
			storableParamCount--;
		if(p->isPublic ())
			publicParamCount--;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamList::sortAll ()
{
	struct Sorter
	{
		static int compare (const void* lhs, const void* rhs)
		{
			const Parameter* p1 = *reinterpret_cast<Parameter*const*> (lhs);
			const Parameter* p2 = *reinterpret_cast<Parameter*const*> (rhs);
			return p1->getTag () - p2->getTag ();
		}
	};

	if(!sortedByTag)
	{
		params.sort (Sorter::compare);
		sortedByTag = true;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////

struct ParamList::TagFinder
{
	static int compare (const void* key, const void* item)
	{
		int tag = *reinterpret_cast<const int*> (key);
		const Parameter* p = *reinterpret_cast<Parameter*const*> (item);
		return tag - p->getTag ();
	}

	static Parameter* binarySearch (Parameter* params[], int count, int tag)
	{
		void* result = ::bsearch (&tag, params, count, sizeof(Parameter*), compare);
		return result ? *reinterpret_cast<Parameter**> (result) : nullptr;
	}

	static int binarySearchIndex (Parameter* params[], int count, int tag)
	{
		void* result = ::bsearch (&tag, params, count, sizeof(Parameter*), compare);
		if(result == nullptr)
			return -1;
		return (int)(reinterpret_cast<Parameter**> (result) - params);
	}

	static Parameter* linearSearch (Parameter* params[], int count, int tag)
	{
		for(int i = 0; i < count; i++)
			if(params[i]->getTag () == tag)
				return params[i];
		return nullptr;
	}

	static int linearSearchIndex (Parameter* params[], int count, int tag)
	{
		for(int i = 0; i < count; i++)
			if(params[i]->getTag () == tag)
				return i;
		return -1;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION Parameter* ParamList::byTag (int tag) const
{
	if(sortedByTag)
	{
		Parameter* p = TagFinder::binarySearch (params, params.count (), tag);
		//ASSERT (p == TagFinder::linearSearch (params, params.count (), tag))
		return p;
	}
	else
		return TagFinder::linearSearch (params, params.count (), tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ParamList::getIndexByTag (int tag) const
{
	if(sortedByTag)
	{
		int index = TagFinder::binarySearchIndex (params, params.count (), tag);
		//ASSERT (index == TagFinder::linearSearchIndex (params, params.count (), tag))
		return index;
	}
	else
		return TagFinder::linearSearchIndex (params, params.count (), tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION Parameter* ParamList::find (CStringPtr _name) const
{
	#if 1
	// LATER TODO: use vector sorted by hash code...
	uint32 hashCode = Parameter::hashName (_name);
	VectorForEachFast (params, Parameter*, p)
		if(p->getHashCode () == hashCode)
			return p;
	EndFor
	#else
	ConstString name (_name);
	VectorForEachFast (params, Parameter*, p)
		if(name.equalsUnsafe (p->getName ()))
			return p;
	EndFor
	#endif
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* ParamList::addAlias (int tag, CStringPtr name)
{
	ParamInfo info = {0};
	info.type = ParamInfo::kAlias;
	info.tag = tag;
	ConstString (name).copyTo (info.name, ParamInfo::kMaxNameLength);
	return add (info, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AliasParam* ParamList::getAlias (int tag) const
{
	return AliasParam::cast (byTag (tag));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamList::storeValues (OutputStorage& s) const
{
	AttributeHandler& writer = s.getWriter ();
	const IStorageFilter* filter = s.getFilter ();

	VectorForEachFast (params, Parameter*, p)
		if(!p->isStorable ())
			continue;

		if(filter && !filter->shouldSave (Parameter::kTypeID, p->getName (), p))
			continue;

		// parameter name can be shared instead of copied if it points to static memory
		int flags = p->isOwnInfo () ? 0 : Attribute::kShareID;

		switch(p->getType ())
		{
		case ParamInfo::kFloat :
			writer.setValue (p->getName (), p->getValue (), flags);
			break;

		case ParamInfo::kString :
			writer.setValue (p->getName (), ((StringParam*)p)->getText (), flags);
			break;

		case ParamInfo::kColor :
			writer.setValue (p->getName (), (int64)((ColorParam*)p)->getColor (), flags);
			break;

		default :
			writer.setValue (p->getName (), p->getIntValue (), flags);
			break;
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION void ParamList::restoreValues (const InputStorage& s, bool edit)
{
	const Attributes& a = s.getAttributes ();
	const IStorageFilter* filter = s.getFilter ();

	#if 1 // optimized access to attributes
	AttributesIterator attributeIter (a);
	const Attribute* nextAttribute = attributeIter.next ();

	VectorForEachFast (params, Parameter*, p)
		if(!p->isStorable () || !nextAttribute)
			continue;

		ConstString paramName (p->getName ());
		if(filter && !filter->shouldLoad (Parameter::kTypeID, paramName, p))
		{
			// skip a matching next attribute, to facilitate further iteration
			if(nextAttribute->getID () == paramName)
				nextAttribute = attributeIter.next ();

			continue;
		}

		// try the next attribute in the list first, since it's very likely
		// that the attributes are in the same order as the parameters (as they were saved)
		const Attribute* attr = nullptr;
		if(nextAttribute->getID () == paramName)
		{
			attr = nextAttribute;
			nextAttribute = attributeIter.next ();
		}
		else
		{
			// next attribute does not match, search in remaining attributes
			AttributesIterator remainingIter (attributeIter);
			while(const Attribute* na = remainingIter.next ())
				if(na->getID () == paramName)
				{
					attr = na;
					break;
				}
		}
		if(attr == nullptr)
			continue;

		switch(p->getType ())
		{
		case ParamInfo::kString :
			p->fromString (attr->getString (), edit);
			break;

		case ParamInfo::kFloat :
			p->setValue ((ParamValue)attr->getFloat (), edit);
			break;

		case ParamInfo::kColor :
			((ColorParam*)p)->setColor ((ColorParam::ColorValue)attr->getInt (), edit);
			break;

		default :
			p->setIntValue ((int)attr->getInt (), edit);
			break;
		}
	EndFor

	#else // non-optimized version with linear search in attributes
	VectorForEachFast (params, Parameter*, p)
		if(!p->isStorable ())
			continue;

		if(filter && !filter->shouldLoad (Parameter::kTypeID, p->getName (), p))
			continue;

		const Attribute* attr = a.lookup (p->getName ());
		if(attr == 0)
			continue;

		switch(p->getType ())
		{
		case ParamInfo::kString :
			p->fromString (attr->getString (), edit);
			break;

		case ParamInfo::kFloat :
			p->setValue ((ParamValue)attr->getFloat (), edit);
			break;

		case ParamInfo::kColor :
			((ColorParam*)p)->setColor ((ColorParam::Color)attr->getInt (), edit);
			break;

		default :
			p->setIntValue ((int)attr->getInt (), edit);
			break;
		}
	EndFor
	#endif
}
