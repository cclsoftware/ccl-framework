//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/skia/skialayer.wasm.h
// Description : Graphics Layer for Skia content
//
//************************************************************************************************

#ifndef _skialayer_wasm_h
#define _skialayer_wasm_h

#include "ccl/gui/graphics/nativegraphics.h"

namespace CCL {
namespace Wasm {

//************************************************************************************************
// SkiaLayerFactory
//************************************************************************************************

class SkiaLayerFactory
{
public:
	static IGraphicsLayer* createLayer (UIDRef classID);
};

} // namespace Wasm
} // namespace CCL

#endif // _skialayer_wasm_h
 
