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
// Filename    : ccl/public/text/itextstreamer.h
// Description : Text Streamer Interface
//
//************************************************************************************************

#ifndef _ccl_itextstreamer_h
#define _ccl_itextstreamer_h

#include "ccl/public/text/textencoding.h"

namespace CCL {

//************************************************************************************************
// ITextStreamer
/**	\ingroup ccl_text */
//************************************************************************************************

interface ITextStreamer: IUnknown
{
	/** Text streamer options. */
	enum Options
	{
		kSuppressByteOrderMark = 1<<0,	///< do not write a byte order mark
		kFlushLineEnd = 1<<1			///< flush internal buffers when writing an end of line
	};

	/** Returns current text encoding. */
	virtual TextEncoding CCL_API getTextEncoding () const = 0;

	/** Returns current line format. */
	virtual TextLineFormat CCL_API getLineFormat () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Text reading methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Check if end of stream is reached. */
	virtual tbool CCL_API isEndOfStream () const = 0;

	/** Read a single UTF-16 character. */
	virtual tbool CCL_API readChar (uchar& c) = 0;

	/** Read a whole line. */
	virtual tbool CCL_API readLine (String& string) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Text writing methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Write a single UTF-16 character. */
	virtual tbool CCL_API writeChar (uchar c) = 0;

	/** Write a whole UTF-16 string with optional newline sequence. */
	virtual tbool CCL_API writeString (StringRef string, tbool appendNewline = false) = 0;

	/** Write a newline sequence, depending on line format. */
	virtual tbool CCL_API writeNewline () = 0;

	DECLARE_IID (ITextStreamer)

	//////////////////////////////////////////////////////////////////////////////////////////////

	tbool writeLine (StringRef string) { return writeString (string, true); }
};

DEFINE_IID (ITextStreamer, 0x30b1ba60, 0x8ab, 0x47d8, 0xab, 0x93, 0x88, 0x85, 0x3f, 0x35, 0x8f, 0x90)

} // namespace CCL

#endif // _ccl_itextstreamer_h
