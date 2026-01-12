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
// Filename    : core/portable/coreparams.h
// Description : Parameter class
//
//************************************************************************************************

#ifndef _coreparams_h
#define _coreparams_h

#include "coreparaminfo.h"
#include "coretypeinfo.h"

#include "core/public/corevector.h"
#include "core/public/coreformatter.h"
#include "core/public/coreinterpolator.h"
#include "core/public/corestringbuffer.h"

namespace Core {
namespace Portable {

class Parameter;
class ListParam;
class InputStorage;
class OutputStorage;

/**
	@class Parameter

	Parameter Protocol
	==================

	A parameter object is the link between application logic and UI elements. It is owned
	by a single controller instance, which is associated to the parameter via setController().
	The controller role is different from other observers. If user input causes the parameter
	to change its value (edit = true), the controller is notified to update the underlying
	data model (msg = kEdit).

	If the underlying data model changes (edit = true/false), changing the parameter value
	forces the UI element to update its value representation (msg = kChanged).
	A single parameter can be associated with multiple UI elements simultaneously via addObserver(),
	and there is no additional coding required to keep all representations in sync.

	Observers need to be removed from the parameter in a clean way. The latest possible
	point in time is when the parameter is about to be destroyed (msg = kDestroyed).

	Interaction between parameters like linking, etc. needs to be implemented by the controller class.

	To simplify scenarios where the controller wants to be notified of non-editing changes of
	its own parameters, setFeedbackNeeded() can be used instead of addObserver(). In this case
	the controller will receive two paramChanged() calls, kEdit and kChanged.

	* String Conversion

	Parameters can customize their string representation by assigning a specialized formatter class
	via setFormatter(). This is the preferred method of customization instead of deriving your own
	parameter classes.

	* Normalization

	For numeric parameters, the normalized value representation [0..1] can be customized via
	interpolator objects. This can be useful if e.g. the parameter uses a logarithmic scale
	instead of a linear scale.
*/

//************************************************************************************************
// IParamObserver
/** Parameter observer interface.
	\ingroup core_portable */
//************************************************************************************************

struct IParamObserver: ITypedObject
{
	DECLARE_CORE_CLASS_ ('IPaO', IParamObserver)
	virtual void paramChanged (Parameter* p, int msg) = 0;
};

//************************************************************************************************
// IParamMenuCustomizer
/** Provide menu customization with ListParam.
	\ingroup core_portable */
//************************************************************************************************

struct IParamMenuCustomizer: ITypedObject
{
	DECLARE_CORE_CLASS_ ('IPMC', IParamMenuCustomizer)
	virtual bool isParamMenuItemEnabled (ListParam* listParam, int index) = 0;
};

//************************************************************************************************
// ParamObserverBase
/** Base class to implement a standalone parameter observer.
	\ingroup core_portable */
//************************************************************************************************

class ParamObserverBase: public IParamObserver
{
public:
	// ITypedObject
	void* castTo (TypeID typeId) override { return nullptr; }
};

//************************************************************************************************
// Parameter
/** \brief Parameter base class.
	\ingroup core_portable */
//************************************************************************************************

class Parameter: public TypedObject
{
public:
	DECLARE_CORE_CLASS ('Para', Parameter, TypedObject)

	Parameter (const ParamInfo& info, bool ownsInfo = false);
	~Parameter ();

	enum MsgType
	{
		kBeginEdit,		///< enter editing state, send to controller (optional)
		kEndEdit,		///< exit editing state, send to controller (optional)
		kEdit,			///< send to controller to update the underlying data model
		kChanged,		///< send to observers to update their UI representation
		kRangeChanged,	///< send to observers if min/max or list entries changed (optional)
		kDestroyed		///< parameter is being destroyed, last chance to remove observers!
	};

	void setFormatter (const Formatter* formatter);
	const Formatter* getFormatter () const;

