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
// Filename    : ccl/gui/graphics/imaging/bitmapfilter.h
// Description : Bitmap Filter
//
//************************************************************************************************

#ifndef _ccl_bitmapfilter_h
#define _ccl_bitmapfilter_h

#include "ccl/base/object.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/collections/vector.h"

#include "ccl/public/gui/graphics/ibitmapfilter.h"

#include "core/gui/corebitmapprimitives.h"

namespace CCL {

using Core::BitmapPrimitives;
using Core::BitmapPrimitives32;
class BitmapFilter;

//************************************************************************************************
// BitmapFilterFactory
//************************************************************************************************

class BitmapFilterFactory
{
public:
	static BitmapFilter* createFilter (StringID which);
};

//************************************************************************************************
// BitmapFilter
//************************************************************************************************

class BitmapFilter: public Object,
					public IBitmapFilter
{
public:
	DECLARE_CLASS_ABSTRACT (BitmapFilter, Object)

	CLASS_INTERFACE (IBitmapFilter, Object)
};

//************************************************************************************************
// BitmapFilterList
//************************************************************************************************

class BitmapFilterList: public BitmapFilter,
						public IBitmapFilterList
{
public:
	DECLARE_CLASS (BitmapFilterList, BitmapFilter)
	DECLARE_METHOD_NAMES (BitmapFilterList)

	BitmapFilterList ();
	~BitmapFilterList ();

	int count () const { return filters.count (); }
	IBitmapFilter* at (int index) const { return filters.at (index); }

	// IBitmapFilterList
	tresult CCL_API addFilter (IBitmapFilter* filter, tbool share = false) override;
	tresult CCL_API processData (BitmapData& dstData, const BitmapData& srcData) override;

	operator IBitmapFilter* () { return static_cast<IBitmapFilterList*> (this); }

	CLASS_INTERFACE (IBitmapFilterList, BitmapFilter)

protected:
	Vector<IBitmapFilter*> filters;

	void removeAll ();

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Bitmap Filter Classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace BitmapFilters {

//************************************************************************************************
// ClearFilter
//************************************************************************************************

class ClearFilter: public BitmapFilter
{
public:
	tresult CCL_API processData (BitmapData& dstData, const BitmapData& srcData) override
	{
		BitmapPrimitives::clear (dstData);
		return kResultOk;
	}
};

//************************************************************************************************
// BasicFilter
//************************************************************************************************

template <BitmapPrimitives32::BasicModifier func>
class BasicFilter: public BitmapFilter
{
public:
	// IBitmapFilter
	tresult CCL_API processData (BitmapData& dstData, const BitmapData& srcData) override
	{
		func (dstData, srcData);
		return kResultOk;
	}
};

typedef BasicFilter<BitmapPrimitives32::premultiplyAlpha> PremultipliedAlpha;
typedef BasicFilter<BitmapPrimitives32::revertPremultipliedAlpha> RevertPremultipliedAlpha;
typedef BasicFilter<BitmapPrimitives32::byteSwapRGB> ByteSwapRGB;
typedef BasicFilter<BitmapPrimitives32::invert> Inverter;
typedef BasicFilter<BitmapPrimitives32::grayScale> GrayScaler;

//************************************************************************************************
// ValueFilter
//************************************************************************************************

class ValueFilter: public BitmapFilter
{
public:
	ValueFilter (): value (0) {}

	PROPERTY_VARIABLE (float, value, Value)

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override
	{
		if(propertyId == kValueID)
		{
			var = value;
			return true;
		}
		return SuperClass::getProperty (var, propertyId);
	}

	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override
	{
		if(propertyId == kValueID)
		{
			value = var.asFloat ();
			return true;
		}
		return SuperClass::setProperty (propertyId, var);
	}
};

template <BitmapPrimitives32::ValueModifier func>
class TValueFilter: public ValueFilter
{
public:
	// IBitmapFilter
	tresult CCL_API processData (BitmapData& dstData, const BitmapData& srcData) override
	{
		func (dstData, srcData, value);
		return kResultOk;
	}
};

typedef TValueFilter<BitmapPrimitives32::setAlpha> AlphaSetter;
typedef TValueFilter<BitmapPrimitives32::scaleAlpha> Blender;
typedef TValueFilter<BitmapPrimitives32::lighten> Lightener;
typedef TValueFilter<BitmapPrimitives32::addNoise> NoiseAdder;
typedef TValueFilter<BitmapPrimitives32::saturate> Saturator;
typedef TValueFilter<BitmapPrimitives32::blurX> BlurXFilter;
typedef TValueFilter<BitmapPrimitives32::blurY> BlurYFilter;

//************************************************************************************************
// ColorFilter
//************************************************************************************************

class ColorFilter: public BitmapFilter
{
public:
	PROPERTY_VARIABLE (Color, color, Color)

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override
	{
		if(propertyId == kColorID)
		{
			var = (int)(uint32)color;
			return true;
		}
		return SuperClass::getProperty (var, propertyId);
	}

	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override
	{
		if(propertyId == kColorID)
		{
			color = Color::fromInt (var.asInt ());
			return true;
		}
		return SuperClass::setProperty (propertyId, var);
	}
};

template <BitmapPrimitives32::ColorModifier func>
class TColorFilter: public ColorFilter
{
public:
	// IBitmapFilter
	tresult CCL_API processData (BitmapData& dstData, const BitmapData& srcData) override
	{
		func (dstData, srcData, color);
		return kResultOk;
	}
};

typedef TColorFilter<BitmapPrimitives32::tint> Tinter;
typedef TColorFilter<BitmapPrimitives32::colorize> Colorizer;
typedef TColorFilter<BitmapPrimitives32::lightAdapt> LightAdapter;

//************************************************************************************************
// FillFilter
//************************************************************************************************

class FillFilter: public ColorFilter
{
public:
	// IBitmapFilter
	tresult CCL_API processData (BitmapData& dstData, const BitmapData& srcData) override
	{
		BitmapPrimitives32::fillRect (dstData, Rect (0, 0, dstData.width, dstData.height), color);
		return kResultOk;
	}
};

//************************************************************************************************
// AnalysisFilter
//************************************************************************************************

class AnalysisFilter: public BitmapFilter
{
public:
	AnalysisFilter ();

	PROPERTY_VARIABLE (double, saturationAverage, SaturationAverage)

	// AnalysisFilter
	tresult CCL_API processData (BitmapData& dstData, const BitmapData& srcData) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

} // namespace BitmapFilters

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_bitmapfilter_h
