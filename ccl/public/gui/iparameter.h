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
// Filename    : ccl/public/gui/iparameter.h
// Description : Parameter Interfaces
//
//************************************************************************************************

#ifndef _ccl_iparameter_h
#define _ccl_iparameter_h

#include "ccl/public/base/variant.h"
#include "ccl/public/base/primitives.h"
#include "ccl/public/gui/graphics/color.h"

namespace CCL {

interface IFormatter;
interface IParameter;
interface IParamObserver;
interface IParamCurve;
interface IImage;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in parameter classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (Parameter, 0xb7856683, 0x5f77, 0x4c4c, 0xa1, 0xd, 0x43, 0x50, 0xe2, 0x51, 0xbe, 0x66)
	DEFINE_CID (AliasParam, 0x249d8e02, 0xe1d6, 0x43e0, 0x8b, 0xe5, 0xa6, 0xc7, 0xdb, 0x57, 0xe0, 0xce)
	DEFINE_CID (StringParam, 0xaf7656dc, 0xdd3e, 0x47b7, 0xa7, 0x63, 0x74, 0x6e, 0x1b, 0xc2, 0xc7, 0xb3)
	DEFINE_CID (ListParam, 0x6e4557d2, 0x8482, 0x469e, 0xb5, 0xb0, 0xe6, 0xc9, 0x2, 0xb1, 0xd3, 0x53)
	DEFINE_CID (MenuParam, 0x5b640b62, 0x3bd9, 0x48f3, 0x8d, 0xca, 0xaf, 0xc3, 0xce, 0x91, 0xea, 0x04)
	DEFINE_CID (PaletteParam, 0x77b397ac, 0xfcf6, 0x441f, 0x8d, 0x53, 0xfa, 0x4a, 0x36, 0x4e, 0x31, 0x22)
	DEFINE_CID (FloatParam, 0xf548b970, 0xe58b, 0x43de, 0xa9, 0xfe, 0x12, 0x1d, 0x43, 0x78, 0xbc, 0xc5)
	DEFINE_CID (IntParam, 0x3ee3eb3d, 0x4a73, 0x4d7d, 0x90, 0x4, 0xfc, 0xfb, 0xe8, 0x19, 0x6, 0x9f)
	DEFINE_CID (CommandParam, 0xe046bde8, 0xd9cd, 0x4a16, 0x94, 0x6a, 0xf0, 0xe5, 0x2e, 0xcd, 0xc6, 0xb3)
	DEFINE_CID (ScrollParam, 0x9ba1808b, 0xf2b8, 0x4cb4, 0x85, 0x2, 0x2f, 0xd3, 0xda, 0xb4, 0x1b, 0x14)
	DEFINE_CID (ColorParam, 0x8167ae15, 0x651, 0x489a, 0x89, 0x84, 0xcb, 0x24, 0x2a, 0x1e, 0xa9, 0x8d)
	DEFINE_CID (ImageProvider, 0xa0b92148, 0xa412, 0x4449, 0x9c, 0x80, 0x3e, 0x6e, 0x63, 0xa3, 0x46, 0x94)
	DEFINE_CID (TextModelProvider, 0x19e52c6d, 0xf51f, 0x46d3, 0xa5, 0x58, 0xd7, 0xa5, 0xfe, 0x61, 0xc9, 0x60)
}

//************************************************************************************************
// IParameter
/**	A parameter object is the link between application logic (controller) and GUI widgets.
	If a GUI widget causes the parameter to change its value by user input, the controller
	is notified to update its data model. If the underlying data model changes, 
	changing the parameter value forces the GUI widget to be updated on screen. 
	A single parameter can be associated to any number of widgets simultaneously. 
	No additional coding is required to keep multiple graphical representations in sync. 
	\ingroup gui_param */
//************************************************************************************************

interface IParameter: IUnknown
{
	/** Parameter types. */
	enum Type
	{
		kToggle,		///< toggle between on/off
		kInteger,		///< numerical value integer
		kFloat,			///< numerical value floating point
		kString,		///< string value
		kList,			///< list of values
		kCommand,		///< command
		kColor,			///< color value
		kImage,			///< image provider
		kSegments,		///< segments using IParamSplitter
		kScroll,		///< scroll parameter
		kTextModel,		///< text model provider
		kNumTypes
	};

