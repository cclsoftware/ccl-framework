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
// Filename    : ccl/extras/audio/audiocurves.cpp
// Description : Audio Interpolator Curves
//
//************************************************************************************************

#include "ccl/extras/audio/audiocurves.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (AudioCurves, kFirstRun)
{
	ParamCurveFactory::instance ().addCurve<AudioFaderDBCurve> (CurveNames::kFaderDB);
	ParamCurveFactory::instance ().addCurve<AudioFaderGainCurve> (CurveNames::kFaderGain);
	ParamCurveFactory::instance ().addCurve<AudioFrequencyCurve> (CurveNames::kFrequency);
	return true;
}

//************************************************************************************************
// AudioFaderDBCurve / AudioFaderGainCurve / AudioFrequencyCurve
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioFaderDBCurve, InterpolatorCurve)
DEFINE_CLASS_HIDDEN (AudioFaderGainCurve, InterpolatorCurve)
DEFINE_CLASS_HIDDEN (AudioFrequencyCurve, InterpolatorCurve)
