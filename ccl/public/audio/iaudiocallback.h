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
// Filename    : ccl/public/audio/iaudiocallback.h
// Description : Audio Callback Interface
//
//************************************************************************************************

#ifndef _ccl_iaudiocallback_h
#define _ccl_iaudiocallback_h

#include "ccl/public/base/iunknown.h"

#include "ccl/public/audio/cclaudiotypes.h"

namespace CCL {

//************************************************************************************************
// AudioCallbackContext
/**	Context information for one audio callback. */
//************************************************************************************************

struct AudioCallbackContext
{
	int byteSize = sizeof(AudioCallbackContext); ///< size of this struct in bytes

	int flags = 0;					///< (see below)
	TSeconds systemTime = 0;		///< system time in seconds
	TSamples samplePos = 0;			///< sample position
	SampleRate sampleRate = 0;		///< current sample rate

	static constexpr int kOverload = 1<<0; ///< an overload has been detected
};

//************************************************************************************************
// IAudioProcessCallback
/** Callback interface for audio processing in a push-model. */
//************************************************************************************************

interface IAudioProcessCallback: IUnknown
{
	/** Called periodically on a realtime-priority thread when audio processing is active. */
	virtual tresult CCL_API processAudio (const AudioCallbackContext& context,
										  const AudioIOData& ioData) = 0;

	/** Called after audio data has been copied. */
	virtual tresult CCL_API postProcess () = 0; 

	DECLARE_IID (IAudioProcessCallback)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioProcessCallback, 0x81919b98, 0xf5a5, 0x4405, 0xbc, 0x51, 0xd5, 0x75, 0xd7, 0x97, 0x27, 0xd7)
#endif

} // namespace CCL

#endif // _ccl_iaudiocallback_h