	/** Parameter flags. */
	enum Flags
	{
		// behavioral flags
		kSignalFirst	= 1<<1,		///< signal with old and with new value
		kSignalAlways	= 1<<2,		///< signal when setValue is called with current value
		kBipolar		= 1<<3,		///< parameter is bipolar, the center is defined by (max-min)/2
		kReverse		= 1<<4,		///< parameter is reverse, display should be upside down
		kOutOfRange		= 1<<5,		///< parameter value is out of range
		kWrapAround		= 1<<6,		///< parameter increment/decrement does not stop at min/max
		
		// semantic flags
		kStorable		= 1<<7,		///< parameter is storable
		kStoreListValue	= 1<<8,		///< store selected list value instead of list index
		kGroupable		= 1<<9,		///< parameter can be grouped
		kPublic			= 1<<10,	///< parameter is public
		kMutable		= 1<<11,	///< parameter is mutable (min/max, list entries can change)
		kReadOnly		= 1<<12,	///< parameter is used for display purposes only, no editing

		// controller flags
		kIsEditing		= 1<<13,	///< reserved for controller to store edit state
		kFeedback		= 1<<14,	///< controller wants notify() calls as well
		kPriorityChange	= 1<<15,	///< priority change pending
		kCanUndo        = 1<<16		///< parameter changes can be undone
	};

	//////////////////////////////////////////////////////////////////////////////////////////////
	///@name Signals
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	DECLARE_STRINGID_MEMBER (kBeginEdit)		///< signal name for beginEdit()
	DECLARE_STRINGID_MEMBER (kEndEdit)			///< signal name for endEdit()
	DECLARE_STRINGID_MEMBER (kExtendMenu)		///< signal for menu extension (args[0]: IMenu)
	DECLARE_STRINGID_MEMBER (kUpdateMenu)		///< signal for a connected control to update its menu (args[0]: IMenu)
	DECLARE_STRINGID_MEMBER (kRequestFocus)		///< signal for a connected control to take focus	
	DECLARE_STRINGID_MEMBER (kReleaseFocus)		///< signal for a connected control to release focus	
	DECLARE_STRINGID_MEMBER (kSetSelection)		///< signal for a connected control to set text selection range (args[0]: (int)start, args[1]: (int)length)
	DECLARE_STRINGID_MEMBER (kRangeChanged)		///< explicit signal if min/max or list entries changed	
	///@}

	//////////////////////////////////////////////////////////////////////////////////////////////
	///@name Attributes
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get parameter type. */
	virtual int CCL_API getType () const = 0;
	
	/** Get parameter name. */
	virtual StringID CCL_API getName () const = 0;

	/** Set parameter name. */
	virtual void CCL_API setName (StringID name) = 0;

	/** Check if parameter is enabled. */
	virtual tbool CCL_API isEnabled () const = 0;

	/** Enable or disable parameter. */
	virtual void CCL_API enable (tbool state) = 0;

	/** Get parameter state. */
	virtual tbool CCL_API getState (int mask) const = 0;

	/** Set parameter state. */
	virtual void CCL_API setState (int mask, tbool state) = 0;

	/** Get visual state. */
	virtual int CCL_API getVisualState () const = 0;

	/** Set visual state. */
	virtual void CCL_API setVisualState (int state) = 0;
	///@}

	//////////////////////////////////////////////////////////////////////////////////////////////
	///@name Controller
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Connect parameter to controller with specified identification tag. */
	virtual void CCL_API connect (IParamObserver* controller, int tag) = 0;
	
	/** Get identification tag used by controller. */
	virtual int CCL_API getTag () const = 0;

	/** Get controller this parameter is associated with. */
	virtual IUnknown* CCL_API getController () const = 0;

	/** Force controller update, usually IParamObserver::paramChanged. */
	virtual void CCL_API performUpdate () = 0;

