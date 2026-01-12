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
// Filename    : argumentparsertest.cpp
// Description : Unit tests for ArgumentParser class.
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/extras/tools/argumentparser.h"
#include "ccl/main/cclargs.h"

using namespace CCL;

/**
 * \test Required positional arguments are parsed in order. Setup two test
 * arguments, parse different argument lists. Expect the arg values to be
 * assigned in the order from the argument list.
 */
CCL_TEST (ArgumentParserTest, TestRequiredPositionals)
{
	ArgumentParser parser;
	parser.add ("input");
	parser.add ("output");

	// Test: positionals in order as defined.
	MutableArgumentList list ("app.exe infile.txt outfile.txt");
	CCL_TEST_ASSERT_EQUAL (kResultOk, parser.parse (list));
	CCL_TEST_ASSERT_EQUAL (String ("infile.txt"), parser.get ("input"));
	CCL_TEST_ASSERT_EQUAL (String ("outfile.txt"), parser.get ("output"));

	// Test: positionals in different order as defined, expect arg values to change.
	list = MutableArgumentList ("app.exe outfile.txt infile.txt");
	CCL_TEST_ASSERT_EQUAL (kResultOk, parser.parse (list));
	CCL_TEST_ASSERT_EQUAL (String ("infile.txt"), parser.get ("output"));
	CCL_TEST_ASSERT_EQUAL (String ("outfile.txt"), parser.get ("input"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \test Required non-positional arguments are parsed in any order. Setup
 * two test arguments, parse from argument lists with different orders.
 * Expect the argument values to be assigned to the correct argument.
 */
CCL_TEST (ArgumentParserTest, TestRequiredShiftables)
{
	ArgumentParser parser;
	parser.add ("mode", {"-m1", "-m2"}, "optional mode flags", Argument::kOptional);
	parser.add ("format", {"-f1", "-f2"}, "format string", Argument::kOptional);

	// Test in order as defined.
	MutableArgumentList list ("app.exe -m1 -f2");
	CCL_TEST_ASSERT_EQUAL (kResultOk, parser.parse (list));
	CCL_TEST_ASSERT_EQUAL (String ("-m1"), parser.get ("mode"));
	CCL_TEST_ASSERT_EQUAL (String ("-f2"), parser.get ("format"));

	// Test in different order than defined, expect same arg values.
	list = MutableArgumentList ("app.exe -f2 -m1");
	CCL_TEST_ASSERT_EQUAL (kResultOk, parser.parse (list));
	CCL_TEST_ASSERT_EQUAL (String ("-m1"), parser.get ("mode"));
	CCL_TEST_ASSERT_EQUAL (String ("-f2"), parser.get ("format"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \test Required non-positional arguments are parsed in any order. Setup
 * two test arguments, both expecting values, parse from argument lists with different orders.
 * Expect the argument values to be assigned to the correct argument.
 */
CCL_TEST (ArgumentParserTest, testRequiredShiftablesExpectingValues)
{
	ArgumentParser parser;
	parser.add ("mode", {"-mode"}, "optional mode flags", Argument::kOptional | Argument::kExpectsValue);
	parser.add ("format", {"-format"}, "format string", Argument::kOptional | Argument::kExpectsValue);

	// Test in order as defined.
	MutableArgumentList list ("app.exe -mode default -format pretty");
	CCL_TEST_ASSERT_EQUAL (kResultOk, parser.parse (list));
	CCL_TEST_ASSERT_EQUAL (String ("default"), parser.get ("mode"));
	CCL_TEST_ASSERT_EQUAL (String ("pretty"), parser.get ("format"));

	// Test in different order than defined, expect same arg values.
	list = MutableArgumentList ("app.exe -format pretty -mode default");
	CCL_TEST_ASSERT_EQUAL (kResultOk, parser.parse (list));
	CCL_TEST_ASSERT_EQUAL (String ("default"), parser.get ("mode"));
	CCL_TEST_ASSERT_EQUAL (String ("pretty"), parser.get ("format"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \test Optional positionals are parsed correctly. Setup one required and three
 * optional positional arguments, including a defaultValue case. Parse an argument
 * list that specified only the first optional argument. Expect the required argument,
 * the first optional argument and the second optional argument (default value) to
 * return the correct argument values.
 */
CCL_TEST (ArgumentParserTest, testOptionalPositionals)
{
	ArgumentParser parser;
	parser.add ("input");
	parser.add ("outfile1", "first output file", Argument::kOptional);
	parser.add ("outfile2", "second output file", Argument::kOptional, "defaultfile.txt");
	parser.add ("outfile3", "third output file", Argument::kOptional);

	MutableArgumentList list ("app.exe input.txt optfile1.txt");
	CCL_TEST_ASSERT_EQUAL (kResultOk, parser.parse (list));
	CCL_TEST_ASSERT_EQUAL (String ("input.txt"), parser.get ("input"));
	CCL_TEST_ASSERT_EQUAL (String ("optfile1.txt"), parser.get ("outfile1"));
	CCL_TEST_ASSERT_EQUAL (String ("defaultfile.txt"), parser.get ("outfile2"));
	CCL_TEST_ASSERT_EQUAL (Variant (), parser.get ("outfile3"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \test Optional non-positionals are parsed correctly. Setup two required and two
 * non-required arguments. Parse an argument list not providing the 'print' parameter.
 * Expect the two required and the optional 'log' arg to be parsed correctly.
 */
CCL_TEST (ArgumentParserTest, testOptionalShiftables)
{
	ArgumentParser parser;
	parser.add ("mode", {"-m1", "-m2"}, "mode", Argument::kShiftable);
	parser.add ("format", {"-f1", "-f2"}, "format", Argument::kShiftable);
	parser.add ("log", {"-l1", "-l2"}, "log", Argument::kOptional | Argument::kShiftable);
	parser.add ("print", {"-p1", "-p2"}, "print", Argument::kOptional | Argument::kShiftable);

	// Skip 'print', must parse successfully.
	MutableArgumentList list ("app.exe -m1 -f2 -l1");
	CCL_TEST_ASSERT_EQUAL (kResultOk, parser.parse (list));
	CCL_TEST_ASSERT_EQUAL (String ("-m1"), parser.get ("mode"));
	CCL_TEST_ASSERT_EQUAL (String ("-f2"), parser.get ("format"));
	CCL_TEST_ASSERT_EQUAL (String ("-l1"), parser.get ("log"));
	CCL_TEST_ASSERT_EQUAL (Variant (), parser.get ("print"));

	// Rearrange
	list = MutableArgumentList ("app.exe -l1 -f2 -m1");
	CCL_TEST_ASSERT_EQUAL (kResultOk, parser.parse (list));
	CCL_TEST_ASSERT_EQUAL (String ("-m1"), parser.get ("mode"));
	CCL_TEST_ASSERT_EQUAL (String ("-f2"), parser.get ("format"));
	CCL_TEST_ASSERT_EQUAL (String ("-l1"), parser.get ("log"));
	CCL_TEST_ASSERT_EQUAL (Variant (), parser.get ("print"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \test Verify that a missing required positional argument results in a parser error.
 * Defines two required args, attempts to parse an argument list with insufficient arg
 * values. Expect 'parse' to return a 'missing' error code.
 */
CCL_TEST (ArgumentParserTest, TestMissingRequiredPositional)
{
	ArgumentParser parser;
	parser.add ("input");
	parser.add ("output");

	// Must fail to parse, required positional not found.
	MutableArgumentList list ("app.exe infile.txt");
	CCL_TEST_ASSERT_EQUAL (kResultFalse, parser.parse (list));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \test Verify that a required non-positional argument results in a parser error.
 * Defines two required args, attempts to parse an argument list with insufficient arg
 * values. Expect 'parse' to return a 'missing' error code.
 */
CCL_TEST (ArgumentParserTest, TestMissingRequiredShiftable)
{
	ArgumentParser parser;
	parser.add ("mode", {"-m1", "-m2"}, "optional mode flags", Argument::kShiftable);
	parser.add ("format", {"-f1", "-f2"}, "format string", Argument::kShiftable);

	// Must fail to parse, required arg not found.
	MutableArgumentList list ("app.exe -m1");
	CCL_TEST_ASSERT_EQUAL (kResultFalse, parser.parse (list));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \test Verify that if the args configuration specifies additional required positional arguments
 * after optional positional arguments, the parser detects the issue and reports an error.
 */
CCL_TEST (ArgumentParserTest, TestInvalidConfigPositionals)
{
	ArgumentParser parser;
	parser.add ("infile", "optional input file", Argument::kOptional);
	parser.add ("outfile");

	MutableArgumentList list ("app.exe infile.txt outfile.txt");
	CCL_TEST_ASSERT_EQUAL (kResultFailed, parser.parse (list));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \test Verify that overlapping choices are detected and result in a parser error. Sets up
 * two non-positional args both sharing the 'b' optionl Expect 'parse' to return an error code.
 */
CCL_TEST (ArgumentParserTest, TestInvalidConfigShiftables)
{
	// Invalid: options share same value.
	ArgumentParser parser;
	parser.add ("option1", {"a", "b"});
	parser.add ("option2", {"b", "c"});

	MutableArgumentList list ("app.exe a b");
	CCL_TEST_ASSERT_EQUAL (kResultFailed, parser.parse (list));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \test Verify that a non-positional arg without choices is detected as an error.
 * Sets up a non-positional arg but leaves the choices empty. Expect 'parse'
 * to return an error code.
 */
CCL_TEST (ArgumentParserTest, TestMissingChoiceRequiredShiftable)
{
	StringList empty = {};
	ArgumentParser parser;
	parser.add ("mode", empty, "mode", Argument::kShiftable);

	MutableArgumentList list ("app.exe");
	CCL_TEST_ASSERT_EQUAL (kResultFailed, parser.parse (list));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \test Verify that an invalid option for a positional required argument with options
 * is detected by the parser. Sets up a required positional arg with options 'a', 'b'
 * but then attempts to parse an argument list with value 'c'. Expect 'parse' to return
 * an error code.
 */
CCL_TEST (ArgumentParserTest, TestInvalidChoiceRequiredPositional)
{
	ArgumentParser parser;
	parser.add ("mode", {"a", "b"});

	// Positional argument exists but has invalid value.
	MutableArgumentList list ("app.exe c");
	CCL_TEST_ASSERT_EQUAL (kResultFalse, parser.parse (list));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \test Verify that a required non-positional arg can not be resolved if the
 * argument list does not provide a choices conform value. Sets up a argument
 * with choices '-m1', '-m2'. Parses an argument list with arg value '-m'.
 * Expect 'parse' to return an error code.
 */
CCL_TEST (ArgumentParserTest, TestInvalidChoiceRequiredShiftable)
{
	ArgumentParser parser;
	parser.add ("mode", {"-m1", "-m2"}, "mode", Argument::kShiftable);

	// Unexpected value 'm3', 'mode' can not be resolved.
	MutableArgumentList list ("app.exe -m3");
	CCL_TEST_ASSERT_EQUAL (kResultFalse, parser.parse (list));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \test Verify that superfluous args are detected by the parser. Sets up a test arg
 * and an argument list with more arguments than defined. Expect 'parse' to return
 * an error code.
 */
CCL_TEST (ArgumentParserTest, TestUnexpectedArg)
{
	ArgumentParser parser;
	parser.add ("mode", {"a", "b"});

	// Positional argument exists but has invalid value.
	MutableArgumentList list ("app.exe a unexpected");
	CCL_TEST_ASSERT_EQUAL (kResultFalse, parser.parse (list));
	
	// Allow unexpected values
	CCL_TEST_ASSERT_EQUAL (kResultTrue, parser.parse (list, ArgumentParser::kAllowUnknownArguments));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * \test Smoke test, combine various scenarios in a larger config.
 */
CCL_TEST (ArgumentParserTest, TestMixedConfig)
{
	ArgumentParser parser;
	parser.add ("infile");
	parser.add ("outfile");
	parser.add ("filetype", {"-xml", "-txt"});
	parser.add ("model1", "first model", Argument::kOptional);
	parser.add ("model2", "second model", Argument::kOptional); // left out in arg list
	parser.add ("mode", {"-m1", "-m2"}, "mode", Argument::kShiftable);
	parser.add ("format", {"-f1", "-f2"}, "format", Argument::kShiftable);
	parser.add ("name", {"-name"}, "the name", Argument::kShiftable | Argument::kExpectsValue);
	parser.add ("option", {"-o1", "-o2"}, "an option", Argument::kOptional | Argument::kShiftable);
	parser.add ("log", {"-l1", "-l2"}, "enable logging", Argument::kOptional | Argument::kShiftable); // left out in arg
	parser.add ("print", {"-p1", "-p2"}, "print", Argument::kOptional | Argument::kShiftable, "-p2"); // left out in arg but has default value

	// Intentionally re-arrange some of the options.
	MutableArgumentList list ("app.exe -m1 -o1 infile.txt -name test outfile.txt -xml -f2 model1.txt");
	CCL_TEST_ASSERT_EQUAL (kResultOk, parser.parse (list));
	CCL_TEST_ASSERT_EQUAL (String ("infile.txt"), parser.get ("infile"));
	CCL_TEST_ASSERT_EQUAL (String ("outfile.txt"), parser.get ("outfile"));
	CCL_TEST_ASSERT_EQUAL (String ("-xml"), parser.get ("filetype"));
	CCL_TEST_ASSERT_EQUAL (String ("model1.txt"), parser.get ("model1"));
	CCL_TEST_ASSERT_EQUAL (Variant (), parser.get ("model2")); // not found
	CCL_TEST_ASSERT_EQUAL (String ("-m1"), parser.get ("mode"));
	CCL_TEST_ASSERT_EQUAL (String ("-f2"), parser.get ("format"));
	CCL_TEST_ASSERT_EQUAL (String ("-o1"), parser.get ("option"));
	CCL_TEST_ASSERT_EQUAL (Variant (), parser.get ("log")); // not found
	CCL_TEST_ASSERT_EQUAL (String ("-p2"), parser.get ("print")); // not found but has default value
	CCL_TEST_ASSERT_EQUAL (String ("test"), parser.get ("name"));
}
