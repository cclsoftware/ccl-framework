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
// Filename    : stringtest.cpp
// Description : Unit tests for string classes
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/iregexp.h"
#include "ccl/public/system/logging.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////////////////////////////////

static const String theConstant1 = CCLSTR ("This is a constant string!");
static const String theConstant2 = CCLSTR ("Hello World!");

//////////////////////////////////////////////////////////////////////////////////////////////////

#define TEST_SYSTEM_ENCODINGS 1 // not available on all platforms!
#define kNumTestEncodings ARRAY_COUNT (testEncodings)
static const struct { TextEncoding encoding; const char* string; } testEncodings[] =
{
	{Text::kASCII,			"US-ASCII"},
	{Text::kISOLatin1,		"ISO Latin 1"},
	{Text::kDOSLatinUS,		"DOS Latin US"}, // encoding used by ZIP files
	{Text::kUTF8,			"UTF-8"},
	#if TEST_SYSTEM_ENCODINGS 
	{Text::kWindowsLatin1,	"Windows Latin 1"},
	{Text::kMacRoman,		"MAC Roman"},
	#endif
	{Text::kSystemEncoding,	"(System)"}
};

static const char* getEncodingString (TextEncoding encoding)
{
	for(int i = 0; i < kNumTestEncodings; i++)
		if(encoding == testEncodings[i].encoding)
			return testEncodings[i].string;
	return "Unknown";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline IString* getIString (StringRef s)
{
	return ((PlainString&)s).theString;
}

//************************************************************************************************
// StringTest
//************************************************************************************************

CCL_TEST (StringTest, TestAccess)
{
	String s (theConstant1);
	int length = s.length ();
	StringChars chars (theConstant1);
	for(int i = 0; i < length; i++)
		CCL_TEST_ASSERT (chars[i] == s[i]);

	uchar buffer[1024] = {0};
	CCL_TEST_ASSERT (s.copyTo (buffer, CCLSTRSIZE (buffer)) == true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TestCopyOnWrite)
{
	String s1 (theConstant1);
	IString* iString1 = getIString (s1);
	s1.appendASCII ("...");
	IString* iString2 = getIString (s1);
	
	String s2 ("BlahBlah");
	IString* iString3 = getIString (s2);
	s2.appendASCII ("...");
	IString* iString4 = getIString (s2);
	
	CCL_TEST_ASSERT (iString1 != iString2 && iString3 == iString4);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TestPlainAccess)
{
	String s1 (theConstant1);
	IString* iString = getIString (s1);
	CCL_TEST_ASSERT (s1.length () == iString->getLength ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TestCharMethods)
{
	uchar c1 = Unicode::toLowercase ('A');
	CCL_TEST_ASSERT (c1 == 'a');
	CCL_TEST_ASSERT (Unicode::isLowercase (c1));
	uchar c2 = Unicode::toUppercase ('z');
	CCL_TEST_ASSERT (c2 == 'Z');
	CCL_TEST_ASSERT (Unicode::isUppercase (c2));

	uchar c3 = (unsigned char)'F';
	c3 = Unicode::toLowercase (c3);
	CCL_TEST_ASSERT (Unicode::isAlpha (c3) == true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TEST_STL_CONTAINERS)
{
	String s (CCLSTR ("  Hello World!  \t"));
	CCL_TEST_ASSERT (s.trimWhitespace () == CCLSTR ("Hello World!"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TestCaseConversion)
{
	const String s1 ("all lowercase letters");
	const String s2 ("ALL LOWERCASE LETTERS");
	String s3 (s1);
	CCL_TEST_ASSERT (s3.toUppercase () == s2);
	CCL_TEST_ASSERT (s3.toLowercase () == s1);
	s3 = s1;
	s3.capitalize ();
	Logging::debug (s3);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TestCStringEncodings)
{
	return; // This test fails (Bug: #75)
	
	#if TEST_SYSTEM_ENCODINGS
	int passCount = 2;
	#else
	int passCount = 1;
	#endif

	for(int pass = 1; pass <= passCount; pass++)
	{
		String prototype;
		if(pass == 1)
			prototype = theConstant1;
		else
			prototype.appendCString (Text::kSystemEncoding, "Ein Text mit \u00FCml\u00E4uten ...!");

		MutableCString message;
		message.appendFormat ("%d) l = %d %s umlauts", pass, prototype.length (), pass == 1 ? "no" : "with");
		Logging::debug (String (message));

		for(int i = 0; i < kNumTestEncodings; i++)
		{
			TextEncoding encoding = testEncodings[i].encoding;
			if(pass == 2 && encoding == Text::kASCII) // no umlauts in ASCII
				continue;

			String s (prototype);
			char cString[256] = {0};
			CCL_TEST_ASSERT (s.toCString (encoding, cString, sizeof(cString)) == true);

			// append null-terminated
			s.empty ();
			CCL_TEST_ASSERT (s.appendCString (encoding, cString) == true);
			CCL_TEST_ASSERT (s == prototype);

			// append with length given
			s.empty ();
			int cStringLength = CString (cString).length ();
			
			message.empty ();
			message.appendFormat ("l = %d %s", cStringLength, getEncodingString (encoding));
			Logging::debug (String (message));

			CCL_TEST_ASSERT (s.appendCString (encoding, cString, cStringLength) == true);
			CCL_TEST_ASSERT (s == prototype);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TestPascalString)
{
	for(int i = 0; i < kNumTestEncodings; i++)
	{
		TextEncoding encoding = testEncodings[i].encoding;

		String s (theConstant1);
		unsigned char pString[256] = {0};
		CCL_TEST_ASSERT (s.toPascalString (encoding, pString, sizeof(pString)) == true);

		s.empty ();
		CCL_TEST_ASSERT (s.appendPascalString (encoding, pString) == true);
		CCL_TEST_ASSERT (s == theConstant1);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TestCompare)
{
	String s1, s2;
	CCL_TEST_ASSERT (s1 == s2); // compare empty string
	
	s1 << "ABC";
	s2 << "DEF";
	CCL_TEST_ASSERT (s1 != s2);
	CCL_TEST_ASSERT (s1.compare (s2) == Text::kLess);
	CCL_TEST_ASSERT (s2.compare (s1) == Text::kGreater);
	
	// case check
	String s3;
	s3 << "abc";
	CCL_TEST_ASSERT (s1.compare (s3, true) != Text::kEqual);
	CCL_TEST_ASSERT (s1.compare (s3, false) == Text::kEqual);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TestFind)
{
	String s1 ("This contains the search string. ");
	String s2 ("contains");
	String s3 ("CONTAINS");
	CCL_TEST_ASSERT (s1.contains (s2) == true);
	CCL_TEST_ASSERT (s1.contains (s3) == false);
	CCL_TEST_ASSERT (s1.contains (s3, false) == true);

	String s4 ("...twice this string contains twice...");
	String s5 ("twice");
	int lastIndex = s4.lastIndex (s5);
	int index = s4.index (s5);
	CCL_TEST_ASSERT (index != lastIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TestTokenizer)
{
	static const String inputString = CCLSTR ("folder1/folder2/folder3/file.xxx:port");
	static const String delimiters = CCLSTR ("/.:");

	ForEachStringToken (inputString, delimiters, token)
		Logging::debug (token);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TestRegularExpression)
{
	AutoPtr<IRegularExpression> regExp = System::CreateRegularExpression ();
	CCL_TEST_ASSERT (regExp.isValid ());
	if(!regExp)
		return;

	CCL_TEST_ASSERT (regExp->construct ("h.*o") == kResultOk);
	CCL_TEST_ASSERT (regExp->isFullMatch ("hello") == 1);
	CCL_TEST_ASSERT (regExp->isFullMatch ("Hello") == 0);

	CCL_TEST_ASSERT (regExp->construct ("h.*o", IRegularExpression::kCaseInsensitive) == kResultOk);
	CCL_TEST_ASSERT (regExp->isFullMatch ("hello") == 1);
	CCL_TEST_ASSERT (regExp->isFullMatch ("Hello") == 1);

	String subject ("$1,$2");
	CCL_TEST_ASSERT (regExp->construct ("(\\$(\\d))") == kResultOk);
	CCL_TEST_ASSERT (regExp->replace (subject, "$$1-$1$2") == 1);
	CCL_TEST_ASSERT (subject == "$1-$11,$2");

	subject = "$1,$2";
	CCL_TEST_ASSERT (regExp->replaceAll (subject, "$$1-$1$2") == 1);
	CCL_TEST_ASSERT (subject == "$1-$11,$1-$22");

	subject = "abcabc";
	CCL_TEST_ASSERT (regExp->construct ("a") == kResultOk);
	CCL_TEST_ASSERT (regExp->replaceAll (subject, String ("$&$'")) == 1);
	CCL_TEST_ASSERT (subject == "abcabcbcabcbc");

	subject = "abcabc";
	CCL_TEST_ASSERT (regExp->replaceAll (subject, String ("$&$`")) == 1);
	CCL_TEST_ASSERT (subject == "abcaabcbc");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TestUnicodeSubstitution)
{
	uchar strangeCharacters[] = {0x0041,0x0042,0x0043,0x2018, 0x2019, 0x201A, 0x201B, 0x201C, 0x201D, 0x201E, 0x201F, 0x301D, 0x301E, 0x301F, 0xFF02, 0xFF07, 0x00DF, 0x0000};
	char expectedSubstitution[] = "ABC\'\'\'\'\"\"\"\"\"\"\"\"\'ss";
	String prototype;
	prototype = strangeCharacters;
	prototype.substitute ();
	int asciiLength = prototype.length ();
	char* asciiBuffer = NEW char[asciiLength + 1];
	prototype.toASCII (asciiBuffer, asciiLength + 1);
	CCL_TEST_ASSERT (strcmp (asciiBuffer, expectedSubstitution) == 0);
	delete[] asciiBuffer;
	
	uchar theUmlauts[] = {0x00c4, 0x00D6, 0x00DC, 0x00E4, 0x00F6, 0x00FC, 0x0000};
	char expectedUmlautSubstitution[] = "AeOeUeaeoeue";
	prototype = theUmlauts;
	prototype.substitute ();
	asciiLength = prototype.length ();
	asciiBuffer = NEW char[asciiLength + 1];
	prototype.toASCII (asciiBuffer, asciiLength + 1);
	CCL_TEST_ASSERT (strcmp (asciiBuffer, expectedUmlautSubstitution) == 0);
	delete[] asciiBuffer;	
	
	uchar someDiacritics[] = {0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 
	0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x0000};
	char expectedDiacriticSubstitution[] = "AAAAEEEEIIIIOOOOOUUUaaaaceeeeiiiinooooouuu";
	prototype = someDiacritics;
	prototype.substitute ();
	asciiLength = prototype.length ();
	asciiBuffer = NEW char[asciiLength + 1];
	prototype.toASCII (asciiBuffer, asciiLength + 1);
	CCL_TEST_ASSERT (strcmp (asciiBuffer, expectedDiacriticSubstitution) == 0);
	delete[] asciiBuffer;	
	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TestUnicodeNormalization)
{
	uchar precomposedChars[] = {0x00E5, 0x0000}; 		// Latin Small Letter a with Ring Above
	uchar decomposedChars[] = {0x0061, 0x030A, 0x0000};	// Latin Small Letter a, Combining Ring Above
	
	String stringA = precomposedChars;
	String stringB = decomposedChars;
	
	stringA.normalize (Text::kNormalizationC);
	stringB.normalize (Text::kNormalizationC);
	CCL_TEST_ASSERT (stringA.contains (stringB));

	CCL_TEST_ASSERT (stringA.length () == 1);
	CCL_TEST_ASSERT (stringA.at (0) == precomposedChars[0]);

	stringA.normalize (Text::kNormalizationD);
	CCL_TEST_ASSERT (stringA.length () == 2);
	CCL_TEST_ASSERT (stringA.at (0) == decomposedChars[0]);
	CCL_TEST_ASSERT (stringA.at (1) == decomposedChars[1]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TestGenerateTables)
{
	// Helper code to generate tables for cross-platform Unicode functions, keep it here!
	#define GENERATE_C_CODE 1

	auto isSurrogatePair = [] (uchar c)
	{
		return c >= 0xD800 && c <= 0xDBFF;
	};

	// Numeric characters
	for(int codePoint = 0; codePoint < 0xFFFF; codePoint++)
	{
		uchar uc = (uchar)codePoint;
		if(isSurrogatePair (uc))
			continue;

		if(Unicode::isAlphaNumeric (uc) && !Unicode::isAlpha (uc))
			Logging::debugf ("0x%04X,\n", codePoint);
	}

	// Encodings
	const TextEncoding encodings[] = {Text::kASCII, Text::kISOLatin1, Text::kDOSLatinUS};
	CStringPtr names[] = {"ascii", "latin1", "doslatinus"};

	for(int i = 0; i < ARRAY_COUNT (encodings); i++)
	{
		int count = 0;
		Logging::debugf ("=== Unicode to %s ===\n", names[i]);
		for(int codePoint = 0x80; codePoint < 0xFFFF; codePoint++)
		{
			uchar uc = (uchar)codePoint;
			if(isSurrogatePair (uc))
				continue;

			uchar temp[2] = {uc, 0};
			String s (temp);
			char cString[8] = {0};
			s.toCString (encodings[i], cString, sizeof(cString));
			char c = cString[0];
			if(c != '?')
			{
				int cValue = (unsigned char)c;
				#if GENERATE_C_CODE
				Logging::debugf ("{0x%04X, 0x%04X},\n", codePoint, cValue);
				#else
				Logging::debugf ("%03d: ", count);
				Debugger::print (s);
				Logging::debugf (" = %04X (%d), %04X '%c'\n", codePoint, codePoint, cValue, c);
				#endif
				count++;
			}
		}

		Logging::debugf ("=== %s to Unicode ===\n", names[i]);
		for(int codePoint = 0x00; codePoint <= 0xFF; codePoint++)
		{
			char cString[2] = {(char)codePoint, 0};
			String s;
			s.appendCString (encodings[i], cString);
			int uValue = s[0];

			if(codePoint != uValue)
				#if GENERATE_C_CODE
				Logging::debugf ("{0x%04X, 0x%04X},\n", codePoint, uValue);
				#else
				Logging::debugf ("%03d: %02X, %02X\n", codePoint, codePoint, uValue);
				#endif
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (StringTest, TestAppendFloatValue)
{
	double value = 1234567.123456789;

	// "%.50g" best fit with full precision
	String bestFit;
	bestFit.appendFloatValue (value);
	CCL_TEST_ASSERT_EQUAL (String ("1234567.12345678894780576229095458984375"), bestFit);

	// "%.*lf" no decimals.
	String none;
	none.appendFloatValue (value, 0);
	CCL_TEST_ASSERT_EQUAL (String ("1234567"), none);

	// "%.*lf" 6 decimals (common use case)
	String common;
	common.appendFloatValue (value, 6);
	CCL_TEST_ASSERT_EQUAL (String ("1234567.123457"), common);

	// "%.*lf" 6 decimals, negative number (common use case)
	String commonNegative;
	commonNegative.appendFloatValue (-1 * value, 6);
	CCL_TEST_ASSERT_EQUAL (String ("-1234567.123457"), commonNegative);

	// "%.*lf" sanity check, round down
	String roundDown;
	roundDown.appendFloatValue (1000.114, 2);
	CCL_TEST_ASSERT_EQUAL (String ("1000.11"), roundDown);

	// "%.*lf" sanity check, round up
	String roundUp;
	roundUp.appendFloatValue (1000.115, 2);
	CCL_TEST_ASSERT_EQUAL (String ("1000.12"), roundUp);

	// "%.*lf" negative decimals (wrong usage), yields best fit result
	String negative;
	negative.appendFloatValue (value, -1);
	CCL_TEST_ASSERT_EQUAL (String ("1234567.12345678894780576229095458984375"), negative);
}