	// Identity
	ParamType getType () const;
	int getTag () const;
	CStringPtr getName () const;
	CStringPtr getTitle (bool shortVersion = false) const; ///< shortVersion: use short version if available
	bool isOwnInfo () const;
	bool isPublic () const;
	void setPublic (bool state = true);
	bool isStorable () const;
	void setStorable (bool state = true);
	bool isLinkable () const;
	void setLinkable (bool state = true);
	bool isMutable () const;
	bool isReadOnly () const;
	bool isUserFlag1 () const;
	bool isUserFlag2 () const;

	virtual Parameter* getOriginal ();

	// Controller
	void setController (IParamObserver* controller);
	IParamObserver* getController () const;
	void setFeedbackNeeded (bool state);

	virtual void beginEdit ();
	virtual void endEdit ();
	virtual bool isEditing () const;
	virtual void performEdit ();

	// Observer
	void addObserver (IParamObserver* observer);
	void removeObserver (IParamObserver* observer);
	void changed ();
	void rangeChanged (); ///< explicit signal if min/max or list entries changed

	// State
	virtual bool isEnabled () const;
	virtual void enable (bool state);
	virtual int getVisualState () const;
	virtual void setVisualState (int state);

	// Value
	virtual bool isBipolar () const;
	virtual bool isNumeric () const = 0;
	virtual ParamValue getMin () const = 0;
	virtual ParamValue getMax () const = 0;
	virtual ParamValue getDefault () const = 0;
	virtual ParamValue getValue () const = 0;
	virtual int getPrecision () const = 0;
	virtual void setMin (ParamValue min) = 0;
	virtual void setMax (ParamValue max) = 0;
	virtual void setDefault (ParamValue value) = 0;
	virtual void setValue (ParamValue value, bool edit = false) = 0;
	virtual void resetValue (bool edit = false) = 0;
	virtual ParamValue getNormalized () const = 0;
	virtual void setNormalized (ParamValue v, bool edit = false) = 0;
	virtual void increment (int steps = 1) = 0;
	virtual void decrement (int steps = 1) = 0;

	// Value conversion for convenience
	int getIntMin () const								{ return (int)getMin (); }
	int getIntMax () const								{ return (int)getMax (); }
	int getIntDefault () const							{ return (int)getDefault (); }
	void setIntMin (int min)							{ setMin ((ParamValue)min); }
	void setIntMax (int max)							{ setMax ((ParamValue)max); }
	void setIntDefault (int value)						{ setDefault ((ParamValue)value); }
	void setIntValue (int value, bool edit = false)		{ setValue ((ParamValue)value, edit); }
	int getIntValue () const							{ return (int)getValue (); }
	void setBoolValue (bool value, bool edit = false)	{ setValue (value ? 1.f : 0.f, edit); }
	bool getBoolValue () const							{ return getValue () != 0.f; }

	// String conversion
	virtual void toString (char* string, int size) const = 0;
	virtual void fromString (CStringPtr string, bool edit = false) = 0;

	// Name hash
	uint32 getHashCode () const;
	static INLINE uint32 hashName (CStringPtr name)
	{
		return name ? CStringFunctions::hashDJB (name) : 0;
	}

protected:
	typedef Vector<IParamObserver*> ObserverList;
	static const int kMaxObserverCount = 64;
	typedef FixedSizeVector<IParamObserver*, kMaxObserverCount> FixedObserverList;

	enum Flags
	{
		kOwnsInfo = 1<<0,
		kDisabled = 1<<1,
		kIsEditing = 1<<2,
		kFeedback = 1<<3
		// TODO: don't copy info to adjust certain flags at runtime...
		//kMutableLinkState = 1<<4
	};

	int flags;
	int visualState;
	uint32 hashCode;
	const ParamInfo* info;
	const Formatter* formatter;
	IParamObserver* controller;
	ObserverList observerList;

	ParamInfo& getMutableInfo ();
	void signal (int msg);
};

//************************************************************************************************
// NumericParam
/** Numeric parameter (int/float).
	\ingroup core_portable */
//************************************************************************************************

class NumericParam: public Parameter
{
public:
	DECLARE_CORE_CLASS ('NPar', NumericParam, Parameter)

	NumericParam (const ParamInfo& info, bool ownsInfo = false);
	~NumericParam ();

