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
// Filename    : ccl/extras/audio/waveform/audiopeakbuffer.h
// Description : Audio Peak Buffer
//
//************************************************************************************************

#ifndef _ccl_audiopeakbuffer_h
#define _ccl_audiopeakbuffer_h

#include "ccl/base/object.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/base/datetime.h"
#include "ccl/public/audio/cclaudiotypes.h"

namespace CCL {

interface IAudioSource;
interface IProgressNotify;

//************************************************************************************************
// AudioPeakDetection
/** Peak detection shared by AudioPeakBuilder and AudioSourcePeakReader. */
//************************************************************************************************

namespace AudioPeakDetection
{
	typedef void (*DetectFunc) (float32& posPeak, float32& negPeak, const float32 buffer[], int numSamples);

	/** Install a custom detection function (optional). */
	void assign (DetectFunc detectFunc);

	void detect (float32& posPeak, float32& negPeak, const float32 buffer[], int numSamples);
}

//************************************************************************************************
// AudioPeakBuffer
/** Stores a downsampled min/max peak envelope of an audio signal in a compact format. */
//************************************************************************************************

class AudioPeakBuffer: public Object
{
public:
	DECLARE_CLASS (AudioPeakBuffer, Object)

	AudioPeakBuffer ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Peak formats
	//////////////////////////////////////////////////////////////////////////////////////////////

	enum class PeakFormat
	{
	   kInt8,
	   kInt16,
	   kFloat
	};

	typedef uint8 PeakValue8;
	typedef uint16 PeakValue16;
	typedef float32 PeakValueF;

	template <typename T> 
	struct TPeak
	{
		T p; // above zero
		T n; // below zero	
	};

	typedef TPeak<PeakValue8> Peak8;
	typedef TPeak<PeakValue16> Peak16;
	typedef TPeak<PeakValueF> PeakF;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Default configuration
	//////////////////////////////////////////////////////////////////////////////////////////////

	static constexpr uint32 kPeakRatioDefault = 128; ///< default audio frames per peak point
	static constexpr PeakFormat kPeakFormatDefault = PeakFormat::kInt16; ///< default peak buffer format
	static constexpr uint32 kPeakAllocationSize = 50000; ///< peak buffer size used for appending
	static constexpr uint32 kMinPeaksToGenerate = 512; ///< minimum number of peaks (can be lower if number of samples is lower)
	static constexpr uint32 kMaxChannels = kMaxAudioStreamChannels;
	static constexpr uint32 kScaledDownFactor = 255; ///< downsampling factor for the low-resolution peak overview

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Buffer metadata
	//////////////////////////////////////////////////////////////////////////////////////////////

	PROPERTY_VARIABLE (uint32, framesPerPeak, FramesPerPeak) ///< resolution in audio frames per peak
	PROPERTY_VARIABLE (TSamples, numSourceFrames, NumSourceFrames) ///< source material length in frames
	PROPERTY_VARIABLE (float32, maxSourcePeak, MaxSourcePeak) ///< max. amplitude (normalized) in source material	
	PROPERTY_OBJECT (DateTime, createTime, CreateTime) ///< timestamp of peak data creation

	/** Set creation timestamp to now, optionally shifted a few seconds into the future. */
	void updateTimestamp (bool shiftAhead = false);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Buffer management
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Allocate per-channel peak storage for the given channel count, capacity and format. */
	void resizeBuffers (uint32 maxChannels, uint32 maxPeakCapacity, PeakFormat format = kPeakFormatDefault);
	
	/** Free all buffers and reset to an empty state. */
	void disposeBuffers ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Buffer properties
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get the storage format peaks are encoded in. */
	PeakFormat getPeakFormat () const;

	/** Get the size in bytes of one encoded peak. */
	uint32 getPeakSize () const;
	
	/** Get the number of channels currently allocated. */
	uint32 getNumPeakChannels () const;

	/** Get how many peaks the current buffers can hold. */
	uint32 getMaxPeakCapacity () const;
	
