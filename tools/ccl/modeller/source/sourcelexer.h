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
// Filename    : sourcelexer.h
// Description : Shared low-level C++ source tokenizer for ClassScanner/DocumentationScanner
//
//************************************************************************************************

#ifndef _ccl_sourcelexer_h
#define _ccl_sourcelexer_h

#include "ccl/public/text/cstring.h"

namespace CCL {

//************************************************************************************************
// SourceToken
/** One token produced by SourceLexer. */
//************************************************************************************************

struct SourceToken
{
	enum Type
	{
		kUndefined,		//< No token produced yet (default state).
		kIdentifier,	//< An identifier.
		kNumber,		//< A numeric literal.
		kString,		//< text is the literal's content, quotes stripped, escapes NOT decoded.
		kChar,			//< Same as kString, for '...' literals.
		kOperator,		//< Single-character, except "::" which is captured as one two-character token.
		kComment,		//< text is the comment's raw text, delimiters included (e.g. "// ..." or "/** ... */")
		kPreprocessor	//< text is the directive's first physical line, '#' stripped; continuations consumed but not included.
	};

	Type type;				//< The token's kind.
	MutableCString text;	//< The token's text - meaning depends on type, see the Type enumerators.
	int line;				//< 1-based line number where the token starts.
	int offset;				//< Byte offset of the token's first character in the lexer's source text.
	int length;				//< Byte length of the token's raw span in the source text (for slicing).

	SourceToken ()
	: type (kUndefined),
	  line (0),
	  offset (0),
	  length (0)
	{}
};

//************************************************************************************************
// SourceLexer
/** Tokenizes a file's text in one pass, tracking line numbers and byte offsets. Comments and
	preprocessor directives are surfaced as tokens (kComment/kPreprocessor) instead of being
	silently consumed, so a caller that needs them (doxygen extraction) can read them while one
	that doesn't (macro-call scanning) just skips over them - implemented once instead of per caller. */
//************************************************************************************************

class SourceLexer
{
public:
	static bool isIdentStart (char c);
	static bool isIdentChar (char c);

	SourceLexer (CStringRef sourceText);

	/** Advance to and return the next token, returns false at end of input. */
	bool next (SourceToken& token);

	/** Skip past the remainder of the current physical line (up to but not including the
		terminating newline), producing no token for the skipped content - for a caller that wants
		to treat the rest of a line as opaque (e.g. a "using ...;" declaration). */
	void skipToEndOfLine ();

	/** The full text being tokenized, e.g. for a caller (MacroCallScanner) that needs to slice an
		exact original source span between two tokens' offsets. */
	CStringRef text () const;

protected:
	static bool isDigit (char c);
	static bool isSpace (char c);

	MutableCString sourceText;
	int pos;
	int length;
	int line;
	bool atDirectivePosition;	//< only whitespace and comments since the last newline, a '#' here starts a directive

	char current () const;
	char peek (int ahead = 1) const;
	void advance ();
	void skipWhitespaceOnly ();
	MutableCString extractSpan (int startOffset, int count) const;
	bool tryLexComment (SourceToken& token);
	bool tryLexPreprocessorLine (SourceToken& token);
	void lexStringOrChar (SourceToken& token, char quote);
	void lexIdentifier (SourceToken& token);
	void lexNumber (SourceToken& token);
	void lexOperator (SourceToken& token);
};

} // namespace CCL

#endif // _ccl_sourcelexer_h
