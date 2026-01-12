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
// Filename    : ccl/public/gui/framework/iviewanimation.h
// Description : View Animation Interfaces
//
//************************************************************************************************

#ifndef _ccl_iviewanimation_h
#define _ccl_iviewanimation_h

#include "ccl/public/gui/graphics/types.h"

namespace CCL {

interface IView;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (ViewScreenCapture, 0xe0c3509e, 0x2e5c, 0x4d75, 0xae, 0xc9, 0x29, 0x33, 0x65, 0x48, 0x96, 0xb)
}

//************************************************************************************************
// IViewScreenCapture
/**
	\ingroup gui */
//************************************************************************************************

interface IViewScreenCapture: IUnknown
{
	enum Options
	{
		kPlatformMode = 1<<0	///< use platform mode to capture foreign views
	};

	/** Take screenshot of given view. */
	virtual IImage* CCL_API takeScreenshot (IView* view, const Rect* rect = nullptr, int options = 0) = 0;

	DECLARE_IID (IViewScreenCapture)
};

DEFINE_IID (IViewScreenCapture, 0xcb194f10, 0xf525, 0x4083, 0xa9, 0x38, 0xef, 0xf9, 0xa, 0x8e, 0x3, 0x7d)

//************************************************************************************************
// IViewAnimator
/**
	\ingroup gui */
//************************************************************************************************

interface IViewAnimator: IUnknown
{
	/** Set transition property, same as IObject::setProperty(). */
	virtual tbool CCL_API setTransitionProperty (StringID propertyId, VariantRef value) = 0;

	DECLARE_STRINGID_MEMBER (kDuration)
	DECLARE_STRINGID_MEMBER (kTimingType)
	DECLARE_STRINGID_MEMBER (kFromRect)			///< IUIValue

	DECLARE_IID (IViewAnimator)
};

DEFINE_IID (IViewAnimator, 0x73cc2c02, 0x381a, 0x4157, 0xbb, 0xcd, 0xf2, 0x1a, 0x95, 0xbf, 0x70, 0xcb)
DEFINE_STRINGID_MEMBER (IViewAnimator, kDuration, "duration")
DEFINE_STRINGID_MEMBER (IViewAnimator, kTimingType, "timingType")
DEFINE_STRINGID_MEMBER (IViewAnimator, kFromRect, "fromRect")

} // namespace CCL

#endif // _ccl_iviewanimation_h
