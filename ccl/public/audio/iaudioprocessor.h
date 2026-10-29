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
// Filename    : ccl/public/audio/iaudioprocessor.h
// Description : Audio Processor Interfaces
//
//************************************************************************************************

#ifndef _ccl_iaudioprocessor_h
#define _ccl_iaudioprocessor_h

#include "ccl/public/audio/cclaudiotypes.h"

namespace CCL {

//************************************************************************************************
// AudioProcessContext
/**	Context information for one audio processing block. */
//************************************************************************************************

struct AudioProcessContext
{	
	int byteSize = sizeof(AudioProcessContext);	///< size of this struct in bytes
	
	int flags = 0;
	TSeconds systemTime = 0;		///< system time in seconds
	TSamples samplePos = 0;			///< sample position
	SampleRate sampleRate = 0;		///< current sample rate
	
	int sampleOffset = 0;			///< offset in samples of the audio being processed,
									///< relative to samplePos and systemTime
};

//************************************************************************************************
// AudioParamValue
/** Plain audio parameter value (transported as double). */
//************************************************************************************************

struct AudioParamValue
{
	double value = 0;

	AudioParamValue (double _value = 0) { setDouble (_value); }

	void setBool (bool _value) { value = double(_value); }
	void setInt (int _value) { value = double(_value); }
	void setFloat (float _value) { value = double(_value); }
	void setDouble (double _value) { value = _value; }

	bool asBool () const { return bool(value); }
	int asInt () const { return int(value); }
	float asFloat () const { return float(value); }
	double asDouble () const { return value; }
};

//************************************************************************************************
// AudioParamEvent
/** Realtime audio parameter change. */
//************************************************************************************************

struct AudioParamEvent
{
	int tag = 0;				///< parameter tag
	AudioParamValue value;		///< parameter value
	int sampleOffset = 0;		///< offset in current audio block
};

//************************************************************************************************
// AudioParamData
/** Realtime audio parameter changes transferred in process call. */
//************************************************************************************************

struct AudioParamData
{
	int numEvents = 0;
	const AudioParamEvent* events = nullptr;

	bool isEmpty () const { return numEvents < 1; }
};

//************************************************************************************************
// AudioProcessData
/**	Audio processing block data. */
//************************************************************************************************

struct AudioProcessData
{
	int byteSize = sizeof(AudioProcessData);	///< size of this struct in bytes
	
	const AudioProcessContext& context;
	const AudioIOData& ioData;
	const AudioParamData& paramData;
};

//************************************************************************************************
// IAudioProcessor
/** Interface for audio processing in a pull-model. */
//************************************************************************************************

interface IAudioProcessor: IUnknown
{
	/** Check if processing is currently enabled. */
	virtual tbool CCL_API isProcessing () const = 0;

	/**	Enable/disable processing (realtime-safe). */
	virtual tresult CCL_API setProcessing (tbool state) = 0;

	/**	Process one audio block.
		Called periodically on a realtime-priority thread when processing is enabled. */
	virtual tresult CCL_API process (const AudioProcessData& data) = 0;

	DECLARE_IID (IAudioProcessor)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioProcessor, 0x8eaf52ef, 0x5234, 0x4cbe, 0x99, 0xaf, 0xcc, 0xdf, 0x6, 0xbb, 0x92, 0x27)
#endif

//************************************************************************************************
// IAudioDisplayLatency
/** Interface to set audio display latency. */
//************************************************************************************************

interface IAudioDisplayLatency: IUnknown
{
	/** Set display latency (output) in audio sample frames. */
	virtual void CCL_API setDisplayLatency (int latencyInSamples) = 0;

	DECLARE_IID (IAudioDisplayLatency)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioDisplayLatency, 0x951d32a, 0x1809, 0x4c67, 0x8b, 0x11, 0x5c, 0x69, 0xbd, 0x5c, 0x1e, 0xdc)
#endif

} // namespace CCL

#endif // _ccl_iaudioprocessor_h
