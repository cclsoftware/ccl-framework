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
// Filename    : symbolresolver.cpp
// Description : Collects symbols into a SymbolRegistry (SymbolCollector), and resolves a macro
//               argument into its literal value against one (SymbolResolver)
//
//************************************************************************************************

#include "symbolresolver.h"
#include "macrocallscanner.h"
#include "sourcelexer.h"
#include "sourcelocation.h"

using namespace CCL;
using namespace SymbolResolverInternal;

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString unescapeStringLiteral (CStringRef body)
{
	// Unescape the contents of a C++ string literal (\" and \\ only).

	MutableCString result;
	CStringPtr text = body.str ();
	int length = body.length ();
	for(int i = 0; i < length; i++)
	{
		if(text[i] == '\\' && i + 1 < length && (text[i + 1] == '"' || text[i + 1] == '\\'))
		{
			result += text[i + 1];
			i++;
		}
		else
			result += text[i];
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class SiteT>
void collectVisibleSites (Vector<SiteT>& pool, const Vector<SiteT>& sites, CStringRef useFile)
{
	// A #define in a .cpp file is only visible within that file - skip other files' local defines,
	// so an unrelated same-named #define elsewhere in the scanned tree doesn't make a reference
	// ambiguous. Falls back to all sites if none is visible from 'useFile'.

	for(int i = 0; i < sites.count (); i++)
		if(!sites.at (i).file.endsWith (".cpp") || sites.at (i).file == useFile)
			pool.add (sites.at (i));

	if(pool.isEmpty ())
		for(int i = 0; i < sites.count (); i++)
			pool.add (sites.at (i));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DefineEntry* SymbolRegistry::findDefine (CStringRef name) const
{
	for(int i = 0; i < defines.count (); i++)
		if(defines.at (i).name == name)
			return &defines.at (i);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringIdEntry* SymbolRegistry::findStringIdEntry (CStringRef member) const
{
	for(int i = 0; i < stringIdMembers.count (); i++)
		if(stringIdMembers.at (i).member == member)
			return &stringIdMembers.at (i);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ConstStringEntry* SymbolRegistry::findConstStringEntry (CStringRef name) const
{
	for(int i = 0; i < constStrings.count (); i++)
		if(constStrings.at (i).name == name)
			return &constStrings.at (i);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FunctionDefineEntry* SymbolRegistry::findFunctionDefine (CStringRef name) const
{
	for(int i = 0; i < functionDefines.count (); i++)
		if(functionDefines.at (i).name == name)
			return &functionDefines.at (i);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SymbolRegistry::addDefineSite (CStringRef name, const DefineSite& site)
{
	DefineEntry* entry = findDefine (name);
	if(entry == nullptr)
	{
		DefineEntry newEntry;
		newEntry.name = name;
		defines.add (newEntry);
		entry = &defines.at (defines.count () - 1);
	}
	entry->sites.add (site);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SymbolRegistry::addStringIdSite (CStringRef member, const StringIdSite& site)
{
	StringIdEntry* entry = findStringIdEntry (member);
	if(entry == nullptr)
	{
		StringIdEntry newEntry;
		newEntry.member = member;
		stringIdMembers.add (newEntry);
		entry = &stringIdMembers.at (stringIdMembers.count () - 1);
	}
	entry->sites.add (site);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SymbolRegistry::addConstStringSite (CStringRef name, const ConstStringSite& site)
{
	ConstStringEntry* entry = findConstStringEntry (name);
	if(entry == nullptr)
	{
		ConstStringEntry newEntry;
		newEntry.name = name;
		constStrings.add (newEntry);
		entry = &constStrings.at (constStrings.count () - 1);
	}
	entry->sites.add (site);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SymbolRegistry::addFunctionDefineSite (CStringRef name, const FunctionDefineSite& site)
{
	FunctionDefineEntry* entry = findFunctionDefine (name);
	if(entry == nullptr)
	{
		FunctionDefineEntry newEntry;
		newEntry.name = name;
		functionDefines.add (newEntry);
		entry = &functionDefines.at (functionDefines.count () - 1);
	}
	entry->sites.add (site);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SymbolCollector::SymbolCollector (SymbolRegistry& registry)
: symbols (registry)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SymbolCollector::addDefine (const ScannedDefine& scanned, CStringRef fileName)
{
	DefineSite site;
	site.text = scanned.value;
	site.file = fileName;
	site.line = scanned.line;
	symbols.addDefineSite (scanned.name, site);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SymbolCollector::addStringIdSymbol (const ScannedMacroCall& call, CStringRef fileName, MutableCString& errorMsg)
{
	MutableCString loc = SourceLocation::makeLocation (fileName, call.line);

	if(call.name == "DEFINE_STRINGID" || call.name == "DEFINE_STRINGID_")
	{
		if(call.args.count () < 2)
		{
			errorMsg.appendFormat ("%s: %s expected at least 2 arguments", loc.str (), call.name.str ());
			return false;
		}
		MutableCString name = call.args.at (0);
		name.trimWhitespace ();

		DefineSite site;
		site.text = call.args.at (1);
		site.file = fileName;
		site.line = call.line;
		symbols.addDefineSite (name, site);
	}
	else if(call.name == "DEFINE_STRINGID_MEMBER" || call.name == "DEFINE_STRINGID_MEMBER_")
	{
		if(call.args.count () < 3)
		{
			errorMsg.appendFormat ("%s: %s expected at least 3 arguments", loc.str (), call.name.str ());
			return false;
		}
		MutableCString className = call.args.at (0);
		className.trimWhitespace ();
		MutableCString memberName = call.args.at (1);
		memberName.trimWhitespace ();

		StringIdSite site;
		site.owner = className;
		site.text = call.args.at (2);
		site.file = fileName;
		site.line = call.line;
		symbols.addStringIdSite (memberName, site);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SymbolCollector::collectConstStringSymbols (CStringRef source, CStringRef fileName)
{
	// Scan 'source' for 'const CStringPtr kName = "literal";' declarations (e.g. TypeNames::kXxx
	// constants) and fold each into 'symbols.constStrings' by bare name, tagged with the enclosing
	// namespace. Recognizes this one narrow pattern directly over SourceLexer tokens, not a
	// general declaration parser.

	SourceLexer lexer (source);
	SourceToken token;

	// Namespace nesting via a brace stack; each entry is the namespace path before that brace.
	Vector<MutableCString> enclosingNamespaces;
	MutableCString currentNamespace;

	while(lexer.next (token))
	{
		if(token.type == SourceToken::kIdentifier && token.text == "namespace")
		{
			// Collect the (possibly nested, e.g. "A::B") name up to '{'; anything else ("using
			// namespace X;", a namespace alias) isn't a namespace scope.
			MutableCString name;
			bool opensNamespace = false;
			while(lexer.next (token))
			{
				if(token.type == SourceToken::kIdentifier || (token.type == SourceToken::kOperator && token.text == "::"))
				{
					name += token.text;
					continue;
				}
				opensNamespace = (token.type == SourceToken::kOperator && token.text == "{");
				break;
			}

			if(opensNamespace)
			{
				enclosingNamespaces.add (currentNamespace);
				if(!name.isEmpty ())
				{
					if(!currentNamespace.isEmpty ())
						currentNamespace += "::";
					currentNamespace += name;
				}
			}
			continue;
		}

		if(token.type == SourceToken::kOperator && token.text == "{")
		{
			enclosingNamespaces.add (currentNamespace);
			continue;
		}

		if(token.type == SourceToken::kOperator && token.text == "}")
		{
			if(!enclosingNamespaces.isEmpty ())
			{
				currentNamespace = enclosingNamespaces.last ();
				enclosingNamespaces.removeLast ();
			}
			continue;
		}

		if(token.type != SourceToken::kIdentifier || token.text != "const")
			continue;

		int declLine = token.line;

		if(!lexer.next (token) || token.type != SourceToken::kIdentifier || token.text != "CStringPtr")
			continue;

		if(!lexer.next (token) || token.type != SourceToken::kIdentifier)
			continue;
		MutableCString name = token.text;

		if(!lexer.next (token) || token.type != SourceToken::kOperator || token.text != "=")
			continue;

		if(!lexer.next (token) || token.type != SourceToken::kString)
			continue; // not a plain string-literal initializer - not this pattern, ignore

		MutableCString value = unescapeStringLiteral (token.text);

		if(!lexer.next (token) || token.type != SourceToken::kOperator || token.text != ";")
			continue; // not a simple "= \"...\";" statement - ignore

		ConstStringSite site;
		site.owner = currentNamespace;
		site.text = value;
		site.file = fileName;
		site.line = declLine;
		symbols.addConstStringSite (name, site);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SymbolCollector::collectFunctionDefines (CStringRef source, CStringRef fileName)
{
	// Scan 'source' for function-like #defines (e.g. 'MAKE_SCHEMA_GROUP(TagName) ...') and fold
	// each into 'symbols.functionDefines' for later substitution in SymbolResolver::resolveStringArg().

	SourceLexer lexer (source);
	SourceToken token;

	while(lexer.next (token))
	{
		if(token.type != SourceToken::kPreprocessor)
			continue;

		ScannedFunctionDefine scanned;
		if(!MacroCallScanner::extractFunctionDefine (scanned, token))
			continue;

		FunctionDefineSite site;
		site.params = scanned.params;
		site.body = scanned.body;
		site.file = fileName;
		site.line = scanned.line;
		symbols.addFunctionDefineSite (scanned.name, site);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SymbolResolver::SymbolResolver (SymbolRegistry& registry)
: symbols (registry)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SymbolResolver::tokenizeForResolution (Vector<MutableCString>& tokens, CStringRef arg)
{
	// Split 'arg' into string-literal, (possibly '::'-qualified) identifier, and function-like-
	// macro-call tokens, skipping whitespace. Returns false on any other character.

	// Find the closing paren matching the '(' at 'openParenPos', skipping over string-literal
	// contents. Returns -1 if unbalanced.
	auto findMatchingParen = [] (CStringPtr text, int openParenPos, int length) -> int
	{
		int depth = 0;
		for(int i = openParenPos; i < length; i++)
		{
			char c = text[i];
			if(c == '"')
			{
				i++;
				while(i < length && text[i] != '"')
				{
					if(text[i] == '\\')
						i++;
					i++;
				}
				continue;
			}
			if(c == '(')
				depth++;
			else if(c == ')')
			{
				depth--;
				if(depth == 0)
					return i;
			}
		}
		return -1;
	};

	CStringPtr text = arg.str ();
	int length = arg.length ();
	int i = 0;
	while(i < length)
	{
		char c = text[i];
		if(c == ' ' || c == '\t' || c == '\r' || c == '\n')
		{
			i++;
			continue;
		}
		if(c == '"')
		{
			// Keep the quotes as part of the token, so expandToken() can tell a literal from
			// a name to look up without needing a separate token-kind field.

			int start = i;
			int j = i + 1;
			while(j < length && text[j] != '"')
			{
				if(text[j] == '\\')
					j++;
				j++;
			}
			int end = (j < length) ? j + 1 : length;
			tokens.add (arg.subString (start, end - start));
			i = end;
			continue;
		}
		if(SourceLexer::isIdentStart (c))
		{
			int start = i;
			i++;
			while(i < length && SourceLexer::isIdentChar (text[i]))
				i++;

			// Keep absorbing "::identifier" suffixes, so "Foo::kBar" stays one token.

			while(i + 1 < length && text[i] == ':' && text[i + 1] == ':')
			{
				i += 2;
				while(i < length && SourceLexer::isIdentChar (text[i]))
					i++;
			}

			// A following '(' (skipping whitespace) makes this a function-like macro call, e.g.
			// "MAKE_SCHEMA_GROUP(TAG_RESOURCES)" - captured whole (name + balanced-paren args,
			// verbatim) as a single token.
			int afterName = i;
			while(afterName < length && (text[afterName] == ' ' || text[afterName] == '\t'))
				afterName++;
			if(afterName < length && text[afterName] == '(')
			{
				int closeParen = findMatchingParen (text, afterName, length);
				if(closeParen < 0)
					return false; // unbalanced parens

				tokens.add (arg.subString (start, closeParen + 1 - start));
				i = closeParen + 1;
				continue;
			}

			tokens.add (arg.subString (start, i - start));
			continue;
		}
		return false; // unexpected character
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString SymbolResolver::substituteParams (CStringRef body, const Vector<MutableCString>& params, const Vector<MutableCString>& args)
{
	// Token-level parameter substitution for a function-like macro's body, the way the C++
	// preprocessor would substitute a call's arguments.

	SourceLexer lexer (body);
	SourceToken token;
	MutableCString result;
	int lastEnd = 0;

	while(lexer.next (token))
	{
		if(token.offset > lastEnd)
			result.append (body.subString (lastEnd, token.offset - lastEnd));

		bool substituted = false;
		if(token.type == SourceToken::kIdentifier)
			for(int i = 0; i < params.count () && !substituted; i++)
				if(params.at (i) == token.text)
				{
					result.append (args.at (i));
					substituted = true;
				}

		if(!substituted)
		{
			if(token.type == SourceToken::kString)
				result.append ("\"").append (token.text).append ("\"");
			else
				result.append (token.text);
		}

		lastEnd = token.offset + token.length;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SymbolResolver::pickUnambiguousFunctionDefine (const FunctionDefineSite*& site, MutableCString& conflictLocations, const Vector<FunctionDefineSite>& sites)
{
	// Same idea as pickUnambiguous(), for FunctionDefineSite (compares both .params and .body).

	site = &sites.at (0); // tentative pick; only used if none of the sites below disagree with it
	bool ambiguous = false;
	conflictLocations.empty ();
	for(int i = 0; i < sites.count (); i++)
	{
		// Record every site's location, for the error message if this turns out ambiguous.
		if(i > 0)
			conflictLocations += ", ";
		conflictLocations += SourceLocation::makeLocation (sites.at (i).file, sites.at (i).line);

		// Same shape means identical parameter list and body text.
		bool sameShape = (sites.at (i).body == site->body) && (sites.at (i).params.count () == site->params.count ());
		if(sameShape)
			for(int p = 0; p < sites.at (i).params.count () && sameShape; p++)
				sameShape = (sites.at (i).params.at (p) == site->params.at (p));
		if(!sameShape)
			ambiguous = true;
	}
	return !ambiguous;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class SiteT>
bool SymbolResolver::pickUnambiguous (MutableCString& replacement, MutableCString& conflictLocations, const Vector<SiteT>& sites)
{
	// Pick the replacement text among same-named declarations; false if more than one distinct
	// text was found, with 'conflictLocations' listing every site.

	replacement = sites.at (0).text;
	bool ambiguous = false;
	conflictLocations.empty ();
	for(int i = 0; i < sites.count (); i++)
	{
		if(i > 0)
			conflictLocations += ", ";
		conflictLocations += SourceLocation::makeLocation (sites.at (i).file, sites.at (i).line);
		if(sites.at (i).text != replacement)
			ambiguous = true;
	}

	return !ambiguous;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SymbolResolver::expandToken (MutableCString& output, MutableCString& errorMsg, CStringRef tok, const ExpansionContext& context) const
{
	// Cycle guard against runaway recursive expansion.
	if(context.depth > 20)
	{
		errorMsg.appendFormat ("%s: macro expansion too deep while resolving '%s' (possible cyclic #define/DEFINE_STRINGID_MEMBER)",
			context.loc.str (), context.rawText.str ());
		return false;
	}

	// A literal string token - unescape and emit as-is, nothing to look up.
	if(tok.startsWith ("\""))
	{
		MutableCString body = tok.subString (1, tok.length () - 2);
		output += unescapeStringLiteral (body);
		return true;
	}

	// A function-like macro call, e.g. "MAKE_SCHEMA_GROUP (x)" - look up its definition, substitute
	// the call's arguments for its parameters, and recursively expand the result.
	if(tok.lastChar () == ')' && tok.contains ("("))
	{
		int parenPos = tok.index ("(");
		MutableCString funcName = tok.subString (0, parenPos);
		funcName.trimWhitespace (); // the '(' may be preceded by whitespace, e.g. "MAKE_SCHEMA_GROUP (x)"

		FunctionDefineEntry* funcDef = symbols.findFunctionDefine (funcName);
		if(funcDef == nullptr)
		{
			errorMsg.appendFormat ("%s: unresolved function-like macro '%s' while resolving '%s'",
				context.loc.str (), funcName.str (), context.rawText.str ());
			return false;
		}

		Vector<ScannedMacroCall> calls;
		Vector<ScannedDefine> nestedDefines; // unused, required by MacroCallScanner::scan
		MutableCString callErr;
		Vector<CStringPtr> targetNames;
		targetNames.add (funcName.str ());
		if(!MacroCallScanner::scan (calls, nestedDefines, callErr, tok, targetNames) || calls.count () != 1)
		{
			errorMsg.appendFormat ("%s: cannot parse function-like macro call '%s' while resolving '%s'",
				context.loc.str (), tok.str (), context.rawText.str ());
			return false;
		}
		const ScannedMacroCall& call = calls.at (0);

		Vector<FunctionDefineSite> pool;
		collectVisibleSites (pool, funcDef->sites, SourceLocation::toLocationFile (context.loc));

		const FunctionDefineSite* site = nullptr;
		MutableCString conflictLocations;
		if(!pickUnambiguousFunctionDefine (site, conflictLocations, pool))
		{
			errorMsg.appendFormat ("%s: ambiguous function-like macro '%s' while resolving '%s' (%s)",
				context.loc.str (), funcName.str (), context.rawText.str (), conflictLocations.str ());
			return false;
		}
		if(call.args.count () != site->params.count ())
		{
			errorMsg.appendFormat ("%s: '%s' expects %d argument(s), got %d, while resolving '%s'",
				context.loc.str (), funcName.str (), site->params.count (), call.args.count (), context.rawText.str ());
			return false;
		}

		MutableCString substituted = substituteParams (site->body, site->params, call.args);

		Vector<MutableCString> subTokens;
		if(!tokenizeForResolution (subTokens, substituted))
		{
			errorMsg.appendFormat ("%s: cannot parse argument text '%s'", context.loc.str (), substituted.str ());
			return false;
		}
		ExpansionContext nested {context.loc, context.rawText, context.depth + 1};
		for(int i = 0; i < subTokens.count (); i++)
			if(!expandToken (output, errorMsg, subTokens.at (i), nested))
				return false;
		return true;
	}

	// A plain #define - substitute its replacement text and recursively expand it.
	DefineEntry* def = symbols.findDefine (tok);
	if(def != nullptr)
	{
		Vector<DefineSite> pool;
		collectVisibleSites (pool, def->sites, SourceLocation::toLocationFile (context.loc));

		MutableCString replacement, conflictLocations;
		if(!pickUnambiguous (replacement, conflictLocations, pool))
		{
			errorMsg.appendFormat ("%s: ambiguous #define for '%s' while resolving '%s' (%s)",
				context.loc.str (), tok.str (), context.rawText.str (), conflictLocations.str ());
			return false;
		}

		Vector<MutableCString> subTokens;
		if(!tokenizeForResolution (subTokens, replacement))
		{
			errorMsg.appendFormat ("%s: cannot parse argument text '%s'", context.loc.str (), replacement.str ());
			return false;
		}
		ExpansionContext nested {context.loc, context.rawText, context.depth + 1};
		for(int i = 0; i < subTokens.count (); i++)
			if(!expandToken (output, errorMsg, subTokens.at (i), nested))
				return false;
		return true;
	}

	// Split off an optional "Owner::member" qualifier, e.g. "TransportIds::kPlay".
	MutableCString qualifier;
	MutableCString member (tok);
	bool hasQualifier = false;
	int sepPos = tok.lastIndex (':');
	if(sepPos > 0 && tok.at (sepPos - 1) == ':')
	{
		qualifier = tok.subString (0, sepPos - 1);
		member = tok.subString (sepPos + 1);
		hasQualifier = true;
	}

	// A DEFINE_STRINGID_MEMBER - narrow to the qualifier's owner if given, else consider every
	// owner, then substitute its replacement text and recursively expand it.
	StringIdEntry* entry = symbols.findStringIdEntry (member);
	if(entry != nullptr)
	{
		Vector<StringIdSite> pool;
		if(hasQualifier)
			for(int i = 0; i < entry->sites.count (); i++)
				if(entry->sites.at (i).owner == qualifier)
					pool.add (entry->sites.at (i));
		if(pool.isEmpty ())
			for(int i = 0; i < entry->sites.count (); i++)
				pool.add (entry->sites.at (i));

		MutableCString label = member;
		if(hasQualifier)
		{
			label = qualifier;
			label += "::";
			label += member;
		}

		MutableCString replacement, conflictLocations;
		if(!pickUnambiguous (replacement, conflictLocations, pool))
		{
			errorMsg.appendFormat ("%s: ambiguous DEFINE_STRINGID_MEMBER for '%s' while resolving '%s' (%s)",
				context.loc.str (), label.str (), context.rawText.str (), conflictLocations.str ());
			return false;
		}

		Vector<MutableCString> subTokens;
		if(!tokenizeForResolution (subTokens, replacement))
		{
			errorMsg.appendFormat ("%s: cannot parse argument text '%s'", context.loc.str (), replacement.str ());
			return false;
		}
		ExpansionContext nested {context.loc, context.rawText, context.depth + 1};
		for(int i = 0; i < subTokens.count (); i++)
			if(!expandToken (output, errorMsg, subTokens.at (i), nested))
				return false;
		return true;
	}

	// 'member' may be a plain C++ constant rather than a #define or DEFINE_STRINGID_MEMBER - narrow
	// to the qualifier's namespace if given (exact or "::"-suffix match, since a "using namespace"
	// at the reference site may drop an outer namespace), else consider every site.

	ConstStringEntry* constStr = symbols.findConstStringEntry (member);
	if(constStr != nullptr)
	{
		Vector<ConstStringSite> pool;
		if(hasQualifier)
		{
			MutableCString qualifierSuffix ("::");
			qualifierSuffix += qualifier;
			for(int i = 0; i < constStr->sites.count (); i++)
			{
				CStringRef owner = constStr->sites.at (i).owner;
				if(owner == qualifier || owner.endsWith (qualifierSuffix.str ()))
					pool.add (constStr->sites.at (i));
			}
		}
		if(pool.isEmpty ())
			for(int i = 0; i < constStr->sites.count (); i++)
				pool.add (constStr->sites.at (i));

		MutableCString label = member;
		if(hasQualifier)
		{
			label = qualifier;
			label += "::";
			label += member;
		}

		MutableCString replacement, conflictLocations;
		if(!pickUnambiguous (replacement, conflictLocations, pool))
		{
			errorMsg.appendFormat ("%s: ambiguous constant for '%s' while resolving '%s' (%s)",
				context.loc.str (), label.str (), context.rawText.str (), conflictLocations.str ());
			return false;
		}
		output += replacement;
		return true;
	}

	// Not a string literal, function-like macro, #define, DEFINE_STRINGID_MEMBER, or known constant.
	errorMsg.appendFormat ("%s: unresolved identifier '%s' while resolving argument '%s' (no matching #define, DEFINE_STRINGID_MEMBER, or const CStringPtr constant)",
		context.loc.str (), tok.str (), context.rawText.str ());

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SymbolResolver::resolveStringArg (MutableCString& result, MutableCString& errorMsg, CStringRef rawArg, CStringRef loc) const
{
	MutableCString raw = rawArg;
	raw.trimWhitespace ();
	if(raw == "nullptr" || raw == "NULL" || raw == "0")
	{
		result.empty ();
		return true;
	}

	Vector<MutableCString> tokens;
	if(!tokenizeForResolution (tokens, raw))
	{
		errorMsg.appendFormat ("%s: cannot parse argument text '%s' (unexpected character)", loc.str (), raw.str ());
		return false;
	}

	result.empty ();
	ExpansionContext context {loc, raw, 0};
	for(int i = 0; i < tokens.count (); i++)
		if(!expandToken (result, errorMsg, tokens.at (i), context))
			return false;

	return true;
}
