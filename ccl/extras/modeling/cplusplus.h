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
// Filename    : ccl/extras/modeling/cplusplus.h
// Description : C++ Code Strings
//
//************************************************************************************************

#ifndef _ccl_cplusplus_h
#define _ccl_cplusplus_h

#include "ccl/public/collections/vector.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/storage/iurl.h"

namespace CCL {
namespace Model {
namespace Cpp {

//////////////////////////////////////////////////////////////////////////////////////////////////
// File types
//////////////////////////////////////////////////////////////////////////////////////////////////

inline const FileType& HeaderFile ()
{
	static const FileType fileType (nullptr, "h");
	return fileType;
}

inline const FileType& SourceFile ()
{
	static const FileType fileType (nullptr, "cpp");
	return fileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// General
//////////////////////////////////////////////////////////////////////////////////////////////////

static const String nl ("\n"); // use consistent line ending style on all platforms
static const String tab ("\t");
static const String openBrace ("{");
static const String closeBrace ("}");

struct CommentLine: String
{
	CommentLine (StringRef comment)
	{
		*this << "// " << comment << nl;
	}
};

struct Literal: String
{
	Literal (StringRef text)
	{
		*this << "\"" << text << "\"";
	}

	Literal (CStringPtr text)
	{
		*this << "\"" << text << "\"";
	}
};

struct ValidName: String
{
	ValidName (StringRef name)
	{
		append (name);
		replace (".", "_");
		replace (" ", "_");
		replace ("-", "_");
		replace ("/", "_");
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Preprocessor
//////////////////////////////////////////////////////////////////////////////////////////////////

struct Include: String
{
	Include (StringRef fileName)
	{
		*this << "#include " << Literal (fileName) << nl;
	}
};

struct IncludeGuardName: String
{
	IncludeGuardName (UrlRef path)
	{
		String fileName;
		path.getName (fileName, false);
		*this << "_" << ValidName (fileName) << "_h";
	}
};

struct IncludeGuard: String
{
	IncludeGuard (StringRef name, bool begin)
	{
		if(begin)
		{
			*this << "#ifndef " << name << nl;
			*this << "#define " << name << nl;
		}
		else
			*this << "#endif " << CommentLine (name);
	}
};

struct MacroIf: String
{
	MacroIf (StringRef condition, bool begin)
	{
		if(begin)
			*this << "#if " << condition << nl;
		else
			*this << "#endif " << CommentLine (condition);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Namespaces
//////////////////////////////////////////////////////////////////////////////////////////////////

struct Namespace: String
{
	Namespace (StringRef name, bool begin)
	{
		if(begin)
			*this << "namespace " << name << " {" << nl;
		else
			*this << "} " << CommentLine (String () << "namespace " << name);
	}
};

struct UsingNamespace: String
{
	UsingNamespace (StringRef name)
	{
		*this << "using namespace " << name << ";" << nl;
	}
};

struct NamespaceBuilder
{
	Vector<String> namespaceList;
	
	NamespaceBuilder (StringRef namespaces, StringRef delimiters = ":")
	{
		ForEachStringToken (namespaces, delimiters, namespaceName)
			namespaceList.add (namespaceName);
		EndFor
	}

	String asCode (bool begin)
	{
		String code;
		if(begin)
		{
			VectorForEach (namespaceList, String, s)
				code << Namespace (s, begin);
			EndFor	
		}
		else
		{
			VectorForEachReverse (namespaceList, String, s)
				code << Namespace (s, begin);
			EndFor	
		}
		return code;
	}

	String asUsing ()
	{
		String code;
		VectorForEach (namespaceList, String, s)
			code << UsingNamespace (s);
		EndFor	
		return code;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Enumerations
//////////////////////////////////////////////////////////////////////////////////////////////////

struct BeginEnum: String
{
	BeginEnum (StringRef name)
	{
		*this << "enum " << name << nl;
		*this << "{" << nl;
	}
};

struct EnumValue: String 
{
	EnumValue (StringRef name, int value, bool last)
	{
		*this << tab << name << " = " << value;
		if(!last)
			*this << ",";
		*this << nl;
	}
};

struct EndEnum: String
{
	EndEnum ()
	{
		*this << "};" << nl;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Classes
//////////////////////////////////////////////////////////////////////////////////////////////////

struct Access: String
{
	enum Specifier
	{
		kPrivate,
		kProtected,
		kPublic
	};

	Access (Specifier which)
	{
		switch(which)
		{
		case kPrivate : *this << "private"; break;
		case kProtected : *this << "protected"; break;
		case kPublic : *this << "public"; break;
		}
	}
};

struct ClassSection: Access
{
	ClassSection (Specifier which)
	: Access (which)
	{
		*this << ":" << nl;
	}
};

struct BeginClass: String
{
	BeginClass (StringRef className, 
				StringRef baseClass = nullptr, Access::Specifier baseClassAccess = Access::kPrivate,
				StringRef secondBaseClass = nullptr, Access::Specifier secondBaseClassAccess = Access::kPrivate)
	{
		*this << "class " << className;
		if(!baseClass.isEmpty ())
		{
			*this << ": " << Access (baseClassAccess) << " " << baseClass;
			if(!secondBaseClass.isEmpty ())
				*this << ", " << Access (secondBaseClassAccess) << " " << secondBaseClass;
		}
		*this << nl;
		*this << "{" << nl;
	}
};

struct DeclareMember: String
{
	DeclareMember (StringRef type, StringRef name)
	{
		*this << type << " " << name << ";" << nl;
	}
};

struct DeclareCtor: String
{
	DeclareCtor (StringRef className, StringRef arguments = nullptr)
	{
		*this << className << " (" << arguments << ");" << nl;
	}
};

struct BeginCtor: String
{
	BeginCtor (StringRef className, StringRef arguments = nullptr)
	{
		*this << className << "::" << className << " (" << arguments << ")" << nl;
	}
};

typedef EndEnum EndClass;

struct CallMethod: String
{
	CallMethod (StringRef member, StringRef method, StringRef arguments)
	{
		if(!member.isEmpty ())
			*this << member << ".";
		*this << method << " (" << arguments << ");" << nl;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Cpp
} // namespace Model
} // namespace CCL

#endif // _ccl_cplusplus_h