	/** Get how many peaks are currently valid. */
	uint32 getNumPeaksValid () const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Buffer access
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Set how many peaks are valid, growing the buffers if needed. */
	void setNumPeaksValid (uint32 newNumPeaks);

	/** Append peaks for all channels, extending number of valid peaks. */
	void appendPeaks (const PeakF* peaks[], uint32 count);
	
	/** Get direct typed access to per-channel peak buffer (TPeak must match getPeakFormat()). */
	template <class TPeak> TPeak* getPeakBuffer (uint32 channel);
	template <class TPeak> const TPeak* getPeakBuffer (uint32 channel) const;

	/** Decode the peak at index for a channel. */
	void getPeak (PeakF& peak, uint32 channel, uint32 index) const;
	
	/** Encode and store a peak at index for a channel. */
	void setPeak (uint32 channel, uint32 index, const PeakF& peak);

	struct Internal;

protected:
	friend class AudioPeakBufferReader;

	PeakFormat peakFormat;
	Buffer peaks[kMaxChannels];
	Buffer peakPeaks[kMaxChannels];
	uint32 numChannels;
	uint32 numPeaksValid;
	uint32 numScaledPeaksValid;

	void updatePeakPeaks ();
	template <class TPeakType> void updatePeakPeaksT ();
};

//************************************************************************************************
// AudioPeakBuilder
/** Fills an AudioPeakBuffer by reading an IAudioSource from start to finsh. */
//************************************************************************************************

class AudioPeakBuilder
{
public:
	AudioPeakBuilder (AudioPeakBuffer& peakBuffer);

	/** Build peak envelope from source. */
	tresult build (IAudioSource& source,
				   AudioPeakBuffer::PeakFormat format = AudioPeakBuffer::kPeakFormatDefault,
				   uint32 framesPerPeak = AudioPeakBuffer::kPeakRatioDefault,
				   IProgressNotify* progress = nullptr);

protected:
	AudioPeakBuffer& peakBuffer;
	float32 maxPeak = -1.f;

	tresult processBlock (const AudioData& data, int sampleFrames, uint32 peakIndex);
};

//************************************************************************************************
// AudioPeakSource
/** Abstract interface for reading peak min/max values over a pixel range. */
//************************************************************************************************

class AudioPeakSource
{
public:
	static constexpr int kAccumulate = 1<<0;

	/** Parameters for one peak request. */
	struct PeakRequest
	{		
		uint32 channel = 0;
		int64 startPixel = 0;
		double framesPerPixel = 1.;
		float* minmax = nullptr;
		int numPixels = 0;
		int flags = 0;

		bool isAccumulate () const { return get_flag (flags, kAccumulate); }
	};

	/** Get number of channels in peak source. */
	virtual uint32 getNumPeakChannels () const = 0;

	/** Get two floats (min/max) for each pixel, i.e. capacity of minmax needs to be 2 x numPixels. */
	virtual int getPeaks (const PeakRequest& request) const = 0;

	/** Get combined peaks of all channels. */
	virtual int getCombinedPeaks (const PeakRequest& request) const;
};

//************************************************************************************************
// AudioPeakBufferReader
/** Reads peak min/max values from an already-built AudioPeakBuffer, using its scaled-down
	overview when zoomed out far enough. */
//************************************************************************************************

class AudioPeakBufferReader: public AudioPeakSource
{
public:
	AudioPeakBufferReader (AudioPeakBuffer& peakBuffer, bool allowScaledDown = true);

	// AudioPeakSource
	uint32 getNumPeakChannels () const override;
	int getPeaks (const PeakRequest& request) const override;

protected:
	AudioPeakBuffer& peakBuffer;
	bool allowScaledDown;
};

//************************************************************************************************
// AudioSourcePeakReader
/**	Detects peaks live from an IAudioSource, can be used for pixel ranges too fine-grained to be
	covered by an pre-built AudioPeakBuffer (e.g. zoomed in past its framesPerPeak resolution). */
//************************************************************************************************
 
class AudioSourcePeakReader: public AudioPeakSource
{
public:
	AudioSourcePeakReader (IAudioSource& source);

