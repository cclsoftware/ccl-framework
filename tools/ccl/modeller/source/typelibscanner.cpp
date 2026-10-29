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
// Filename    : typelibscanner.cpp
// Description : Static Type Library Source Scanner
//
//************************************************************************************************

#include "typelibscanner.h"
#include "ccl/extras/modeling/classrepository.h"
#include "macrocallscanner.h"
#include "sourcefiletraverser.h"
#include "sourcelexer.h"
#include "sourcelocation.h"
#include "symbolresolver.h"

#include "ccl/base/storage/textfile.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/system/logging.h"

using namespace CCL;
using namespace Model;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Recognized macro names (see ccl/base/typelibmacros.h)
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace TypeLibScannerInternal {

const CStringPtr kMacroNames[] =
{
	"DEFINE_TYPELIB_CLASS", "DEFINE_TYPELIB_CLASS_WITH_PARENT",
	"DEFINE_TYPELIB_ENTRY", "DEFINE_TYPELIB_ENTRY_LOCAL", "DEFINE_TYPELIB_ENTRY_WITH_PARENT",
	"DEFINE_TYPELIB_ABSTRACT", "DEFINE_TYPELIB_DOCGROUP",
	"BEGIN_TYPELIB_MEMBERS", "END_TYPELIB_MEMBERS",
	"ADD_TYPELIB_MEMBER", "ADD_TYPELIB_MEMBER_WITH_TYPE", "ADD_TYPELIB_MEMBER_BOOL",
	"ADD_TYPELIB_MEMBER_ENUM", "ADD_TYPELIB_MEMBER_FLOAT", "ADD_TYPELIB_MEMBER_INT",
	"ADD_TYPELIB_MEMBER_OBJECT", "ADD_TYPELIB_MEMBER_STRING",
	"BEGIN_TYPELIB_ATTRIBUTES", "END_TYPELIB_ATTRIBUTES",
	"ADD_TYPELIB_ATTRIBUTE", "ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE", "ADD_TYPELIB_CHILDGROUP_ATTRIBUTE",
	"DEFINE_TYPELIB_ENUMERATION", "DEFINE_TYPELIB_ENUMERATION_WITH_PARENT",
	"DEFINE_STRINGID", "DEFINE_STRINGID_", "DEFINE_STRINGID_MEMBER", "DEFINE_STRINGID_MEMBER_"
};

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<CStringPtr>& recognizedMacroNames ()
{
	static Vector<CStringPtr> names;
	if(names.isEmpty ())
		for(CStringPtr name : kMacroNames)
			names.add (name);

	return names;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

class FileCollector: public SourceFileVisitor
{
public:
	Vector<Url> files;

	void visitFile (UrlRef path) override
	{
		files.add (path);
	}
};

//************************************************************************************************
// MemberMacroAlias
/** A discovered custom ADD_TYPELIB_MEMBER_* shortcut #define, e.g. ADD_TYPELIB_MEMBER_COLOR in
	guitypelib.h - same discovery technique as TypeLibDocuScanner::AliasCollector. */
//************************************************************************************************

struct MemberMacroAlias
{
	MutableCString name;			///< The shortcut macro's name, e.g. "ADD_TYPELIB_MEMBER_COLOR".
	DataType dataType;				///< The member's data type this shortcut expands to, e.g. ITypeInfo::kObject.
	MutableCString typeNameRaw;		///< Unresolved type-name argument, resolved once per use, e.g. "TypeNames::kColor".

	MemberMacroAlias ()
	: dataType (ITypeInfo::kString)
	{}
};

//************************************************************************************************
// EntryDefSite
/** One DEFINE_TYPELIB_CLASS(_WITH_PARENT)/DEFINE_TYPELIB_ENTRY(_LOCAL/_WITH_PARENT) call site. */
//************************************************************************************************

struct EntryDefSite
{
	MutableCString uniqueName;			///< Identity used by BEGIN_TYPELIB_MEMBERS/_ABSTRACT/_DOCGROUP/_ATTRIBUTES, e.g. "AnimationElement".
	bool classBacked;					///< True for DEFINE_TYPELIB_CLASS(_WITH_PARENT), false for a plain DEFINE_TYPELIB_ENTRY(...).
	MutableCString tagNameRaw;			///< Unresolved tag-name argument, e.g. "TAG_ANIMATION".
	bool hasParent;						///< True if a parent-entry argument was given.
	MutableCString parentUniqueName;	///< Bare identity of the parent entry - resolved by lookup, not resolveStringArg.
	MutableCString libraryToken;		///< Trailing 'library' argument, raw macro-name text, verbatim, e.g. "StudioProTypeLib".
	MutableCString file;				///< Source file this call site was found in.
	int line;							///< 1-based line number of the call site.

	EntryDefSite ()
	: classBacked (false),
	  hasParent (false),
	  line (0)
	{}
};

//************************************************************************************************
// AbstractSite
/** One DEFINE_TYPELIB_ABSTRACT(uniqueName) call site. */
//************************************************************************************************

struct AbstractSite
{
	MutableCString uniqueName;	///< Identity of the entry this marks abstract, e.g. "AnimationElement".
	MutableCString file;		///< Source file this call site was found in.
	int line;					///< 1-based line number of the call site.

	AbstractSite ()
	: line (0)
	{}
};

//************************************************************************************************
// DocGroupSite
/** One DEFINE_TYPELIB_DOCGROUP(uniqueName, groupName) call site. */
//************************************************************************************************

struct DocGroupSite
{
	MutableCString uniqueName;		///< Identity of the entry this assigns a doc group to.
	MutableCString groupNameRaw;	///< Unresolved doc-group-name argument, e.g. "TAG_ANIMATION_GROUP".
	MutableCString file;			///< Source file this call site was found in.
	int line;						///< 1-based line number of the call site.

	DocGroupSite ()
	: line (0)
	{}
};

//************************************************************************************************
// MemberItem
/** One ADD_TYPELIB_MEMBER(_WITH_TYPE/_BOOL/_ENUM/_FLOAT/_INT/_OBJECT/_STRING) item within a
	BEGIN_TYPELIB_MEMBERS block. */
//************************************************************************************************

struct MemberItem
{
	MutableCString nameRaw;			///< Unresolved member-name argument, e.g. "TAG_COLOR".
	DataType dataType;				///< The member's data type, e.g. ITypeInfo::kInt.
	MutableCString typeNameRaw;		///< Unresolved type-name argument, used when fixedTypeName is null.
	CStringPtr fixedTypeName;		///< The real TypeNames::kXxx constant for a built-in shortcut, already resolved, used verbatim; null otherwise.

	MemberItem ()
	: dataType (ITypeInfo::kString),
	  fixedTypeName (nullptr)
	{}
};

//************************************************************************************************
// MembersBlockSite
/** One BEGIN_TYPELIB_MEMBERS(uniqueName) ... END_TYPELIB_MEMBERS block. */
//************************************************************************************************

struct MembersBlockSite
{
	MutableCString uniqueName;	///< Identity of the entry this member block belongs to.
	Vector<MemberItem> items;	///< One entry per ADD_TYPELIB_MEMBER* call within the block.
	MutableCString file;		///< Source file the block's BEGIN_TYPELIB_MEMBERS call was found in.
	int line;					///< 1-based line number of the block's BEGIN_TYPELIB_MEMBERS call.

	MembersBlockSite ()
	: line (0)
	{}
};

//************************************************************************************************
// AttributeItem
/** One ADD_TYPELIB_ATTRIBUTE(_SCHEMAGROUP/_CHILDGROUP) item within a BEGIN_TYPELIB_ATTRIBUTES block. */
//************************************************************************************************

struct AttributeItem
{
	MutableCString keyRaw;		///< Unresolved key argument, used when fixedKey is null.
	CStringPtr fixedKey;		///< kClassSchemaGroups/kClassChildGroup constant, already resolved, used verbatim; null otherwise.
	MutableCString valueRaw;	///< Unresolved value argument, e.g. "TAG_RESOURCES".

	AttributeItem ()
	: fixedKey (nullptr)
	{}
};

//************************************************************************************************
// AttributesBlockSite
/** One BEGIN_TYPELIB_ATTRIBUTES(uniqueName) ... END_TYPELIB_ATTRIBUTES block. */
//************************************************************************************************

struct AttributesBlockSite
{
	MutableCString uniqueName;		///< Identity of the entry this attribute block belongs to.
	Vector<AttributeItem> items;	///< One entry per ADD_TYPELIB_ATTRIBUTE* call within the block.
	MutableCString file;			///< Source file the block's BEGIN_TYPELIB_ATTRIBUTES call was found in.
	int line;						///< 1-based line number of the block's BEGIN_TYPELIB_ATTRIBUTES call.

	AttributesBlockSite ()
	: line (0)
	{}
};

//************************************************************************************************
// EnumerationSite
/** One DEFINE_TYPELIB_ENUMERATION(_WITH_PARENT) call site. */
//************************************************************************************************

struct EnumerationSite
{
	MutableCString ownerTagNameRaw;			///< Unresolved owner-tag-name argument, e.g. "TAG_ANIMATION".
	MutableCString memberNameRaw;			///< Unresolved member-name argument, e.g. "TAG_TIMING_TYPE".
	MutableCString enumDefName;				///< Bare identity of a BEGIN_STYLEDEF/BEGIN_ENUMINFO table, matched by name - empty if "nullptr".
	bool hasParent;							///< True if a parent owner/member argument pair was given.
	MutableCString parentOwnerTagNameRaw;	///< Unresolved parent owner-tag-name argument.
	MutableCString parentMemberNameRaw;		///< Unresolved parent member-name argument.
	MutableCString libraryToken;			///< Trailing 'library' argument, raw macro-name text, verbatim.
	MutableCString file;					///< Source file this call site was found in.
	int line;								///< 1-based line number of the call site.

	EnumerationSite ()
	: hasParent (false),
	  line (0)
	{}
};

//************************************************************************************************
// ValueTableItem
/** One {label} row from a BEGIN_STYLEDEF/BEGIN_ENUMINFO block; the runtime value is intentionally
	not exported. */
//************************************************************************************************

struct ValueTableItem
{
	MutableCString labelRaw;	///< Unresolved label argument, e.g. "TAG_LINEAR".
};

//************************************************************************************************
// ValueTableSite
/** One BEGIN_STYLEDEF/BEGIN_ENUMINFO(name) ... table, matched to an EnumerationSite by name. */
//************************************************************************************************

struct ValueTableSite
{
	MutableCString name;			///< The table's own (possibly '::'-qualified) name, e.g. "AnimationElement::timingTypes".
	Vector<ValueTableItem> items;	///< One {label} entry per row in the table.
	MutableCString file;			///< Source file this table was found in.
	int line;						///< 1-based line number of the table's BEGIN_STYLEDEF/BEGIN_ENUMINFO call.

	ValueTableSite ()
	: line (0)
	{}
};

//************************************************************************************************
// ParsedSites
/** Output of processCallsForFile(), consumed by linkEntries()/linkEnumerations(). */
//************************************************************************************************

struct ParsedSites
{
	Vector<EntryDefSite> entryDefs;					///< One entry per DEFINE_TYPELIB_CLASS/_ENTRY(_WITH_PARENT/_LOCAL) call.
	Vector<AbstractSite> abstractSites;				///< One entry per DEFINE_TYPELIB_ABSTRACT call.
	Vector<DocGroupSite> docGroupSites;				///< One entry per DEFINE_TYPELIB_DOCGROUP call.
	Vector<MembersBlockSite> membersBlocks;			///< One entry per BEGIN_TYPELIB_MEMBERS...END_TYPELIB_MEMBERS block.
	Vector<AttributesBlockSite> attributesBlocks;	///< One entry per BEGIN_TYPELIB_ATTRIBUTES...END_TYPELIB_ATTRIBUTES block.
	Vector<EnumerationSite> enumerations;			///< One entry per DEFINE_TYPELIB_ENUMERATION(_WITH_PARENT) call.
	Vector<ValueTableSite> valueTables;				///< One entry per BEGIN_STYLEDEF/BEGIN_ENUMINFO table.
};

//************************************************************************************************
// FileCalls
/** Every recognized macro call found in one scanned source file. */
//************************************************************************************************

struct FileCalls
{
	MutableCString file;				///< The scanned file's path, relative to the scan root.
	Vector<ScannedMacroCall> calls;		///< Every recognized macro call found in 'file', in source order.
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool matchDataTypeToken (DataType& type, CStringRef rawToken)
{
	// A single identifier naming an ITypeInfo::DataType enumerator (e.g. "CCL::ITypeInfo::kBool") -
	// unlike DEFINE_PROPERTY_TYPE's expression grammar (ClassScanner's matchFlagToken), typelib
	// member data types are never combined with '|', so this only needs to match one bare name.

	MutableCString token = SourceLocation::toBareName (rawToken);

	if(token == "kBool")
	{
		type = ITypeInfo::kBool;
		return true;
	}
	if(token == "kInt")
	{
		type = ITypeInfo::kInt;
		return true;
	}
	if(token == "kFloat")
	{
		type = ITypeInfo::kFloat;
		return true;
	}
	if(token == "kString")
	{
		type = ITypeInfo::kString;
		return true;
	}
	if(token == "kObject")
	{
		type = ITypeInfo::kObject;
		return true;
	}
	if(token == "kContainer")
	{
		type = ITypeInfo::kContainer;
		return true;
	}
	if(token == "kVariant")
	{
		type = ITypeInfo::kVariant;
		return true;
	}
	if(token == "kBlob")
	{
		type = ITypeInfo::kBlob;
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Custom ADD_TYPELIB_MEMBER_* alias discovery (mirrors TypeLibDocuScanner::AliasCollector)
//////////////////////////////////////////////////////////////////////////////////////////////////

void collectMemberMacroAliases (Vector<MemberMacroAlias>& aliases, CStringRef source, CStringRef fileName)
{
	SourceLexer lexer (source);
	SourceToken token;

	while(lexer.next (token))
	{
		if(token.type != SourceToken::kPreprocessor)
			continue;

		ScannedFunctionDefine candidate;
		if(!MacroCallScanner::extractFunctionDefine (candidate, token))
			continue;

		if(!candidate.name.startsWith ("ADD_TYPELIB_MEMBER_"))
			continue;

		if(candidate.name == "ADD_TYPELIB_MEMBER_OBJECT" || candidate.name == "ADD_TYPELIB_MEMBER_BOOL" ||
			candidate.name == "ADD_TYPELIB_MEMBER_ENUM" || candidate.name == "ADD_TYPELIB_MEMBER_FLOAT" ||
			candidate.name == "ADD_TYPELIB_MEMBER_INT" || candidate.name == "ADD_TYPELIB_MEMBER_STRING")
			continue; // built-in shortcuts, handled directly - not real aliases

		Vector<MutableCString> forwardedArgs;
		if(!MacroCallScanner::matchSingleParamForward (forwardedArgs, candidate, "ADD_TYPELIB_MEMBER_WITH_TYPE", 3))
			continue;

		DataType dataType;
		if(!matchDataTypeToken (dataType, forwardedArgs.at (0)))
			continue;

		MemberMacroAlias alias;
		alias.name = candidate.name;
		alias.dataType = dataType;
		alias.typeNameRaw = forwardedArgs.at (1);
		alias.typeNameRaw.trimWhitespace ();

		bool replaced = false;
		for(int i = 0; i < aliases.count () && !replaced; i++)
			if(aliases.at (i).name == alias.name)
			{
				aliases.at (i) = alias; // last definition wins, matching #define redefinition
				replaced = true;
			}
		if(!replaced)
			aliases.add (alias);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const MemberMacroAlias* findMemberMacroAlias (const Vector<MemberMacroAlias>& aliases, CStringRef name)
{
	for(int i = 0; i < aliases.count (); i++)
		if(aliases.at (i).name == name)
			return &aliases.at (i);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void scanValueTables (Vector<ValueTableSite>& tables, CStringRef source, CStringRef fileName)
{
	// Value table (BEGIN_STYLEDEF / BEGIN_ENUMINFO) extraction
	// The payload is a bare brace-initializer-list ('{ label, value }, ...'), not macro
	// calls, so this walks SourceLexer tokens directly.

	SourceLexer lexer (source);
	SourceToken token;

	while(lexer.next (token))
	{
		if(token.type != SourceToken::kIdentifier)
			continue;

		bool isStyleDef = (token.text == "BEGIN_STYLEDEF");
		bool isEnumInfo = (token.text == "BEGIN_ENUMINFO");
		if(!isStyleDef && !isEnumInfo)
			continue;

		int startLine = token.line;
		if(!lexer.next (token) || token.type != SourceToken::kOperator || token.text != "(")
			continue;

		MutableCString name;
		bool nameClosed = false;
		while(!nameClosed && lexer.next (token))
		{
			if(token.type == SourceToken::kOperator && token.text == ")")
				nameClosed = true;
			else
				name.append (token.text); // absorbs identifiers and "::" alike
		}
		if(!nameClosed)
			return; // EOF before the name's closing ')'

		ValueTableSite tableSite;
		tableSite.name = name;
		tableSite.file = fileName;
		tableSite.line = startLine;

		CStringPtr endToken = isStyleDef ? "END_STYLEDEF" : "END_ENUMINFO";
		bool closed = false;
		while(!closed && lexer.next (token))
		{
			if(token.type == SourceToken::kIdentifier && token.text == endToken)
			{
				closed = true;
				break;
			}
			if(token.type != SourceToken::kOperator || token.text != "{")
				continue;

			ValueTableItem item;
			bool inLabel = true;
			bool itemClosed = false;
			while(!itemClosed && lexer.next (token))
			{
				if(token.type == SourceToken::kOperator && token.text == "}")
					itemClosed = true;
				else if(token.type == SourceToken::kOperator && token.text == ",")
					inLabel = false; // reached the value - not kept, just skip its tokens below
				else if(inLabel)
				{
					// A kString token's own .text is de-quoted content (SourceLexer's usual
					// convention) - re-wrap it so SymbolResolver::resolveStringArg() (expects raw, quoted
					// argument text, same as a real macro-call argument) still recognizes it
					// as a literal rather than a bare identifier to look up.

					if(token.type == SourceToken::kString)
						item.labelRaw.append ("\"").append (token.text).append ("\"");
					else
						item.labelRaw.append (token.text);
				}
			}
			if(!itemClosed)
				return; // EOF mid-item
			tableSite.items.add (item);
		}

		if(!closed)
			return; // EOF before END_STYLEDEF/END_ENUMINFO

		tables.add (tableSite);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void loadFile (MutableCString& raw, UrlRef path)
{
	String rawUnicode = TextUtils::loadRawString (path);
	raw = MutableCString (rawUnicode, Text::kASCII);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool scanFile (FileCalls& fileCalls, Vector<ValueTableSite>& valueTables, MutableCString& errorMsg,
	CStringRef raw, CStringRef relFile, SymbolRegistry& symbols, const Vector<CStringPtr>& macroNames)
{
	fileCalls.file = relFile;

	Vector<ScannedDefine> defines;
	if(!MacroCallScanner::scan (fileCalls.calls, defines, errorMsg, raw, macroNames))
		return false;

	SymbolCollector collector (symbols);
	for(int i = 0; i < defines.count (); i++)
		collector.addDefine (defines.at (i), relFile);

	collector.collectConstStringSymbols (raw, relFile);
	collector.collectFunctionDefines (raw, relFile);
	scanValueTables (valueTables, raw, relFile);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool extractStringIdSymbols (SymbolRegistry& symbols, MutableCString& errorMsg, const Vector<FileCalls>& allCalls)
{
	SymbolCollector collector (symbols);

	for(int fi = 0; fi < allCalls.count (); fi++)
	{
		const FileCalls& fileCalls = allCalls.at (fi);
		for(int ci = 0; ci < fileCalls.calls.count (); ci++)
		{
			const ScannedMacroCall& call = fileCalls.calls.at (ci);
			if(call.name == "DEFINE_STRINGID" || call.name == "DEFINE_STRINGID_" ||
			   call.name == "DEFINE_STRINGID_MEMBER" || call.name == "DEFINE_STRINGID_MEMBER_")
				if(!collector.addStringIdSymbol (call, fileCalls.file, errorMsg))
					return false;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool buildMemberItem (MemberItem& item, MutableCString& errorMsg, const ScannedMacroCall& call, CStringRef loc,
	const Vector<MemberMacroAlias>& aliases)
{
	if(call.name == "ADD_TYPELIB_MEMBER")
	{
		if(call.args.count () < 2)
		{
			errorMsg.appendFormat ("%s: ADD_TYPELIB_MEMBER expected 2 arguments", loc.str ());
			return false;
		}
		item.dataType = ITypeInfo::kString;
		item.nameRaw = call.args.at (0);
		item.typeNameRaw = call.args.at (1);
		return true;
	}
	if(call.name == "ADD_TYPELIB_MEMBER_WITH_TYPE")
	{
		if(call.args.count () < 3)
		{
			errorMsg.appendFormat ("%s: ADD_TYPELIB_MEMBER_WITH_TYPE expected 3 arguments", loc.str ());
			return false;
		}
		if(!matchDataTypeToken (item.dataType, call.args.at (1)))
		{
			errorMsg.appendFormat ("%s: unrecognized data type '%s' in ADD_TYPELIB_MEMBER_WITH_TYPE", loc.str (), call.args.at (1).str ());
			return false;
		}
		item.nameRaw = call.args.at (0);
		item.typeNameRaw = call.args.at (2);
		return true;
	}
	if(call.name == "ADD_TYPELIB_MEMBER_OBJECT")
	{
		if(call.args.count () < 2)
		{
			errorMsg.appendFormat ("%s: ADD_TYPELIB_MEMBER_OBJECT expected 2 arguments", loc.str ());
			return false;
		}
		item.dataType = ITypeInfo::kObject;
		item.nameRaw = call.args.at (0);
		item.typeNameRaw = call.args.at (1);
		return true;
	}

	// The remaining 4 built-in shortcuts (BOOL/ENUM/FLOAT/INT/STRING) all take just 'name' and use
	// a fixed data type + typeName - the real TypeNames::kXxx constants, matching typelibmacros.h.
	CStringPtr fixedTypeName = nullptr;
	if(call.name == "ADD_TYPELIB_MEMBER_BOOL")
	{
		item.dataType = ITypeInfo::kBool;
		fixedTypeName = TypeNames::kBool;
	}
	else if(call.name == "ADD_TYPELIB_MEMBER_ENUM")
	{
		item.dataType = ITypeInfo::kInt;
		fixedTypeName = TypeNames::kEnum;
	}
	else if(call.name == "ADD_TYPELIB_MEMBER_FLOAT")
	{
		item.dataType = ITypeInfo::kFloat;
		fixedTypeName = TypeNames::kFloat;
	}
	else if(call.name == "ADD_TYPELIB_MEMBER_INT")
	{
		item.dataType = ITypeInfo::kInt;
		fixedTypeName = TypeNames::kInt;
	}
	else if(call.name == "ADD_TYPELIB_MEMBER_STRING")
	{
		item.dataType = ITypeInfo::kString;
		fixedTypeName = TypeNames::kString;
	}
	else if(const MemberMacroAlias* alias = findMemberMacroAlias (aliases, call.name))
	{
		if(call.args.count () < 1)
		{
			errorMsg.appendFormat ("%s: %s expected 1 argument", loc.str (), call.name.str ());
			return false;
		}
		item.dataType = alias->dataType;
		item.nameRaw = call.args.at (0);
		item.typeNameRaw = alias->typeNameRaw;
		return true;
	}
	else
	{
		errorMsg.appendFormat ("%s: internal error - unhandled member macro '%s'", loc.str (), call.name.str ());
		return false;
	}

	if(call.args.count () < 1)
	{
		errorMsg.appendFormat ("%s: %s expected 1 argument", loc.str (), call.name.str ());
		return false;
	}
	item.fixedTypeName = fixedTypeName;
	item.nameRaw = call.args.at (0);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool processCallsForFile (ParsedSites& sites, MutableCString& errorMsg, const Vector<ScannedMacroCall>& calls,
	CStringRef file, const Vector<MemberMacroAlias>& aliases)
{
	enum class OpenBlockKind { kNone, kMembers, kAttributes };
	
	OpenBlockKind openKind = OpenBlockKind::kNone;
	MembersBlockSite openMembersBlock;
	AttributesBlockSite openAttributesBlock;

	for(int ci = 0; ci < calls.count (); ci++)
	{
		const ScannedMacroCall& call = calls.at (ci);
		MutableCString loc = SourceLocation::makeLocation (file, call.line);

		if(call.name == "DEFINE_STRINGID" || call.name == "DEFINE_STRINGID_" ||
		   call.name == "DEFINE_STRINGID_MEMBER" || call.name == "DEFINE_STRINGID_MEMBER_")
			continue; // already consumed in the symbol pre-pass

		// A class-backed or plain entry.
		if(call.name == "DEFINE_TYPELIB_CLASS" || call.name == "DEFINE_TYPELIB_CLASS_WITH_PARENT" ||
		   call.name == "DEFINE_TYPELIB_ENTRY" || call.name == "DEFINE_TYPELIB_ENTRY_LOCAL" || call.name == "DEFINE_TYPELIB_ENTRY_WITH_PARENT")
		{
			bool classBacked = (call.name == "DEFINE_TYPELIB_CLASS" || call.name == "DEFINE_TYPELIB_CLASS_WITH_PARENT");
			bool withParent = (call.name == "DEFINE_TYPELIB_CLASS_WITH_PARENT" || call.name == "DEFINE_TYPELIB_ENTRY_WITH_PARENT");
			int needed = withParent ? 4 : 3;
			if(call.args.count () < needed)
			{
				errorMsg.appendFormat ("%s: %s expected %d arguments", loc.str (), call.name.str (), needed);
				return false;
			}
			EntryDefSite site;
			site.uniqueName = SourceLocation::toBareName (call.args.at (0));
			site.classBacked = classBacked;
			site.hasParent = withParent;
			if(withParent)
				site.parentUniqueName = SourceLocation::toBareName (call.args.at (1));
			int tagArgIndex = withParent ? 2 : 1;
			int libArgIndex = withParent ? 3 : 2;
			site.tagNameRaw = call.args.at (tagArgIndex);
			site.libraryToken = call.args.at (libArgIndex);
			site.libraryToken.trimWhitespace ();
			site.file = file;
			site.line = call.line;
			sites.entryDefs.add (site);
			continue;
		}

		// Marks an entry abstract.
		if(call.name == "DEFINE_TYPELIB_ABSTRACT")
		{
			if(call.args.count () < 1)
			{
				errorMsg.appendFormat ("%s: DEFINE_TYPELIB_ABSTRACT expected 1 argument", loc.str ());
				return false;
			}
			AbstractSite site;
			site.uniqueName = SourceLocation::toBareName (call.args.at (0));
			site.file = file;
			site.line = call.line;
			sites.abstractSites.add (site);
			continue;
		}

		// Assigns an entry's doc group.
		if(call.name == "DEFINE_TYPELIB_DOCGROUP")
		{
			if(call.args.count () < 2)
			{
				errorMsg.appendFormat ("%s: DEFINE_TYPELIB_DOCGROUP expected 2 arguments", loc.str ());
				return false;
			}
			DocGroupSite site;
			site.uniqueName = SourceLocation::toBareName (call.args.at (0));
			site.groupNameRaw = call.args.at (1);
			site.file = file;
			site.line = call.line;
			sites.docGroupSites.add (site);
			continue;
		}

		// Opens a member block.
		if(call.name == "BEGIN_TYPELIB_MEMBERS")
		{
			if(openKind != OpenBlockKind::kNone)
			{
				errorMsg.appendFormat ("%s: BEGIN_TYPELIB_MEMBERS nested inside an already-open block", loc.str ());
				return false;
			}
			if(call.args.count () < 1)
			{
				errorMsg.appendFormat ("%s: BEGIN_TYPELIB_MEMBERS expected 1 argument", loc.str ());
				return false;
			}
			openMembersBlock = MembersBlockSite ();
			openMembersBlock.uniqueName = SourceLocation::toBareName (call.args.at (0));
			openMembersBlock.file = file;
			openMembersBlock.line = call.line;
			openKind = OpenBlockKind::kMembers;
			continue;
		}

		// Closes the member block and commits it.
		if(call.name == "END_TYPELIB_MEMBERS")
		{
			if(openKind != OpenBlockKind::kMembers)
			{
				errorMsg.appendFormat ("%s: END_TYPELIB_MEMBERS without a matching BEGIN_TYPELIB_MEMBERS", loc.str ());
				return false;
			}
			if(call.args.count () < 1 || SourceLocation::toBareName (call.args.at (0)) != openMembersBlock.uniqueName)
			{
				errorMsg.appendFormat ("%s: END_TYPELIB_MEMBERS does not match the open block for '%s'", loc.str (), openMembersBlock.uniqueName.str ());
				return false;
			}
			sites.membersBlocks.add (openMembersBlock);
			openKind = OpenBlockKind::kNone;
			continue;
		}

		// A member item within the open member block.
		if(call.name == "ADD_TYPELIB_MEMBER" || call.name == "ADD_TYPELIB_MEMBER_WITH_TYPE" ||
		   call.name == "ADD_TYPELIB_MEMBER_BOOL" || call.name == "ADD_TYPELIB_MEMBER_ENUM" ||
		   call.name == "ADD_TYPELIB_MEMBER_FLOAT" || call.name == "ADD_TYPELIB_MEMBER_INT" ||
		   call.name == "ADD_TYPELIB_MEMBER_OBJECT" || call.name == "ADD_TYPELIB_MEMBER_STRING" ||
		   findMemberMacroAlias (aliases, call.name) != nullptr)
		{
			if(openKind != OpenBlockKind::kMembers)
			{
				errorMsg.appendFormat ("%s: %s used outside of a BEGIN_TYPELIB_MEMBERS/END_TYPELIB_MEMBERS block", loc.str (), call.name.str ());
				return false;
			}
			MemberItem item;
			if(!buildMemberItem (item, errorMsg, call, loc, aliases))
				return false;
			openMembersBlock.items.add (item);
			continue;
		}

		// Opens an attribute block.
		if(call.name == "BEGIN_TYPELIB_ATTRIBUTES")
		{
			if(openKind != OpenBlockKind::kNone)
			{
				errorMsg.appendFormat ("%s: BEGIN_TYPELIB_ATTRIBUTES nested inside an already-open block", loc.str ());
				return false;
			}
			if(call.args.count () < 1)
			{
				errorMsg.appendFormat ("%s: BEGIN_TYPELIB_ATTRIBUTES expected 1 argument", loc.str ());
				return false;
			}
			openAttributesBlock = AttributesBlockSite ();
			openAttributesBlock.uniqueName = SourceLocation::toBareName (call.args.at (0));
			openAttributesBlock.file = file;
			openAttributesBlock.line = call.line;
			openKind = OpenBlockKind::kAttributes;
			continue;
		}

		// Closes the attribute block and commits it.
		if(call.name == "END_TYPELIB_ATTRIBUTES")
		{
			if(openKind != OpenBlockKind::kAttributes)
			{
				errorMsg.appendFormat ("%s: END_TYPELIB_ATTRIBUTES without a matching BEGIN_TYPELIB_ATTRIBUTES", loc.str ());
				return false;
			}
			if(call.args.count () < 1 || SourceLocation::toBareName (call.args.at (0)) != openAttributesBlock.uniqueName)
			{
				errorMsg.appendFormat ("%s: END_TYPELIB_ATTRIBUTES does not match the open block for '%s'", loc.str (), openAttributesBlock.uniqueName.str ());
				return false;
			}
			sites.attributesBlocks.add (openAttributesBlock);
			openKind = OpenBlockKind::kNone;
			continue;
		}

		// An attribute item within the open attribute block.
		if(call.name == "ADD_TYPELIB_ATTRIBUTE" || call.name == "ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE" || call.name == "ADD_TYPELIB_CHILDGROUP_ATTRIBUTE")
		{
			if(openKind != OpenBlockKind::kAttributes)
			{
				errorMsg.appendFormat ("%s: %s used outside of a BEGIN_TYPELIB_ATTRIBUTES/END_TYPELIB_ATTRIBUTES block", loc.str (), call.name.str ());
				return false;
			}
			AttributeItem item;
			if(call.name == "ADD_TYPELIB_ATTRIBUTE")
			{
				if(call.args.count () < 2)
				{
					errorMsg.appendFormat ("%s: ADD_TYPELIB_ATTRIBUTE expected 2 arguments", loc.str ());
					return false;
				}
				item.keyRaw = call.args.at (0);
				item.valueRaw = call.args.at (1);
			}
			else
			{
				if(call.args.count () < 1)
				{
					errorMsg.appendFormat ("%s: %s expected 1 argument", loc.str (), call.name.str ());
					return false;
				}
				item.fixedKey = (call.name == "ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE") ? kClassSchemaGroups : kClassChildGroup;
				item.valueRaw = call.args.at (0);
			}
			openAttributesBlock.items.add (item);
			continue;
		}

		// An enumeration definition.
		if(call.name == "DEFINE_TYPELIB_ENUMERATION" || call.name == "DEFINE_TYPELIB_ENUMERATION_WITH_PARENT")
		{
			bool withParent = (call.name == "DEFINE_TYPELIB_ENUMERATION_WITH_PARENT");
			int needed = withParent ? 6 : 4;
			if(call.args.count () < needed)
			{
				errorMsg.appendFormat ("%s: %s expected %d arguments", loc.str (), call.name.str (), needed);
				return false;
			}
			EnumerationSite site;
			site.ownerTagNameRaw = call.args.at (0);
			site.memberNameRaw = call.args.at (1);
			site.enumDefName = call.args.at (2); // kept verbatim (e.g. "Foo::bar"), matching BEGIN_STYLEDEF/BEGIN_ENUMINFO's own qualified name capture
			site.enumDefName.trimWhitespace ();
			site.hasParent = withParent;
			if(withParent)
			{
				site.parentOwnerTagNameRaw = call.args.at (3);
				site.parentMemberNameRaw = call.args.at (4);
			}
			site.libraryToken = call.args.at (withParent ? 5 : 3);
			site.libraryToken.trimWhitespace ();
			site.file = file;
			site.line = call.line;
			sites.enumerations.add (site);
			continue;
		}

		errorMsg.appendFormat ("%s: internal error - unhandled recognized macro '%s'", loc.str (), call.name.str ());
		return false;
	}

	if(openKind != OpenBlockKind::kNone)
	{
		errorMsg.appendFormat ("%s: unterminated %s block for '%s'", file.str (),
			openKind == OpenBlockKind::kMembers ? "BEGIN_TYPELIB_MEMBERS" : "BEGIN_TYPELIB_ATTRIBUTES",
			openKind == OpenBlockKind::kMembers ? openMembersBlock.uniqueName.str () : openAttributesBlock.uniqueName.str ());
		return false;
	}

	return true;
}

//************************************************************************************************
// ResolvedMember
/** One fully-resolved typelib entry member, ready for export. */
//************************************************************************************************

struct ResolvedMember
{
	MutableCString name;		///< The member's resolved name, e.g. "color".
	DataType dataType;			///< The member's data type, e.g. ITypeInfo::kInt.
	MutableCString typeName;	///< The member's resolved type name, e.g. "Boxed.Color".

	ResolvedMember ()
	: dataType (ITypeInfo::kString)
	{}
};

//************************************************************************************************
// ResolvedAttribute
/** One fully-resolved typelib entry attribute, ready for export. */
//************************************************************************************************

struct ResolvedAttribute
{
	MutableCString key;		///< The attribute's resolved key, e.g. "Resources".
	MutableCString value;	///< The attribute's resolved value, e.g. "TAG_RESOURCES.Children".
};

//************************************************************************************************
// ResolvedEntry
/** One fully-resolved, library-filtered typelib entry, ready for export. */
//************************************************************************************************

struct ResolvedEntry
{
	MutableCString uniqueName;				///< The entry's identity, e.g. "AnimationElement".
	bool classBacked;						///< True if this entry originated from DEFINE_TYPELIB_CLASS(_WITH_PARENT).
	MutableCString tagName;					///< The entry's resolved tag name, e.g. "AnimationElement".
	bool hasParent;							///< True if this entry has a parent entry.
	MutableCString parentUniqueName;		///< Only used while linking, cleared implicitly once parentTagName is filled in.
	MutableCString parentTagName;			///< The resolved parent entry's tag name, once linked.
	bool abstract;							///< True if a matching DEFINE_TYPELIB_ABSTRACT call was found.
	MutableCString docGroup;				///< The entry's resolved doc group; empty if none.
	Vector<ResolvedMember> members;			///< One entry per resolved ADD_TYPELIB_MEMBER* item.
	Vector<ResolvedAttribute> attributes;	///< Pre-space-joined attribute list, in first-seen key order.

	ResolvedEntry ()
	: classBacked (false),
	  hasParent (false),
	  abstract (false)
	{}
};

//************************************************************************************************
// ResolvedEnumeration
/** One fully-resolved, library-filtered typelib enumeration, ready for export. */
//************************************************************************************************

struct ResolvedEnumeration
{
	MutableCString name;					///< "ownerTagName.memberName", e.g. "AnimationElement.timingType".
	bool hasParent;							///< True if this enumeration has a parent enumeration.
	MutableCString parentName;				///< "parentOwnerTagName.parentMemberName", once linked.
	bool hasEnumerators;					///< False for pure parent-chaining (enumDefName == "nullptr").
	Vector<MutableCString> enumerators;		///< The enumeration's resolved enumerator names, in table order.

	ResolvedEnumeration ()
	: hasParent (false),
	  hasEnumerators (false)
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

ResolvedEntry* findResolvedEntry (Vector<ResolvedEntry>& entries, CStringRef uniqueName)
{
	for(int i = 0; i < entries.count (); i++)
		if(entries.at (i).uniqueName == uniqueName)
			return &entries.at (i);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void addAttribute (Vector<ResolvedAttribute>& attributes, CStringRef key, CStringRef value)
{
	// Reproduces TypeLibEntry::getDetails()'s real AttributeModifier behavior (typelib.cpp): a
	// repeated key's value is space-joined onto the existing one, not duplicated or overwritten.

	for(int i = 0; i < attributes.count (); i++)
		if(attributes.at (i).key == key)
		{
			MutableCString joined = attributes.at (i).value;
			joined.append (" ").append (value);
			attributes.at (i).value = joined;
			return;
		}

	ResolvedAttribute item;
	item.key = key;
	item.value = value;
	attributes.add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool linkEntries (Vector<ResolvedEntry>& entries, MutableCString& errorMsg, const ParsedSites& sites, SymbolRegistry& symbols, CStringRef libraryToken)
{
	SymbolResolver resolver (symbols);

	// Filter to 'libraryToken' first: fields stay RAW until now, since resolving eagerly could
	// hard-fail on an unresolvable value belonging to a different, unrequested library.
	for(int i = 0; i < sites.entryDefs.count (); i++)
	{
		const EntryDefSite& site = sites.entryDefs.at (i);
		if(site.libraryToken != libraryToken)
			continue;

		MutableCString loc = SourceLocation::makeLocation (site.file, site.line);
		if(findResolvedEntry (entries, site.uniqueName) != nullptr)
		{
			errorMsg.appendFormat ("%s: duplicate type library entry '%s'", loc.str (), site.uniqueName.str ());
			return false;
		}

		ResolvedEntry entry;
		entry.uniqueName = site.uniqueName;
		entry.classBacked = site.classBacked;
		if(!resolver.resolveStringArg (entry.tagName, errorMsg, site.tagNameRaw, loc))
			return false;
		entry.hasParent = site.hasParent;
		entry.parentUniqueName = site.parentUniqueName;
		entries.add (entry);
	}

	// No survivors: fail with the list of library tokens actually found, to help spot a typo.
	if(entries.isEmpty ())
	{
		Vector<MutableCString> distinctTokens;
		for(int i = 0; i < sites.entryDefs.count (); i++)
		{
			const MutableCString& token = sites.entryDefs.at (i).libraryToken;
			bool known = false;
			for(int t = 0; t < distinctTokens.count () && !known; t++)
				known = (distinctTokens.at (t) == token);
			if(!known)
				distinctTokens.add (token);
		}

		MutableCString tokenList;
		for(int i = 0; i < distinctTokens.count (); i++)
		{
			if(i > 0)
				tokenList.append (", ");
			tokenList.append (distinctTokens.at (i));
		}

		errorMsg.appendFormat ("no type library entries found for '%s'; found: [%s]", libraryToken.str (), tokenList.str ());
		return false;
	}

	// Link parent entries by uniqueName, now that every entry's own tagName is resolved.
	for(int i = 0; i < entries.count (); i++)
	{
		ResolvedEntry& entry = entries.at (i);
		if(!entry.hasParent)
			continue;

		ResolvedEntry* parent = findResolvedEntry (entries, entry.parentUniqueName);
		if(parent == nullptr)
		{
			errorMsg.appendFormat ("entry '%s': parent '%s' not found in type library '%s'",
				entry.uniqueName.str (), entry.parentUniqueName.str (), libraryToken.str ());
			return false;
		}
		entry.parentTagName = parent->tagName;
	}

	// Apply DEFINE_TYPELIB_ABSTRACT flags.
	for(int i = 0; i < sites.abstractSites.count (); i++)
	{
		ResolvedEntry* entry = findResolvedEntry (entries, sites.abstractSites.at (i).uniqueName);
		if(entry != nullptr)
			entry->abstract = true;
	}

	// Resolve and apply DEFINE_TYPELIB_DOCGROUP doc groups.
	for(int i = 0; i < sites.docGroupSites.count (); i++)
	{
		const DocGroupSite& site = sites.docGroupSites.at (i);
		ResolvedEntry* entry = findResolvedEntry (entries, site.uniqueName);
		if(entry == nullptr)
			continue;
		MutableCString loc = SourceLocation::makeLocation (site.file, site.line);
		if(!resolver.resolveStringArg (entry->docGroup, errorMsg, site.groupNameRaw, loc))
			return false;
	}

	// Resolve and apply each entry's BEGIN_TYPELIB_MEMBERS block.
	for(int i = 0; i < sites.membersBlocks.count (); i++)
	{
		const MembersBlockSite& block = sites.membersBlocks.at (i);
		ResolvedEntry* entry = findResolvedEntry (entries, block.uniqueName);
		if(entry == nullptr)
			continue;

		MutableCString loc = SourceLocation::makeLocation (block.file, block.line);
		for(int m = 0; m < block.items.count (); m++)
		{
			const MemberItem& item = block.items.at (m);
			ResolvedMember member;
			member.dataType = item.dataType;
			if(!resolver.resolveStringArg (member.name, errorMsg, item.nameRaw, loc))
				return false;
			if(item.fixedTypeName != nullptr)
				member.typeName = item.fixedTypeName;
			else if(!resolver.resolveStringArg (member.typeName, errorMsg, item.typeNameRaw, loc))
				return false;
			entry->members.add (member);
		}
	}

	// Resolve and apply each entry's BEGIN_TYPELIB_ATTRIBUTES block.
	for(int i = 0; i < sites.attributesBlocks.count (); i++)
	{
		const AttributesBlockSite& block = sites.attributesBlocks.at (i);
		ResolvedEntry* entry = findResolvedEntry (entries, block.uniqueName);
		if(entry == nullptr)
			continue;

		MutableCString loc = SourceLocation::makeLocation (block.file, block.line);
		for(int a = 0; a < block.items.count (); a++)
		{
			const AttributeItem& item = block.items.at (a);
			MutableCString key;
			if(item.fixedKey != nullptr)
				key = item.fixedKey;
			else if(!resolver.resolveStringArg (key, errorMsg, item.keyRaw, loc))
				return false;
			MutableCString value;
			if(!resolver.resolveStringArg (value, errorMsg, item.valueRaw, loc))
				return false;
			addAttribute (entry->attributes, key, value);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool linkEnumerations (Vector<ResolvedEnumeration>& enumerations, MutableCString& errorMsg, const ParsedSites& sites, SymbolRegistry& symbols, CStringRef libraryToken)
{
	SymbolResolver resolver (symbols);

	for(int i = 0; i < sites.enumerations.count (); i++)
	{
		const EnumerationSite& site = sites.enumerations.at (i);
		if(site.libraryToken != libraryToken)
			continue;

		MutableCString siteLoc = SourceLocation::makeLocation (site.file, site.line);

		MutableCString ownerTagName, memberName;
		if(!resolver.resolveStringArg (ownerTagName, errorMsg, site.ownerTagNameRaw, siteLoc))
			return false;
		if(!resolver.resolveStringArg (memberName, errorMsg, site.memberNameRaw, siteLoc))
			return false;

		ResolvedEnumeration resolved;
		resolved.name = MutableCString (ownerTagName).append (".").append (memberName);
		resolved.hasParent = site.hasParent;
		if(site.hasParent)
		{
			MutableCString parentOwnerTagName, parentMemberName;
			if(!resolver.resolveStringArg (parentOwnerTagName, errorMsg, site.parentOwnerTagNameRaw, siteLoc))
				return false;
			if(!resolver.resolveStringArg (parentMemberName, errorMsg, site.parentMemberNameRaw, siteLoc))
				return false;
			resolved.parentName = MutableCString (parentOwnerTagName).append (".").append (parentMemberName);
		}

		if(site.enumDefName != "nullptr")
		{
			const ValueTableSite* table = nullptr;
			for(int t = 0; t < sites.valueTables.count () && table == nullptr; t++)
				if(sites.valueTables.at (t).name == site.enumDefName)
					table = &sites.valueTables.at (t);

			if(table == nullptr)
			{
				Logging::warningf ("%s: enum definition '%s' for '%s' not found - enumerators omitted",
					siteLoc.str (), site.enumDefName.str (), resolved.name.str ());
			}
			else
			{
				resolved.hasEnumerators = true;
				MutableCString tableLoc = SourceLocation::makeLocation (table->file, table->line);
				for(int e = 0; e < table->items.count (); e++)
				{
					const ValueTableItem& item = table->items.at (e);

					MutableCString enumeratorName, labelErr;
					if(!resolver.resolveStringArg (enumeratorName, labelErr, item.labelRaw, tableLoc))
					{
						Logging::warningf ("%s: could not resolve enumerator label '%s' of '%s' - skipped (%s)",
							tableLoc.str (), item.labelRaw.str (), resolved.name.str (), labelErr.str ());
						continue;
					}

					resolved.enumerators.add (enumeratorName);
				}
			}
		}

		enumerations.add (resolved);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Class* buildClass (const ResolvedEntry& entry)
{
	Class* c = NEW Class (entry.tagName);
	if(!entry.parentTagName.isEmpty ())
		c->setParentName (entry.parentTagName);

	int flags = 0;
	if(entry.abstract)
		flags |= ITypeInfo::kAbstract;
	c->setFlags (flags);

	for(int m = 0; m < entry.members.count (); m++)
	{
		const ResolvedMember& member = entry.members.at (m);
		c->addMember (NEW Member (member.name, member.dataType, member.typeName));
	}

	// Upcast to the protected ITypeInfoDetails override, same as the real runtime does. Attributes
	// are set before Class:DocGroup to match that runtime's own export order.

	ITypeInfoDetails& details = *c;
	for(int a = 0; a < entry.attributes.count (); a++)
		details.setAttribute (entry.attributes.at (a).key, Variant (entry.attributes.at (a).value));
	if(!entry.docGroup.isEmpty ())
		details.setAttribute (kClassDocGroup, Variant (entry.docGroup));

	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Enumeration* buildEnumeration (const ResolvedEnumeration& enumeration)
{
	Enumeration* e = NEW Enumeration (enumeration.name, enumeration.hasParent ? StringID (enumeration.parentName.str ()) : nullptr);
	for(int k = 0; k < enumeration.enumerators.count (); k++)
		e->addEnumerator (NEW Enumerator (enumeration.enumerators.at (k)));
	return e;
}

} // namespace TypeLibScannerInternal

using namespace TypeLibScannerInternal;

//************************************************************************************************
// TypeLibScanner
//************************************************************************************************

bool TypeLibScanner::scan (ClassRepository& repository, const Vector<Url>& sourceRoots, CStringRef libraryToken)
{
	FileCollector collector;
	String rootDisplay;
	for(UrlRef sourceRoot : sourceRoots)
	{
		int countBefore = collector.files.count ();
		if(!SourceFileTraverser::traverse (sourceRoot, SourceFileTraverser::cppSourceFilter (), collector))
			return false;

		String display;
		sourceRoot.toDisplayString (display);
		Logging::info ("found %(1) source file(s) under '%(2)'", Variant (collector.files.count () - countBefore), display);

		if(!rootDisplay.isEmpty ())
			rootDisplay << ", ";
		rootDisplay << display;
	}
	const Vector<Url>& files = collector.files;

	SymbolRegistry symbols;
	MutableCString errorMsg;

	// Pass 1: load every file's text once (cached for pass 2) and discover ADD_TYPELIB_MEMBER_*
	// alias #defines across the whole tree - needed up front so pass 2's MacroCallScanner::scan()
	// (which only extracts calls to a fixed name list) knows their names too.

	Vector<MutableCString> rawContents;
	Vector<MutableCString> relFiles;
	Vector<MemberMacroAlias> aliases;

	for(int i = 0; i < files.count (); i++)
	{
		UrlRef filePath = files.at (i);
		String relDisplay;
		filePath.toDisplayString (relDisplay);
		MutableCString relFile (relDisplay, Text::kASCII);

		MutableCString raw;
		loadFile (raw, filePath);
		collectMemberMacroAliases (aliases, raw, relFile);

		rawContents.add (raw);
		relFiles.add (relFile);
	}

	Vector<CStringPtr> macroNames;
	for(CStringPtr name : recognizedMacroNames ())
		macroNames.add (name);
	for(int i = 0; i < aliases.count (); i++)
		macroNames.add (aliases.at (i).name.str ());

	// Pass 2: extract macro calls (now against the complete name list) and every other per-file
	// symbol kind, from the same cached text.

	Vector<FileCalls> allCalls;
	Vector<ValueTableSite> valueTables;

	for(int i = 0; i < rawContents.count (); i++)
	{
		FileCalls fileCalls;
		if(!scanFile (fileCalls, valueTables, errorMsg, rawContents.at (i), relFiles.at (i), symbols, macroNames))
		{
			Logging::error (String (errorMsg));
			return false;
		}
		allCalls.add (fileCalls);
	}

	if(!extractStringIdSymbols (symbols, errorMsg, allCalls))
	{
		Logging::error (String (errorMsg));
		return false;
	}

	ParsedSites sites;
	sites.valueTables = valueTables;
	for(int i = 0; i < allCalls.count (); i++)
	{
		if(!processCallsForFile (sites, errorMsg, allCalls.at (i).calls, allCalls.at (i).file, aliases))
		{
			Logging::error (String (errorMsg));
			return false;
		}
	}

	Vector<ResolvedEntry> entries;
	if(!linkEntries (entries, errorMsg, sites, symbols, libraryToken))
	{
		Logging::error (String (errorMsg));
		return false;
	}

	Vector<ResolvedEnumeration> enumerations;
	if(!linkEnumerations (enumerations, errorMsg, sites, symbols, libraryToken))
	{
		Logging::error (String (errorMsg));
		return false;
	}

	// Emit in display-name order for reproducible output, matching ClassScanner's own convention.
	entries.sort (LAMBDA_VECTOR_COMPARE_OBJECT (ResolvedEntry, lhs, rhs)
		return lhs->tagName.compare (rhs->tagName);
	});
	enumerations.sort (LAMBDA_VECTOR_COMPARE_OBJECT (ResolvedEnumeration, lhs, rhs)
		return lhs->name.compare (rhs->name);
	});

	for(int i = 0; i < entries.count (); i++)
		repository.addClass (buildClass (entries.at (i)));

	for(int i = 0; i < enumerations.count (); i++)
		repository.addEnumeration (buildEnumeration (enumerations.at (i)));

	Logging::info ("scanned '%(1)' for type library '%(2)': %(3) class(es), %(4) enumeration(s)",
		rootDisplay, String (libraryToken), Variant (entries.count ()), Variant (enumerations.count ()));

	return true;
}
