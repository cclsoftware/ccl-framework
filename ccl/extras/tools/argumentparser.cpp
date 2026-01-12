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
// Filename    : ccl/extras/tools/argumentparser.cpp
// Description : Argument parser utility class.
//
//************************************************************************************************

#include "ccl/extras/tools/argumentparser.h"

#include "ccl/main/cclargs.h"

using namespace CCL;

//************************************************************************************************
// ArgumentParser::Description
//************************************************************************************************

ArgumentParser::Description::Description (StringRef name, const StringList& choices, StringRef description, int flags, VariantRef defaultValue)
: name (name),
  description (description),
  choices (choices),
  flags (flags),
  defaultValue (defaultValue)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ArgumentParser::Description::getName () const
{
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant ArgumentParser::Description::getDefaultValue () const
{
	return defaultValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArgumentParser::Description::matches (StringRef arg) const
{
	if(!choices.isEmpty ())
	{
		if(expectsValue () && arg.contains ("="))
			return choices.contains (arg.subString (0, arg.index ("=")));
		return choices.contains (arg);
	}

	// No choices, may be any string.
	return true;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArgumentParser::Description::hasChoices () const
{
	return !choices.isEmpty ();
}

//************************************************************************************************
// ArgumentParser::Result
//************************************************************************************************

ArgumentParser::Result::Result (StringRef name)
: name (name),
  status (kUnresolved)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ArgumentParser::Result::getName () const
{
	return name;
}

//************************************************************************************************
// ArgumentParser
//************************************************************************************************

void ArgumentParser::add (StringRef name, StringRef description, int flags, VariantRef defaultValue)
{
	args.add (Description (name, {}, description, flags, defaultValue));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ArgumentParser::add (StringRef name, const StringList& choices, StringRef description, int flags, VariantRef defaultValue)
{
	args.add (Description (name, choices, description, flags, defaultValue));
}
//////////////////////////////////////////////////////////////////////////////////////////////////

Variant ArgumentParser::get (StringRef name) const
{
	for(const Result& a : results)
	{
		if(a.getName () == name)
		{
			ASSERT (a.getStatus () != Result::kUnresolved)
			return a.getValue ();
		}
	}
	return Variant ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const StringList& ArgumentParser::getUnparsedArguments () const
{
	return unparsedArguments;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ArgumentParser::parse (ArgsRef argsList, int flags)
{
	results.empty ();

	StringList choices;

	// Verify argument choices do not overlap as they
	// are used to identify non-positional arguments.
	auto checkUniqueChoices = [&choices] (const StringList& list) -> bool
	{
		if(choices.containsAnyOf (list))
			return false;

		choices.addAllFrom (list);
		return true;
	};

	// Setup list of available values, skip executable name (arg 0).
	StringList pendingArgs;
	for(int i = 1; i < argsList.count (); i++)
		pendingArgs.add (argsList.at (i));

	// Consume all non-positional args, identified by their 'choices'.
	for(const auto& a : args)
	{
		if(!a.isShiftable ())
			continue;

		if(!a.hasChoices ())
			return kResultFailed;

		if(!checkUniqueChoices (a.getChoices ()))
			return kResultFailed;

		Result result (a.getName ());
		resolve (result, pendingArgs, a);
		if(result.getStatus () == Result::kUnresolved)
			return kResultFalse;

		results.add (result);
	}

	// Consume all positional args in order.
	bool optionalFound = false;
	for(const auto& a : args)
	{
		if(a.isShiftable ())
			continue;

		if(!checkUniqueChoices (a.getChoices ()))
			return kResultFailed;

		// Guard against misconfiguration: optional positional args must
		// always be at the end of the list to not break argument order.
		if(a.isOptional ())
			optionalFound = true;
		else
		{
			if(optionalFound)
				return kResultFailed;
		}

		Result result (a.getName ());
		resolve (result, pendingArgs, a);
		if(result.getStatus () == Result::kUnresolved)
			return kResultFalse;

		results.add (result);
	}

	// More arguments than expected.
	if(!pendingArgs.isEmpty ())
	{
		if(flags & kAllowUnknownArguments)
			unparsedArguments.addAllFrom (pendingArgs);
		else
			return kResultFalse;
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ArgumentParser::resolve (Result& data, StringList& pendingArgs, const Description& arg)
{
	ASSERT (data.getStatus () == Result::kUnresolved)

	// Attempt to match argument list.
	bool valueExpected = false;
	ForEach (pendingArgs, Boxed::String, str)
		bool matches = arg.matches (*str);
		if((matches && !arg.expectsValue ()) || valueExpected)
		{
			data.setValue (*str);
			data.setStatus (Result::kFound);

			// Consume matched argument values.
			pendingArgs.remove (*str);
			return;
		}
		else if(matches && arg.expectsValue ())
		{
			if(str->contains ("="))
			{
				data.setValue (Variant (str->subString (str->index ("=") + 1), true));
				data.setStatus (Result::kFound);
				pendingArgs.remove (*str);
				return;
			}
			else
				valueExpected = true;

			// Consume matched argument.
			pendingArgs.remove (*str);
			continue;
		}
	EndFor

	// Not found, fallback to default value for optionals.
	if(arg.isOptional ())
	{
		data.setStatus (Result::kNotFound);
		data.setValue (arg.getDefaultValue ());
	}
	else
		data.setStatus (Result::kUnresolved);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ArgumentParser::printUsage (System::IConsole& console, StringRef command, StringRef additionalArguments)
{
	static const String kExpectedArgString = " <...>";
	static const String kOptionalString = " optional";
	static const int kMaxChoicesLength = 10;

	auto getChoicesString = [&] (String& choices, const Description& a) -> bool
	{
		for(int i = 0; i < a.getChoices ().count (); i++)
		{
			if(i > 0)
				choices.append ("|");
			choices.append (a.getChoices ().at (i));
		}
		return choices.length () < kMaxChoicesLength || a.getChoices ().count () == 1;
	};

	String commandLine ("\t");
	commandLine.append (command);

	int maxLength = 0;
	for(const auto& a : args)
	{
		int length = 0;
		if(a.hasChoices ())
		{
			length += 3;
			commandLine.append (" [");
			String choices;
			if(!getChoicesString (choices, a))
				choices = a.getName ();
			commandLine.append (choices);
			length += choices.length ();

			if(a.expectsValue ())
			{
				commandLine.appendFormat (" <%(1)>", a.getName ());
				length += kExpectedArgString.length ();
			}
			commandLine.append ("]");
		}
		else
		{
			commandLine.appendFormat (" <%(1)>", a.getName ());
			length += a.getName ().length ();
		}

		if(a.isOptional ())
			length += kOptionalString.length ();

		maxLength = ccl_max (length, maxLength);
	}

	if(!additionalArguments.isEmpty ())
		commandLine.appendFormat (" %(1)", additionalArguments);
	console.writeLine (commandLine);
	console.writeLine ("");

	for(const auto& a : args)
	{
		String line ("\t");
		String choices;
		if(a.hasChoices ())
		{
			line.append ("[");
			if(getChoicesString (choices, a))
			{
				line.append (choices);
				choices.empty ();
			}
			else
				line.append (a.getName ());

			if(a.expectsValue ())
				line.append (kExpectedArgString);
			line.append ("]");
		}
		else
			line.appendFormat ("%(1)", a.getName ());

		if(a.isOptional ())
			line.append (kOptionalString);
		line.append (":");

		if(line.length () < maxLength)
			line.append (String (" ", maxLength - line.length ()));
		line.append ("\t");

		line.append (a.getDescription ());
		if(!choices.isEmpty ())
			line.appendFormat (" [%(1)]", choices);
		if(a.getDefaultValue ().isValid ())
			line.appendFormat (", default: %(1)", a.getDefaultValue ().toString ());

		console.writeLine (line);
	}
}
