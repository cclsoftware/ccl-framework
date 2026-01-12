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
// Filename    : ccl/gui/graphics/imaging/tiler.h
// Description : Tiler
//
//************************************************************************************************

#ifndef _ccl_tiler_h
#define _ccl_tiler_h

#include "ccl/gui/graphics/imaging/image.h"

namespace CCL {

//************************************************************************************************
// Blitter
//************************************************************************************************

class Blitter
{
public:
	virtual void blit (RectRef src, RectRef dst) = 0;
};

//************************************************************************************************
// Tiler
//************************************************************************************************

class Tiler
{
public:
	static tresult tile (Blitter& blitter, int method, RectRef src, RectRef dst, RectRef clip, RectRef margins);
};

} // namespace CCL

#endif // _ccl_tiler_h
