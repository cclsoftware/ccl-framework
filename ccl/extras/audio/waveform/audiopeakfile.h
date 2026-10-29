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
// Filename    : ccl/extras/audio/waveform/audiopeakfile.h
// Description : Audio Peak File
//
//************************************************************************************************

#ifndef _ccl_audiopeakfile_h
#define _ccl_audiopeakfile_h

#include "ccl/extras/audio/waveform/audiopeakbuffer.h"

#include "ccl/public/storage/filetype.h"

namespace CCL {

interface IStream;

//************************************************************************************************
// AudioPeakFile
/**	Reads and writes an AudioPeakBuffer to/from RIFF-based .peak file format
	(BWF peak envelope). */
//************************************************************************************************

class AudioPeakFile: public Object
{
public:
	DECLARE_CLASS (AudioPeakFile, Object)

	AudioPeakFile (AudioPeakBuffer* peakBuffer = nullptr);

	static const FileType& getFileType ();

	PROPERTY_SHARED_AUTO (AudioPeakBuffer, peakBuffer, PeakBuffer)

	bool write (UrlRef path) const;
	bool read (UrlRef path);

	bool write (IStream& stream) const;
	bool read (IStream& stream);

protected:
	/** Write just the peak envelope chunk (no RIFF header or extra chunks). */
	virtual bool writeChunk (IStream& stream) const;

	/** Read just the peak envelope chunk into the peak buffer. */
	virtual bool readChunk (IStream& stream);
};

} // namespace CCL

#endif // _ccl_audiopeakfile_h