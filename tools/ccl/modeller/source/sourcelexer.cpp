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
// Filename    : sourcelexer.cpp
// Description : Shared low-level C++ source tokenizer for ClassScanner/DocumentationScanner
//
//************************************************************************************************

#include "sourcelexer.h"

using namespace CCL;

//************************************************************************************************
// SourceLexer
//************************************************************************************************

SourceLexer::SourceLexer (CStringRef source)
: sourceText (source),
  pos (0),
  length (source.length ()),
  line (1),
  atDirectivePosition (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SourceLexer::isIdentStart (char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SourceLexer::isIdentChar (char c)
{
	return isIdentStart (c) || (c >= '0' && c <= '9');
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SourceLexer::isDigit (char c)
{
	return c >= '0' && c <= '9';
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SourceLexer::isSpace (char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

//////////////////////////////////////////////////////////////////////////////////////////////////

char SourceLexer::current () const
{
	return (pos < length) ? sourceText.at (pos) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

char SourceLexer::peek (int ahead) const
{
	int i = pos + ahead;
	return (i < length) ? sourceText.at (i) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SourceLexer::advance ()
{
	if(pos < length)
	{
		if(sourceText.at (pos) == '\n')
			line++;
		pos++;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SourceLexer::skipWhitespaceOnly ()
{
	while(pos < length && isSpace (current ()))
	{
		if(current () == '\n')
			atDirectivePosition = true;
		advance ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString SourceLexer::extractSpan (int startOffset, int count) const
{
	// Extract 'count' characters of 'sourceText' starting at 'startOffset', without CString::
	// subString()'s length() bounds check - which re-scans all of 'sourceText' (the whole file)
	// on every call, since it isn't aware of the offset being requested. This lexer already knows
	// 'startOffset'/'count' are in bounds, so that check is pure, expensive, per-token overhead.

	MutableCString result;
	if(count > 0)
		result.append (sourceText.str () + startOffset, count);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SourceLexer::tryLexComment (SourceToken& token)
{
	// Recognize a '//...' or '/*...*/' comment at the current position, string/char-literal-unaware
	// callers never reach here mid-literal since lexStringOrChar always consumes its literal whole.
	// Produces a kComment token whose 'text' is the comment's raw text, delimiters included - unlike
	// other token types, classifying a comment further (e.g. doxygen-relevance) is left to the
	// caller, since only the caller knows what kind of comment matters to it.

	if(current () != '/' || (peek () != '/' && peek () != '*'))
		return false;

	int startOffset = pos;
	int startLine = line;

	if(peek () == '/') // line comment
	{
		int contentEnd = pos;
		while(contentEnd < length && sourceText.at (contentEnd) != '\n')
			contentEnd++;

		token.type = SourceToken::kComment;
		token.text = extractSpan (startOffset, contentEnd - startOffset);
		token.line = startLine;
		token.offset = startOffset;

		pos = contentEnd; // stop before the newline, so it's still seen by the next advance()/token
		token.length = pos - startOffset;
		return true;
	}

	// Block comment.
	int contentEnd = pos + 2;
	while(contentEnd < length && !(sourceText.at (contentEnd) == '*' && contentEnd + 1 < length && sourceText.at (contentEnd + 1) == '/'))
		contentEnd++;
	int afterComment = (contentEnd < length) ? contentEnd + 2 : length; // past the closing "*/"

	token.type = SourceToken::kComment;
	token.text = extractSpan (startOffset, afterComment - startOffset);
	token.line = startLine;
	token.offset = startOffset;

	while(pos < afterComment)
		advance (); // character-at-a-time, so embedded newlines still advance 'line' correctly

	token.length = pos - startOffset;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SourceLexer::tryLexPreprocessorLine (SourceToken& token)
{
	// Recognizes a preprocessor directive line, joining any backslash-continued follow-on lines into
	// one token (stripping each line's '\' and surrounding whitespace, joined by a space) so a
	// multi-line macro body (e.g. the ADD_TYPELIB_MEMBER_* shortcuts) is visible whole, rather than
	// truncated to its first line. A directive with no continuation keeps its original single-line text.

	if(current () != '#' || !atDirectivePosition)
		return false;

	int startOffset = pos;
	int startLine = line;

	advance (); // past '#'
	int firstLineTextStart = pos;
	int firstLineTextEnd = firstLineTextStart;
	while(pos < length && current () != '\n')
	{
		char c = current ();
		if(c == '"' || c == '\'')
		{
			char quote = c;
			advance ();
			while(pos < length && current () != quote)
			{
				if(current () == '\\')
					advance ();
				if(pos < length)
					advance ();
			}
			if(pos < length)
				advance (); // past closing quote
			firstLineTextEnd = pos;
			continue;
		}
		if(c == '/' && peek () == '/')
			break; // trailing line comment: stop capturing text here, same as stripComments
		if(c == '/' && peek () == '*')
			break; // trailing block comment starting on the directive line: same accepted stop

		advance ();
		firstLineTextEnd = pos;
	}

	// Re-check ignoring trailing whitespace before a possible '\', matching maskPreprocessorDirectives.
	int firstLineTrimEnd = firstLineTextEnd;
	while(firstLineTrimEnd > firstLineTextStart && (sourceText.at (firstLineTrimEnd - 1) == ' ' || sourceText.at (firstLineTrimEnd - 1) == '\t' || sourceText.at (firstLineTrimEnd - 1) == '\r'))
		firstLineTrimEnd--;
	bool continues = firstLineTrimEnd > firstLineTextStart && sourceText.at (firstLineTrimEnd - 1) == '\\';

	if(!continues)
	{
		token.type = SourceToken::kPreprocessor;
		token.text = extractSpan (firstLineTextStart, firstLineTextEnd - firstLineTextStart);
		token.line = startLine;
		token.offset = startOffset;
		token.length = pos - startOffset;
		return true;
	}

	MutableCString joined = extractSpan (firstLineTextStart, firstLineTrimEnd - 1 - firstLineTextStart);

	while(continues)
	{
		if(pos < length)
			advance (); // past the newline ending this (continued) physical line
		int lineStart = pos;
		while(pos < length && current () != '\n')
			advance ();
		int lineTrimEnd = pos;
		while(lineTrimEnd > lineStart && (sourceText.at (lineTrimEnd - 1) == ' ' || sourceText.at (lineTrimEnd - 1) == '\t' || sourceText.at (lineTrimEnd - 1) == '\r'))
			lineTrimEnd--;
		continues = lineTrimEnd > lineStart && sourceText.at (lineTrimEnd - 1) == '\\';

		int lineTextEnd = continues ? lineTrimEnd - 1 : lineTrimEnd;
		int lineTextStart = lineStart;
		while(lineTextStart < lineTextEnd && (sourceText.at (lineTextStart) == ' ' || sourceText.at (lineTextStart) == '\t'))
			lineTextStart++; // leading indentation on continuation lines isn't meaningful

		joined.append (" ");
		joined.append (extractSpan (lineTextStart, lineTextEnd - lineTextStart));
	}

	token.type = SourceToken::kPreprocessor;
	token.text = joined;
	token.line = startLine;
	token.offset = startOffset;
	token.length = pos - startOffset;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SourceLexer::lexStringOrChar (SourceToken& token, char quote)
{
	// String/char literal: content between the quotes, verbatim (escapes not decoded) - matching
	// DocumentationScanner::Token's existing kString/kChar semantics exactly, so callers built on top
	// of this lexer (e.g. resolveStringArg's unescapeStringLiteral) keep working unchanged.

	int startOffset = pos;
	int startLine = line;

	advance (); // past opening quote
	int contentStart = pos;
	while(pos < length && current () != quote)
	{
		if(current () == '\\')
			advance ();
		if(pos < length)
			advance ();
	}
	int contentEnd = pos;
	if(pos < length)
		advance (); // past closing quote

	token.type = (quote == '"') ? SourceToken::kString : SourceToken::kChar;
	token.text = extractSpan (contentStart, contentEnd - contentStart);
	token.line = startLine;
	token.offset = startOffset;
	token.length = pos - startOffset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SourceLexer::lexIdentifier (SourceToken& token)
{
	int startOffset = pos;
	int startLine = line;
	while(pos < length && isIdentChar (current ()))
		advance ();

	token.type = SourceToken::kIdentifier;
	token.text = extractSpan (startOffset, pos - startOffset);
	token.line = startLine;
	token.offset = startOffset;
	token.length = pos - startOffset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SourceLexer::lexNumber (SourceToken& token)
{
	int startOffset = pos;
	int startLine = line;
	while(pos < length && isDigit (current ()))
		advance ();

	token.type = SourceToken::kNumber;
	token.text = extractSpan (startOffset, pos - startOffset);
	token.line = startLine;
	token.offset = startOffset;
	token.length = pos - startOffset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SourceLexer::lexOperator (SourceToken& token)
{
	int startOffset = pos;
	int startLine = line;
	char c = current ();
	advance ();

	if(c == ':' && current () == ':')
		advance (); // "::" captured as one two-character operator token, as today

	token.type = SourceToken::kOperator;
	token.text = extractSpan (startOffset, pos - startOffset);
	token.line = startLine;
	token.offset = startOffset;
	token.length = pos - startOffset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SourceLexer::skipToEndOfLine ()
{
	while(pos < length && current () != '\n')
		advance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringRef SourceLexer::text () const
{
	return sourceText;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SourceLexer::next (SourceToken& token)
{
	token = SourceToken ();

	skipWhitespaceOnly ();
	if(pos >= length)
		return false;

	// A comment counts as whitespace (as in translation phase 3), so it keeps 'atDirectivePosition',
	// e.g. for "/* XSTRING */#define ...". Any other token ends it until the next newline.
	if(tryLexComment (token))
		return true;

	if(!tryLexPreprocessorLine (token))
	{
		char c = current ();
		if(isDigit (c))
			lexNumber (token);
		else if(isIdentStart (c))
			lexIdentifier (token);
		else if(c == '"' || c == '\'')
			lexStringOrChar (token, c);
		else
			lexOperator (token);
	}

	atDirectivePosition = false;
	return true;
}