	/** Notify controller about start of user interaction. */
	virtual void CCL_API beginEdit () = 0;

	/** Notify controller about end of user interaction. */
	virtual void CCL_API endEdit () = 0;
	
	/** Get original parameter (usually this, delegated by alias otherwise). */
	virtual IParameter* CCL_API getOriginal () = 0;

	/** Create object identifying this parameter (controller must implement IResolver). */
	virtual IUnknown* CCL_API createIdentity () = 0;
	///@}

	//////////////////////////////////////////////////////////////////////////////////////////////
	///@name Plain Value
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get current value. */
	virtual Variant	CCL_API getValue () const = 0;

	/** Set value and (optionally) force controller update. */
	virtual void CCL_API setValue (VariantRef value, tbool update = false) = 0;
	
	/** Get minimum numerical value (does not apply to string type). */
	virtual Variant	CCL_API getMin () const = 0;

	/** Get maximum numerical value (does not apply to string type). */
	virtual Variant	CCL_API getMax () const = 0;

	/** Set minimum numerical value (does not apply to string type). */
	virtual void CCL_API setMin (VariantRef min) = 0;

	/** Set maximum numerical value (does not apply to string type).*/
	virtual void CCL_API setMax (VariantRef max) = 0;

	/** Get associated default value. */
	virtual Variant	CCL_API getDefaultValue () const = 0;

	/** Set default value. */
	virtual void CCL_API setDefaultValue (VariantRef value) = 0;
	
	/** Bound value between minimum and maximum numerical value (does not apply to string type). */
	virtual Variant	CCL_API boundValue (VariantRef value) const = 0;

	/** Check if value can be incremented/decremented (does not apply to string type). */
	virtual tbool CCL_API canIncrement () const = 0;

	/** Get precision used for increment/decrement. */
	virtual int CCL_API getPrecision () const = 0;

	/** Set precision used for increment/decrement (delta = range / precision). */
	virtual tbool CCL_API setPrecision (int precision) = 0;
	
	/** Increment numerical value, depending on precision. */
	virtual void CCL_API increment () = 0;

	/** Decrement numerical value, depending on precision. */
	virtual void CCL_API decrement () = 0;

	/** Take value from other parameter. */
	virtual void CCL_API takeValue (const IParameter& param, tbool update = false) = 0;
	///@}

	//////////////////////////////////////////////////////////////////////////////////////////////
	///@name Normalized Value
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	/** Get normalized value between 0.0 and 1.0. */
	virtual float CCL_API getNormalized () const = 0;

	/** Set value normalized and (optionally) force controller update. */
	virtual void CCL_API setNormalized (float value, tbool update = false) = 0;

	/** Get normalized float by plain value. */
	virtual float CCL_API getValueNormalized (VariantRef value) const = 0;

	/** Get plain value by normalized float. */
	virtual Variant CCL_API getValuePlain (float valueNormalized) const = 0;

	/** Get associated curve object. */
	virtual IParamCurve* CCL_API getCurve () const = 0;

	/** Set curve object (shared by parameter). */
	virtual void CCL_API setCurve (IParamCurve* curve) = 0;
	///@}

	//////////////////////////////////////////////////////////////////////////////////////////////
	///@name String conversion
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get associated formatter object. */
	virtual IFormatter* CCL_API getFormatter () const = 0;

	/** Set formatter object (shared by parameter). */
	virtual void CCL_API setFormatter (IFormatter* formatter) = 0;

	/** Shortcut to retrieve string representation of given value. */
	virtual void CCL_API getString (String& string, VariantRef value) const = 0;

	/** Convert current value to string. */
	virtual void CCL_API toString (String& string) const = 0;

	/** Set current value by string and (optionally) force controller update. */
	virtual void CCL_API fromString (StringRef string, tbool update = false) = 0;
	///@}

