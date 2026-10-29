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
// Filename    : ccl/extras/audio/waveform/audiopeakfile.cpp
// Description : Audio Peak File
//
//************************************************************************************************

#include "ccl/extras/audio/waveform/audiopeakfile.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/base/streamer.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/cclversion.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Peak Envelope Format according to BWF Specification (see http://www.ebu.ch)
//////////////////////////////////////////////////////////////////////////////////////////////////

#define PEAKFILE_BYTEORDER kLittleEndian // Peak files are always stored in little endian

static DEFINE_FOURCC (kRiffID, 'R', 'I', 'F', 'F')
static DEFINE_FOURCC (kPeakFormatID, 'P', 'E', 'A', 'K')
static DEFINE_FOURCC (kPeakEnvelopeChunkID, 'l', 'e', 'v', 'l')

enum PeakFileConstants
{
	kPeakChunkVersion = 0x0000,
	kPeakFormatUInt8  = 1,
	kPeakFormatUInt16 = 2,
	kPeakFormatFloat  = 3
};

struct PeakEnvelopeChunk
{
	/////////////////////////////
	// First 120 Bytes:
	uint32 version;					///< version information
	uint32 format;					///< format of a peak point (1 = unsigned char, 2 = unsigned short, 3 = float)
	uint32 pointsPerValue;			///< 1 = only positive peak point, 2 = positive AND negative peak point
	uint32 blockSize;				///< frames per value
	uint32 channels;				///< number of channels
	uint32 numPeakFrames;			///< number of peak frames
	uint32 posPeakOfPeaks;			///< audio sample frame index or 0xFFFFFFFF if unknown
	uint32 offsetToPeaks;			///< should usually be equal to the size of this header, but could also be higher
	char timestamp[28];				///< ASCII: time stamp of the peak data
	char reserved[60];				///< reserved set to 0x00
	/////////////////////////////

	static constexpr uint32 kSize = 120;

	PeakEnvelopeChunk ()
	{ memset (this, 0, kSize); }

	bool write (Streamer& s) const
	{
		if(!s.write (version))
			return false;
		s.write (format);
		s.write (pointsPerValue);
		s.write (blockSize);
		s.write (channels);
		s.write (numPeakFrames);
		s.write (posPeakOfPeaks);
		s.write (offsetToPeaks);
		s.write (timestamp, 28);
		return s.write (reserved, 60) == 60;
	};

	bool read (Streamer& s)
	{
		int64 oldPos = s->tell ();
		
		if(!s.read (version))
			return false;
		s.read (format);
		s.read (pointsPerValue);
		s.read (blockSize);
		s.read (channels);
		s.read (numPeakFrames);
		s.read (posPeakOfPeaks);
		s.read (offsetToPeaks);
		if(!s.read (timestamp, 28))
			return false;

		int bytesToRead = (int)(offsetToPeaks - (s->tell () - oldPos));
		while(bytesToRead-- > 0)
		{
			char c;
			if(!s.read (c))
				return false;
		}
		return true;
	}
};

static CStringPtr strPeakTimeFormat = "%04d:%02d:%02d:%02d:%02d:%02d:%03d"; ///< "YYYY:MM:DD:hh:mm:ss:uuu"

//************************************************************************************************
// PeakStreamer
//************************************************************************************************

struct PeakStreamer
{
	template<class TPeak> 
	static bool writePeaks (Streamer& s, const AudioPeakBuffer& peakBuffer)
	{
		for(uint32 iPeak = 0; iPeak < peakBuffer.getNumPeaksValid (); iPeak++)
			for(uint32 ch = 0; ch < peakBuffer.getNumPeakChannels (); ch++)
			{
				const TPeak& peak = peakBuffer.getPeakBuffer<TPeak> (ch)[iPeak];
				if(!s.write (peak.p))
					return false;
				if(!s.write (peak.n))
					return false;			
			}	
		return true;
	}

