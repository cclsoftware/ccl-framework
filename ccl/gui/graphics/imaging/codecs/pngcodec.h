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
// Filename    : ccl/gui/graphics/imaging/codecs/pngcodec.h
// Description : PNG Bitmap Codec
//
//************************************************************************************************

#ifndef _ccl_pngcodec_h
#define _ccl_pngcodec_h

#include "ccl/gui/graphics/imaging/bitmapcodec.h"

namespace CCL {

//************************************************************************************************
// PNGBitmapCodec
//************************************************************************************************

class PNGBitmapCodec: public BitmapCodec
{
public:
	// BitmapCodec
	const FileType& CCL_API getFileType () const override;
	IBitmapDecoder* CCL_API createBitmapDecoder (IMemoryStream& stream) override;
	IBitmapEncoder* CCL_API createBitmapEncoder (IStream& stream) override;
};

} // namespace CCL

#endif // _ccl_pngcodec_h