	DECLARE_IID (IParameter)

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Inline methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	inline bool isSignalFirst () const				{ return getState (kSignalFirst) != 0; }
	inline void setSignalFirst (bool state = true)	{ setState (kSignalFirst, state); }
	inline bool isSignalAlways () const				{ return getState (kSignalAlways) != 0; }
	inline void setSignalAlways (bool state = true)	{ setState (kSignalAlways, state); }
	inline bool isBipolar () const					{ return getState (kBipolar) != 0; }
	inline void setBipolar (bool state = true)		{ setState (kBipolar, state); }
	inline bool isReverse () const					{ return getState (kReverse) != 0; }
	inline void setReverse (bool state = true)		{ setState (kReverse, state); }
	inline bool isOutOfRange () const				{ return getState (kOutOfRange) != 0; }
	inline void setOutOfRange (bool state = true)	{ setState (kOutOfRange, state); }
	inline bool isWrapAround () const				{ return getState (kWrapAround) != 0; }
	inline void setWrapAround (bool state = true)	{ setState (kWrapAround, state); }

	inline bool isStorable () const					{ return getState (kStorable) != 0; }
	inline void setStorable (bool state = true)		{ setState (kStorable, state); }
	inline bool isStoreListValue () const			{ return getState (kStoreListValue) != 0; }
	inline void setStoreListValue (bool state = true) { setState (kStoreListValue, state); }

	inline bool isGroupable () const				{ return getState (kGroupable) != 0; }
	inline void setGroupable (bool state = true)	{ setState (kGroupable, state); }
	inline bool isPublic () const					{ return getState (kPublic) != 0; }
	inline void setPublic (bool state = true)		{ setState (kPublic, state); }
	inline bool isMutable () const					{ return getState (kMutable) != 0; }
	inline void setMutable (bool state = true)		{ setState (kMutable, state); }
	inline bool isReadOnly () const					{ return getState (kReadOnly) != 0; }
	inline void setReadOnly (bool state = true)		{ setState (kReadOnly, state); }

	inline void setFeedbackNeeded (bool state)		{ setState (kFeedback, state); }
	inline void setPriorityChange ()				{ setState (kPriorityChange, true); }
	inline bool isPriorityChange () const			{ return getState (kPriorityChange) != 0; }

	inline void setCanUndo (bool state = true)		{ setState (kCanUndo, state); }
	inline bool canUndo () const			        { return getState (kCanUndo) != 0; }

	inline double getIncrement () const             { return (getMax ().asDouble () - getMin ().asDouble ()) / getPrecision (); }
	inline void setIncrement (double increment)     { setPrecision (ccl_to_int ((getMax ().asDouble () - getMin ().asDouble ()) / increment)); }
};

DEFINE_IID (IParameter, 0xa9a3dd8, 0x1262, 0x4152, 0xbe, 0xfa, 0xcd, 0xcb, 0x8d, 0x1a, 0x1e, 0xf6)
DEFINE_STRINGID_MEMBER (IParameter, kBeginEdit, "beginEdit")
DEFINE_STRINGID_MEMBER (IParameter, kEndEdit, "endEdit")
DEFINE_STRINGID_MEMBER (IParameter, kExtendMenu, "extendMenu")
DEFINE_STRINGID_MEMBER (IParameter, kUpdateMenu, "updateMenu")
DEFINE_STRINGID_MEMBER (IParameter, kRequestFocus, "requestFocus")
DEFINE_STRINGID_MEMBER (IParameter, kReleaseFocus, "releaseFocus")
DEFINE_STRINGID_MEMBER (IParameter, kSetSelection, "setSelection")
DEFINE_STRINGID_MEMBER (IParameter, kRangeChanged, "rangeChanged")

//************************************************************************************************
// IParamCurve
/** Non-linear parameter curve. 
	\ingroup gui_param */
//************************************************************************************************

interface IParamCurve: IUnknown
{
	/** Get value modified by parameter curve (non-linear), i.e. from display domain to internal. */
	virtual double CCL_API displayToNormalized (double displayValue) const = 0;
	
	/** Get inverse value from parameter curve (linear), i.e. to display domain. */
	virtual double CCL_API normalizedToDisplay (double normalized) const = 0;

