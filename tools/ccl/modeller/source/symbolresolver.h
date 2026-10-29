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
// Filename    : symbolresolver.h
// Description : Collects #define/DEFINE_STRINGID(_MEMBER)/const-value/function-like #define
//               symbols into a SymbolRegistry (SymbolCollector), and resolves a macro argument
//               into its literal value against one (SymbolResolver), without compiling or
//               running the scanned code. Shared by ClassScanner and TypeLibScanner.
//
//************************************************************************************************

#ifndef _ccl_symbolresolver_h
#define _ccl_symbolresolver_h

#include "macrocallscanner.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/vector.h"

namespace CCL {
namespace SymbolResolverInternal {

//************************************************************************************************
// DefineSite
/** One #define declaration site. */
//************************************************************************************************

struct DefineSite
{
	MutableCString text;	///< The #define's replacement text, e.g. "\"Resources\"".
	MutableCString file;	///< Source file this declaration was found in.
	int line;				///< 1-based line number of the declaration.

	DefineSite ()
	: line (0)
	{}
};

//************************************************************************************************
// DefineEntry
/** All declaration sites found for one #define name. */
//************************************************************************************************

struct DefineEntry
{
	MutableCString name;		///< The #define's name, e.g. "TAG_RESOURCES".
	Vector<DefineSite> sites;	///< One entry per declaration site.
};

//************************************************************************************************
// StringIdSite
/** One DEFINE_STRINGID_MEMBER(_) declaration site. */
//************************************************************************************************

struct StringIdSite
{
	MutableCString owner;	///< The DEFINE_STRINGID_MEMBER's owning-class argument, e.g. "MetaClass".
	MutableCString text;	///< The string ID's replacement text, e.g. "\"color\"".
	MutableCString file;	///< Source file this declaration was found in.
	int line;				///< 1-based line number of the declaration.

	StringIdSite ()
	: line (0)
	{}
};

//************************************************************************************************
// StringIdEntry
/** All declaration sites found for one string-ID member name. */
//************************************************************************************************

struct StringIdEntry
{
	MutableCString member;		///< The member name shared by all matching DEFINE_STRINGID_MEMBER calls, e.g. "kColor".
	Vector<StringIdSite> sites;	///< One entry per declaration site.
};

//************************************************************************************************
// ConstStringSite
/** One const CStringPtr constant declaration site. */
//************************************************************************************************

struct ConstStringSite
{
	MutableCString owner;	///< The constant's enclosing (possibly nested) namespace, e.g. "StyleID::BIV", empty if none.
	MutableCString text;	///< Already-unescaped literal value, e.g. "Color" (quotes/escapes stripped).
	MutableCString file;	///< Source file this declaration was found in.
	int line;				///< 1-based line number of the declaration.

	ConstStringSite ()
	: line (0)
	{}
};

//************************************************************************************************
// ConstStringEntry
/** All declaration sites found for one const-string constant name. */
//************************************************************************************************

struct ConstStringEntry
{
	MutableCString name;			///< The constant's bare (unqualified) name, e.g. "kColor".
	Vector<ConstStringSite> sites;	///< One entry per declaration site.
};

//************************************************************************************************
// FunctionDefineSite
/** One function-like #define declaration site. */
//************************************************************************************************

struct FunctionDefineSite
{
	Vector<MutableCString> params;	///< Parameter names, in order, e.g. ["TagName"].
	MutableCString body;			///< Unexpanded replacement text, e.g. "TagName \".Children\"".
	MutableCString file;			///< Source file this declaration was found in.
	int line;						///< 1-based line number of the declaration.

	FunctionDefineSite ()
	: line (0)
	{}
};

//************************************************************************************************
// FunctionDefineEntry
/** All declaration sites found for one function-like macro name. */
//************************************************************************************************

struct FunctionDefineEntry
{
	MutableCString name;				///< The macro's name, e.g. "MAKE_SCHEMA_GROUP".
	Vector<FunctionDefineSite> sites;	///< One entry per declaration site.
};

} // namespace SymbolResolverInternal

using namespace SymbolResolverInternal;

//************************************************************************************************
// SymbolRegistry
/** Every #define, DEFINE_STRINGID(_MEMBER), const-string, and function-like #define found so far. */
//************************************************************************************************

class SymbolRegistry
{
public:
	/** Find the DefineEntry for 'name', or nullptr if none has been added yet. */
	DefineEntry* findDefine (CStringRef name) const;

	/** Find the StringIdEntry for 'member', or nullptr if none has been added yet. */
	StringIdEntry* findStringIdEntry (CStringRef member) const;

