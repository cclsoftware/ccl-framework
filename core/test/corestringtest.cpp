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
// Filename    : core/test/corestringtest.cpp
// Description : Core String Tests
//
//************************************************************************************************

#include "corestringtest.h"

#include "core/public/corestringbuffer.h"

using namespace Core;
using namespace Test;

//************************************************************************************************
// StringTest
//************************************************************************************************

CORE_REGISTER_TEST (StringTest)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr StringTest::getName () const
{
	return "Core String";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringTest::run (ITestContext& testContext)
{
	bool success = true;
	success &= testAppendInteger (testContext);
	success &= testTokenizer (testContext);
	success &= testTokenizerInplace (testContext);
	success &= testTokenizerInplaceWithEmptyTokens (testContext);
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringTest::testAppendInteger (ITestContext& testContext)
{
	{
		CString64 string;
		string.appendInteger (uint32(0));
		if(string != "0")
			CORE_TEST_FAILED ("Failed to append 0 (uint32)")
	}
	{
		CString64 string;
		string.appendInteger (int32(-1));
		if(string != "-1")
			CORE_TEST_FAILED ("Failed to append -1 (int32)")
	}
	{
		CString64 string;
		string.appendInteger (uint64(18446744073709551615));
		if(string != "18446744073709551615")
			CORE_TEST_FAILED ("Failed to append 2^64-1 (uint64)")
	}
	{
		CString64 string;
		string.appendInteger (int64(-9223372036854775808));
		if(string != "-9223372036854775808")
			CORE_TEST_FAILED ("Failed to append -2^63 (int64)")
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringTest::testTokenizer (ITestContext& testContext)
{
	{
		CStringTokenizer tokenizer ("", " ");
		if(ConstString (tokenizer.next ()) != "")
			CORE_TEST_FAILED ("Failed to tokenize an empty string: It must return an empty string.")
		if(tokenizer.next () != nullptr)
			CORE_TEST_FAILED ("Failed to tokenize an empty string: It only has one token.")
	}

	auto checkSentence = [&testContext] (CStringTokenizer& tokenizer, CStringPtr errorMessage)
	{
		bool success = true;
		success &= (ConstString (tokenizer.next ()) == "This");
		success &= (ConstString (tokenizer.next ()) == "is");
		success &= (ConstString (tokenizer.next ()) == "a");
		success &= (ConstString (tokenizer.next ()) == "sentence.");
		success &= (tokenizer.next () == nullptr);
		if(!success)
			CORE_TEST_FAILED (errorMessage)
	};

	{
		CStringTokenizer tokenizer ("This is a sentence.", " ");
		checkSentence (tokenizer, "Failed to tokenize a string.");
	}
	{
		CStringTokenizer tokenizer (" This\tis\na sentence.\n", " \t\n");
		checkSentence (tokenizer, "Failed to tokenize a string using multiple delimiters.");
	}
	{
		CStringTokenizer tokenizer ("111This43is21a30sentence.421", " 01234");
		checkSentence (tokenizer, "Failed to tokenize a string with multiple delimiters in a row.");
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringTest::testTokenizerInplace (ITestContext& testContext)
{
	{
		CString64 string ("");
		CStringTokenizerInplace tokenizer (string.getBuffer (), " ", false);
		if(ConstString (tokenizer.next ()) != "")
			CORE_TEST_FAILED ("Failed to tokenize an empty string inplace: It must return an empty string.")
		if(tokenizer.next () != nullptr)
			CORE_TEST_FAILED ("Failed to tokenize an empty string inplace: It only has one token.")
	}

	auto checkSentence = [&testContext] (CStringTokenizerInplace& tokenizer, CStringPtr errorMessage)
	{
		bool success = true;
		success &= (ConstString (tokenizer.next ()) == "This");
		success &= (ConstString (tokenizer.next ()) == "is");
		success &= (ConstString (tokenizer.next ()) == "a");
		success &= (ConstString (tokenizer.next ()) == "sentence.");
		success &= (tokenizer.next () == nullptr);
		if(!success)
			CORE_TEST_FAILED (errorMessage)
	};

	{
		CString64 string ("This is a sentence.");
		CStringTokenizerInplace tokenizer (string.getBuffer (), " ", false);
		checkSentence (tokenizer, "Failed to tokenize a string inplace.");
	}
	{
		CString64 string (" This\tis\na sentence.\n");
		CStringTokenizerInplace tokenizer (string.getBuffer (), " \t\n", false);
		checkSentence (tokenizer, "Failed to tokenize a string inplace using multiple delimiters.");
	}
	{
		CString64 string ("111This43is21a30sentence.421");
		CStringTokenizerInplace tokenizer (string.getBuffer (), " 01234", false);
		checkSentence (tokenizer, "Failed to tokenize a string inplace with multiple delimiters in a row.");
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringTest::testTokenizerWithEmptyTokens (ITestContext& testContext)
{
	{
		CStringTokenizer tokenizer (";,,This,is;;a,;sentence.;", ",;", true);
		bool success = true;
		success &= (ConstString (tokenizer.next ()) == "");
		success &= (ConstString (tokenizer.next ()) == "");
		success &= (ConstString (tokenizer.next ()) == "");
		success &= (ConstString (tokenizer.next ()) == "This");
		success &= (ConstString (tokenizer.next ()) == "is");
		success &= (ConstString (tokenizer.next ()) == "");
		success &= (ConstString (tokenizer.next ()) == "a");
		success &= (ConstString (tokenizer.next ()) == "");
		success &= (ConstString (tokenizer.next ()) == "sentence.");
		success &= (ConstString (tokenizer.next ()) == "");
		success &= (tokenizer.next () == nullptr);
		if(!success)
			CORE_TEST_FAILED ("Failed to tokenize a string, preserving empty tokens.")
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringTest::testTokenizerInplaceWithEmptyTokens (ITestContext& testContext)
{
	{
		CString64 string (";,,This,is;;a,;sentence.;");
		CStringTokenizerInplace tokenizer (string.getBuffer (), ",;", true);
		bool success = true;
		success &= (ConstString (tokenizer.next ()) == "");
		success &= (ConstString (tokenizer.next ()) == "");
		success &= (ConstString (tokenizer.next ()) == "");
		success &= (ConstString (tokenizer.next ()) == "This");
		success &= (ConstString (tokenizer.next ()) == "is");
		success &= (ConstString (tokenizer.next ()) == "");
		success &= (ConstString (tokenizer.next ()) == "a");
		success &= (ConstString (tokenizer.next ()) == "");
		success &= (ConstString (tokenizer.next ()) == "sentence.");
		success &= (ConstString (tokenizer.next ()) == "");
		success &= (tokenizer.next () == nullptr);
		if(!success)
			CORE_TEST_FAILED ("Failed to tokenize a string inplace, preserving empty tokens.")
	}
	return true;
}