	static NumericParam* cast (Parameter* p) { return core_cast<NumericParam> (p); }

	void setInterpolator (Interpolator* interpolator); ///< takes ownerhip!
	const Interpolator* getInterpolator () const;

 	ParamValue normalizedToRange (ParamValue normalized) const;
	ParamValue rangeToNormalized (ParamValue value) const;

	// bypass virtual table and validity test
	INLINE void setValueFast (ParamValue v)
	{
		if(v != value)
		{
			value = v;
			changed ();
		}
	}

	// Parameter
	bool isNumeric () const override;
	ParamValue getMin () const override;
	ParamValue getMax () const override;
	ParamValue getDefault () const override;
	ParamValue getValue () const override;
	int getPrecision () const override;
	void setMin (ParamValue min) override;
	void setMax (ParamValue max) override;
	void setDefault (ParamValue value) override;
	void setValue (ParamValue value, bool edit = false) override;
	void resetValue (bool edit = false) override;
	ParamValue getNormalized () const override;
	void setNormalized (ParamValue v, bool edit = false) override;
	void increment (int steps = 1) override;
	void decrement (int steps = 1) override;
	void toString (char* string, int size) const override;
	void fromString (CStringPtr string, bool edit = false) override;

protected:
	ParamValue value;
	Interpolator* interpolator;

	void incDecNormalized (ParamValue delta, int steps);
};

//************************************************************************************************
// ListParam
/** List of strings.
	\ingroup core_portable */
//************************************************************************************************

class ListParam: public NumericParam
{
public:
	DECLARE_CORE_CLASS ('LPar', ListParam, NumericParam)

	ListParam (const ParamInfo& info, bool ownsInfo = false);
	~ListParam ();

	static ListParam* cast (Parameter* p) { return core_cast<ListParam> (p); }

	bool hasModifiedRange () const; // range doesn't include all strings

	void setSharedStrings (CStringPtr* strings, int count);
	CStringPtr* getSharedStrings () const;

	void removeAll ();
	void appendString (CStringPtr string);

	// access to string list (doesn't work with formatter)
	bool isEmpty () const;
	int getStringCount () const;
	CStringPtr getStringAt (int index) const;
	CStringPtr getStringAtUnsafe (int index) const;
	CStringPtr getSelectedString () const;
	int getStringIndex (CStringPtr string) const;

	// string conversion (works with both, string list and formatter)
	void getStringForValue (char* string, int size, int value) const;

	// NumericParam
	void toString (char* string, int size) const override;
	void fromString (CStringPtr string, bool edit = false) override;

protected:
	struct StringList
	{
		virtual ~StringList () {}
		virtual int getCount () const = 0;
		virtual CStringPtr getStringAt (int index) const = 0;
	};

	struct SharedList: StringList
	{
		CStringPtr* strings;
		int count;

		SharedList (CStringPtr* strings, int count)
		: strings (strings),
		  count (strings ? count : 0)
		{}

		// StringList
		int getCount () const override { return count; }
		CStringPtr getStringAt (int index) const override { return strings[index]; }
	};

	struct MutableList: StringList
	{
		Vector<CString128> strings;

		// StringList
		int getCount () const override { return strings.count (); }
		CStringPtr getStringAt (int index) const override { return strings[index].str (); }
	};

	SharedList sharedList;
	MutableList* mutableList;

	const StringList& getStringList () const;
	MutableList& getMutableList ();
};

//************************************************************************************************
// StringParam
/** String value.
	\ingroup core_portable */
//************************************************************************************************

class StringParam: public Parameter
{
public:
	DECLARE_CORE_CLASS ('SPar', StringParam, Parameter)

	StringParam (const ParamInfo& info, bool ownsInfo = false);
	~StringParam ();

	static StringParam* cast (Parameter* p) { return core_cast<StringParam> (p); }

	typedef CString256 TextValue;
	const TextValue& getText () const;

	const TextValue& getDefaultText () const;
	void setDefaultText (CStringPtr defaultText);

