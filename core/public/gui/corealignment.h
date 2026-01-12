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
// Filename    : core/public/gui/corealignment.h
// Description : Geometry Alignment
//
//************************************************************************************************

#ifndef _corealignment_h
#define _corealignment_h

#include "core/public/coretypes.h"

#include "core/meta/generated/cpp/coregui-constants-generated.h"

namespace Core {

struct Alignment;

/** Alignment reference type. */
typedef const Alignment& AlignmentRef;

//************************************************************************************************
// Alignment
/** Horizontal and vertical alignment.
	\ingroup core_gui */
//************************************************************************************************

struct Alignment
{
	/** Alignment flags. */
	enum Flags
	{
		kHCenter = kAlignmentHCenter,
		kLeft = kAlignmentLeft,
		kRight = kAlignmentRight,
		kHMask = kAlignmentHMask,

		kVCenter = kAlignmentVCenter,
		kTop = kAlignmentTop,
		kBottom = kAlignmentBottom,
		kVMask = kAlignmentVMask,

		kCenter = kAlignmentCenter,
		kLeftCenter = kAlignmentLeftCenter,
		kRightCenter = kAlignmentRightCenter,
		kLeftTop = kAlignmentLeftTop,
		kRightTop = kAlignmentRightTop,
		kLeftBottom = kAlignmentLeftBottom,
		kRightBottom = kAlignmentRightBottom,
	};

	int align;

	Alignment (int align = kLeftCenter)
	: align (align)
	{}

	/** Get horizontal alignment. */
	int getAlignH () const;

	/** Get vertical alignment. */
	int getAlignV () const;

	/** Set horizontal alignment. */
	void setAlignH (int alignH);

	/** Set vertical alignment. */
	void setAlignV (int alignV);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Alignment inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline int Alignment::getAlignH () const
{ return align & kHMask; }

inline int Alignment::getAlignV () const 
{ return align & kVMask; }

inline void Alignment::setAlignH (int alignH)
{ align = getAlignV () | alignH; }

inline void Alignment::setAlignV (int alignV) 
{ align = getAlignH () | alignV; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _corealignment_h
