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
// Filename    : ccl/app/params.h
// Description : Parameter classes
//
//************************************************************************************************

#ifndef _ccl_params_h
#define _ccl_params_h

#include "ccl/base/message.h"
#include "ccl/base/collections/objectlist.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/iparametermenu.h"
#include "ccl/public/gui/framework/ipalette.h"
#include "ccl/public/gui/framework/itextmodel.h"

namespace Core {
class Interpolator; }

namespace CCL {

//************************************************************************************************
// Parameter
/** Basic parameter class. */
//************************************************************************************************

class Parameter: public Object,
				 public IParameter
{
public:
	DECLARE_CLASS (Parameter, Object)
	DECLARE_METHOD_NAMES (Parameter)
	DECLARE_PROPERTY_NAMES (Parameter)

	Parameter (StringID name = nullptr);
	Parameter (const Parameter& p);
	~Parameter ();

	static Parameter* createInstance (StringID className);
	static IUnknown* createIdentity (IParameter* This);
	static void restoreValue (IParameter* This, VariantRef value, bool update = false);

	// IParameter
	// Attributes
	int CCL_API getType () const override;
	StringID CCL_API getName () const override;
	void CCL_API setName (StringID name) override;
	tbool CCL_API isEnabled () const override;
	void CCL_API enable (tbool state) override;
	tbool CCL_API getState (int mask) const override;
	void CCL_API setState (int mask, tbool state) override;
	int CCL_API getVisualState () const override;
	void CCL_API setVisualState (int state) override;

	// Controller
	void CCL_API connect (IParamObserver* controller, int tag) override;
	int CCL_API getTag () const override;
	IUnknown* CCL_API getController () const override;
	void CCL_API performUpdate () override;
	void CCL_API beginEdit () override;
	void CCL_API endEdit () override;
	IParameter* CCL_API getOriginal () override;
	IUnknown* CCL_API createIdentity () override;

	// Plain Value
	Variant CCL_API getValue () const override;
	void CCL_API setValue (VariantRef value, tbool update = false) override;
	Variant	CCL_API getMin () const override;
	Variant	CCL_API getMax () const override;
	void CCL_API setMin (VariantRef min) override;
	void CCL_API setMax (VariantRef max) override;
	Variant CCL_API getDefaultValue () const override;
	void CCL_API setDefaultValue (VariantRef value) override;
	Variant CCL_API boundValue (VariantRef value) const override;
	tbool CCL_API canIncrement () const override;
	int CCL_API getPrecision () const override;
	tbool CCL_API setPrecision (int precision) override;
	void CCL_API increment () override;
	void CCL_API decrement () override;
	void CCL_API takeValue (const IParameter& param, tbool update = false) override;

	// Normalized Value
	float CCL_API getNormalized () const override;
	void CCL_API setNormalized (float value, tbool update = false) override;
	float CCL_API getValueNormalized (VariantRef value) const override;
	Variant CCL_API getValuePlain (float valueNormalized) const override;
	IParamCurve* CCL_API getCurve () const override;
	void CCL_API setCurve (IParamCurve* curve) override;

	// String Conversion
	IFormatter* CCL_API getFormatter () const override;
	void CCL_API setFormatter (IFormatter* formatter) override;
	void CCL_API getString (String& string, VariantRef value) const override;
	void CCL_API toString (String& string) const override;
	void CCL_API fromString (StringRef string, tbool update = false) override;

	void rangeChanged (); ///< explicit signal if min/max/list changed (not in interface)

	// Object
	void CCL_API signal (MessageRef msg = Message (kChanged)) override;
	void deferChanged () override;

	CLASS_INTERFACE (IParameter, Object)

protected:
	enum PrivateFlags
	{
		kToggleOn  = 1<<24,
		kDefaultOn = 1<<25,
		kDisabled  = 1<<26,
		kEditing   = 1<<27
	};

	int tag;
	int flags;
	int visualState;
	MutableCString name;
	IParamObserver* controller;
	IParamCurve* curve;
	IFormatter* formatter;

	bool getFlag (int mask) const;
	void setFlag (int mask, bool state);
	void resetPriority ();
	void checkSignalFirst ();

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, VariantRef var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// FloatParam
/** A float parameter. */
//************************************************************************************************

class FloatParam: public Parameter
{
public:
	DECLARE_CLASS (FloatParam, Parameter)
	DECLARE_METHOD_NAMES (FloatParam)

	FloatParam (double min = 0, double max = 100, StringID name = nullptr);
	FloatParam (const FloatParam& p);

	// IParameter
	int CCL_API getType () const override;
	Variant CCL_API getValue () const override;
	void CCL_API setValue (VariantRef value, tbool update = false) override;
	Variant	CCL_API getMin () const override;
	Variant	CCL_API getMax () const override;
	void CCL_API setMin (VariantRef min) override;
	void CCL_API setMax (VariantRef max) override;
	Variant CCL_API getDefaultValue () const override;
	void CCL_API setDefaultValue (VariantRef value) override;
	int CCL_API getPrecision () const override;
	tbool CCL_API setPrecision (int precision) override;
	void CCL_API increment () override;
	void CCL_API decrement () override;
	void CCL_API getString (String& string, VariantRef value) const override;
	void CCL_API fromString (StringRef string, tbool update = false) override;
	Variant CCL_API boundValue (VariantRef value) const override;
	float CCL_API getValueNormalized (VariantRef value) const override;
	Variant CCL_API getValuePlain (float valueNormalized) const override;

protected:
	double min;
	double max;
	double value;
	double defaultValue;
	int precision;
};

//************************************************************************************************
// IntParam
/** An integer parameter. */
//************************************************************************************************

class IntParam: public Parameter
{
public:
	DECLARE_CLASS (IntParam, Parameter)
	DECLARE_METHOD_NAMES (IntParam)

	IntParam (int min = 0, int max = 100, StringID name = nullptr);
	IntParam (const IntParam& p);

	// IParameter
	int CCL_API getType () const override;
	Variant CCL_API getValue () const override;
	void CCL_API setValue (VariantRef value, tbool update = false) override;
	Variant	CCL_API getMin () const override;
	Variant	CCL_API getMax () const override;
	void CCL_API setMin (VariantRef min) override;
	void CCL_API setMax (VariantRef max) override;
	Variant CCL_API getDefaultValue () const override;
	void CCL_API setDefaultValue (VariantRef value) override;
	int CCL_API getPrecision () const override;
	void CCL_API getString (String& string, VariantRef value) const override;
	void CCL_API fromString (StringRef string, tbool update = false) override;
	Variant CCL_API boundValue (VariantRef value) const override;
	float CCL_API getValueNormalized (VariantRef value) const override;
	Variant CCL_API getValuePlain (float valueNormalized) const override;

protected:
	int min;
	int max;
	int value;
	int defaultValue;
};

//************************************************************************************************
// StringParam 
/** A string parameter. */
//************************************************************************************************

class StringParam: public Parameter
{
public:
	DECLARE_CLASS (StringParam, Parameter)

	StringParam (StringID name = nullptr);
	StringParam (const StringParam& p);

	StringRef getString () const;
	StringRef getDefaultString () const;

	// IParameter
	int CCL_API getType () const override;
	Variant CCL_API getValue () const override;
	void CCL_API setValue (VariantRef value, tbool update = false) override;
	Variant CCL_API getDefaultValue () const override;
	void CCL_API setDefaultValue (VariantRef value) override;
	Variant	CCL_API getMax () const override;
	void CCL_API getString (String& string, VariantRef value) const override;
	void CCL_API fromString (StringRef string, tbool update = false) override;

protected:
	String string;
	String defaultString;
};

//************************************************************************************************
// ListParam
/** A list parameter. */
//************************************************************************************************

class ListParam: public IntParam,
				 public IListParameter
{
public:
	DECLARE_CLASS (ListParam, IntParam)
	DECLARE_METHOD_NAMES (ListParam)

	ListParam (StringID name = nullptr);
	ListParam (const ListParam& p);

	void appendObject (Object* object, int index = -1);	///< parameter takes ownership!
	int getObjectIndex (const Object* object) const;
	int getObjectIndex (const Object& object) const;
	int getObjectCount  () const;
	bool contains (const Object& object) const; ///< looks for equal object
	template <class T> T* getObject (int index) const;
	template<class Lambda> Object* findObject (const Lambda& recognize) const; ///< bool recognize (const Object*) 
	Object* getSelectedObject () const;
	bool selectObject (Object* object, tbool update = false);

	// IListParameter
	void CCL_API appendString (StringRef string, int index = -1) override;
	void CCL_API appendValue (VariantRef value, int index = -1) override;
	void CCL_API appendValue (VariantRef value, StringRef string, int index = -1) override;
	int CCL_API getValueIndex (VariantRef value) const override;
	Variant CCL_API getValueAt (int index) const override;
	tbool CCL_API setValueAt (int index, VariantRef value) override;
	Variant CCL_API getSelectedValue () const override;
	tbool CCL_API selectValue (VariantRef value, tbool update = false) override;
	void CCL_API removeAt (int index) override;
	void CCL_API removeAll () override;
	tbool CCL_API isEmpty () const override;
	int CCL_API getNearestValueIndex (VariantRef value) const override;
	tbool CCL_API selectNearestValue (VariantRef value, tbool update = false) override;

	// IParameter
	int CCL_API getType () const override;
	void CCL_API setMax (VariantRef max) override;
	void CCL_API increment () override;
	void CCL_API decrement () override;
	void CCL_API getString (String& string, VariantRef value) const override;
	void CCL_API fromString (StringRef string, tbool update = false) override;

	CLASS_INTERFACE (IListParameter, IntParam)

protected:
	ObjectArray list;

	bool isSeparatorAt (int index) const;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// MenuParam
/** A list parameter that can have additional menu items when displayed in a menu. */
//************************************************************************************************

class MenuParam: public ListParam,
				 public IMenuExtension
{
public:
	DECLARE_CLASS (MenuParam, ListParam)

	MenuParam (StringID name = nullptr);

	// IMenuExtension
	void CCL_API extendMenu (IMenu& menu, StringID name) override;

	CLASS_INTERFACE (IMenuExtension, ListParam)
};

//************************************************************************************************
// CustomizedMenuParam
/** A menu parameter that allows to customize the menu type. */
//************************************************************************************************

class CustomizedMenuParam: public MenuParam,
						   public IParameterMenuCustomize
{
public:
	CustomizedMenuParam (StringID name = nullptr, StringID menuType = MenuPresentation::kTree);

	void setMenuType (StringID type);

	// IParameterMenuCustomize
	StringID CCL_API getMenuType () const override;
	tbool CCL_API buildMenu (IMenu& menu, IParameterMenuBuilder& builder) override;
	tbool CCL_API onMenuKeyDown (const KeyEvent& event) override;

	CLASS_INTERFACE (IParameterMenuCustomize, MenuParam)

private:
	MutableCString menuType;
};

//************************************************************************************************
// PaletteProvider
/** Helper class for parameters with associated palette. */
//************************************************************************************************

class PaletteProvider: public IPaletteProvider
{
public:
	PaletteProvider (IPalette* palette = nullptr);

	// IPaletteProvider
	IPalette* CCL_API getPalette () const override;
	void CCL_API setPalette (IPalette* palette) override;

	// IUnknown - will be overwritten by subclass!
	IMPLEMENT_DUMMY_UNKNOWN (IPaletteProvider)

protected:
	SharedPtr<IPalette> palette;

	bool getProperty (Variant& var, StringID propertyId) const;
	bool setProperty (StringID propertyId, VariantRef var);
};

//************************************************************************************************
// PaletteParam
/** A list parameter that can have an associated palette. */
//************************************************************************************************

class PaletteParam: public ListParam,
					public PaletteProvider
{
public:
	DECLARE_CLASS (PaletteParam, ListParam)

	PaletteParam (StringID name = nullptr, IPalette* palette = nullptr);

	// PaletteProvider
	void CCL_API setPalette (IPalette* palette) override;

	CLASS_INTERFACE (IPaletteProvider, ListParam)
};

//************************************************************************************************
// StructuredParameter
/** Helper class for structured parameters. */
//************************************************************************************************

class StructuredParameter: public IStructuredParameter
{
public:
	virtual ~StructuredParameter ();

	void addSubParameter (IParameter* p);
	void removeSubParameters ();

	// IStructuredParameter
	void CCL_API prepareStructure () override;
	void CCL_API cleanupStructure () override;
	int CCL_API countSubParameters () const override;
	IParameter* CCL_API getSubParameter (int index) const override;

	// IUnknown - will be overwritten by subclass
	IMPLEMENT_DUMMY_UNKNOWN (IStructuredParameter)

protected:
	LinkedList<IParameter*> parameters;
};

//************************************************************************************************
// CommandParam
/** A command parameter. */
//************************************************************************************************

class CommandParam: public Parameter,
					public ICommandParameter
{
public:
	DECLARE_CLASS (CommandParam, Parameter)

	CommandParam (StringID name = nullptr,
				  StringID commandCategory = nullptr, 
				  StringID commandName = nullptr);

	// ICommandParameter
	StringID CCL_API getCommandCategory () const override;
	StringID CCL_API getCommandName () const override;
	void CCL_API setCommand (StringID category, StringID name) override;
	tbool CCL_API checkEnabled () override;

	// Parameter
	int CCL_API getType () const override;
	void CCL_API performUpdate () override;

	CLASS_INTERFACE (ICommandParameter, Parameter)

protected:
	MutableCString commandCategory;
	MutableCString commandName;

	bool interpretCommand (int flags = 0);
};

//************************************************************************************************
// ScrollParam
/** A scroll parameter. */
//************************************************************************************************

class ScrollParam: public IntParam,
				   public IScrollParameter
{
public:
	DECLARE_CLASS (ScrollParam, IntParam)

	ScrollParam (int max = 100, StringID name = nullptr);

	int getStepSize () const;

	// IScrollParameter
	float CCL_API getPageSize () const override;
	void CCL_API setPageSize (float pageSize) override;
	void CCL_API setRange (int range, float pageSize = 0.f) override;
	tbool CCL_API canScroll () const override;

	// IntParam
	int CCL_API getType () const override;
	void CCL_API increment () override;
	void CCL_API decrement () override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

	CLASS_INTERFACE (IScrollParameter, IntParam)

protected:
	float pageSize;
};

//************************************************************************************************
// ColorParam
/** A color parameter. */
//************************************************************************************************

class ColorParam: public Parameter,
				  public IColorParam,
				  public PaletteProvider
{
public:
	DECLARE_CLASS (ColorParam, Parameter)

	ColorParam (StringID name = nullptr);
	~ColorParam ();
	
	bool isBitSet (int index) const;
	void setBit (int index, bool state);

	// IColorParam
	Color& CCL_API getColor (Color& color) const override;
	void CCL_API setColor (const Color& color, tbool update = false) override;

	// PaletteProvider
	void CCL_API setPalette (IPalette* palette) override;

	// Parameter
	int CCL_API getType () const override;
	Variant CCL_API getValue () const override;
	void CCL_API setValue (VariantRef value, tbool update = false) override;
	tbool CCL_API canIncrement () const override;
	void CCL_API increment () override;
	void CCL_API decrement () override;
	void CCL_API getString (String& string, const Variant& value) const override;
	void CCL_API toString (String& string) const override;
	void CCL_API fromString (StringRef string, tbool update) override;
	float CCL_API getValueNormalized (VariantRef value) const override;
	Variant CCL_API getValuePlain (float valueNormalized) const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE2 (IColorParam, IPaletteProvider, Parameter)

protected:
	Color* colorValue;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, VariantRef var) override;
};

//************************************************************************************************
// ImageProvider
/** An image provider. */
//************************************************************************************************

class ImageProvider: public Parameter,
					 public IImageProvider,
					 public PaletteProvider
{
public:
	DECLARE_CLASS (ImageProvider, Parameter)
	DECLARE_METHOD_NAMES (ImageProvider)

	ImageProvider (StringID name = nullptr);
	~ImageProvider ();

	// IImageProvider
	IImage* CCL_API getImage () const override;
	void CCL_API setImage (IImage* image, tbool update = false) override;

	// Parameter
	int CCL_API getType () const override;
	Variant CCL_API getValue () const override;
	void CCL_API setValue (VariantRef value, tbool update = false) override;
	void CCL_API getString (String& string, const Variant& value) const override;

	CLASS_INTERFACE2 (IImageProvider, IPaletteProvider, Parameter)

protected:
	IImage* imageValue;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// TextModelProvider
/** A text model provider. */
//************************************************************************************************

class TextModelProvider: public Parameter,
						 public ITextModelProvider
{
public:
	DECLARE_CLASS (TextModelProvider, Parameter)

	TextModelProvider (StringID name = nullptr);
	TextModelProvider (const TextModelProvider& p);
	~TextModelProvider ();

	// Parameter
	int CCL_API getType () const override;
	Variant CCL_API getValue () const override;
	void CCL_API setValue (VariantRef value, tbool update = false) override;
	void CCL_API toString (String& string) const override;
	void CCL_API fromString (StringRef string, tbool update) override;
	tbool CCL_API canIncrement () const override;

	// ITextModelProvider
	ITextModel* CCL_API getTextModel () override;
	void CCL_API setTextModel (ITextModel* model, tbool update = false) override;

	CLASS_INTERFACE (ITextModelProvider, Parameter)

private:
	ITextModel* textModel;
};

//************************************************************************************************
// ParamCurve
/** Base class for parameter curves. */
//************************************************************************************************

class ParamCurve: public Object,
				  public IParamCurve
{
public:
	DECLARE_CLASS_ABSTRACT (ParamCurve, Object)

	// IParamCurve
	double CCL_API getRelativeValue (double startValue, double endValue, double linearValue) const override;
	CStringPtr CCL_API getFactoryName () const override;

	CLASS_INTERFACE (IParamCurve, Object)
};

//************************************************************************************************
// ConcaveCurve
//************************************************************************************************

class ConcaveCurve: public ParamCurve
{
public:
	DECLARE_CLASS (ConcaveCurve, ParamCurve)

	// IParamCurve
	double CCL_API displayToNormalized (double linearValue) const override;
	double CCL_API normalizedToDisplay (double curveValue) const override;
};

//************************************************************************************************
// ConvexCurve
//************************************************************************************************

class ConvexCurve: public ParamCurve
{
public:
	DECLARE_CLASS (ConvexCurve, ParamCurve)

	// IParamCurve
	double CCL_API displayToNormalized (double linearValue) const override;
	double CCL_API normalizedToDisplay (double curveValue) const override;
};

//************************************************************************************************
// InterpolatorCurve
/** Wrapper for Core::Interpolator. */
//************************************************************************************************

class InterpolatorCurve: public ParamCurve
{
public:
	DECLARE_CLASS (InterpolatorCurve, ParamCurve)

	InterpolatorCurve (Core::Interpolator* interpolator = nullptr); ///< takes ownership!
	~InterpolatorCurve ();

	float getMinRange () const;
	float getMaxRange () const;
	float getMidRange () const;
	
	void setRange (float minRange, float maxRange, float midRange = 1.f);

	// ParamCurve
	double CCL_API displayToNormalized (double linearValue) const override;
	double CCL_API normalizedToDisplay (double curveValue) const override;
	double CCL_API getRelativeValue (double startValue, double endValue, double linearValue) const override;
	CStringPtr CCL_API getFactoryName () const override;

protected:
	Core::Interpolator* interpolator;
	Core::Interpolator* normalizer;

	void setInterpolator (Core::Interpolator* interpolator = nullptr);
};

//************************************************************************************************
// ParamCurveFactory
//************************************************************************************************

class ParamCurveFactory
{
public:
	static ParamCurveFactory& instance ();
	
	IParamCurve* create (StringID name);

	typedef IParamCurve* (*CreateFunc) ();
	void add (StringID name, CreateFunc createFunc);

	template <class Type> 
	void addCurve (StringID name) { add (name, createCurve<Type>); }

protected:
	struct CurveClass
	{
		MutableCString name;
		CreateFunc createFunc;

		CurveClass (StringID name = nullptr, CreateFunc createFunc = nullptr)
		: name (name),
		  createFunc (createFunc)
		{}
	};

	Vector<CurveClass> classes;

	template <class Type> 
	static IParamCurve* createCurve () { return NEW Type; }
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Parameter inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Parameter::getFlag (int mask) const
{ return (flags & mask) != 0; }

inline void Parameter::setFlag (int mask, bool state)
{ if(state) flags |= mask; else flags &= ~mask; }

inline void Parameter::resetPriority ()
{ setFlag (kPriorityChange, false); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// ListParam inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T> T* ListParam::getObject (int index) const
{ return ccl_cast<T> (list.at (index)); }

inline bool ListParam::contains (const Object& object) const
{ return list.contains (object); }

inline int ListParam::getObjectCount  () const
{ return list.count (); }

template<class Lambda>
inline Object* ListParam::findObject (const Lambda& recognize) const
{ return list.findIf (recognize); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_params_h
