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
// Filename    : ccl/extras/audio/waveform/audiopeakbuffer.cpp
// Description : Audio Peak Buffer
//
//************************************************************************************************

#define AVOID_REPEATS 1

#include "ccl/extras/audio/waveform/audiopeakbuffer.h"
#include "ccl/extras/audio/audiodatabuffer.h"
#include "ccl/extras/audio/audioscratchbuffer.h"
#include "ccl/extras/audio/cclaudiodataops.h"

#include "ccl/public/audio/iaudiosource.h"
#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// AudioPeakDetection
//************************************************************************************************

namespace AudioPeakDetection
{
	static DetectFunc detectFunc = nullptr;
	void assign (DetectFunc _detectFunc) { detectFunc = _detectFunc; }

	void detect (float32& posPeak, float32& negPeak, const float32 buffer[], int numSamples)
	{
		if(detectFunc)
			(*detectFunc) (posPeak, negPeak, buffer, numSamples);
		else
			AudioVectorOpsGeneric::vectorGetMinMax (negPeak, posPeak, buffer, numSamples);
	}
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// AudioPeakBuffer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioPeakBuffer, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioPeakBuffer::AudioPeakBuffer ()
: framesPerPeak (kPeakRatioDefault),
  numSourceFrames (0),
  maxSourcePeak (-1),
  peakFormat (kPeakFormatDefault),
  numChannels (0),
  numPeaksValid (0),
  numScaledPeaksValid (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPeakBuffer::updateTimestamp (bool shiftAhead)
{
	System::GetSystem ().getLocalTime (createTime);	

	if(shiftAhead)
	{
		// Shift the timestamp a little bit into the future.
		// Some file systems (FAT) report a later modification time for new files.
		Time t;
		t.fromSeconds (createTime.getTime ().toSeconds () + 5);
		createTime.setTime (t);		
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPeakBuffer::resizeBuffers (uint32 maxChannels, uint32 maxPeakCapacity, PeakFormat newFormat)
{
	numChannels = ccl_min<uint32> (maxChannels, kMaxChannels);
	peakFormat = newFormat;

	uint32 bufferSize = maxPeakCapacity * getPeakSize ();
	for(uint32 ch = 0; ch < numChannels; ch++)
	{
		peaks[ch].resize (bufferSize);
		peaks[ch].zeroFill ();
		peakPeaks[ch].resize (0);
	}
	numScaledPeaksValid = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPeakBuffer::disposeBuffers ()
{
	numChannels = 0;
	numPeaksValid = 0;
	numScaledPeaksValid = 0;

	for(int i = 0; i < kMaxChannels; i++)
	{
		peaks[i].resize (0);
		peakPeaks[i].resize (0);
	}

	numSourceFrames = 0;
	maxSourcePeak = -1;
	createTime = DateTime ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioPeakBuffer::PeakFormat AudioPeakBuffer::getPeakFormat () const
{
	return peakFormat; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 AudioPeakBuffer::getPeakSize () const
{
	switch(peakFormat)
	{
	case PeakFormat::kInt16 : return sizeof(Peak16);
	case PeakFormat::kFloat : return sizeof(PeakF);
	default :
		ASSERT (peakFormat == PeakFormat::kInt8)
		return sizeof(Peak8);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 AudioPeakBuffer::getNumPeakChannels () const
{
	return numChannels;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 AudioPeakBuffer::getMaxPeakCapacity () const
{
	return peaks[0].getBufferSize () / getPeakSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 AudioPeakBuffer::getNumPeaksValid () const
{
	return numPeaksValid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPeakBuffer::setNumPeaksValid (uint32 newNumPeaks)
{
	int maxPeaks = getMaxPeakCapacity ();
	if(newNumPeaks > maxPeaks)
	{
		uint32 newBufferSize = uint32((newNumPeaks / kPeakAllocationSize + 1) * kPeakAllocationSize + .5);
		newBufferSize *= getPeakSize ();
		for(int ch = 0; ch < numChannels; ch++)
			peaks[ch].resize (newBufferSize);
	}
	
	numPeaksValid = newNumPeaks;

	TSamples minSampleFrameCount = numPeaksValid * framesPerPeak;
	TSamples maxSampleFrameCount = ((numPeaksValid + 1) * framesPerPeak) - 1;
	numSourceFrames = ccl_bound (numSourceFrames, minSampleFrameCount, maxSampleFrameCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPeakBuffer::appendPeaks (const PeakF* src[], uint32 count)
{
	uint32 oldNumPeaks = numPeaksValid;
	setNumPeaksValid (numPeaksValid + count);

	for(uint32 ch = 0; ch < numChannels; ch++)
	{
		Buffer& channelBuffer = peaks[ch];
		for(int i = 0; i < count; i++)
			Internal::setPeak (channelBuffer, peakFormat, oldNumPeaks + i, src[ch][i]);
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPeakBuffer::updatePeakPeaks ()
{
	if(numScaledPeaksValid == numPeaksValid)
		return; // already up to date, nothing changed since the last rebuild

	switch(peakFormat)
	{
	case PeakFormat::kInt8 :
		updatePeakPeaksT<Peak8> ();
		break;
	case PeakFormat::kInt16 :
		updatePeakPeaksT<Peak16> ();
		break;
	case PeakFormat::kFloat :
		updatePeakPeaksT<PeakF> ();
		break;
	}

	numScaledPeaksValid = numPeaksValid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TPeakType>
void AudioPeakBuffer::updatePeakPeaksT ()
{
	int validPeakCount = int(numPeaksValid); // scan only valid data, not the full allocated capacity
	int scaleDownPeakCount = (validPeakCount + kScaledDownFactor - 1) / kScaledDownFactor;
	int scaleDownBufferSize = scaleDownPeakCount * getPeakSize ();

	for(uint32 c = 0; c < numChannels; c++)
	{
		peakPeaks[c].resize (scaleDownBufferSize);

		auto p1 = peaks[c].as<TPeakType> ();
		auto p2 = peakPeaks[c].as<TPeakType> ();

		TPeakType p = {};

		for(int i = 0, i2 = 0, count = validPeakCount; i < count && i2 < scaleDownPeakCount; )
		{
			auto t = p1[i].p;
			if(t > p.p)
				p.p = t;
			t = p1[i].n;
			if(t > p.n)
				p.n = t;
			i++;
			if((i % kScaledDownFactor) == 0 || i == (count - 1))
			{
				p2[i2++] = p;
				p.n = 0;
				p.p = 0;
			}
		}
	}
}

//************************************************************************************************
// AudioPeakBuilder
//************************************************************************************************

AudioPeakBuilder::AudioPeakBuilder (AudioPeakBuffer& peakBuffer)
: peakBuffer (peakBuffer)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AudioPeakBuilder::build (IAudioSource& source, AudioPeakBuffer::PeakFormat format, uint32 framesPerPeak, IProgressNotify* progress)
{
	uint32 numChannels = ccl_min<uint32> (ccl_max<uint32> (source.getNumChannels (), 0), AudioPeakBuffer::kMaxChannels);
	ASSERT (numChannels > 0 && framesPerPeak > 0)
	if(numChannels < 1 || framesPerPeak == 0)
		return kResultInvalidArgument;

	TSamples numSourceFrames = source.getFrameCount ();
	if(numSourceFrames / framesPerPeak < AudioPeakBuffer::kMinPeaksToGenerate)
		framesPerPeak = ccl_max (1, int(numSourceFrames / AudioPeakBuffer::kMinPeaksToGenerate));

	int64 peakCount = int64(0.5f + (numSourceFrames + framesPerPeak - 1.f) / framesPerPeak);
	if(peakCount < 0) // audio sources with zero length should not fail!
		return kResultInvalidArgument;

	peakBuffer.resizeBuffers (numChannels, uint32(peakCount), format);
	peakBuffer.setFramesPerPeak (framesPerPeak);
	peakBuffer.setNumSourceFrames (numSourceFrames);
	peakBuffer.setNumPeaksValid (uint32(peakCount));

	maxPeak = -1.f;

	static constexpr int kFramesPerBufferFactor = 5000; ///< large-block read size, in multiples of framesPerPeak

	int framesPerBuffer = framesPerPeak * kFramesPerBufferFactor;
	AudioDataBuffer largeBuffer (framesPerBuffer, int(numChannels), kAudioFloat32);

	uint32 totalPeaks = uint32(peakCount);
	bool corrupt = false;
	bool canceled = false;

	TSamples oldPos = source.tell ();

	if(progress)
		progress->beginProgress ();

	source.seek (0);

	uint32 iPeak = 0;
	while(!corrupt && iPeak < totalPeaks)
	{
		int numBuffered = source.read (largeBuffer.getAudioData (), framesPerBuffer);
		if(numBuffered <= 0)
		{
			corrupt = numBuffered < 0; // corrupt only if a reading error occurs!
			break;
		}

		int bufferOffset = 0;
		while(bufferOffset < numBuffered && iPeak < totalPeaks)
		{
			int numRead = ccl_min<int> (framesPerPeak, numBuffered - bufferOffset);
			AudioDataBuffer block (largeBuffer, bufferOffset, numRead);

			tresult tr = processBlock (block.getAudioData (), numRead, iPeak);
			ASSERT (tr == kResultOk)

			bufferOffset += numRead;
			iPeak++;
		}

		if(progress)
		{
			double state = double(iPeak) / double(totalPeaks);
			progress->updateProgress (IProgressNotify::State (state));
			if(progress->isCanceled ()) // maybe canceled by user?
			{
				canceled = true;
				break;
			}
		}
	}

	if(progress)
		progress->endProgress ();

	source.seek (oldPos);

	if(corrupt || canceled)
		peakBuffer.setNumPeaksValid (0);
	else
	{
		peakBuffer.setMaxSourcePeak (maxPeak);
		peakBuffer.updateTimestamp ();
	}

	return corrupt ? kResultFailed : canceled ? kResultAborted : kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AudioPeakBuilder::processBlock (const AudioData& data, int sampleFrames, uint32 peakIndex)
{
	AudioPeakBuffer::PeakF peakF = {};
	for(uint32 ch = 0; ch < peakBuffer.getNumPeakChannels (); ch++)
	{
		float32 posPeak = 0.f, negPeak = 0.f;
		AudioPeakDetection::detect (posPeak, negPeak, data.buffers32[ch], sampleFrames);

		if(posPeak > maxPeak)
			maxPeak = posPeak;
		if(-negPeak > maxPeak)
			maxPeak = -negPeak;

		peakF.p = ccl_bound<float32> (fabsf (posPeak), 0.f, AudioPeakBuffer::Internal::kMaxPeakF);
		peakF.n = ccl_bound<float32> (fabsf (negPeak), 0.f, AudioPeakBuffer::Internal::kMaxPeakF);
		peakBuffer.setPeak (ch, peakIndex, peakF);
	}
	return kResultOk;
}

//************************************************************************************************
// AudioPeakSource
//************************************************************************************************

int AudioPeakSource::getCombinedPeaks (const PeakRequest& request) const
{
	uint32 numChannels = getNumPeakChannels ();
	if(numChannels == 1)
		return getPeaks (request);

	if(request.isAccumulate () == false)
		::memset (request.minmax, 0, sizeof(float) * request.numPixels * 2);

	int result = 0;
	for(uint32 ch = 0; ch < numChannels; ch++)
	{			
		PeakRequest request2 = request;
		request2.channel = ch;
		request2.flags |= kAccumulate;
		result = getPeaks (request2);
	}
	return result;
}

//************************************************************************************************
// AudioPeakBufferReader
//************************************************************************************************

AudioPeakBufferReader::AudioPeakBufferReader (AudioPeakBuffer& peakBuffer, bool allowScaledDown)
: peakBuffer (peakBuffer),
  allowScaledDown (allowScaledDown)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 AudioPeakBufferReader::getNumPeakChannels () const
{
	return peakBuffer.getNumPeakChannels ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AudioPeakBufferReader::getPeaks (const PeakRequest& request) const
{
	ASSERT (request.channel < peakBuffer.getNumPeakChannels ())

	uint32 total = peakBuffer.getNumPeaksValid ();
	double peaksPerPixel = request.framesPerPixel / double(peakBuffer.getFramesPerPeak ());
	float* minmax = request.minmax;

	// Look for scaled down version
	const Buffer* sourceBuffer = nullptr;
	if(allowScaledDown && peaksPerPixel > AudioPeakBuffer::kScaledDownFactor)
	{
		peakBuffer.updatePeakPeaks (); // no-op if already current
		if(peakBuffer.peakPeaks[request.channel].getSize () > 0)
		{
			sourceBuffer = &peakBuffer.peakPeaks[request.channel];
			peaksPerPixel /= AudioPeakBuffer::kScaledDownFactor;
			total = peakBuffer.peakPeaks[request.channel].getSize () / peakBuffer.getPeakSize ();
		}
	}

	if(sourceBuffer == nullptr)
		sourceBuffer = &peakBuffer.peaks[request.channel];

	int count = 0;
	for(int i = 0; i < request.numPixels; i++)
	{
		double pixel = double(request.startPixel) + i;
		uint32 start = uint32(ccl_max (ccl_to_int64 (pixel * peaksPerPixel), int64(0)));
		if(start >= total)
			break;

		#if AVOID_REPEATS
		uint32 end = uint32(ccl_max (ccl_to_int64 ((pixel + 1.) * peaksPerPixel) - 1, int64(0)));
		if(end < start)
			end = start;
		#else
		uint32 end = uint32(ccl_max (ccl_to_int64 ((pixel + 1.) * peaksPerPixel), int64(0)));
		#endif
		if(end >= total)
			end = total - 1;

		AudioPeakBuffer::PeakF pixelMax = {};
		for(uint32 j = start; j <= end; j++)
		{
			AudioPeakBuffer::PeakF peakF = {};
			AudioPeakBuffer::Internal::getPeak (peakF, *sourceBuffer, peakBuffer.getPeakFormat (), j);

			if(peakF.p > pixelMax.p)
				pixelMax.p = peakF.p;
			if(peakF.n > pixelMax.n)
				pixelMax.n = peakF.n;
		}

		if(request.isAccumulate ())
		{
			*minmax++ = ccl_max (pixelMax.p, *minmax);
			*minmax++ = ccl_max (pixelMax.n, *minmax);
		}
		else
		{
			*minmax++ = pixelMax.p;
			*minmax++ = pixelMax.n;
		}
		count++;
	}
	return count;
}

//************************************************************************************************
// AudioSourcePeakReader
//************************************************************************************************

AudioSourcePeakReader::AudioSourcePeakReader (IAudioSource& source)
: source (source)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 AudioSourcePeakReader::getNumPeakChannels () const
{
	return source.getNumChannels ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AudioSourcePeakReader::getPeaks (const PeakRequest& request) const
{
	uint32 numChannels = ccl_min<uint32> (source.getNumChannels (), AudioPeakBuffer::kMaxChannels);
	if(request.channel >= numChannels || request.framesPerPixel <= 0)
		return 0;

	static constexpr int kReadBufferSize = 10000;
	AudioScratchDataBuffer::ScopedBuffer scratchBuffer (kReadBufferSize, int(numChannels), kAudioFloat32);
	if(!scratchBuffer)
		return 0;

	AudioData& scratchData = *scratchBuffer;

	float* minmax = request.minmax;
	int i = 0;
	while(i < request.numPixels)
	{
		double k = double(request.startPixel + i) * request.framesPerPixel;
		TSamples samplePosition = TSamples(k);
		k -= samplePosition;
		k /= request.framesPerPixel;

		source.seek (samplePosition);
		int numRead = source.read (scratchData, kReadBufferSize);
		if(numRead <= 0)
			break;

		float32* channelBuffer = scratchData.buffers32[request.channel];
		if(numRead < kReadBufferSize)
			AudioVectorOpsGeneric::vectorClear (channelBuffer + numRead, kReadBufferSize - numRead);

		while(i < request.numPixels)
		{
			int start = ccl_to_int (k * request.framesPerPixel);
			k += 1.;
			#if AVOID_REPEATS
			int end = ccl_to_int (k * request.framesPerPixel) - 1;
			if(end < start)
				end = start;
			#else
			int end = ccl_to_int (k * request.framesPerPixel);
			#endif

			if(end >= kReadBufferSize)
				break; // need to refill the read buffer for the next pixel

			float32 posPeak = 0.f, negPeak = 0.f;
			AudioPeakDetection::detect (posPeak, negPeak, channelBuffer + start, (end - start) + 1);

			if(AudioSampleOpsGeneric::isValid (posPeak) && AudioSampleOpsGeneric::isValid (negPeak))
			{
				posPeak = ccl_bound<float32> (fabsf (posPeak), 0.f, AudioPeakBuffer::Internal::kMaxPeakF);
				negPeak = ccl_bound<float32> (fabsf (negPeak), 0.f, AudioPeakBuffer::Internal::kMaxPeakF);
			}
			else
				posPeak = negPeak = 1.f;

			if(request.isAccumulate ())
			{
				*minmax++ = ccl_max (posPeak, *minmax);
				*minmax++ = ccl_max (negPeak, *minmax);
			}
			else
			{
				*minmax++ = posPeak;
				*minmax++ = negPeak;
			}
			i++;
		}
	}
	return i;
}
