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
// Filename    : macrocallscanner.h
// Description : Shared macro-call and #define extraction, built on SourceLexer
//
//************************************************************************************************

#ifndef _ccl_macrocallscanner_h
#define _ccl_macrocallscanner_h

#include "sourcelexer.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// ScannedMacroCall
//************************************************************************************************

struct ScannedMacroCall
{
	MutableCString name;
	Vector<MutableCString> args;
	int line = 0;
};

//************************************************************************************************
// ScannedDefine
//************************************************************************************************

struct ScannedDefine
{
	MutableCString name;
	MutableCString value; ///< unparsed replacement text, trimmed
	int line = 0;
};

//************************************************************************************************
// ScannedFunctionDefine
//************************************************************************************************

struct ScannedFunctionDefine
{
	MutableCString name;
	Vector<MutableCString> params; ///< parameter names, in order - empty for a zero-argument macro
	MutableCString body; ///< unparsed replacement text, trimmed
	int line = 0;
};

//************************************************************************************************
// MacroCallScanner
/** Tokenizes 'source' once via SourceLexer: extracts calls to any name in 'macroNames' (balanced-paren,
	comma-aware, so a nested call in an argument doesn't confuse the closing paren) and every object-like
	#define (function-like macros excluded). Shared by ClassScanner, DocumentationScanner, and
	TypeLibScanner. */
//************************************************************************************************

class MacroCallScanner
{
public:
	static bool scan (Vector<ScannedMacroCall>& outCalls, Vector<ScannedDefine>& outDefines,
		MutableCString& errorMsg, CStringRef source, const Vector<CStringPtr>& macroNames);

	/** Extract an object-like #define from a single kPreprocessor token. Excludes function-like
		macros; returns false if 'token' isn't a #define. */
	static bool extractDefine (ScannedDefine& outDefine, const SourceToken& token);

	/** Extract a function-like #define (name immediately followed by '(') from a single
		kPreprocessor token - the case extractDefine() excludes. Returns false if 'token' isn't one. */
	static bool extractFunctionDefine (ScannedFunctionDefine& outDefine, const SourceToken& token);

	/** Detects whether 'candidate' forwards its single parameter, unchanged, as the first argument
		of a call to 'targetMacroName' with exactly 'expectedArgCount' arguments - the shape used by
		a shortcut macro that expands to a more general one. On success, 'args' holds the matched
		call's remaining arguments (excluding the forwarded first one). */
	static bool matchSingleParamForward (Vector<MutableCString>& args, const ScannedFunctionDefine& candidate,
		CStringPtr targetMacroName, int expectedArgCount);

protected:
	static bool isRecognizedName (const Vector<CStringPtr>& macroNames, CStringRef name);
	static MutableCString rawSpan (CStringRef source, int startOffset, int endOffset);
	static int parseDefineNamePrefix (CStringRef text, MutableCString& outName);
};

} // namespace CCL

#endif // _ccl_macrocallscanner_h