	// Parameter
	bool isNumeric () const override { return false; }
	void toString (char* string, int size) const override;
	void fromString (CStringPtr string, bool edit = false) override;
	void resetValue (bool edit = false) override;

protected:
	TextValue text;
	TextValue* defaultText;

	// Parameter (not implemented):
	ParamValue getMin () const override { return 0; }
	ParamValue getMax () const override { return 0; }
	ParamValue getValue () const override { return 0; }
	ParamValue getDefault () const override { return 0; }
	int getPrecision () const override { return 0; }
	void setMin (ParamValue min) override {}
	void setMax (ParamValue max) override {}
	void setDefault (ParamValue value) override {}
	void setValue (ParamValue value, bool edit = false) override {}
	ParamValue getNormalized () const override { return 0; }
	void setNormalized (ParamValue v, bool edit = false) override {}
	void increment (int steps = 1) override {}
	void decrement (int steps = 1) override {}
};

//************************************************************************************************
// ColorParam
/** Color value.
	\ingroup core_portable */
//************************************************************************************************

class ColorParam: public Parameter
{
public:
	DECLARE_CORE_CLASS ('CPar', ColorParam, Parameter)

	ColorParam (const ParamInfo& info, bool ownsInfo = false);

	static ColorParam* cast (Parameter* p) { return core_cast<ColorParam> (p); }

	typedef uint32 ColorValue;
	typedef uint8 ColorPart;

	ColorValue getColor () const;
	void setColor (ColorValue, bool edit);

	ColorPart getRed () const;
	ColorPart getGreen () const;
	ColorPart getBlue () const;
	ColorPart getAlpha () const;

	// Parameter
	bool isNumeric () const override { return false; }
	void setValue (ParamValue value, bool edit = false) override;
	ParamValue getValue () const override;
	void resetValue (bool edit = false) override;

protected:
	ColorValue color;

	// Parameter (not implemented):
	ParamValue getMin () const override { return 0; }
	ParamValue getMax () const override { return 0; }
	ParamValue getDefault () const override { return 0; }
	int getPrecision () const override { return 0; }
	void setMin (ParamValue min) override {}
	void setMax (ParamValue max) override {}
	void setDefault (ParamValue value) override {}
	ParamValue getNormalized () const override { return 0; }
	void setNormalized (ParamValue v, bool edit = false) override {}
	void increment (int steps = 1) override {}
	void decrement (int steps = 1) override {}
	void toString (char* string, int size) const override {}
	void fromString (CStringPtr string, bool edit = false) override {}
};

//************************************************************************************************
// AliasParam
/** Alias to other parameter.
	\ingroup core_portable */
//************************************************************************************************

class AliasParam: public Parameter,
				  public IParamObserver
{
public:
	BEGIN_CORE_CLASS ('APar', AliasParam)
		ADD_CORE_CLASS_ (IParamObserver)
	END_CORE_CLASS (Parameter)

	AliasParam (const ParamInfo& info, bool ownsInfo = false);
	~AliasParam ();

	static AliasParam* cast (Parameter* p) { return core_cast<AliasParam> (p); }

	void setOriginal (Parameter* p);

	// Parameter
	Parameter* getOriginal () override;
	void beginEdit () override;
	void endEdit () override;
	bool isEditing () const override;
	void performEdit () override;
	bool isEnabled () const override;
	void enable (bool state) override;
	int getVisualState () const override;
	void setVisualState (int state) override;
	bool isBipolar () const override;
	bool isNumeric () const override;
	ParamValue getMin () const override;
	ParamValue getMax () const override;
	ParamValue getDefault () const override;
	ParamValue getValue () const override;
	int getPrecision () const override;
	void setMin (ParamValue min) override;
	void setMax (ParamValue max) override;
	void setDefault (ParamValue value) override;
	void setValue (ParamValue value, bool edit = false) override;
	void resetValue (bool edit = false) override;
	ParamValue getNormalized () const override;
	void setNormalized (ParamValue v, bool edit = false) override;
	void increment (int steps = 1) override;
	void decrement (int steps = 1) override;
	void toString (char* string, int size) const override;
	void fromString (CStringPtr string, bool edit = false) override;

protected:
	Parameter* original;

	// IParamObserver
	void paramChanged (Parameter* p, int msg) override;
};

//************************************************************************************************
// ParamList
/** List of parameters identified by tag or name.
	\ingroup core_portable */
//************************************************************************************************

class ParamList
{
public:
	ParamList ();
	~ParamList ();

