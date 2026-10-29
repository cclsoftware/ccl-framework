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
// Filename    : ccl/public/audio/iaudiometer.h
// Description : Audio Meter Interfaces
//
//************************************************************************************************

#ifndef _ccl_iaudiometer_h
#define _ccl_iaudiometer_h

#include "ccl/public/base/iunknown.h"

#include "ccl/public/audio/cclaudiotypes.h"

namespace CCL {

//************************************************************************************************
// IAudioMeterSource
/** Provides read access to a VU meter's current level snapshot. */
//************************************************************************************************

interface IAudioMeterSource: IUnknown
{
	/** Get number of meter channels. */
	virtual int CCL_API getChannelCount () const = 0;
	
	/**	Read the current meter snapshot, time-aligned with what is currently audible.
		Returns kResultFalse if not enough data has been processed yet. */
	virtual tresult CCL_API readMeterData (AudioMeterData& meterData) = 0;

	DECLARE_IID (IAudioMeterSource)
};

DEFINE_IID (IAudioMeterSource, 0x1a2b3c4d, 0x5e6f, 0x4a1b, 0x9c, 0x3d, 0x7f, 0x2e, 0x8b, 0x4c, 0x6d, 0x91)

} // namespace CCL

#endif // _ccl_iaudiometer_h
