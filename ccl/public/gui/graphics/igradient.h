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
// Filename    : ccl/public/gui/graphics/igradient.h
// Description : Gradient Interface
//
//************************************************************************************************

#ifndef _ccl_igradient_h
#define _ccl_igradient_h

#include "ccl/public/base/iunknown.h"

#include "ccl/public/gui/graphics/color.h"
#include "ccl/public/gui/graphics/point.h"

namespace CCL {

//************************************************************************************************
// IGradient
/** Basic interface for gradients.
	\ingroup gui_graphics */
//************************************************************************************************

interface IGradient: IUnknown
{
	/** Gradient type hint. */
	DEFINE_ENUM (TypeHint)
	{
		kLinearGradient,	///< linear gradient
		kRadialGradient		///< radial gradient
	};

	/** Gradient stop. */
	struct Stop
	{
		float position = 0.f;	///< relative position [0..1]
		Color color;			///< color of gradient stop
	};

	DECLARE_IID (IGradient)
};

DEFINE_IID (IGradient, 0x71c68bdf, 0x3b7d, 0x4134, 0xa8, 0x50, 0xae, 0xb, 0x90, 0x96, 0x6, 0xc7)

//************************************************************************************************
// ILinearGradient
/** Interface for linear gradient used in brush.
	\ingroup gui_graphics */
//************************************************************************************************

interface ILinearGradient: IGradient
{
	/** Construct linear gradient (optional: copy stops from other gradient). */
	virtual tresult CCL_API construct (PointFRef startPoint, PointFRef endPoint, const Stop stops[], int stopCount,
									   IGradient* other = nullptr) = 0;

	DECLARE_IID (ILinearGradient)
};

DEFINE_IID (ILinearGradient, 0x7bd22d20, 0x7b00, 0x44e8, 0x9b, 0xfe, 0x23, 0x33, 0xad, 0xce, 0xa6, 0x12)

//************************************************************************************************
// IRadialGradient
/** Interface for radial gradient used in brush.
	\ingroup gui_graphics */
//************************************************************************************************

interface IRadialGradient: IGradient
{
	/** Construct radial gradient (optional: copy stops from other gradient) */
	virtual tresult CCL_API construct (PointFRef center, float radius, const Stop stops[], int stopCount,
									   IGradient* other = nullptr) = 0;

	DECLARE_IID (IRadialGradient)
};

DEFINE_IID (IRadialGradient, 0x3bdd90df, 0x598f, 0x4971, 0xa6, 0x99, 0x40, 0xfe, 0xc9, 0x70, 0xb6, 0x16)

} // namespace CCL

#endif // _ccl_igradient_h
