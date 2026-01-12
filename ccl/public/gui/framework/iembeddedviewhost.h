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
// Filename    : ccl/public/gui/framework/iembeddedviewhost.h
// Description : Interface to get information about views of a foreign GUI toolkit (used by CCL Spy)
//
//************************************************************************************************

#ifndef _ccl_iembeddedviewhost_h
#define _ccl_iembeddedviewhost_h

#include "ccl/public/base/iunknown.h"

#include "core/public/gui/coreuiproperties.h"

namespace CCL {

//************************************************************************************************
// IEmbeddedViewHost
/** Interface to get information about views of a foreign GUI toolkit. */
//************************************************************************************************

interface IEmbeddedViewHost: IUnknown
{
	/** Opaque view reference, root is addressed with null. */
	typedef void* ViewRef;

	/** View property - ViewSizeProperty, ColorProperty, etc. */
	typedef Core::Property ViewProperty;

	/** Get property for given view. */
	virtual tbool CCL_API getViewProperty (ViewProperty& value, ViewRef view) const = 0;

	/** Get number of sub views in given parent. */
	virtual int CCL_API getSubViewCount (ViewRef parent) const = 0;

	/** Get sub view at given index. */
	virtual ViewRef CCL_API getSubViewAt (ViewRef parent, int index) const = 0;

	DECLARE_IID (IEmbeddedViewHost)

	struct ScreenScalingProperty;
};

DEFINE_IID (IEmbeddedViewHost, 0xB6C5B550, 0x8EC7, 0x44C4, 0x94, 0x63, 0xEC, 0xBD, 0xC4, 0x31, 0xEC, 0x0D)

//************************************************************************************************
// IEmbeddedViewHost::ScreenScalingProperty
/** Scaling factor applied to the foreign view tree when drawn on screen. */
//************************************************************************************************

struct IEmbeddedViewHost::ScreenScalingProperty: Core::Property
{
	enum { kPropertyID = FOUR_CHAR_ID ('S', 'c', 'r', 'S') };

	PointF scaleFactor;

	ScreenScalingProperty (PointFRef scaleFactor = PointF (1.f, 1.f))
	: Property (kPropertyID, sizeof(ScreenScalingProperty)),
	  scaleFactor (scaleFactor)
	{}
};

} // namespace CCL

#endif // _ccl_iembeddedviewhost_h
