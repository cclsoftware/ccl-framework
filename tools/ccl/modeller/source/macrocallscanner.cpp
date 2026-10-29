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
// Filename    : macrocallscanner.cpp
// Description : Shared macro-call and #define extraction, built on SourceLexer
//
//************************************************************************************************

#include "macrocallscanner.h"
#include "sourcelexer.h"

using namespace CCL;

//************************************************************************************************
// MacroCallScanner
//************************************************************************************************

bool MacroCallScanner::scan (Vector<ScannedMacroCall>& outCalls, Vector<ScannedDefine>& outDefines,
	MutableCString& errorMsg, CStringRef source, const Vector<CStringPtr>& macroNames)
{
	Vector<SourceToken> tokens;
	{
		SourceLexer lexer (source);
		SourceToken token;
		while(lexer.next (token))
			tokens.add (token);
	}

	int count = tokens.count ();
	int i = 0;
	while(i < count)
	{
		const SourceToken& token = tokens.at (i);

		if(token.type == SourceToken::kPreprocessor)
		{
			ScannedDefine define;
			if(extractDefine (define, token))
				outDefines.add (define);
			i++;
			continue;
		}

		if(token.type != SourceToken::kIdentifier || !isRecognizedName (macroNames, token.text))
		{
			i++;
			continue;
		}

		// An identifier is only a call if '(' follows, possibly after whitespace/comments.
		int openParenIdx = i + 1;
		while(openParenIdx < count && tokens.at (openParenIdx).type == SourceToken::kComment)
			openParenIdx++;
		if(openParenIdx >= count || tokens.at (openParenIdx).type != SourceToken::kOperator || tokens.at (openParenIdx).text != "(")
		{
			i++;
			continue;
		}

		// Find the matching close paren and every top-level (depth == 1) comma, tracking depth via
		// operator tokens only - a nested call inside an argument, e.g. DEFINE_METHOD_NAME
		// (Foo::bar ()), can't confuse this, since strings/identifiers are already opaque tokens.

		int depth = 1;
		int closeParenIdx = -1;
		Vector<int> commaTokenIndices;
		int k = openParenIdx + 1;
		while(k < count)
		{
			const SourceToken& t = tokens.at (k);
			if(t.type == SourceToken::kOperator && t.text == "(")
				depth++;
			else if(t.type == SourceToken::kOperator && t.text == ")")
			{
				depth--;
				if(depth == 0)
				{
					closeParenIdx = k;
					break;
				}
			}
			else if(t.type == SourceToken::kOperator && t.text == "," && depth == 1)
				commaTokenIndices.add (k);
			k++;
		}

		if(closeParenIdx < 0)
		{
			errorMsg.appendFormat ("unbalanced parentheses in macro call starting at line %d", token.line);
			return false;
		}

		int innerStart = tokens.at (openParenIdx).offset + tokens.at (openParenIdx).length;
		int innerEnd = tokens.at (closeParenIdx).offset;

		ScannedMacroCall call;
		call.name = token.text;
		call.line = token.line;

		MutableCString wholeInner = rawSpan (source, innerStart, innerEnd);
		if(!wholeInner.isEmpty ())
		{
			int partStart = innerStart;
			for(int c = 0; c < commaTokenIndices.count (); c++)
			{
				const SourceToken& comma = tokens.at (commaTokenIndices.at (c));
				call.args.add (rawSpan (source, partStart, comma.offset));
				partStart = comma.offset + comma.length;
			}
			call.args.add (rawSpan (source, partStart, innerEnd)); // final part, always added -
				// matches splitTopLevelCommas: a trailing comma still yields an empty final argument.
		}

		outCalls.add (call);
		i = closeParenIdx + 1; // resume right after the call's ')', skipping over its arguments.
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MacroCallScanner::extractDefine (ScannedDefine& outDefine, const SourceToken& token)
{
	// Matches ClassScanner's original scanDefines exactly: function-like macros (name immediately
	// followed by '(') are excluded.

	MutableCString name;
	int cursor = parseDefineNamePrefix (token.text, name);
	int length = token.text.length ();
	if(cursor < 0 || (cursor < length && token.text.str ()[cursor] == '(')) // object-like macros only
		return false;

	outDefine.name = name;
	outDefine.value = token.text.subString (cursor, length - cursor);
	outDefine.value.trimWhitespace ();
	outDefine.line = token.line;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MacroCallScanner::extractFunctionDefine (ScannedFunctionDefine& outDefine, const SourceToken& token)
{
	// Same "define NAME" parsing as extractDefine, but for the function-like case (name immediately
	// followed by '(') that extractDefine deliberately excludes.

	MutableCString name;
	int cursor = parseDefineNamePrefix (token.text, name);
	CStringPtr text = token.text.str ();
	int length = token.text.length ();
	if(cursor < 0 || cursor >= length || text[cursor] != '(') // function-like macros only
		return false;

	cursor++; // past '('

	// Parameter names are plain identifiers - no nested parens expected, but tracked anyway so a
	// stray '(' in a malformed macro doesn't misidentify the closing ')' of the parameter list.

	Vector<MutableCString> params;
	int depth = 1;
	int paramStart = cursor;
	bool closed = false;
	while(cursor < length)
	{
		char c = text[cursor];
		if(c == '(')
		{
			depth++;
			cursor++;
		}
		else if(c == ')')
		{
			depth--;
			if(depth == 0)
			{
				MutableCString param = token.text.subString (paramStart, cursor - paramStart);
				param.trimWhitespace ();
				if(!param.isEmpty ())
					params.add (param);
				cursor++; // past ')'
				closed = true;
				break;
			}
			cursor++;
		}
		else if(c == ',' && depth == 1)
		{
			MutableCString param = token.text.subString (paramStart, cursor - paramStart);
			param.trimWhitespace ();
			params.add (param);
			paramStart = cursor + 1;
			cursor++;
		}
		else
			cursor++;
	}

	if(!closed)
		return false; // unbalanced parameter list

	outDefine.name = name;
	outDefine.params = params;
	outDefine.body = token.text.subString (cursor, length - cursor);
	outDefine.body.trimWhitespace ();
	outDefine.line = token.line;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MacroCallScanner::matchSingleParamForward (Vector<MutableCString>& args, const ScannedFunctionDefine& candidate,
	CStringPtr targetMacroName, int expectedArgCount)
{
	if(candidate.params.count () != 1)
		return false;

	Vector<ScannedMacroCall> calls;
	Vector<ScannedDefine> nestedDefines; // unused, required by scan()
	MutableCString errorMsg;
	Vector<CStringPtr> targetNames;
	targetNames.add (targetMacroName);

	if(!scan (calls, nestedDefines, errorMsg, candidate.body, targetNames))
		return false;
	if(calls.count () != 1)
		return false; // not a single, direct forward to the target macro

	const ScannedMacroCall& call = calls.at (0);
	if(call.args.count () != expectedArgCount)
		return false;
	if(call.args.at (0) != candidate.params.at (0))
		return false; // first argument must be exactly the macro's own parameter, passed straight through

	for(int i = 1; i < call.args.count (); i++)
		args.add (call.args.at (i));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MacroCallScanner::isRecognizedName (const Vector<CStringPtr>& macroNames, CStringRef name)
{
	for(CStringPtr candidate : macroNames)
		if(name == candidate)
			return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString MacroCallScanner::rawSpan (CStringRef source, int startOffset, int endOffset)
{
	// A straight substring, not a token-by-token reconstruction - so e.g. a comment embedded in an
	// argument (DEFINE_METHOD_NAME (Foo /* bar */)) would appear verbatim. Accepted, since that
	// case doesn't occur in this codebase's macro calls.
	//
	// Built via append(ptr, count) rather than CString::subString(), which bounds-checks against
	// length() - an O(source length) scan - on every call; 'source' here is the whole file's text,
	// so subString() would re-scan it in full for every argument extracted.

	MutableCString result;
	if(endOffset > startOffset)
		result.append (source.str () + startOffset, endOffset - startOffset);
	result.trimWhitespace ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MacroCallScanner::parseDefineNamePrefix (CStringRef text, MutableCString& outName)
{
	// Parses the "#define NAME" prefix shared by extractDefine/extractFunctionDefine, requiring a
	// word boundary after "define" (so e.g. "#defineFoo" isn't mistaken for a directive). Returns
	// the cursor right after 'outName', or -1 if malformed.

	CStringPtr str = text.str ();
	int length = text.length ();
	int cursor = 0;
	while(cursor < length && (str[cursor] == ' ' || str[cursor] == '\t'))
		cursor++;

	static const char kDefine[] = "define";
	int matchCount = 0;
	while(matchCount < 6 && cursor < length && str[cursor] == kDefine[matchCount])
	{
		cursor++;
		matchCount++;
	}
	if(matchCount != 6 || (cursor < length && str[cursor] != ' ' && str[cursor] != '\t'))
		return -1;

	while(cursor < length && (str[cursor] == ' ' || str[cursor] == '\t'))
		cursor++;
	int nameStart = cursor;
	while(cursor < length && ((str[cursor] >= 'a' && str[cursor] <= 'z') || (str[cursor] >= 'A' && str[cursor] <= 'Z')
		|| (str[cursor] >= '0' && str[cursor] <= '9') || str[cursor] == '_'))
		cursor++;

	if(cursor == nameStart)
		return -1;

	outName = text.subString (nameStart, cursor - nameStart);
	return cursor;
}
