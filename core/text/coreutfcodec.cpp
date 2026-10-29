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
// Filename    : core/text/coretextcodec.cpp
// Description : Text Format Conversion
//
//************************************************************************************************

#include "coreutfcodec.h"

namespace Core { 
namespace Text {
namespace UTFCodec {

//////////////////////////////////////////////////////////////////////////////////////////////////

inline uchar32 get16Bit (const unsigned char* sourceBuffer, int byteOrder)
{
	if(byteOrder == CORE_BIG_ENDIAN)
		return (sourceBuffer[0] << 8) + sourceBuffer[1];
	else
		return (sourceBuffer[1] << 8) + sourceBuffer[0];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void write16Bit (uint16 w, unsigned char* destBuffer, int byteOrder)
{
	if(byteOrder == CORE_BIG_ENDIAN)
	{
		destBuffer[0] = (unsigned char)(w >> 8);
		destBuffer[1] = (unsigned char) w;
	}
	else
	{
		destBuffer[0] = (unsigned char) w;
		destBuffer[1] = (unsigned char)(w >> 8);
	}
}

} // namespace UTFCodec
} // namespace Text
} // namespace Core

using namespace Core;
using namespace Text;

//************************************************************************************************
// UTFCodec
//************************************************************************************************

int UTFCodec::decodeUTF8 (uchar32& c, const unsigned char* sourceBuffer, int sourceSize)
{
	if(sourceSize <= 0)
		return 0;

	// algorithm: see RFC 2279
	unsigned char s0 = sourceBuffer[0];

	if(s0 < 0x80)
	{
		c = s0;
		return 1;
	}
	else if(s0 < 0xc2)
	{
		return kIllegalInput;
	}
	else if(s0 < 0xe0)
	{
		if(sourceSize < 2)
			return kBufferTooSmall;
		if(!((sourceBuffer[1] ^ 0x80) < 0x40))
			return kIllegalInput;
		c = ((uchar32) (s0 & 0x1f) << 6) | (uchar32) (sourceBuffer[1] ^ 0x80);
		return 2;
	}
	else if(s0 < 0xf0)
	{
		if(sourceSize < 3)
			return kBufferTooSmall;
		if(! ((sourceBuffer[1] ^ 0x80) < 0x40
			&& (sourceBuffer[2] ^ 0x80) < 0x40
			&& (s0 >= 0xe1 || sourceBuffer[1] >= 0xa0)))
			return kIllegalInput;
		c =	(  (uchar32) (s0 & 0x0f) << 12)
			| ((uchar32) (sourceBuffer[1] ^ 0x80) << 6)
			|  (uchar32) (sourceBuffer[2] ^ 0x80);
		return 3;
	}
	else if(s0 < 0xf8)
	{
		if(sourceSize < 4)
			return kBufferTooSmall;
		if(!  ((sourceBuffer[1] ^ 0x80) < 0x40
			&& (sourceBuffer[2] ^ 0x80) < 0x40
			&& (sourceBuffer[3] ^ 0x80) < 0x40
			&& (s0 >= 0xf1 || sourceBuffer[1] >= 0x90)))
			return kIllegalInput;

		c = ((uchar32) (s0 & 0x07) << 18)
			| ((uchar32) (sourceBuffer[1] ^ 0x80) << 12)
			| ((uchar32) (sourceBuffer[2] ^ 0x80) << 6)
			|  (uchar32) (sourceBuffer[3] ^ 0x80);
		return 4;
	}
	else if(s0 < 0xfc)
	{
		if(sourceSize < 5)
			return kBufferTooSmall;
		if(!  ((sourceBuffer[1] ^ 0x80) < 0x40
			&& (sourceBuffer[2] ^ 0x80) < 0x40
			&& (sourceBuffer[3] ^ 0x80) < 0x40
			&& (sourceBuffer[4] ^ 0x80) < 0x40
			&& (s0 >= 0xf9 || sourceBuffer[1] >= 0x88)))
				return kIllegalInput;
		c = (  (uchar32) (s0 & 0x03) << 24)
			| ((uchar32) (sourceBuffer[1] ^ 0x80) << 18)
			| ((uchar32) (sourceBuffer[2] ^ 0x80) << 12)
			| ((uchar32) (sourceBuffer[3] ^ 0x80) << 6)
			|  (uchar32) (sourceBuffer[4] ^ 0x80);
		return 5;
	}
	else if(s0 < 0xfe)
	{
		if(sourceSize < 6)
			return kBufferTooSmall;
		if(!  ((sourceBuffer[1] ^ 0x80) < 0x40
			&& (sourceBuffer[2] ^ 0x80) < 0x40
			&& (sourceBuffer[3] ^ 0x80) < 0x40
			&& (sourceBuffer[4] ^ 0x80) < 0x40
			&& (sourceBuffer[5] ^ 0x80) < 0x40
			&& (s0 >= 0xfd || sourceBuffer[1] >= 0x84)))
			return kIllegalInput;
		c =   ((uchar32) (s0 & 0x01) << 30)
			| ((uchar32) (sourceBuffer[1] ^ 0x80) << 24)
			| ((uchar32) (sourceBuffer[2] ^ 0x80) << 18)
			| ((uchar32) (sourceBuffer[3] ^ 0x80) << 12)
			| ((uchar32) (sourceBuffer[4] ^ 0x80) << 6)
			|  (uchar32) (sourceBuffer[5] ^ 0x80);
		return 6;
	}
	return kIllegalInput;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int UTFCodec::encodeUTF8 (uchar32 c, unsigned char* destBuffer, int destSize)
{
	// algorithm: see RFC 2279
	int bytesNeeded;

	if(c < 0x80)
		bytesNeeded = 1;
	else if(c < 0x800)
		bytesNeeded = 2;
	else if(c < 0x10000)
		bytesNeeded = 3;
	else if(c < 0x200000)
		bytesNeeded = 4;
	else if(c < 0x4000000)
		bytesNeeded = 5;
	else if(c <= 0x7fffffff)
		bytesNeeded = 6;
	else
		return kIllegalInput;

	if(destSize < bytesNeeded)
		return kBufferTooSmall;

	switch(bytesNeeded)
	{
	// fall through!
	case 6 : destBuffer[5] = 0x80 | (c & 0x3f); c = c >> 6; c |= 0x4000000; CORE_FALLTHROUGH
	case 5 : destBuffer[4] = 0x80 | (c & 0x3f); c = c >> 6; c |= 0x200000; CORE_FALLTHROUGH
	case 4 : destBuffer[3] = 0x80 | (c & 0x3f); c = c >> 6; c |= 0x10000; CORE_FALLTHROUGH
	case 3 : destBuffer[2] = 0x80 | (c & 0x3f); c = c >> 6; c |= 0x800; CORE_FALLTHROUGH
	case 2 : destBuffer[1] = 0x80 | (c & 0x3f); c = c >> 6; c |= 0xc0; CORE_FALLTHROUGH
	case 1 : destBuffer[0] = (unsigned char)c;
	}
	return bytesNeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UTFCodec::isHighSurrogateUTF16 (uchar c)
{
	return c >= 0xD800 && c <= 0xDBFF;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UTFCodec::isLowSurrogateUTF16 (uchar c)
{
	return c >= 0xDC00 && c <= 0xDFFF;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uchar32 UTFCodec::makeSurrogatePairUTF16 (uchar high, uchar low)
{
	return 0x10000 + ((high - 0xd800) << 10) + (low - 0xdc00);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int UTFCodec::decodeUTF16 (uchar32& c, const unsigned char* sourceBuffer, int sourceSize, int byteOrder)
{
	// algorithm: see RFC 2781
	if(sourceSize >= 2)
	{
		uchar32 w1 = get16Bit (sourceBuffer, byteOrder);
		if(w1 < 0xD800 || w1 > 0xDFFF)
		{
			c = w1;
			return 2;
		}
		else if(w1 >= 0xD800 && w1 <= 0xDBFF)
		{
			// surrogate pair
			if(sourceSize >= 4)
			{
				uchar32 w2 = get16Bit (sourceBuffer + 2, byteOrder);
				if(w2 >= 0xDC00 && w2 <= 0xDFFF)
				{
					c = makeSurrogatePairUTF16 (static_cast<uchar> (w1), static_cast<uchar> (w2));
					return 4;
				}
				else
					return kIllegalInput;
			}
			else
				return kBufferTooSmall; // second word may follow in next call
		}
		else
			return kIllegalInput;
	}
	return kBufferTooSmall;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int UTFCodec::encodeUTF16 (uchar32 c, unsigned char* destBuffer, int destSize, int byteOrder)
{
	// algorithm: see RFC 2781
	if(!(c >= 0xd800 && c < 0xe000))
	{
		if(c < 0x10000)
		{
			if(destSize >= 2)
			{
				write16Bit ((uint16)c, destBuffer, byteOrder);
				return 2;
			}
			else
				return kBufferTooSmall;
		}
		else if(c < 0x110000)
		{
			if(destSize >= 4)
			{
				write16Bit ((uint16)(0xd800 + ((c - 0x10000) >> 10)),  destBuffer,     byteOrder);
				write16Bit ((uint16)(0xdc00 + ((c - 0x10000) & 0x3ff)), destBuffer + 2, byteOrder);
				return 4;
			}
			else
				return kBufferTooSmall;
		}
	}
	return kIllegalInput;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UTFCodec::GraphemeBreakClass UTFCodec::getGraphemeBreakClass (uchar32 c)
{
	// CR
	if(c == 0x000D)
		return kGraphemeBreak_CR;

	// LF
	if(c == 0x000A)
		return kGraphemeBreak_LF;

	// ZWJ (Zero Width Joiner) - must check BEFORE Control since 0x200D is in Control range
	if(c == 0x200D)
		return kGraphemeBreak_ZWJ;

	// Control
	if(c <= 0x001F || (c >= 0x007F && c <= 0x009F) || c == 0x00AD ||
	   c == 0x061C || c == 0x180E || (c >= 0x200B && c <= 0x200F) ||
	   (c >= 0x202A && c <= 0x202E) || (c >= 0x2060 && c <= 0x2064) ||
	   (c >= 0x2066 && c <= 0x206F) || c == 0xFEFF || (c >= 0xFFF0 && c <= 0xFFF8) ||
	   (c >= 0x1BCA0 && c <= 0x1BCA3) || (c >= 0x1D173 && c <= 0x1D17A) ||
	   c == 0xE0000 || (c >= 0xE0001 && c <= 0xE001F) || (c >= 0xE0080 && c <= 0xE00FF) ||
	   (c >= 0xE01F0 && c <= 0xE0FFF))
		return kGraphemeBreak_Control;

	// Regional Indicator
	if(isRegionalIndicator (c))
		return kGraphemeBreak_RegionalIndicator;

	// Prepend
	if((c >= 0x0600 && c <= 0x0605) || c == 0x06DD || c == 0x070F || c == 0x0890 ||
	   c == 0x0891 || c == 0x08E2 || (c >= 0x0D4E && c <= 0x0D4F) ||
	   (c >= 0x110BD && c <= 0x110CD) || (c >= 0x111C2 && c <= 0x111C3))
		return kGraphemeBreak_Prepend;

	// Extend (combining marks, emoji modifiers, variation selectors, etc.)
	// NOTE: Must check BEFORE Extended_Pictographic since emoji modifiers fall in emoji range
	if(isCombiningMark (c) || isEmojiModifier (c) || isVariationSelector (c))
		return kGraphemeBreak_Extend;

	// Extended_Pictographic
	if(isExtendedPictographic (c))
		return kGraphemeBreak_ExtendedPictographic;

	// Hangul syllable types
	int hangulType = -1;
	if(isHangulSyllable (c, hangulType))
	{
		switch(hangulType)
		{
		case 0 : return kGraphemeBreak_L;
		case 1 : return kGraphemeBreak_V;
		case 2 : return kGraphemeBreak_T;
		case 3 : return kGraphemeBreak_LV;
		case 4 : return kGraphemeBreak_LVT;
		}
	}

	// SpacingMark
	if((c >= 0x0903 && c <= 0x0903) || (c >= 0x093B && c <= 0x093B) ||
	   (c >= 0x093E && c <= 0x0940) || (c >= 0x0949 && c <= 0x094C) ||
	   (c >= 0x094E && c <= 0x094F) || (c >= 0x0982 && c <= 0x0983) ||
	   (c >= 0x09BF && c <= 0x09C0) || (c >= 0x09C7 && c <= 0x09C8) ||
	   (c >= 0x09CB && c <= 0x09CC) || (c >= 0x0A03 && c <= 0x0A03) ||
	   (c >= 0x0A3E && c <= 0x0A40) || (c >= 0x0A83 && c <= 0x0A83) ||
	   (c >= 0x0ABE && c <= 0x0AC0) || (c >= 0x0AC9 && c <= 0x0AC9) ||
	   (c >= 0x0ACB && c <= 0x0ACC) || (c >= 0x0B02 && c <= 0x0B03) ||
	   (c >= 0x0B40 && c <= 0x0B40) || (c >= 0x0B47 && c <= 0x0B48) ||
	   (c >= 0x0B4B && c <= 0x0B4C) || c == 0x1100 || c == 0x1160 || c == 0x11A8)
		return kGraphemeBreak_SpacingMark;

	// Extend check moved earlier to come before Extended_Pictographic (see above)

	return kGraphemeBreak_Other;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UTFCodec::isCombiningMark (uchar32 c)
{
	// Combining Diacritical Marks
	if(c >= 0x0300 && c <= 0x036F)
		return true;

	// Combining Diacritical Marks Extended
	if(c >= 0x1AB0 && c <= 0x1AFF)
		return true;

	// Combining Diacritical Marks Supplement
	if(c >= 0x1DC0 && c <= 0x1DFF)
		return true;

	// Combining Diacritical Marks for Symbols
	if(c >= 0x20D0 && c <= 0x20FF)
		return true;

	// Combining Half Marks
	if(c >= 0xFE20 && c <= 0xFE2F)
		return true;

	// Other common combining marks
	if((c >= 0x0483 && c <= 0x0489) ||  // Cyrillic
	   (c >= 0x0591 && c <= 0x05BD) ||  // Hebrew
	   (c >= 0x064B && c <= 0x065F) ||  // Arabic
	   (c >= 0x0670 && c <= 0x0670) ||  // Arabic
	   (c >= 0x06D6 && c <= 0x06DC) ||  // Arabic
	   (c >= 0x06DF && c <= 0x06E4) ||  // Arabic
	   (c >= 0x06E7 && c <= 0x06E8) ||  // Arabic
	   (c >= 0x06EA && c <= 0x06ED) ||  // Arabic
	   (c >= 0x0711 && c <= 0x0711) ||  // Syriac
	   (c >= 0x0730 && c <= 0x074A) ||  // Syriac
	   (c >= 0x07A6 && c <= 0x07B0) ||  // Thaana
	   (c >= 0x07EB && c <= 0x07F3) ||  // NKo
	   (c >= 0x0816 && c <= 0x0819) ||  // Samaritan
	   (c >= 0x081B && c <= 0x0823) ||  // Samaritan
	   (c >= 0x0825 && c <= 0x0827) ||  // Samaritan
	   (c >= 0x0829 && c <= 0x082D) ||  // Samaritan
	   (c >= 0x0859 && c <= 0x085B) ||  // Mandaic
	   (c >= 0x08D4 && c <= 0x08E1) ||  // Arabic Extended-A
	   (c >= 0x08E3 && c <= 0x0903) ||  // Arabic Extended-A / Devanagari
	   (c >= 0x093A && c <= 0x093C) ||  // Devanagari
	   (c >= 0x093E && c <= 0x094F) ||  // Devanagari
	   (c >= 0x0951 && c <= 0x0957))    // Devanagari
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UTFCodec::isEmojiModifier (uchar32 c)
{
	// Emoji Skin Tone Modifiers (Fitzpatrick scale)
	return c >= 0x1F3FB && c <= 0x1F3FF;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UTFCodec::isVariationSelector (uchar32 c)
{
	// Variation Selectors (VS1-VS16)
	if(c >= 0xFE00 && c <= 0xFE0F)
		return true;

	// Variation Selectors Supplement (VS17-VS256)
	if(c >= 0xE0100 && c <= 0xE01EF)
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UTFCodec::isRegionalIndicator (uchar32 c)
{
	// Regional Indicator Symbols (used for flag emojis)
	return c >= 0x1F1E6 && c <= 0x1F1FF;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UTFCodec::isExtendedPictographic (uchar32 c)
{
	// Extended_Pictographic property - emojis and emoji-related characters
	// This is a simplified version covering the most common emoji ranges

	// Miscellaneous Symbols and Pictographs
	if(c >= 0x1F300 && c <= 0x1F5FF)
		return true;

	// Emoticons
	if(c >= 0x1F600 && c <= 0x1F64F)
		return true;

	// Transport and Map Symbols
	if(c >= 0x1F680 && c <= 0x1F6FF)
		return true;

	// Supplemental Symbols and Pictographs
	if(c >= 0x1F900 && c <= 0x1F9FF)
		return true;

	// Symbols and Pictographs Extended-A
	if(c >= 0x1FA70 && c <= 0x1FAFF)
		return true;

	// Dingbats
	if(c >= 0x2700 && c <= 0x27BF)
		return true;

	// Miscellaneous Symbols
	if(c >= 0x2600 && c <= 0x26FF)
		return true;

	// Supplemental Arrows-B
	if(c >= 0x2900 && c <= 0x297F)
		return true;

	// Additional common emoji
	if(c == 0x00A9 || c == 0x00AE ||  // © ®
	   (c >= 0x203C && c <= 0x3299))  // Various symbols
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UTFCodec::isHangulSyllable (uchar32 c, int& type)
{
	// Hangul Jamo Leading Consonants (L)
	if(c >= 0x1100 && c <= 0x115F)
	{
		type = 0;  // L
		return true;
	}

	// Hangul Jamo Vowels (V)
	if(c >= 0x1160 && c <= 0x11A7)
	{
		type = 1;  // V
		return true;
	}

	// Hangul Jamo Trailing Consonants (T)
	if(c >= 0x11A8 && c <= 0x11FF)
	{
		type = 2;  // T
		return true;
	}

	// Hangul Syllables (precomposed)
	if(c >= 0xAC00 && c <= 0xD7A3)
	{
		int SIndex = c - 0xAC00;
		int TIndex = SIndex % 28;

		if(TIndex == 0)
			type = 3;  // LV syllable
		else
			type = 4;  // LVT syllable

		return true;
	}

	return false;
}
