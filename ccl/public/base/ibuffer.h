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
// Filename    : ccl/public/base/ibuffer.h
// Description : Buffer interface
//
//************************************************************************************************

#ifndef _ccl_ibuffer_h
#define _ccl_ibuffer_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IBuffer
//************************************************************************************************

interface IBuffer: IUnknown
{
	virtual void* CCL_API getBufferAddress () const = 0;

	virtual uint32 CCL_API getBufferSize () const = 0;

	DECLARE_IID (IBuffer)
};

DEFINE_IID (IBuffer, 0xe01a2165, 0x2790, 0x4106, 0xb3, 0x5d, 0x21, 0xe0, 0x12, 0x52, 0xa, 0xb)

} // namespace CCL

#endif // _ccl_ibuffer_h