	// AudioPeakSource
	uint32 getNumPeakChannels () const override;
	int getPeaks (const PeakRequest& request) const override;

protected:
	IAudioSource& source;
};

//************************************************************************************************
// AudioPeakBuffer::Internal
/** Peak buffer internals for format conversion. */
//************************************************************************************************

struct AudioPeakBuffer::Internal
{
	static constexpr float kPeakScaler16 = (float)0xFFFF;
	static constexpr float kPeakScaler8 = (float)0xFF;
	static constexpr float kMaxPeakF = 100.f;

	static PeakValue16 peakValueFToInt16 (float value)
	{ return PeakValue16(ccl_bound (value, 0.f, 1.f) * kPeakScaler16); }
	
	static PeakValue8 peakValueFToInt8 (float value)
	{ return PeakValue8(ccl_bound (value, 0.f, 1.f) * kPeakScaler8); }

	static PeakValueF peakValue16ToFloat (PeakValue16 value)
	{ return PeakValueF(value) / kPeakScaler16; }
	
	static PeakValueF peakValue8ToFloat (PeakValue8 value)
	{ return PeakValueF(value) / kPeakScaler8; }

	static void peakFToInt16 (Peak16& dst, const PeakF& src)
	{ dst.n = peakValueFToInt16 (src.n); dst.p = peakValueFToInt16 (src.p); }
	
	static void peakFToInt8 (Peak8& dst, const PeakF& src)
	{ dst.n = peakValueFToInt8 (src.n); dst.p = peakValueFToInt8 (src.p); }

	static void peakInt16ToF (PeakF& dst, const Peak16& src)
	{ dst.n = peakValue16ToFloat (src.n); dst.p = peakValue16ToFloat (src.p); }
	
	static void peakInt8ToF (PeakF& dst, const Peak8& src)
	{ dst.n = peakValue8ToFloat (src.n); dst.p = peakValue8ToFloat (src.p); }

	static void getPeak (PeakF& peak, const Buffer& buffer, PeakFormat bufferFormat, uint32 index)
	{
		switch(bufferFormat)
		{
		case PeakFormat::kFloat :
			peak = buffer.as<PeakF> ()[index];
			break;
		case PeakFormat::kInt16 :
			peakInt16ToF (peak, buffer.as<Peak16> ()[index]);
			break;
		case PeakFormat::kInt8 :
			peakInt8ToF (peak, buffer.as<Peak8> ()[index]);
			break;
		}	
	}

	static void setPeak (Buffer& buffer, PeakFormat bufferFormat, uint32 index, const PeakF& peak)
	{
		switch(bufferFormat)
		{
		case PeakFormat::kFloat : 
			buffer.as<PeakF> ()[index] = peak; 
			break;
		case PeakFormat::kInt16 :
			peakFToInt16 (buffer.as<Peak16> ()[index], peak);
			break;
		case PeakFormat::kInt8 :
			peakFToInt8 (buffer.as<Peak8> ()[index], peak);
			break;
		}	
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AudioPeakBuffer inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class TPeakType>
TPeakType* AudioPeakBuffer::getPeakBuffer (uint32 channel)
{
	ASSERT (channel < numChannels)
	return peaks[channel].as<TPeakType> ();
}

template <class TPeakType>
const TPeakType* AudioPeakBuffer::getPeakBuffer (uint32 channel) const
{
	ASSERT (channel < numChannels)
	return peaks[channel].as<TPeakType> ();
}

INLINE void AudioPeakBuffer::getPeak (PeakF& peak, uint32 channel, uint32 index) const
{
	ASSERT (channel < numChannels)
	Internal::getPeak (peak, peaks[channel], peakFormat, index);
}

INLINE void AudioPeakBuffer::setPeak (uint32 channel, uint32 index, const PeakF& peak)
{
	ASSERT (channel < numChannels)
	Internal::setPeak (peaks[channel], peakFormat, index, peak);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_audiopeakbuffer_h