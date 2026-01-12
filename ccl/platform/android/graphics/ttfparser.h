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
// Filename    : ccl/platform/android/graphics/ttfparser.h
// Description : TrueType/OpenType Font Info Parser
//
//************************************************************************************************

#include "ccl/public/base/streamer.h"
#include "ccl/public/base/buffer.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

//************************************************************************************************
// TTFParser
//************************************************************************************************

class TTFParser
{
public:
	struct TT_OFFSET_TABLE
	{
		uint32	uVersion;
		uint16	uNumOfTables;
		uint16	uSearchRange;
		uint16	uEntrySelector;
		uint16	uRangeShift;
	};

	struct TT_TABLE_DIRECTORY
	{
		char	szTag[4];			// table name
		uint32	uCheckSum;			// Check sum
		uint32	uOffset;			// Offset from beginning of file
		uint32	uLength;			// length of the table in bytes
	};

	struct TT_CMAP_TABLE_HEADER
	{
		uint16	uVersion;			// version
		uint16	uNRSubTables;		// sub table count
	};

	struct TT_CMAP_SUBTABLE
	{
		uint16	uPlatformID;		// platform ID
		uint16	uEncodingID;		// platform-specific encoding ID
		uint32	uStorageOffset;		// offset for mapping table storage, from start of the table
	};

	struct TT_NAME_TABLE_HEADER
	{
		uint16	uFSelector;			// format selector. Always 0
		uint16	uNRCount;			// Name Records count
		uint16	uStorageOffset;		// Offset for strings storage, from start of the table
	};

	struct TT_NAME_RECORD
	{
		uint16	uPlatformID;
		uint16	uEncodingID;
		uint16	uLanguageID;
		uint16	uNameID;
		uint16	uStringLength;		// string length in bytes
		uint16	uStringOffset;		// from start of storage area
	};

	enum PlatformID
	{
		kPlatformMac = 1,
		kPlatformWin = 3
	};

	enum EncodingID
	{
		// platform = 1 (Macintosh)
		kEncodingMacRoman = 0,

		// platform = 3 (Windows)
		kEncodingWinSymbol = 0,
		kEncodingWinUnicode = 1
	};

	enum NameID
	{
		kCopyrightNotice,
		kFamilyName,
		kSubFamilyName,
		kUniqueFontIdentifier,
		kFullFontName,
		#if 0
		kVersionString,
		kPostscriptName,
		kTrademark,
		kManufacturer,
		kDesigner,
		kDescription,
		kURLVendor,
		kURLDesigner,
		kLicenseDescription,
		kLicenseURL,
		kReserved,
		kWinPreferredFamily,
		kWinPreferredSubfamily,
		// there's more but we don't care about it
		#endif
		kNameStringCount,

		// additional entries
		kSampleText = 19
	};

	class FontInfo
	{
	public:
		StringRef getString (int id) const
		{
			VectorForEach (strings, StringEntry&, e)
				if(e.id == id)
					return e.string;
			EndFor
			return String::kEmpty;
		}

		void setString (int id, StringRef string)
		{
			strings.add (StringEntry ({id, string}));
		}

		void setSymbolFont (bool value) { symbolFont = value; }
		bool isSymbolFont () const { return symbolFont; }

	private:
		struct StringEntry
		{
			int id;
			String string;
		};

		Vector<StringEntry> strings;
		bool symbolFont = false;
	};

