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
// Filename    : ccl/public/gui/framework/ihandwriting.h
// Description : Interfaces for handwriting recognition
//
//************************************************************************************************

#ifndef _ccl_ihandwriting_h
#define _ccl_ihandwriting_h

#include "ccl/public/base/iunknown.h"

#include "ccl/public/gui/graphics/point.h"

namespace CCL {

//************************************************************************************************
// StrokePoint
//************************************************************************************************

struct StrokePoint
{
	PointF position;
	float32 pressure;
	float32 tiltX;
	float32 tiltY;
	double strokeTime; // in seconds

	StrokePoint ()
	: position (0.f, 0.f),
	  pressure (0.f),
	  tiltX (0.f),
	  tiltY (0.f),
	  strokeTime (0)
	{}
};

//************************************************************************************************
// IStroke
//************************************************************************************************

interface IStroke: IUnknown
{
	virtual uint32 CCL_API getPointCount () const = 0;
	
	virtual StrokePoint CCL_API getPointAt (uint32 pointIndex) const = 0;

	virtual PointF CCL_API calculateCenter () const = 0;

	virtual RectF CCL_API calculateBoundingRect () const = 0;

	DECLARE_IID (IStroke)
};

DEFINE_IID (IStroke, 0x47c15c3b, 0xc389, 0x49fd, 0x83, 0xa5, 0x40, 0xe8, 0x99, 0x85, 0xba, 0x38)

//************************************************************************************************
// IStrokeContainer
//************************************************************************************************

interface IStrokeContainer: IUnknown
{
	virtual uint32 CCL_API getStrokeCount () const = 0;
	
	virtual IStroke* CCL_API getStrokeAt (uint32 strokeIndex) const = 0;

	DECLARE_IID (IStrokeContainer)
};

DEFINE_IID (IStrokeContainer, 0x1c60aa27, 0xba21, 0x4e71, 0x9d, 0x4a, 0x35, 0x29, 0x66, 0x9, 0x73, 0xd5)

} // namespace CCL

#endif // _ccl_ihandwriting_h