	/** Calculate value for relative parameter editing (in/out values are linear). */
	virtual double CCL_API getRelativeValue (double startValue, double endValue, double linearValue) const = 0;

	/** Get name of factory which created this instance. */
	virtual CStringPtr CCL_API getFactoryName () const = 0;

	DECLARE_IID (IParamCurve)
};

DEFINE_IID (IParamCurve, 0xfcd72f78, 0x6aa7, 0x402f, 0x95, 0x45, 0xf, 0xad, 0x30, 0x4, 0xd9, 0x2c)

//************************************************************************************************
// ITickScale 
/** Interface used to paint a grid. \see ControlGridPainter
	\ingroup gui_param */
//************************************************************************************************

interface ITickScale: IUnknown
{
	/** Get number of ticks for drawing a scale. */
	virtual int CCL_API getNumTicks (int weight) const = 0;

	/** Get value of tick.*/
	virtual tbool CCL_API getTick (double& pos, String* label, int weight, int index) const = 0;

	/** Certain ticks can be hilited.*/
	virtual tbool CCL_API isHiliteTick (int weight, int index) const = 0;

	DECLARE_IID (ITickScale)
};

DEFINE_IID (ITickScale, 0x32762611, 0x58f4, 0x4168, 0x80, 0xd7, 0xf2, 0x8c, 0x55, 0x3a, 0xd8, 0xab)

//************************************************************************************************
// NormalizedValue
/** Helper to set/get normalized value respecting flags and parameter curve. 
	\ingroup gui_param */
//************************************************************************************************

struct NormalizedValue
{
	IParameter* p;
	
	NormalizedValue (IParameter* p)
	: p (p)
	{}
	
	double paramToDisplay (double v) const
	{
		if(p->isBipolar ())
		{
			double vb = (v - 0.5) * 2.;
			double sign = vb < 0 ? -1. : 1.;
			vb = ccl_abs (vb);

			if(p->getCurve ())
				vb = p->getCurve ()->normalizedToDisplay (vb);

			vb *= sign;
			if(p->isReverse ())
				vb = vb * -1.;
		
			return (vb * 0.5) + 0.5;
		}
		else
		{
			if(p->getCurve ())
				v = p->getCurve ()->normalizedToDisplay (v);
			
			if(p->isReverse ())
				v = 1. - v;

			return v;
		}
	}

	double displayToParam (double v) const
	{
		if(p->isBipolar ())
		{
			double vb = v * 2. - 1.;
			double sign = vb < 0 ? -1. : 1.;
			vb = ccl_abs (vb);

			if(p->getCurve ())
				vb = p->getCurve ()->displayToNormalized (vb);

			if(p->isReverse ())
				sign = sign * -1.;
			vb = vb * sign;

			return ((vb / 2) + 0.5);
		}
		else
		{
			if(p->isReverse ())
				v = 1. - v;

			if(p->getCurve ())
				v = p->getCurve ()->displayToNormalized (v);

			return v;
		}
	}
	
	double get () const
	{
		if(p == nullptr)
			return 0;

		return paramToDisplay (p->getNormalized ());	
	}

	double getForValue (double v) const 
	{
		return paramToDisplay (v);
	}
		
	void set (double v, bool update = false)
	{
		if(p == nullptr)
			return;

		p->setNormalized ((float)displayToParam (v), update);
	}
};

//************************************************************************************************
// IAliasParameter
/** Additional interface of alias parameter.
	\ingroup gui_param */
//************************************************************************************************

interface IAliasParameter: IUnknown
{
	/** Set original parameter. */
	virtual void CCL_API setOriginal (IParameter* p) = 0;

	/** Check if original parameter is set. */
	virtual tbool CCL_API hasOriginal () const = 0;

	/** Signaled when original parameter is about to be destroyed */
	DECLARE_STRINGID_MEMBER (kOriginalDestroyed)
	
	/** Argument of kChanged message when original has changed. */
	static const int kOriginalChanged = 'Orig';

	DECLARE_IID (IAliasParameter)
};

