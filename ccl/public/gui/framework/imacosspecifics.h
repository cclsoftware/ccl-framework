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
// Filename    : ccl/public/gui/framework/imacosspecifics.h
// Description : Interfaces specific to macOS
//
//************************************************************************************************

#ifndef _ccl_imacosspecifics_h
#define _ccl_imacosspecifics_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
namespace MacOS {

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (MetalGraphicsInfo, 0x6266d1a9, 0xaaf9, 0x48f6, 0x97, 0x6b, 0x42, 0xf8, 0xd0, 0xc4, 0x1e, 0x94)
}

//************************************************************************************************
// IMetalGraphicsInfo
/** 
	\ingroup gui */
//************************************************************************************************

interface IMetalGraphicsInfo: IUnknown
{
	/** Check if Metal is available. */
	virtual tbool CCL_API isMetalAvailable () const = 0;

	/** Check if Metal is currently enabled. */
	virtual tbool CCL_API isMetalEnabled () const = 0;

	/** Enable or disable Metal (requires application restart). */
	virtual void CCL_API setMetalEnabled (tbool state) = 0;

	DECLARE_IID (IMetalGraphicsInfo)
};

DEFINE_IID (IMetalGraphicsInfo, 0xf8b79037, 0x3fbb, 0x4b78, 0xb3, 0xdc, 0x23, 0xfb, 0xb3, 0x2a, 0x6d, 0xe5)

} // namespace MacOS
} // namespace CCL

#endif // _ccl_imacosspecifics_h
