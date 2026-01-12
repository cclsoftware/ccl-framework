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
// Filename    : ccl/gui/graphics/imaging/bitmapfilter.cpp
// Description : Bitmap Filter
//
//************************************************************************************************

#include "ccl/gui/graphics/imaging/bitmapfilter.h"

#if !CCL_STATIC_LINKAGE // already in corelib when linked statically
#include "core/gui/corebitmapprimitives.impl.h"
#endif

using namespace CCL;

//************************************************************************************************
// BitmapFilterFactory
//************************************************************************************************

BitmapFilter* BitmapFilterFactory::createFilter (StringID which)
{
	#define CREATE_FILTER(which, filterName, FilterClass) \
	if(which.compare (filterName, false) == 0) return NEW FilterClass;

	CREATE_FILTER (which, BitmapFilters::kFilterList, BitmapFilterList)
	CREATE_FILTER (which, BitmapFilters::kClear, BitmapFilters::ClearFilter)
	CREATE_FILTER (which, BitmapFilters::kPremultiplyAlpha, BitmapFilters::PremultipliedAlpha)
	CREATE_FILTER (which, BitmapFilters::kRevertPremulAlpha, BitmapFilters::RevertPremultipliedAlpha)
	CREATE_FILTER (which, BitmapFilters::kByteSwapRGB, BitmapFilters::ByteSwapRGB)
	CREATE_FILTER (which, BitmapFilters::kInvert, BitmapFilters::Inverter)
	CREATE_FILTER (which, BitmapFilters::kGrayScale, BitmapFilters::GrayScaler)
	CREATE_FILTER (which, BitmapFilters::kAlpha, BitmapFilters::AlphaSetter)
	CREATE_FILTER (which, BitmapFilters::kBlend, BitmapFilters::Blender)
	CREATE_FILTER (which, BitmapFilters::kLighten, BitmapFilters::Lightener)
	CREATE_FILTER (which, BitmapFilters::kTint, BitmapFilters::Tinter)
	CREATE_FILTER (which, BitmapFilters::kLightAdapt, BitmapFilters::LightAdapter)
	CREATE_FILTER (which, BitmapFilters::kNoise, BitmapFilters::NoiseAdder)
	CREATE_FILTER (which, BitmapFilters::kColorize, BitmapFilters::Colorizer)
	CREATE_FILTER (which, BitmapFilters::kFill, BitmapFilters::FillFilter)
	CREATE_FILTER (which, BitmapFilters::kAnalyze, BitmapFilters::AnalysisFilter)
	CREATE_FILTER (which, BitmapFilters::kSaturator, BitmapFilters::Saturator)
	CREATE_FILTER (which, BitmapFilters::kBlurX, BitmapFilters::BlurXFilter)
	CREATE_FILTER (which, BitmapFilters::kBlurY, BitmapFilters::BlurYFilter)
	
	#undef CREATE_FILTER

	CCL_DEBUGGER ("Unknown bitmap filter!")
	return nullptr;
}

//************************************************************************************************
// BitmapFilter
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (BitmapFilter, Object)
DEFINE_CLASS_NAMESPACE (BitmapFilter, NAMESPACE_CCL)

//************************************************************************************************
// BitmapFilterList
//************************************************************************************************

DEFINE_CLASS_HIDDEN (BitmapFilterList, BitmapFilter)

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapFilterList::BitmapFilterList ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapFilterList::~BitmapFilterList ()
{
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapFilterList::removeAll ()
{
	VectorForEach (filters, IBitmapFilter*, filter)
		filter->release ();
	EndFor
	filters.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BitmapFilterList::addFilter (IBitmapFilter* filter, tbool share)
{
	ASSERT (filter != nullptr)
	if(filter == nullptr)
		return kResultInvalidPointer;
	filters.add (share ? return_shared (filter) : filter);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BitmapFilterList::processData (BitmapData& dstData, const BitmapData& _srcData)
{
	if(dstData.scan0 != _srcData.scan0)
		BitmapPrimitives32::copyFrom (dstData, _srcData);

	VectorForEach (filters, IBitmapFilter*, filter)
		tresult tr = filter->processData (dstData, dstData); // dst to dst = inplace processing
		if(tr != kResultOk)
			return tr;
	EndFor
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (BitmapFilterList)
	DEFINE_METHOD_ARGS ("addFilter", "filter")
END_METHOD_NAMES (BitmapFilterList)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BitmapFilterList::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "addFilter")
	{
		UnknownPtr<IBitmapFilter> filter (msg[0].asUnknown ());
		returnValue = addFilter (filter, true);
		return true;
	}
	else
		return Object::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// AnalysisFilter
//************************************************************************************************

BitmapFilters::AnalysisFilter::AnalysisFilter ()
: saturationAverage (0.)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BitmapFilters::AnalysisFilter::processData (BitmapData& dstData, const BitmapData& srcData)
{
	// must be inplace!
	ASSERT (dstData.scan0 == srcData.scan0 && srcData.format == IBitmap::kRGBAlpha)
	if(dstData.scan0 != srcData.scan0 || srcData.format != IBitmap::kRGBAlpha)
		return kResultFailed;

	int pixelCount = srcData.width * srcData.height;
	double saturationSum = 0.;

	for(int y = 0; y < srcData.height; y++)
		for(int x = 0; x < srcData.width; x++)
		{
			const RGBA& src = srcData.rgbaAt (x, y);
			Color c (src.red, src.green, src.blue, src.alpha);
			ColorHSV hsv (c);
			saturationSum += hsv.s;
		}

	setSaturationAverage (saturationSum / (double)pixelCount);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BitmapFilters::AnalysisFilter::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "saturationAverage")
	{
		var = getSaturationAverage ();
		return true;
	}
	return BitmapFilter::getProperty (var, propertyId);
}
