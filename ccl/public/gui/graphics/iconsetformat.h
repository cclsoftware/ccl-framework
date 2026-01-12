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
// Filename    : ccl/public/gui/graphics/iconsetformat.h
// Description : Icon Set Format Definitions
//
//************************************************************************************************

#ifndef _ccl_iconsetformat_h
#define _ccl_iconsetformat_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/primitives.h"

namespace CCL {

//************************************************************************************************
// IconSetFormat
//************************************************************************************************

class IconSetFormat
{
public:
	enum IconSizeIntegerID
	{
		kIcon16x16 = 1<<0,
		kIcon32x32 = 1<<1,
		kIcon48x48 = 1<<2,
		kIcon64x64 = 1<<3,
		kIcon128x128 = 1<<4,
		kIcon256x256 = 1<<5,
		kIcon512x512 = 1<<6
	};

	struct IconSize
	{
		int intId;
		StringID name;
		int size;
	};

	static const int kIconSizesMin = 4; // normal / small / medium / large only
	static const int kIconSizesAll = 7;

	static const IconSize& getIconSizeAt (int index)
	{
		static const IconSize iconSizes[kIconSizesAll] =
		{
			{kIcon32x32,	CSTR ("normal"),	32},	// keep "normal" on top!
			{kIcon16x16,	CSTR ("small"),		16},
			{kIcon48x48,	CSTR ("medium"),	48}, 
			{kIcon64x64,	CSTR ("large"),		64},
			//--------------------------------------
			{kIcon128x128,	CSTR ("128x128"),	128},	// "xlarge"
			{kIcon256x256,	CSTR ("256x256"),	256},	// "jumbo"
			{kIcon512x512,	CSTR ("512x512"),	512}
		};

		return iconSizes[ccl_bound (index, 0, kIconSizesAll-1)];
	}
};

} // namespace CCL

#endif // _ccl_iconsetformat_h
