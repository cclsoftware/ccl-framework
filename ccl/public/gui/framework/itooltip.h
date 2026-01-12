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
// Filename    : ccl/public/gui/framework/itooltip.h
// Description : Tooltip Interface
//
//************************************************************************************************

#ifndef _ccl_itooltip_h
#define _ccl_itooltip_h

#include "ccl/public/base/iunknown.h"

#include "ccl/public/gui/graphics/point.h"

namespace CCL {

interface IView;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (TooltipPopup, 0xA077C193, 0x3A76, 0x4834, 0xB2, 0x34, 0x05, 0x78, 0xF1, 0x13, 0xAA, 0x32);
}

//************************************************************************************************
// ITooltipPopup
/** 
	\ingroup gui_dialog */
//************************************************************************************************

interface ITooltipPopup: IUnknown
{
	enum DurationCodes ///< special codes for setDuration
	{
		kDefaultDuration = 0,	///< system default tooltip display duration
		kForever = -1			///< tooltip is not automatically hidden
	};
	
	/** Initialize popup. */
	virtual void CCL_API construct (IView* view) = 0;

	/** Show popup. */
	virtual void CCL_API show () = 0;
	
	/** Hide popup. */
	virtual void CCL_API hide () = 0;

	/** Hide popup after given ticks. */
	virtual void CCL_API setDuration (int64 ticks = kDefaultDuration) = 0;

	/** Get time to hide (system ticks). */
	virtual int64 CCL_API getTimeToHide () = 0;

	/** Set position in client coordinates of a given view, or in screen coordinates. */
	virtual void CCL_API setPosition (PointRef pos, IView* view = nullptr) = 0;
	
	/** Move near mouse position. */
	virtual void CCL_API moveToMouse () = 0;

	/** Set tooltip text. */
	virtual void CCL_API setText (StringRef text) = 0;

	/** Get tooltip text. */
	virtual StringRef CCL_API getText () const = 0;
	
	/** Prevent others from showing tooltips, e.g. when dragging a knob. */
	virtual tbool CCL_API isReserved () const = 0;
	virtual void CCL_API reserve (tbool state) = 0;	

	DECLARE_IID (ITooltipPopup)
};

DEFINE_IID (ITooltipPopup, 0xd8c6e8d, 0xc09a, 0x4b88, 0xb1, 0x7, 0x29, 0x29, 0xe2, 0xdc, 0xcf, 0xdf)

} // namespace CCL

#endif // _ccl_itooltip_h