	template<class TPeak> 
	static bool readPeaks (AudioPeakBuffer& peakBuffer, Streamer& s, uint32 numPeaks)
	{
		for(int iPeak = 0; iPeak < numPeaks; iPeak++)
			for(int ch = 0; ch < peakBuffer.getNumPeakChannels (); ch++)
			{
				TPeak& peak = peakBuffer.getPeakBuffer<TPeak> (ch)[iPeak];
				if(!s.read (peak.p))
					return false;
				if(!s.read (peak.n))
					return false;
			}				
		return true;
	}
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// AudioPeakFile
//************************************************************************************************

const FileType& AudioPeakFile::getFileType ()
{
	static FileType peakFileType ("Peak File", "peak", CCL_MIME_TYPE "-audiopeak");
	return peakFileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (AudioPeakFile, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioPeakFile::AudioPeakFile (AudioPeakBuffer* peakBuffer)
{
	setPeakBuffer (peakBuffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPeakFile::write (UrlRef path) const
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kCreateMode);
	if(stream)
		return write (*stream);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPeakFile::write (IStream& stream) const
{
	ASSERT (peakBuffer != nullptr)
	if(!peakBuffer)
		return false;

	// Note: We must not seek when writing to a transformed stream.
	uint32 pointCount = peakBuffer->getNumPeaksValid () * peakBuffer->getNumPeakChannels ();
	uint32 peakChunkSize = PeakEnvelopeChunk::kSize + pointCount * peakBuffer->getPeakSize () * 2;
	uint32 rootChunkSize = 12/* 3 x 4: RIFF + size + PEAK */ + peakChunkSize;

	// *** RIFF Header ***
	Streamer s (stream, PEAKFILE_BYTEORDER);
	if(!s.write (kRiffID))
		return false;

	if(!s.write (rootChunkSize))
		return false;
	if(!s.write (kPeakFormatID))
		return false;

	// *** Peak Chunk ***
	if(!s.write (kPeakEnvelopeChunkID))
		return false;

	if(!s.write (peakChunkSize))
		return false;
	if(!writeChunk (stream))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPeakFile::writeChunk (IStream& _stream) const
{
	ASSERT (peakBuffer != nullptr)
	if(!peakBuffer)
		return false;

	AutoPtr<IStream> bufferedStream = System::GetFileUtilities ().createBufferedStream (_stream);
	ASSERT (bufferedStream != nullptr)
	if(!bufferedStream)
		return false;

	Streamer s (*bufferedStream, PEAKFILE_BYTEORDER);

	// *** Peak Chunk ***
	PeakEnvelopeChunk chunk;
	chunk.version = kPeakChunkVersion;
	
	AudioPeakBuffer::PeakFormat peakFormat = peakBuffer->getPeakFormat ();
	switch(peakFormat)
	{
	case AudioPeakBuffer::PeakFormat::kInt8 : chunk.format = kPeakFormatUInt8; break;
	case AudioPeakBuffer::PeakFormat::kInt16 : chunk.format = kPeakFormatUInt16; break;
	case AudioPeakBuffer::PeakFormat::kFloat : chunk.format = kPeakFormatFloat; break;
	default: ASSERT (0)	return false;
	}
		
	chunk.pointsPerValue = 2; // positive AND negative peaks
	chunk.blockSize = peakBuffer->getFramesPerPeak ();
	chunk.channels = peakBuffer->getNumPeakChannels ();
	chunk.numPeakFrames = peakBuffer->getNumPeaksValid ();
	chunk.posPeakOfPeaks = 0xFFFFFFFF; // unknown
	chunk.offsetToPeaks = PeakEnvelopeChunk::kSize;

	const DateTime& time = peakBuffer->getCreateTime ();
	snprintf (chunk.timestamp, sizeof(chunk.timestamp), strPeakTimeFormat, 
			 time.getDate ().getYear (), time.getDate ().getMonth (), time.getDate ().getDay (),
			 time.getTime ().getHour (), time.getTime ().getMinute (), time.getTime ().getSecond (), 
			 time.getTime ().getMilliseconds ());

	if(!chunk.write (s))
		return false;

	// *** Peak Points ***
	switch(peakFormat)
	{
	case AudioPeakBuffer::PeakFormat::kInt8 : 
		if(PeakStreamer::writePeaks<AudioPeakBuffer::Peak8> (s, *peakBuffer) == false)
			return false;
		break;

	case AudioPeakBuffer::PeakFormat::kInt16 :
		if(PeakStreamer::writePeaks<AudioPeakBuffer::Peak16> (s, *peakBuffer) == false)
			return false;
		break;		
			
	case AudioPeakBuffer::PeakFormat::kFloat :
		if(PeakStreamer::writePeaks<AudioPeakBuffer::PeakF> (s, *peakBuffer) == false)
			return false;
		break;					
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPeakFile::read (UrlRef path)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kOpenMode);
	if(stream)
		return read (*stream);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPeakFile::read (IStream& stream)
{
	ASSERT (peakBuffer != nullptr)
	if(!peakBuffer)
		return false;

	Streamer s (stream, PEAKFILE_BYTEORDER);
	FOURCC chunkID = {};
	uint32 size = 0;

	// *** RIFF Header ***
	if(!(s.read (chunkID) && chunkID == kRiffID))
		return false;
	if(!s.read (size)) // root chunk size
		return false;
	if(!(s.read (chunkID) && chunkID == kPeakFormatID))
		return false;

	// *** Peak Chunk ***
	if(!(s.read (chunkID) && chunkID == kPeakEnvelopeChunkID))
		return false;
	if(!s.read (size)) // peak chunk size
		return false;

	// ensure that stream is limited to peak chunk
	AutoPtr<IStream> peakStream = System::GetFileUtilities ().createSectionStream (stream, stream.tell (), size);
	if(!readChunk (*peakStream))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPeakFile::readChunk (IStream& _stream)
{
	ASSERT (peakBuffer != nullptr)
	if(!peakBuffer)
		return false;

	AutoPtr<IStream> bufferedStream = System::GetFileUtilities ().createBufferedStream (_stream);
	ASSERT (bufferedStream != nullptr)
	if(!bufferedStream)
		return false;

	Streamer s (*bufferedStream, PEAKFILE_BYTEORDER);

	// *** Peak Chunk ***
	PeakEnvelopeChunk chunk;
	if(!chunk.read (s))
		return false;

	if(!(chunk.version == kPeakChunkVersion &&		 
		 chunk.pointsPerValue == 2 &&
		 chunk.blockSize == AudioPeakBuffer::kPeakRatioDefault &&
		 chunk.channels <= AudioPeakBuffer::kMaxChannels))
		 return false;

	int y = 0, m = 0, d = 0, h = 0, min = 0, sec = 0, ms = 0;
	if(sscanf (chunk.timestamp, strPeakTimeFormat, &y, &m, &d, &h, &min, &sec, &ms) != 7)
		return false;

	AudioPeakBuffer::PeakFormat peakFormat = AudioPeakBuffer::kPeakFormatDefault;
	if(chunk.format == kPeakFormatUInt8)
		peakFormat = AudioPeakBuffer::PeakFormat::kInt8;
	else if(chunk.format == kPeakFormatUInt16)
		peakFormat = AudioPeakBuffer::PeakFormat::kInt16;
	else if(chunk.format == kPeakFormatFloat)
		peakFormat = AudioPeakBuffer::PeakFormat::kFloat;
	else
		return false;

	peakBuffer->disposeBuffers ();
	peakBuffer->setCreateTime (DateTime (Date (y, m, d), Time (h, min, sec, ms)));
	peakBuffer->resizeBuffers (chunk.channels, chunk.numPeakFrames, peakFormat);
	peakBuffer->setFramesPerPeak (chunk.blockSize);

	// *** Peak Points ***
	switch(peakFormat)
	{
	case AudioPeakBuffer::PeakFormat::kInt8: 
		if(PeakStreamer::readPeaks<AudioPeakBuffer::Peak8> (*peakBuffer, s, chunk.numPeakFrames) == false)
			return false;
		break;

	case AudioPeakBuffer::PeakFormat::kInt16:
		if(PeakStreamer::readPeaks<AudioPeakBuffer::Peak16> (*peakBuffer, s, chunk.numPeakFrames) == false)
			return false;
		break;		
			
	case AudioPeakBuffer::PeakFormat::kFloat:
		if(PeakStreamer::readPeaks<AudioPeakBuffer::PeakF> (*peakBuffer, s, chunk.numPeakFrames) == false)
			return false;
		break;					
	}

	peakBuffer->setNumPeaksValid (chunk.numPeakFrames);
	return true;
}
