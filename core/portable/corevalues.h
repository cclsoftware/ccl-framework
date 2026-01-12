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
// Filename    : core/portable/corevalues.h
// Description : Value Controller
//
//************************************************************************************************

#ifndef _corevalues_h
#define _corevalues_h

#include "core/portable/coreparaminfo.h"
#include "core/portable/coretypeinfo.h"

#include "core/public/corestringbuffer.h"
#include "core/public/corevector.h"

namespace Core {

struct AttributeHandler;

namespace Portable {

class ValueController;
class RootValueController;

//************************************************************************************************
// Value
/**	Container type for different types of values
\ingroup core_portable */
//************************************************************************************************

class Value
{
public:
	enum Type
	{
		kUnspecified, // kind of NULL
		kBool,
		kInt,
		kFloat
	};

	Value (): type (kUnspecified) { data.intValue = 0; }
	Value (bool value): type (kBool) { data.boolValue = value; }
	Value (int value): type (kInt) { data.intValue = value; }
	Value (float value): type (kFloat) { data.floatValue = value; }
	Value (const Value& other): type (other.type), data (other.data) {}

	Type getType () const { return type; }
	bool asBool () const;
	int asInt () const;
	float asFloat () const;

	Value& operator = (const Value& rhs);
	Value& operator = (bool value);
	Value& operator = (int value);
	Value& operator = (float value);
	bool operator == (const Value& rhs) const;
	bool operator != (const Value& rhs) const;

private:
	union Data
	{
		bool boolValue;
		int intValue;
		float floatValue;
	};

	Type type;
	Data data;
};

//************************************************************************************************
// IValueObserver
/**	Observer for value changes.
	\ingroup core_portable */
//************************************************************************************************

struct IValueObserver
{
	virtual void valueChanged (ValueController* controller, int paramTag) = 0;
};

//************************************************************************************************
// ValueController
/**	Lightweight value controller. Can be used as alternative to component and parameter objects.
	Derived class has to implement getModelValue(), setModelValue(), and optionally getModelString().
	\ingroup core_portable */
//************************************************************************************************

class ValueController: public TypedObject
{
public:
	ValueController (const ParamInfo infos[] = nullptr, int count = 0);
	ValueController (const ParamInfo infos1[], int count1, const ParamInfo infos2[], int count2);

	PROPERTY_CSTRING_BUFFER (32, name, Name)

	// Nesting
	PROPERTY_POINTER (ValueController, parent, Parent)
	virtual RootValueController* getRootController () const;
	virtual ValueController* findChild (CStringPtr name) const;

	// Values
    enum SetValueFlags
	{
		kOverrideGroup = 1 << 0 ///< override grouping behavior, i.e. don't apply value to a whole group
	};

	Value getValue (int paramTag) const;
	bool setValue (int paramTag, Value value, int flags = 0);
	bool resetValue (int paramTag, int flags = 0);
	ParamValue getNormalized (int paramTag) const;
	bool getRange (Value& min, Value& max, int paramTag) const;
	bool getDelta (Value& delta, int paramTag) const;
	bool setValueRelative (int paramTag, int steps, int flags = 0);
	bool getTagByName (int& paramTag, CStringPtr name) const;

	// Type conversion
	int getIntValue (int paramTag) const			{ return getValue (paramTag).asInt (); }
	bool setIntValue (int paramTag, int value)		{ return setValue (paramTag, value); }
	bool getBoolValue (int paramTag) const			{ return getValue (paramTag).asBool (); }
	bool setBoolValue (int paramTag, bool value)	{ return setValue (paramTag, value); }
	float getFloatValue (int paramTag) const		{ return getValue (paramTag).asFloat (); }
	bool setFloatValue (int paramTag, float value)	{ return setValue (paramTag, value); }

	// String conversion
	bool toString (StringResult& string, int paramTag) const;

	// Persistence
	void storeValues (AttributeHandler& writer) const;

	// Other
	virtual ITypedObject* getObject (CStringPtr name) const;

protected:	
	const ParamInfo* getInfo (int paramTag) const;
	const ParamInfo* lookup (CStringPtr name) const;

	virtual Value getModelValue (int paramTag) const = 0;
	virtual bool setModelValue (int paramTag, Value value, int flags) = 0;
	virtual bool getModelString (StringResult& string, int paramTag) const;

	IValueObserver* getObserver () const;
	void signalValueChange (int paramTag, const ParamInfo* paramInfo);

private:
	static const int kMaxParamLists = 2;
	
	struct
	{
		const ParamInfo* infos;
		int length;
	} paramLists[kMaxParamLists];
	int paramListCount;
};

//************************************************************************************************
// RootValueController
/**	Root of nested value controllers, provides the observer.
	\ingroup core_portable */
//************************************************************************************************

class RootValueController: public ValueController
{
public:
	RootValueController (const ParamInfo infos1[] = nullptr, int count1 = 0,
						 const ParamInfo infos2[] = nullptr, int count2 = 0);

	bool addObserver (IValueObserver* observer);
	bool removeObserver (IValueObserver* observer);
	void signalValueChange (ValueController* controller, int paramTag);

	bool needsSave () const;
	void setNeedsSave (bool needsSave);

	// ValueController
	RootValueController* getRootController () const override;

private:
	static const int kMaxObservers = 2;
	Core::FixedSizeVector<IValueObserver*, kMaxObservers> observerList;
	bool dirty;
};

} // namespace Portable
} // namespace Core

#endif // _corevalues_h