DEFINE_IID (IAliasParameter, 0x18de48ee, 0x1b7a, 0x462f, 0x9e, 0x24, 0xf9, 0x38, 0x6d, 0xb8, 0x92, 0xea)
DEFINE_STRINGID_MEMBER (IAliasParameter, kOriginalDestroyed, "originalDestroyed")

//************************************************************************************************
// IListParameter
/** Additional interface of list parameter.
	\ingroup gui_param */
//************************************************************************************************

interface IListParameter: IUnknown
{
	/** Append string to value list. */
	virtual void CCL_API appendString (StringRef string, int index = -1) = 0;

	/** Append variable type value. */
	virtual void CCL_API appendValue (VariantRef value, int index = -1) = 0;

	/** Append variable type value with string. */
	virtual void CCL_API appendValue (VariantRef value, StringRef string, int index = -1) = 0;

	/** Get index of given value. */
	virtual int	CCL_API getValueIndex (VariantRef value) const = 0;

	/** Get value at specified index. */
	virtual Variant	CCL_API getValueAt (int index) const = 0;

	/** Set value at specified index. */
	virtual tbool CCL_API setValueAt (int index, VariantRef value) = 0;

	/** Get value at current index. */
	virtual Variant CCL_API getSelectedValue () const = 0;

	/** Set current index to the index of the given value. */
	virtual tbool CCL_API selectValue (VariantRef value, tbool update = false) = 0;

	/** Remove value entry at specified index. */
	virtual void CCL_API removeAt (int index) = 0;

	/** Remove all value entries. */
	virtual void CCL_API removeAll () = 0;

	/** Check if value list is empty. */
	virtual tbool CCL_API isEmpty () const = 0;

	/** Get index of nearest value. Useful for float values that might not match exactly. Works for int and float values. */
	virtual int CCL_API getNearestValueIndex (VariantRef value) const = 0;

	/** Set current index to the index of the value nearest to the given value. */
	virtual tbool CCL_API selectNearestValue (VariantRef value, tbool update = false) = 0;

	DECLARE_IID (IListParameter)
};

DEFINE_IID (IListParameter, 0x143641b5, 0x92a0, 0x4a2e, 0xb7, 0xc, 0x84, 0xc5, 0x7e, 0x8f, 0xbd, 0xb3)

//************************************************************************************************
// IStructuredParameter
/** Additional interface of structured (nested) parameter. 
	\ingroup gui_param */
//************************************************************************************************

interface IStructuredParameter: IUnknown
{
	/** Set up parameter structure. */
	virtual void CCL_API prepareStructure () = 0;

	/** Clean up parameter structure.*/
	virtual void CCL_API cleanupStructure () = 0;

	/** Get number of child parameters. */
	virtual int CCL_API countSubParameters () const = 0;

	/** Get child parameter at given index. */
	virtual IParameter* CCL_API getSubParameter (int index) const = 0;

	DECLARE_IID (IStructuredParameter)
};

DEFINE_IID (IStructuredParameter, 0xc8b4992d, 0xf767, 0x457d, 0xb5, 0x2f, 0xca, 0x2c, 0x3a, 0xbe, 0xb4, 0xc)

//************************************************************************************************
// ICommandParameter
/**	Instead of IParamObserver::paramChanged, this parameter class generates a command message.
	The associated controller has to implement ICommandHandler::interpretCommand. 
	\ingroup gui_param */
//************************************************************************************************

interface ICommandParameter: IUnknown
{
	/** Get command category. */
	virtual StringID CCL_API getCommandCategory () const = 0;

	/** Get command name. */
	virtual StringID CCL_API getCommandName () const = 0;

	/** Set command category and name. */
	virtual void CCL_API setCommand (StringID category, StringID name) = 0;

	/** Enable if command is executable. */
	virtual tbool CCL_API checkEnabled () = 0;

	DECLARE_IID (ICommandParameter)
};

DEFINE_IID (ICommandParameter, 0xa99cc915, 0x729d, 0x4ed8, 0xa8, 0xa2, 0x54, 0x9a, 0x6d, 0x30, 0x9a, 0x13)