	static bool parseFontInfo (FontInfo& info, IStream& stream)
	{
		ASSERT (stream.isSeekable ())
		Streamer s (stream, kBigEndian);

		TT_OFFSET_TABLE ttOffsetTable = {0};
		s.read (ttOffsetTable.uVersion);
		s.read (ttOffsetTable.uNumOfTables);
		s.read (ttOffsetTable.uSearchRange);
		s.read (ttOffsetTable.uEntrySelector);
		s.read (ttOffsetTable.uRangeShift);

		// check if it's a TrueType 1.0 or OpenType font
		static const uint32 kTrueType = 0x00010000;
		static const uint32 kOpenType = 'OTTO';
		if(!(ttOffsetTable.uVersion == kTrueType || ttOffsetTable.uVersion == kOpenType))
			return false;

		for(int i = 0; i < ttOffsetTable.uNumOfTables; i++)
		{
			TT_TABLE_DIRECTORY tblDir = {0};
			s.read (tblDir.szTag, 4);
			s.read (tblDir.uCheckSum);
			s.read (tblDir.uOffset);
			s.read (tblDir.uLength);

			// search for cmap table
			if(::strncmp (tblDir.szTag, "cmap", 4) == 0)
			{
				int64 pos = stream.tell ();
				stream.seek (tblDir.uOffset, IStream::kSeekSet);

				TT_CMAP_TABLE_HEADER ttCTHeader = { 0 };
				s.read (ttCTHeader.uVersion);
				s.read (ttCTHeader.uNRSubTables);

				for(int i = 0; i < ttCTHeader.uNRSubTables; i++)
				{
					TT_CMAP_SUBTABLE ttSubTable = { 0 };
					s.read (ttSubTable.uPlatformID);
					s.read (ttSubTable.uEncodingID);
					s.read (ttSubTable.uStorageOffset);

					if(ttSubTable.uPlatformID == kPlatformWin && ttSubTable.uEncodingID == kEncodingWinSymbol)
						info.setSymbolFont (true);
				}

				stream.seek (pos, IStream::kSeekSet);
			}

			// search for name table
			if(::strncmp (tblDir.szTag, "name", 4) == 0)
			{
				int64 pos = stream.tell ();
				stream.seek (tblDir.uOffset, IStream::kSeekSet);

				TT_NAME_TABLE_HEADER ttNTHeader = {0};
				s.read (ttNTHeader.uFSelector);
				s.read (ttNTHeader.uNRCount);
				s.read (ttNTHeader.uStorageOffset);

				for(int i = 0; i < ttNTHeader.uNRCount; i++)
				{
					TT_NAME_RECORD ttRecord = {0};
					s.read (ttRecord.uPlatformID);
					s.read (ttRecord.uEncodingID);
					s.read (ttRecord.uLanguageID);
					s.read (ttRecord.uNameID);
					s.read (ttRecord.uStringLength);
					s.read (ttRecord.uStringOffset);

					if(ttRecord.uNameID < kNameStringCount || ttRecord.uNameID == kSampleText)
					{
						if(!info.getString (ttRecord.uNameID).isEmpty ()) // use first appearance
							continue;

						int64 pos = stream.tell ();
						stream.seek (tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset, IStream::kSeekSet);

						static const uint16 kMaxStringLength = STRING_STACK_SPACE_MAX;
						char buffer[kMaxStringLength];
						int bytesRead = s.read (buffer, ccl_min (ttRecord.uStringLength, kMaxStringLength));

						String string;
						if(ttRecord.uPlatformID == kPlatformMac && ttRecord.uEncodingID == kEncodingMacRoman)
						{
							string.appendASCII (buffer, bytesRead);
						}
						else if(ttRecord.uPlatformID == kPlatformWin && ttRecord.uEncodingID == kEncodingWinUnicode)
						{
							uchar* unicodeBuffer = (uchar*)buffer;
							int length = bytesRead/sizeof(uchar);

							#if CCL_NATIVE_BYTEORDER == CCL_LITTLE_ENDIAN
							for(int i = 0;  i < length; i++)
								unicodeBuffer[i] = byte_swap<uint16> (unicodeBuffer[i]);
							#endif

							string.append (unicodeBuffer, length);
						}

						#if (0 && DEBUG)
						Debugger::printf ("%d: ", int(ttRecord.uNameID));
						Debugger::println (string);
						#endif

						info.setString (ttRecord.uNameID, string);

						stream.seek (pos, IStream::kSeekSet);
					}
				}

				stream.seek (pos, IStream::kSeekSet);
			}
		}
		return true;
	}
};

} // namespace CCL