	/** Find the ConstStringEntry for 'name', or nullptr if none has been added yet. */
	ConstStringEntry* findConstStringEntry (CStringRef name) const;

	/** Find the FunctionDefineEntry for 'name', or nullptr if none has been added yet. */
	FunctionDefineEntry* findFunctionDefine (CStringRef name) const;

	/** Fold 'site' into the DefineEntry for 'name', creating it if this is the first site seen. */
	void addDefineSite (CStringRef name, const DefineSite& site);

	/** Fold 'site' into the StringIdEntry for 'member', creating it if this is the first site seen. */
	void addStringIdSite (CStringRef member, const StringIdSite& site);

	/** Fold 'site' into the ConstStringEntry for 'name', creating it if this is the first site seen. */
	void addConstStringSite (CStringRef name, const ConstStringSite& site);

	/** Fold 'site' into the FunctionDefineEntry for 'name', creating it if this is the first site seen. */
	void addFunctionDefineSite (CStringRef name, const FunctionDefineSite& site);

protected:
	Vector<DefineEntry> defines;					///< Every #define and DEFINE_STRINGID(_) found, by name.
	Vector<StringIdEntry> stringIdMembers;			///< Every DEFINE_STRINGID_MEMBER(_) found, by member name.
	Vector<ConstStringEntry> constStrings;			///< Every const CStringPtr constant found, by bare name.
	Vector<FunctionDefineEntry> functionDefines;	///< Every function-like #define found, by name.
};

//************************************************************************************************
// SymbolCollector
//************************************************************************************************

class SymbolCollector
{
public:
	SymbolCollector (SymbolRegistry& registry);

	/** Fold a freshly-scanned #define into the registry, grouped by name into sites. */
	void addDefine (const ScannedDefine& scanned, CStringRef fileName);

	/** Fold a freshly-scanned DEFINE_STRINGID(_MEMBER) call into the registry, same grouping as
		addDefine(). Returns false if 'call' lacks enough arguments for its macro name. */
	bool addStringIdSymbol (const ScannedMacroCall& call, CStringRef fileName, MutableCString& errorMsg);

	/** Scan 'source' for 'const CStringPtr kName = "literal";' declarations (e.g. TypeNames::kXxx
		constants) and fold each into the registry's const strings, by bare name. */
	void collectConstStringSymbols (CStringRef source, CStringRef fileName);

	/** Scan 'source' for every function-like #define (e.g. 'MAKE_SCHEMA_GROUP(TagName) ...') and
		fold each into the registry's function defines, for later substitution in
		SymbolResolver::resolveStringArg(). */
	void collectFunctionDefines (CStringRef source, CStringRef fileName);

protected:
	SymbolRegistry& symbols;
};

//************************************************************************************************
// SymbolResolver
//************************************************************************************************

class SymbolResolver
{
public:
	SymbolResolver (SymbolRegistry& registry);

	/** Resolve a macro argument into its literal string value, expanding #define/
		DEFINE_STRINGID(_MEMBER)/const-string identifiers and concatenating adjacent string literals. */
	bool resolveStringArg (MutableCString& result, MutableCString& errorMsg, CStringRef rawArg, CStringRef loc) const;

protected:
	struct ExpansionContext;

	SymbolRegistry& symbols;

	static bool tokenizeForResolution (Vector<MutableCString>& tokens, CStringRef arg);
	static MutableCString substituteParams (CStringRef body, const Vector<MutableCString>& params, const Vector<MutableCString>& args);
	static bool pickUnambiguousFunctionDefine (const FunctionDefineSite*& site, MutableCString& conflictLocations, const Vector<FunctionDefineSite>& sites);

	template <class SiteT>
	static bool pickUnambiguous (MutableCString& replacement, MutableCString& conflictLocations, const Vector<SiteT>& sites);

	bool expandToken (MutableCString& output, MutableCString& errorMsg, CStringRef tok, const ExpansionContext& context) const;
};

//************************************************************************************************
// SymbolResolver::ExpansionContext
/** Context threaded through expandToken()'s recursion, for error messages and cycle detection. */
//************************************************************************************************

struct SymbolResolver::ExpansionContext
{
	CStringRef loc;		///< "file:line" prefix for error messages, e.g. "foo.h:42".
	CStringRef rawText;	///< The original, unresolved argument text, for error messages.
	int depth;			///< Recursion depth, cycle-guarded (see expandToken()).

	ExpansionContext (CStringRef loc, CStringRef rawText, int depth = 0)
	: loc (loc),
	  rawText (rawText),
	  depth (depth)
	{}
};

} // namespace CCL

#endif // _ccl_symbolresolver_h