//************************************************************************************************
// IScrollParameter
/** 
	\ingroup gui_param */
//************************************************************************************************

interface IScrollParameter: IUnknown
{	
	/** Get page size (normalized 0..1). */
	virtual float CCL_API getPageSize () const = 0;
	
	/** Set page size (normalized 0..1). */
	virtual void CCL_API setPageSize (float pageSize) = 0;

	/** Set scroll range and page size. */
	virtual void CCL_API setRange (int range, float pageSize = 0.f) = 0;

	/** Check if scrolling possible. */
	virtual tbool CCL_API canScroll () const = 0;

	DECLARE_STRINGID_MEMBER (kStopAnimations)	///< signal to connected scroll view to stop running animations
	DECLARE_STRINGID_MEMBER (kAnimationAdded)	///< signal animation towards the parameter value started
	DECLARE_STRINGID_MEMBER (kAnimationRemoved)	///< signal animation towards the parameter value ended

	DECLARE_IID (IScrollParameter)
};

DEFINE_IID (IScrollParameter, 0xe7b2515e, 0x937, 0x4a79, 0xaf, 0xc2, 0xa, 0x58, 0x73, 0xf0, 0x6d, 0x6f)
DEFINE_STRINGID_MEMBER (IScrollParameter, kStopAnimations, "stopAnimations")
DEFINE_STRINGID_MEMBER (IScrollParameter, kAnimationAdded, "animationAdded")
DEFINE_STRINGID_MEMBER (IScrollParameter, kAnimationRemoved, "animationRemoved")

//************************************************************************************************
// IColorParam
/** 
	\ingroup gui_param */
//************************************************************************************************

interface IColorParam: IUnknown
{
	/** Get color value. */
	virtual Color& CCL_API getColor (Color& color) const = 0;

	/** Set color value. */
	virtual void CCL_API setColor (const Color& color, tbool update = false) = 0;

	DECLARE_IID (IColorParam)
};

DEFINE_IID (IColorParam, 0x82b6bb43, 0xb4ed, 0x4d1c, 0xb1, 0xab, 0x1b, 0x25, 0x25, 0x4c, 0xc3, 0x73)

//************************************************************************************************
// IImageProvider
/** 
	\ingroup gui_param */
//************************************************************************************************

interface IImageProvider: IUnknown
{
	/** Get the image. */
	virtual IImage* CCL_API getImage () const = 0;

	/** Set image and update controller. */
	virtual void CCL_API setImage (IImage* image, tbool update = false) = 0;

	DECLARE_IID (IImageProvider)
};

DEFINE_IID (IImageProvider, 0x9E539BC2, 0x6155, 0x4851, 0x8A, 0xDD, 0x5B, 0x10, 0x83, 0x57, 0x0A, 0xB1)

//************************************************************************************************
// IParamSplitter
/**	
	\ingroup gui_param */
//************************************************************************************************

interface IParamSplitter: IUnknown
{	
	/** Returns the number of parts */
	virtual int CCL_API countParts () const = 0;
	
	/** Write the parts into the array */
	virtual int CCL_API getParts (int parts[], int& sign, int sizeOfArray) const = 0;

	/** Sets the parameter from the parts */
	virtual int CCL_API setParts (const int parts[], int sign, int sizeOfArray) = 0;

	/** Write the delimiterss into the array */
	virtual int CCL_API getDelimiter (int delimiter[], int sizeOfArray) const = 0;

	/** Write the number of digits for each part into the array */
	virtual int CCL_API getPartSizes (int sizes[], int sizeOfArray) const = 0;

	/** Increment / Decrement */
	virtual void CCL_API incrementPart (int part, int amount) = 0;

	/** Integer value for splitter comparison. */
	virtual int CCL_API getSplitterID () const = 0;

	DECLARE_IID (IParamSplitter)
};

DEFINE_IID (IParamSplitter, 0xD1FAA9EC, 0x8672, 0x4707, 0x8C, 0xA4, 0x16, 0x03, 0xD0, 0x9C, 0x0D, 0x22)

} // namespace CCL

#endif // _ccl_iparameter_h