	void setController (IParamObserver* controller);
	PROPERTY_BOOL (feedbackNeeded, FeedbackNeeded)

	void add (Parameter* p);
	Parameter* add (const ParamInfo& info, bool ownsInfo = false);
	void add (const ParamInfo infos[], int count, bool ownsInfo = false);
	void remove (Parameter* p);
	void sortAll ();

	// optimized access
	INLINE int count () const { return params.count (); }
	INLINE Parameter* at (int index) const { return params.at (index); }
	INLINE operator Parameter** () const { return params; }
	INLINE bool hasStorableParams () const { return storableParamCount > 0; }
	INLINE bool hasPublicParams () const { return publicParamCount > 0; }

	Parameter* byTag (int tag) const;
	int getIndexByTag (int tag) const;
	Parameter* find (CStringPtr name) const;

	Parameter* addAlias (int tag, CStringPtr name);
	AliasParam* getAlias (int tag) const;

	void storeValues (OutputStorage& s) const;
	void restoreValues (const InputStorage& s, bool edit = false);

protected:
	struct TagFinder;
	IParamObserver* controller;
	Vector<Parameter*> params;
	int storableParamCount;
	int publicParamCount;
	bool sortedByTag;
};

//************************************************************************************************
// ParamArrayAccessor
/** Helper for faster access to indexed parameters.
	\ingroup core_portable */
//************************************************************************************************

struct ParamArrayAccessor
{
	ParamArrayAccessor (int baseTag)
	: baseTag (baseTag),
	  offsetToFirst (-1),
	  initDone (false)
	{}

	Parameter* getAt (const ParamList& paramList, int offset) const
	{
		if(initDone == false)
		{
			offsetToFirst = paramList.getIndexByTag (baseTag);
			initDone = true;
		}
		return offsetToFirst != -1 ? paramList.at (offsetToFirst + offset) : nullptr;
	}

private:
	int baseTag;
	mutable int offsetToFirst;
	mutable bool initDone;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void Parameter::setFormatter (const Formatter* formatter) { this->formatter = formatter; }
inline const Formatter* Parameter::getFormatter () const { return formatter; }
inline ParamType Parameter::getType () const { return info->type; }
inline int Parameter::getTag () const { return info->tag; }
inline CStringPtr Parameter::getName () const { return info->name; }
inline uint32 Parameter::getHashCode () const { return hashCode; }
inline bool Parameter::isOwnInfo () const { return (flags & kOwnsInfo) != 0; }
inline bool Parameter::isPublic () const { return (info->flags & ParamInfo::kPrivate) == 0; }
inline bool Parameter::isStorable () const { return (info->flags & ParamInfo::kStorable) != 0; }
inline bool Parameter::isLinkable () const { return (info->flags & ParamInfo::kLinkable) != 0; }
inline bool Parameter::isMutable () const { return (info->flags & ParamInfo::kMutable) != 0; }
inline bool Parameter::isReadOnly () const { return (info->flags & ParamInfo::kReadOnly) != 0; }
inline bool Parameter::isUserFlag1 () const { return (info->flags & ParamInfo::kUserFlag1) != 0; }
inline bool Parameter::isUserFlag2 () const { return (info->flags & ParamInfo::kUserFlag2) != 0; }

inline CStringPtr* ListParam::getSharedStrings () const { return mutableList == nullptr ? sharedList.strings : nullptr; }
inline int ListParam::getStringCount () const { return getStringList ().getCount (); }
inline CStringPtr ListParam::getStringAtUnsafe (int index) const { return getStringList ().getStringAt (index); }

inline const StringParam::TextValue& StringParam::getText () const { return text; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Portable
} // namespace Core

#endif // _coreparams_h
