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
// Filename    : ccl/public/text/textencoding.h
// Description : Text Encodings
//
//************************************************************************************************

#ifndef _ccl_textencoding_h
#define _ccl_textencoding_h

#include "ccl/public/base/platform.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Text Encodings and Line Endings
//////////////////////////////////////////////////////////////////////////////////////////////////

/**	\addtogroup ccl_text */
namespace Text 
{
	/** Text Encodings. */
	DEFINE_ENUM (Encoding)
	{
		kUnknownEncoding = 0,		///< Unknown Encoding
		
		kASCII = 'ASCI',			///< US-ASCII
		kISOLatin1 = 'Lat1',		///< ISO 8859-1 Latin I
		kWindowsLatin1 = 'ANSI',	///< ANSI Codepage 1252
		kDOSLatinUS = 'C437',		///< IBM PC/MS-DOS Codepage 437
		kMacRoman = 'MacR',			///< MAC - Roman
		kShiftJIS = 'SJIS',			///< Japanese Codepage 932
		kUTF8 = 'UTF8',				///< UTF-8
		kUTF16LE = 'U16L',			///< UTF-16 Little Endian
		kUTF16BE = 'U16B',			///< UTF-16 Big Endian

		#if CCL_NATIVE_BYTEORDER == CCL_LITTLE_ENDIAN
		kUTF16 = kUTF16LE,			///< UTF-16 (Native Byte-order)
		#else
		kUTF16 = kUTF16BE,			///< UTF-16 (Native Byte-order)
		#endif

		kSystemEncoding = 'Syst'	///< Current System Encoding
	};

	/** Line Endings. */
	DEFINE_ENUM (LineFormat)
	{
		kUnknownLineFormat = 0,		///< Unknown style (e.g. auto-detect when reading)

		kCRLFLineFormat,			///< CR LF (Windows)
		kCRLineFormat,				///< CR (Classic Mac OS)
		kLFLineFormat,				///< LF (Unix, macOS)

		#if CCL_PLATFORM_WINDOWS
		kSystemLineFormat = kCRLFLineFormat
		#else
		kSystemLineFormat = kLFLineFormat
		#endif
	};

	/** Verify C-String Encoding. */
	inline bool isValidCStringEncoding (Encoding encoding)
	{
		return encoding == kASCII || encoding == kISOLatin1 || encoding == kWindowsLatin1 || 
			   encoding == kDOSLatinUS || encoding == kMacRoman || encoding == kShiftJIS ||
			   encoding == kUTF8 || encoding == kSystemEncoding;
	}
	
	/** Verify Unicode (UTF-16) Encoding. */
	inline bool isUTF16Encoding (Encoding encoding)
	{
		return encoding == kUTF16LE || encoding == kUTF16BE;
	}

	/** Verify Text Encoding. */
	inline bool isValidEncoding (Encoding encoding)
	{
		return isValidCStringEncoding (encoding) || isUTF16Encoding (encoding);
	}

	/** Get maximum number of bytes per character for given encoding. */
	inline int getMaxEncodingBytesPerCharacter (Encoding encoding)
	{
		if(encoding == kUTF8 || isUTF16Encoding (encoding))
			return 4;
		else if(encoding == kShiftJIS)
			return 2;

		return 1;
	}
}

/** Text Encoding Type. */
typedef Text::Encoding TextEncoding;

/** Text Line Format Type. */
typedef Text::LineFormat TextLineFormat;

} // namespace CCL

#endif // _ccl_textencoding_h
