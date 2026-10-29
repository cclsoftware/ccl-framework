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
// Filename    : classscanner.cpp
// Description : Static Class Model Source Scanner
//
//************************************************************************************************

#include "classscanner.h"
#include "ccl/extras/modeling/classrepository.h"
#include "macrocallscanner.h"
#include "sourcefiletraverser.h"
#include "sourcelocation.h"
#include "symbolresolver.h"

#include "ccl/base/storage/textfile.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/system/logging.h"

using namespace CCL;
using namespace Model;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Recognized macro names (see ccl/base/objectmacros.h)
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassScannerInternal {

const CStringPtr kMacroNames[] =
{
	"DEFINE_CLASS", "DEFINE_CLASS_ABSTRACT", "DEFINE_CLASS_HIDDEN", "DEFINE_CLASS_ABSTRACT_HIDDEN",
	"DEFINE_CLASS_PERSISTENT", "DEFINE_SINGLETON_CLASS", "DEFINE_BASE_CLASS", "DEFINE_CLASS_NAMESPACE",
	"DEFINE_CLASS_FLAGS",
	"BEGIN_METHOD_NAMES", "END_METHOD_NAMES",
	"DEFINE_METHOD_NAME", "DEFINE_METHOD_ARGS", "DEFINE_METHOD_ARGR",
	"BEGIN_PROPERTY_NAMES", "END_PROPERTY_NAMES",
	"DEFINE_PROPERTY_NAME", "DEFINE_PROPERTY_TYPE", "DEFINE_PROPERTY_CLASS", "DEFINE_PROPERTY_CLASS_",
	"DEFINE_PROPERTY_METACLASS", "DEFINE_PROPERTY_CONTAINER",
	"DEFINE_STRINGID", "DEFINE_STRINGID_", "DEFINE_STRINGID_MEMBER", "DEFINE_STRINGID_MEMBER_",
	"REGISTER_STUB_CLASS"
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Intermediate data model
//////////////////////////////////////////////////////////////////////////////////////////////////

struct FileCalls
{
	MutableCString file;
	Vector<ScannedMacroCall> calls;
};

struct MethodEntry
{
	MutableCString name;
	MutableCString args;	// resolved text, ready for ITypeInfo::MethodDefinition
	MutableCString retval;	// resolved text
};

struct MemberEntry
{
	MutableCString name;
	DataType type = ITypeInfo::kVoid;
	MutableCString typeName;
	bool readOnly = false;
};

struct ClassDefEvent
{
	MutableCString bareName;
	MutableCString displayName;
	MutableCString parentRaw;
	bool hasParent = false;
	MutableCString kind; // "normal", "abstract", "hidden", "abstract_hidden"
	MutableCString file;
	int line = 0;
	MutableCString persistentName; // DEFINE_CLASS_PERSISTENT's 3rd arg, resolved; empty if none
};

struct NamespaceEvent
{
	MutableCString bareName;
	MutableCString namespaceName;
	MutableCString file;
	int line = 0;
};

struct ClassFlagsEvent
{
	MutableCString bareName;
	int flags = 0;
	MutableCString file;
	int line = 0;
};

// A REGISTER_STUB_CLASS(Interface, Stub) call site - these register interfaces with
// StubFactory at runtime, so linkClasses() synthesizes UID members for them below.
struct StubClassEvent
{
	MutableCString interfaceName;
	MutableCString file;
	int line = 0;
};

struct PendingBlock
{
	bool method = true;
	MutableCString bareName;
	MutableCString file;
	int line = 0;
	Vector<MethodEntry> methodItems;
	Vector<MemberEntry> memberItems;
};

struct ClassRecord
{
	MutableCString bareName;
	MutableCString displayName;
	MutableCString parentLookupKey;
	MutableCString parentDisplayName;
	MutableCString file;
	int line = 0;
	MutableCString namespaceName;
	MutableCString parentNamespace;
	bool abstract = false;
	int explicitFlags = 0; // additional flags from DEFINE_CLASS_FLAGS, e.g. ITypeInfo::kMutable
	bool hasMethodBlock = false;
	bool hasPropertyBlock = false;
	Vector<MemberEntry> members;
	Vector<MethodEntry> methods;
	MutableCString persistentName; // DEFINE_CLASS_PERSISTENT's 3rd arg, resolved; empty if none

	bool isScriptable () const { return hasMethodBlock || hasPropertyBlock; }
};

// Output of processCallsForFile(), consumed by linkClasses().
struct ParsedEvents
{
	Vector<ClassDefEvent> classDefs;
	Vector<NamespaceEvent> namespaceEvents;
	Vector<ClassFlagsEvent> classFlagsEvents;
	Vector<PendingBlock> pendingBlocks;
	Vector<StubClassEvent> stubClassEvents;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void splitPipeSeparated (Vector<MutableCString>& parts, CStringRef raw)
{
	// Splits 'raw' on '|' into 'parts', e.g. "kA|kB|kC" -> "kA", "kB", "kC".

	MutableCString rawCopy (raw);
	CStringPtr text = rawCopy.str ();
	int length = rawCopy.length ();
	int start = 0;
	for(int i = 0; i <= length; i++)
	{
		if(i == length || text[i] == '|')
		{
			parts.add (rawCopy.subString (start, i - start));
			start = i + 1;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool matchFlagToken (DataType& outType, bool& outIsReadOnly, CStringRef rawToken)
{
	MutableCString token = SourceLocation::toBareName (rawToken);

	bool recognized = true;
	outIsReadOnly = false;
	if(token.isEmpty ())
	{
		// Empty segment, ignore.
	}
	else if(token == "kReadOnly")
		outIsReadOnly = true;
	else if(token == "kInt")
		outType = ITypeInfo::kInt;
	else if(token == "kFloat")
		outType = ITypeInfo::kFloat;
	else if(token == "kString")
		outType = ITypeInfo::kString;
	else if(token == "kBool")
		outType = ITypeInfo::kBool;
	else if(token == "kBlob")
		outType = ITypeInfo::kBlob;
	else if(token == "kObject")
		outType = ITypeInfo::kObject;
	else if(token == "kContainer")
		outType = ITypeInfo::kContainer;
	else if(token == "kVariant")
		outType = ITypeInfo::kVariant;
	else if(token == "kVoid" || token == "kComposite" || token == "kPrimitive")
		outType = ITypeInfo::kVoid;
	else
		recognized = false;

	return recognized;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool parsePropertyTypeExpr (DataType& outType, bool& outReadOnly, CStringRef raw)
{
	// Parse a DEFINE_PROPERTY_TYPE 'type' expression, e.g. "ITypeInfo::kBool | ITypeInfo::kReadOnly".

	outType = ITypeInfo::kVoid;
	outReadOnly = false;

	Vector<MutableCString> parts;
	splitPipeSeparated (parts, raw);

	for(int i = 0; i < parts.count (); i++)
	{
		DataType type = ITypeInfo::kVoid;
		bool readOnly = false;
		if(!matchFlagToken (type, readOnly, parts.at (i)))
			return false;
		if(readOnly)
			outReadOnly = true;
		else if(type != ITypeInfo::kVoid)
			outType = type;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool parseExtraFlags (bool& outReadOnly, CStringRef raw)
{
	// Parse the trailing 'flags' argument of DEFINE_PROPERTY_CLASS_, expected to only ever contain
	// ITypeInfo::kReadOnly.

	DataType type;
	return parsePropertyTypeExpr (type, outReadOnly, raw);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool matchClassFlagToken (int& outFlag, CStringRef rawToken)
{
	MutableCString token = SourceLocation::toBareName (rawToken);

	outFlag = 0;
	if(token.isEmpty ())
	{
		// Empty segment, ignore.
	}
	else if(token == "kAbstract")
		outFlag = ITypeInfo::kAbstract;
	else if(token == "kScriptable")
		outFlag = ITypeInfo::kScriptable;
	else if(token == "kSingleton")
		outFlag = ITypeInfo::kSingleton;
	else if(token == "kMutable")
		outFlag = ITypeInfo::kMutable;
	else
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool parseClassFlagsExpr (int& outFlags, CStringRef raw)
{
	// Parse a DEFINE_CLASS_FLAGS 'flags' expression, e.g. "CCL::ITypeInfo::kMutable".

	outFlags = 0;

	Vector<MutableCString> parts;
	splitPipeSeparated (parts, raw);

	for(int i = 0; i < parts.count (); i++)
	{
		int flag = 0;
		if(!matchClassFlagToken (flag, parts.at (i)))
			return false;
		outFlags |= flag;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Macro call -> model entry builders
//////////////////////////////////////////////////////////////////////////////////////////////////

bool buildClassDefine (ClassDefEvent& outEvent, MutableCString& errorMsg, const ScannedMacroCall& call, CStringRef file, SymbolRegistry& symbols)
{
	MutableCString loc = SourceLocation::makeLocation (file, call.line);
	outEvent.file = file;
	outEvent.line = call.line;

	if(call.name == "DEFINE_BASE_CLASS")
	{
		if(call.args.count () < 1)
		{
			errorMsg.appendFormat ("%s: DEFINE_BASE_CLASS expected at least 1 argument, got %d", loc.str (), call.args.count ());
			return false;
		}
		MutableCString raw = call.args.at (0);
		raw.trimWhitespace ();
		outEvent.bareName = SourceLocation::toBareName (raw);
		outEvent.displayName = SourceLocation::toCanonicalName (raw);
		outEvent.hasParent = false;
		outEvent.kind = "normal";
		return true;
	}

	if(call.args.count () < 2)
	{
		errorMsg.appendFormat ("%s: %s expected at least 2 argument(s), got %d", loc.str (), call.name.str (), call.args.count ());
		return false;
	}

	MutableCString raw = call.args.at (0);
	raw.trimWhitespace ();
	MutableCString parent = call.args.at (1);
	parent.trimWhitespace ();

	outEvent.bareName = SourceLocation::toBareName (raw);
	outEvent.displayName = SourceLocation::toCanonicalName (raw);
	outEvent.parentRaw = parent;
	outEvent.hasParent = true;

	if(call.name == "DEFINE_CLASS_ABSTRACT")
		outEvent.kind = "abstract";
	else if(call.name == "DEFINE_CLASS_HIDDEN")
		outEvent.kind = "hidden";
	else if(call.name == "DEFINE_CLASS_ABSTRACT_HIDDEN")
		outEvent.kind = "abstract_hidden";
	else
		outEvent.kind = "normal"; // DEFINE_CLASS(_PERSISTENT), DEFINE_SINGLETON_CLASS

	if(call.name == "DEFINE_CLASS_PERSISTENT" && call.args.count () >= 3)
	{
		// Mirrors MetaClass::getClassName()/getPersistentName(), which returns this 3rd
		// argument (when set) instead of the C++ class name at runtime.
		if(!SymbolResolver (symbols).resolveStringArg (outEvent.persistentName, errorMsg, call.args.at (2), loc))
			return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool buildMethodEntry (MethodEntry& outMethod, MutableCString& errorMsg, const ScannedMacroCall& call, CStringRef loc, SymbolRegistry& symbols)
{
	SymbolResolver resolver (symbols);

	outMethod.args.empty ();
	outMethod.retval.empty ();

	if(call.name == "DEFINE_METHOD_NAME")
	{
		if(call.args.count () < 1)
		{
			errorMsg.appendFormat ("%s: DEFINE_METHOD_NAME expected at least 1 argument", loc.str ());
			return false;
		}
		return resolver.resolveStringArg (outMethod.name, errorMsg, call.args.at (0), loc);
	}
	if(call.name == "DEFINE_METHOD_ARGS")
	{
		if(call.args.count () < 2)
		{
			errorMsg.appendFormat ("%s: DEFINE_METHOD_ARGS expected at least 2 arguments", loc.str ());
			return false;
		}
		if(!resolver.resolveStringArg (outMethod.name, errorMsg, call.args.at (0), loc))
			return false;
		return resolver.resolveStringArg (outMethod.args, errorMsg, call.args.at (1), loc);
	}
	if(call.name == "DEFINE_METHOD_ARGR")
	{
		if(call.args.count () < 3)
		{
			errorMsg.appendFormat ("%s: DEFINE_METHOD_ARGR expected at least 3 arguments", loc.str ());
			return false;
		}
		if(!resolver.resolveStringArg (outMethod.name, errorMsg, call.args.at (0), loc))
			return false;
		if(!resolver.resolveStringArg (outMethod.args, errorMsg, call.args.at (1), loc))
			return false;
		return resolver.resolveStringArg (outMethod.retval, errorMsg, call.args.at (2), loc);
	}

	errorMsg.appendFormat ("%s: unrecognized method macro '%s'", loc.str (), call.name.str ());

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool buildMemberEntry (MemberEntry& outMember, MutableCString& errorMsg, const ScannedMacroCall& call, CStringRef loc, SymbolRegistry& symbols)
{
	SymbolResolver resolver (symbols);

	outMember.type = ITypeInfo::kVoid;
	outMember.typeName.empty ();
	outMember.readOnly = false;

	if(call.name == "DEFINE_PROPERTY_NAME")
	{
		if(call.args.count () < 1)
		{
			errorMsg.appendFormat ("%s: DEFINE_PROPERTY_NAME expected at least 1 argument", loc.str ());
			return false;
		}
		return resolver.resolveStringArg (outMember.name, errorMsg, call.args.at (0), loc);
	}
	if(call.name == "DEFINE_PROPERTY_TYPE")
	{
		if(call.args.count () < 2)
		{
			errorMsg.appendFormat ("%s: DEFINE_PROPERTY_TYPE expected at least 2 arguments", loc.str ());
			return false;
		}
		if(!resolver.resolveStringArg (outMember.name, errorMsg, call.args.at (0), loc))
			return false;
		if(!parsePropertyTypeExpr (outMember.type, outMember.readOnly, call.args.at (1)))
		{
			errorMsg.appendFormat ("%s: unrecognized identifier in property type expression '%s'", loc.str (), call.args.at (1).str ());
			return false;
		}
		return true;
	}
	if(call.name == "DEFINE_PROPERTY_CLASS")
	{
		if(call.args.count () < 2)
		{
			errorMsg.appendFormat ("%s: DEFINE_PROPERTY_CLASS expected at least 2 arguments", loc.str ());
			return false;
		}
		if(!resolver.resolveStringArg (outMember.name, errorMsg, call.args.at (0), loc))
			return false;
		if(!resolver.resolveStringArg (outMember.typeName, errorMsg, call.args.at (1), loc))
			return false;
		outMember.type = ITypeInfo::kObject;
		return true;
	}
	if(call.name == "DEFINE_PROPERTY_CLASS_")
	{
		if(call.args.count () < 3)
		{
			errorMsg.appendFormat ("%s: DEFINE_PROPERTY_CLASS_ expected at least 3 arguments", loc.str ());
			return false;
		}
		if(!resolver.resolveStringArg (outMember.name, errorMsg, call.args.at (0), loc))
			return false;
		if(!resolver.resolveStringArg (outMember.typeName, errorMsg, call.args.at (1), loc))
			return false;
		if(!parseExtraFlags (outMember.readOnly, call.args.at (2)))
		{
			errorMsg.appendFormat ("%s: unexpected flag in property flags expression '%s'", loc.str (), call.args.at (2).str ());
			return false;
		}
		outMember.type = ITypeInfo::kObject;
		return true;
	}
	if(call.name == "DEFINE_PROPERTY_METACLASS")
	{
		if(call.args.count () < 2)
		{
			errorMsg.appendFormat ("%s: DEFINE_PROPERTY_METACLASS expected at least 2 arguments", loc.str ());
			return false;
		}
		if(!resolver.resolveStringArg (outMember.name, errorMsg, call.args.at (0), loc))
			return false;
		// The 2nd argument is stringified by the macro itself (the '#' operator): its literal
		// source text, not a #define/StringID lookup, is the type name.
		outMember.typeName = call.args.at (1);
		outMember.typeName.trimWhitespace ();
		outMember.type = ITypeInfo::kObject;
		return true;
	}
	if(call.name == "DEFINE_PROPERTY_CONTAINER")
	{
		if(call.args.count () < 2)
		{
			errorMsg.appendFormat ("%s: DEFINE_PROPERTY_CONTAINER expected at least 2 arguments", loc.str ());
			return false;
		}
		if(!resolver.resolveStringArg (outMember.name, errorMsg, call.args.at (0), loc))
			return false;
		outMember.typeName = call.args.at (1);
		outMember.typeName.trimWhitespace ();
		outMember.type = ITypeInfo::kContainer;
		return true;
	}

	errorMsg.appendFormat ("%s: unrecognized property macro '%s'", loc.str (), call.name.str ());

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Per-file processing (BEGIN/END block state machine)
//////////////////////////////////////////////////////////////////////////////////////////////////

bool processCallsForFile (ParsedEvents& outEvents, MutableCString& errorMsg, const Vector<ScannedMacroCall>& calls, CStringRef file, SymbolRegistry& symbols)
{
	bool blockOpen = false;
	PendingBlock openBlock;

	for(int ci = 0; ci < calls.count (); ci++)
	{
		const ScannedMacroCall& call = calls.at (ci);
		MutableCString loc = SourceLocation::makeLocation (file, call.line);

		if(call.name == "DEFINE_STRINGID" || call.name == "DEFINE_STRINGID_" ||
		   call.name == "DEFINE_STRINGID_MEMBER" || call.name == "DEFINE_STRINGID_MEMBER_")
			continue; // already consumed in the symbol pre-pass

		if(call.name == "DEFINE_CLASS" || call.name == "DEFINE_CLASS_ABSTRACT" ||
		   call.name == "DEFINE_CLASS_HIDDEN" || call.name == "DEFINE_CLASS_ABSTRACT_HIDDEN" ||
		   call.name == "DEFINE_CLASS_PERSISTENT" || call.name == "DEFINE_SINGLETON_CLASS" ||
		   call.name == "DEFINE_BASE_CLASS")
		{
			ClassDefEvent event;
			if(!buildClassDefine (event, errorMsg, call, file, symbols))
				return false;
			outEvents.classDefs.add (event);
			continue;
		}

		if(call.name == "DEFINE_CLASS_NAMESPACE")
		{
			if(call.args.count () < 2)
			{
				errorMsg.appendFormat ("%s: DEFINE_CLASS_NAMESPACE expected at least 2 arguments", loc.str ());
				return false;
			}
			NamespaceEvent event;
			event.bareName = SourceLocation::toBareName (call.args.at (0));
			if(!SymbolResolver (symbols).resolveStringArg (event.namespaceName, errorMsg, call.args.at (1), loc))
				return false;
			event.file = file;
			event.line = call.line;
			outEvents.namespaceEvents.add (event);
			continue;
		}

		if(call.name == "DEFINE_CLASS_FLAGS")
		{
			if(call.args.count () < 2)
			{
				errorMsg.appendFormat ("%s: DEFINE_CLASS_FLAGS expected at least 2 arguments", loc.str ());
				return false;
			}
			ClassFlagsEvent event;
			event.bareName = SourceLocation::toBareName (call.args.at (0));
			if(!parseClassFlagsExpr (event.flags, call.args.at (1)))
			{
				errorMsg.appendFormat ("%s: unrecognized identifier in class flags expression '%s'", loc.str (), call.args.at (1).str ());
				return false;
			}
			event.file = file;
			event.line = call.line;
			outEvents.classFlagsEvents.add (event);
			continue;
		}

		if(call.name == "BEGIN_METHOD_NAMES" || call.name == "BEGIN_PROPERTY_NAMES")
		{
			if(blockOpen)
			{
				errorMsg.appendFormat ("%s: %s nested inside already-open block for class '%s' (opened at %s)",
					loc.str (), call.name.str (), openBlock.bareName.str (), SourceLocation::makeLocation (openBlock.file, openBlock.line).str ());
				return false;
			}
			if(call.args.count () < 1)
			{
				errorMsg.appendFormat ("%s: %s expected at least 1 argument", loc.str (), call.name.str ());
				return false;
			}

			openBlock = PendingBlock ();
			openBlock.method = (call.name == "BEGIN_METHOD_NAMES");
			openBlock.bareName = SourceLocation::toBareName (call.args.at (0));
			openBlock.file = file;
			openBlock.line = call.line;
			blockOpen = true;
			continue;
		}

		if(call.name == "END_METHOD_NAMES" || call.name == "END_PROPERTY_NAMES")
		{
			bool expectMethod = (call.name == "END_METHOD_NAMES");
			if(!blockOpen || openBlock.method != expectMethod)
			{
				errorMsg.appendFormat ("%s: %s without a matching %s", loc.str (), call.name.str (),
					expectMethod ? "BEGIN_METHOD_NAMES" : "BEGIN_PROPERTY_NAMES");
				return false;
			}
			if(call.args.count () < 1)
			{
				errorMsg.appendFormat ("%s: %s expected at least 1 argument", loc.str (), call.name.str ());
				return false;
			}

			MutableCString closeName = SourceLocation::toBareName (call.args.at (0));
			if(closeName != openBlock.bareName)
			{
				errorMsg.appendFormat ("%s: %s(%s) does not match open block for class '%s' (opened at %s)",
					loc.str (), call.name.str (), closeName.str (), openBlock.bareName.str (), SourceLocation::makeLocation (openBlock.file, openBlock.line).str ());
				return false;
			}
			outEvents.pendingBlocks.add (openBlock);
			blockOpen = false;
			continue;
		}

		if(call.name == "DEFINE_METHOD_NAME" || call.name == "DEFINE_METHOD_ARGS" || call.name == "DEFINE_METHOD_ARGR")
		{
			if(!blockOpen || !openBlock.method)
			{
				errorMsg.appendFormat ("%s: %s used outside of a BEGIN_METHOD_NAMES/END_METHOD_NAMES block", loc.str (), call.name.str ());
				return false;
			}
			MethodEntry methodEntry;
			if(!buildMethodEntry (methodEntry, errorMsg, call, loc, symbols))
				return false;
			openBlock.methodItems.add (methodEntry);
			continue;
		}

		if(call.name == "DEFINE_PROPERTY_NAME" || call.name == "DEFINE_PROPERTY_TYPE" ||
		   call.name == "DEFINE_PROPERTY_CLASS" || call.name == "DEFINE_PROPERTY_CLASS_" ||
		   call.name == "DEFINE_PROPERTY_METACLASS" || call.name == "DEFINE_PROPERTY_CONTAINER")
		{
			if(!blockOpen || openBlock.method)
			{
				errorMsg.appendFormat ("%s: %s used outside of a BEGIN_PROPERTY_NAMES/END_PROPERTY_NAMES block", loc.str (), call.name.str ());
				return false;
			}
			MemberEntry member;
			if(!buildMemberEntry (member, errorMsg, call, loc, symbols))
				return false;
			openBlock.memberItems.add (member);
			continue;
		}

		if(call.name == "REGISTER_STUB_CLASS")
		{
			// Standalone call, unrelated by lexical scope to the InterfaceList class that
			// ends up exposing it - see linkClasses().
			if(call.args.count () < 1)
			{
				errorMsg.appendFormat ("%s: REGISTER_STUB_CLASS expected at least 1 argument", loc.str ());
				return false;
			}

			// 'Interface' is stringified by the macro's own '#' operator - take the raw
			// source text verbatim, same rationale as DEFINE_PROPERTY_METACLASS's type name.
			StubClassEvent event;
			event.interfaceName = call.args.at (0);
			event.interfaceName.trimWhitespace ();
			event.file = file;
			event.line = call.line;
			outEvents.stubClassEvents.add (event);
			continue;
		}

		errorMsg.appendFormat ("%s: internal error - unhandled recognized macro '%s'", loc.str (), call.name.str ());
		return false;
	}

	if(blockOpen)
	{
		errorMsg.appendFormat ("%s: unterminated %s block for class '%s' opened at %s", file.str (),
			openBlock.method ? "method" : "property", openBlock.bareName.str (), SourceLocation::makeLocation (openBlock.file, openBlock.line).str ());
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool extractStringIdSymbols (SymbolRegistry& symbols, MutableCString& errorMsg, const Vector<FileCalls>& allCalls)
{
	// Pre-pass populating 'symbols' from DEFINE_STRINGID/DEFINE_STRINGID_MEMBER calls, so they're
	// available like #define constants once processCallsForFile() resolves class/method/property data.

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
// Linking (bare-name registry, parent/namespace resolution, disambiguation)
//////////////////////////////////////////////////////////////////////////////////////////////////

void findCandidates (Vector<int>& outIndices, const Vector<ClassRecord>& registry, CStringRef bareNameKey)
{
	for(int i = 0; i < registry.count (); i++)
		if(registry.at (i).bareName == bareNameKey)
			outIndices.add (i);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void noteUnregisteredReference (CStringRef location, CStringPtr macroDesc, CStringRef className)
{
	// A block referenced a class with no matching DEFINE_CLASS in the scanned root - expected, not
	// an error, for classes declared only via an out-of-scope vendor macro family.

	Logging::warning ("%(1): %(2) references '%(3)', which has no matching DEFINE_CLASS in scope "
		"(likely declared via an unsupported vendor macro family) - skipped",
		String (location), String (macroDesc), String (className));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool selectCandidate (int& outIndex, MutableCString& errorMsg, const Vector<ClassRecord>& registry,
	const Vector<int>& indices, CStringRef location, CStringPtr macroDesc)
{
	// Pick which same-named candidate a file-scoped reference belongs to, preferring the one
	// declared in the same file as 'location'.

	if(indices.count () == 1)
	{
		outIndex = indices.at (0);
		return true;
	}

	MutableCString refFile = SourceLocation::toLocationFile (location);
	Vector<int> sameFile;
	for(int i = 0; i < indices.count (); i++)
		if(registry.at (indices.at (i)).file == refFile)
			sameFile.add (indices.at (i));

	if(sameFile.count () == 1)
	{
		outIndex = sameFile.at (0);
		return true;
	}

	MutableCString locations;
	for(int i = 0; i < indices.count (); i++)
	{
		if(i > 0)
			locations += ", ";
		locations += SourceLocation::makeLocation (registry.at (indices.at (i)).file, registry.at (indices.at (i)).line);
	}
	errorMsg.appendFormat ("%s: ambiguous %s for '%s' - %d same-named classes declared in different files, %s matching this reference's file (%s)",
		location.str (), macroDesc, registry.at (indices.at (0)).bareName.str (), indices.count (),
		sameFile.isEmpty () ? "none" : "more than one", locations.str ());

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString effectiveExportName (const ClassRecord& record)
{
	// The name a class is exported under: its persistent storage name if DEFINE_CLASS_PERSISTENT
	// set one, else its plain (canonicalized) C++ class name - mirrors MetaClass::getClassName()/
	// getPersistentName() at runtime.

	return record.persistentName.isEmpty () ? record.displayName : SourceLocation::toCanonicalName (record.persistentName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool linkClasses (Vector<ClassRecord>& outRegistry, MutableCString& errorMsg, const ParsedEvents& events)
{
	for(int i = 0; i < events.classDefs.count (); i++)
	{
		const ClassDefEvent& event = events.classDefs.at (i);

		Vector<int> candidates;
		findCandidates (candidates, outRegistry, event.bareName);
		for(int k = 0; k < candidates.count (); k++)
		{
			const ClassRecord& existing = outRegistry.at (candidates.at (k));
			if(existing.displayName == event.displayName && existing.file == event.file)
			{
				errorMsg.appendFormat ("%s: class '%s' already defined at %s",
					SourceLocation::makeLocation (event.file, event.line).str (), event.displayName.str (), SourceLocation::makeLocation (existing.file, existing.line).str ());
				return false;
			}
		}

		ClassRecord record;
		record.bareName = event.bareName;
		record.displayName = event.displayName;
		record.persistentName = event.persistentName;
		record.file = event.file;
		record.line = event.line;
		record.abstract = (event.kind == "abstract" || event.kind == "abstract_hidden");
		if(event.hasParent)
		{
			record.parentLookupKey = SourceLocation::toBareName (event.parentRaw);
			record.parentDisplayName = SourceLocation::toCanonicalName (event.parentRaw);
		}
		outRegistry.add (record);
	}

	for(int i = 0; i < events.namespaceEvents.count (); i++)
	{
		const NamespaceEvent& event = events.namespaceEvents.at (i);
		MutableCString loc = SourceLocation::makeLocation (event.file, event.line);

		Vector<int> candidates;
		findCandidates (candidates, outRegistry, event.bareName);
		if(candidates.isEmpty ())
		{
			noteUnregisteredReference (loc, "DEFINE_CLASS_NAMESPACE", event.bareName);
			continue;
		}
		int candidateIndex = 0;
		if(!selectCandidate (candidateIndex, errorMsg, outRegistry, candidates, loc, "DEFINE_CLASS_NAMESPACE"))
			return false;
		outRegistry.at (candidateIndex).namespaceName = event.namespaceName;
	}

	for(int i = 0; i < events.classFlagsEvents.count (); i++)
	{
		const ClassFlagsEvent& event = events.classFlagsEvents.at (i);
		MutableCString loc = SourceLocation::makeLocation (event.file, event.line);

		Vector<int> candidates;
		findCandidates (candidates, outRegistry, event.bareName);
		if(candidates.isEmpty ())
		{
			noteUnregisteredReference (loc, "DEFINE_CLASS_FLAGS", event.bareName);
			continue;
		}
		int candidateIndex = 0;
		if(!selectCandidate (candidateIndex, errorMsg, outRegistry, candidates, loc, "DEFINE_CLASS_FLAGS"))
			return false;
		outRegistry.at (candidateIndex).explicitFlags |= event.flags;
	}

	// Best-effort parent name/namespace resolution - only possible when the parent is also
	// declared unambiguously somewhere in this scanned root.
	for(int i = 0; i < outRegistry.count (); i++)
	{
		ClassRecord& record = outRegistry.at (i);
		if(record.parentLookupKey.isEmpty ())
			continue;
		Vector<int> parentCandidates;
		findCandidates (parentCandidates, outRegistry, record.parentLookupKey);

		int parentIndex = -1;
		if(parentCandidates.count () == 1)
			parentIndex = parentCandidates.at (0);
		else if(parentCandidates.count () > 1)
		{
			// The bare lookup key is ambiguous (e.g. a nested "Outer::Name" collides with an
			// unrelated top-level "Name") - disambiguate using the fully qualified parent name.
			Vector<int> exactMatch;
			for(int k = 0; k < parentCandidates.count (); k++)
				if(outRegistry.at (parentCandidates.at (k)).displayName == record.parentDisplayName)
					exactMatch.add (parentCandidates.at (k));
			if(exactMatch.count () == 1)
				parentIndex = exactMatch.at (0);
		}

		if(parentIndex >= 0)
		{
			const ClassRecord& parentRecord = outRegistry.at (parentIndex);
			record.parentDisplayName = effectiveExportName (parentRecord);
			if(!parentRecord.namespaceName.isEmpty ())
				record.parentNamespace = parentRecord.namespaceName;
		}
	}

	for(int i = 0; i < events.pendingBlocks.count (); i++)
	{
		const PendingBlock& block = events.pendingBlocks.at (i);
		MutableCString loc = SourceLocation::makeLocation (block.file, block.line);
		CStringPtr macroDesc = block.method ? "BEGIN_METHOD_NAMES" : "BEGIN_PROPERTY_NAMES";

		Vector<int> candidates;
		findCandidates (candidates, outRegistry, block.bareName);
		if(candidates.isEmpty ())
		{
			noteUnregisteredReference (loc, macroDesc, block.bareName);
			continue;
		}
		int candidateIndex = 0;
		if(!selectCandidate (candidateIndex, errorMsg, outRegistry, candidates, loc, macroDesc))
			return false;

		ClassRecord& record = outRegistry.at (candidateIndex);
		if(block.method)
		{
			record.hasMethodBlock = true;
			for(int k = 0; k < block.methodItems.count (); k++)
				record.methods.add (block.methodItems.at (k));
		}
		else
		{
			record.hasPropertyBlock = true;
			for(int k = 0; k < block.memberItems.count (); k++)
				record.members.add (block.memberItems.at (k));
		}
	}

	// REGISTER_STUB_CLASS(Interface, Stub) calls register interfaces with StubFactory at
	// runtime rather than through a static property table, so synthesize a readonly UID
	// member per distinct interface name - same shape as a hand-written DEFINE_PROPERTY_CLASS_.
	if(!events.stubClassEvents.isEmpty ())
	{
		Vector<int> candidates;
		findCandidates (candidates, outRegistry, "InterfaceList");
		if(candidates.count () == 1)
		{
			ClassRecord& record = outRegistry.at (candidates.at (0));
			Vector<MutableCString> seen;
			for(int i = 0; i < events.stubClassEvents.count (); i++)
			{
				const StubClassEvent& event = events.stubClassEvents.at (i);

				bool dup = false;
				for(int s = 0; s < seen.count () && !dup; s++)
					dup = (seen.at (s) == event.interfaceName);
				if(dup)
					continue;
				seen.add (event.interfaceName);

				bool alreadyMember = false;
				for(int m = 0; m < record.members.count () && !alreadyMember; m++)
					alreadyMember = (record.members.at (m).name == event.interfaceName);
				if(alreadyMember)
					continue;

				MemberEntry member;
				member.name = event.interfaceName;
				member.type = ITypeInfo::kObject;
				member.typeName = "UID";
				member.readOnly = true;
				record.members.add (member);
			}
			record.hasPropertyBlock = true;
		}
		else if(candidates.count () > 1)
			Logging::warning ("ambiguous 'InterfaceList' class - skipped REGISTER_STUB_CLASS member synthesis");
		else
			Logging::warning ("found %(1) REGISTER_STUB_CLASS call(s) but no 'InterfaceList' class in scope - skipped member synthesis",
				Variant (events.stubClassEvents.count ()));
	}

	return true;
}

//************************************************************************************************
// ClassScannerInternal::FileCollector
/** Collects matching files up front (rather than processing them inline as DocumentationScanner
 does) so the "found N source file(s)" log line's count is known before any file is parsed -
 preserves that log line's exact wording/timing from before this was re-hosted on the shared walker. */
//************************************************************************************************

class FileCollector: public SourceFileVisitor
{
public:
	Vector<Url> files;

	void visitFile (UrlRef path) override
	{
		files.add (path);
	}
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

bool scanFile (FileCalls& outFileCalls, MutableCString& errorMsg, UrlRef path, CStringRef relFile, SymbolRegistry& symbols)
{
	String rawUnicode = TextUtils::loadRawString (path);
	MutableCString raw (rawUnicode, Text::kASCII);

	outFileCalls.file = relFile;

	Vector<ScannedDefine> defines;
	if(!MacroCallScanner::scan (outFileCalls.calls, defines, errorMsg, raw, recognizedMacroNames ()))
		return false;

	SymbolCollector collector (symbols);
	for(int i = 0; i < defines.count (); i++)
		collector.addDefine (defines.at (i), relFile);

	return true;
}

} // namespace ClassScannerInternal

using namespace ClassScannerInternal;

//************************************************************************************************
// ClassScanner
//************************************************************************************************

bool ClassScanner::scan (ClassRepository& repository, const Vector<Url>& sourceRoots)
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
	Vector<FileCalls> allCalls;
	MutableCString errorMsg;

	for(int i = 0; i < files.count (); i++)
	{
		UrlRef filePath = files.at (i);
		String relDisplay;
		filePath.toDisplayString (relDisplay);
		MutableCString relFile (relDisplay, Text::kASCII);

		FileCalls fileCalls;
		if(!scanFile (fileCalls, errorMsg, filePath, relFile, symbols))
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

	ParsedEvents events;
	for(int i = 0; i < allCalls.count (); i++)
	{
		if(!processCallsForFile (events, errorMsg, allCalls.at (i).calls, allCalls.at (i).file, symbols))
		{
			Logging::error (String (errorMsg));
			return false;
		}
	}

	Vector<ClassRecord> registry;
	if(!linkClasses (registry, errorMsg, events))
	{
		Logging::error (String (errorMsg));
		return false;
	}

	// Emit in display-name order for reproducible output, rather than filesystem scan order.
	registry.sort (LAMBDA_VECTOR_COMPARE_OBJECT (ClassRecord, lhs, rhs)
		return lhs->displayName.compare (rhs->displayName);
	});

	int classCount = 0;
	for(int i = 0; i < registry.count (); i++)
	{
		ClassRecord& record = registry.at (i);

		Class* c = NEW Class (effectiveExportName (record));
		if(!record.namespaceName.isEmpty ())
			c->setNamespaceName (record.namespaceName);
		if(!record.parentDisplayName.isEmpty ())
			c->setParentName (record.parentDisplayName);
		if(!record.parentNamespace.isEmpty ())
			c->setParentNamespace (record.parentNamespace);

		int flags = 0;
		if(record.abstract)
			flags |= ITypeInfo::kAbstract;
		if(record.isScriptable ())
			flags |= ITypeInfo::kScriptable;
		flags |= record.explicitFlags;
		c->setFlags (flags);

		for(int m = 0; m < record.members.count (); m++)
		{
			const MemberEntry& member = record.members.at (m);
			// Variable's constructor derives isReadOnly() from a kReadOnly bit ORed into 'type',
			// matching how a real DEFINE_PROPERTY_TYPE (..., ITypeInfo::kReadOnly) expression works.
			DataType memberType = member.readOnly ? static_cast<DataType> (member.type | ITypeInfo::kReadOnly) : member.type;
			c->addMember (NEW Member (member.name, memberType, member.typeName));
		}

		for(int m = 0; m < record.methods.count (); m++)
		{
			const MethodEntry& methodEntry = record.methods.at (m);
			Method* method = NEW Method (methodEntry.name);

			ITypeInfo::MethodDefinition def;
			def.name = methodEntry.name;
			def.args = methodEntry.args.isEmpty () ? nullptr : methodEntry.args.str ();
			def.retval = methodEntry.retval.isEmpty () ? nullptr : methodEntry.retval.str ();
			method->assign (def);

			c->addMethod (method);
		}

		repository.addClass (c);
		classCount++;
	}

	Logging::info ("scanned '%(1)': %(2) class(es)", rootDisplay, Variant (classCount));

	return true;
}
