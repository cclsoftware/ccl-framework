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
// Filename    : ccl/public/plugins/iscriptengine.h
// Description : Scripting Interfaces
//
//************************************************************************************************

#ifndef _ccl_iscriptengine_h
#define _ccl_iscriptengine_h

#include "ccl/public/system/alerttypes.h"

namespace CCL {

class FileType;
interface IObject;
interface IUnknownIterator;

namespace Scripting {

interface IContext;
interface IScript;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Component Categories
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Class category for scripting engines. */
#define PLUG_CATEGORY_SCRIPTENGINE CCLSTR ("ScriptEngine")

//////////////////////////////////////////////////////////////////////////////////////////////////
// Scripting Languages
//////////////////////////////////////////////////////////////////////////////////////////////////

/** JavaScript language identifier (RFC 4329). */
DEFINE_STRINGID (kJavaScript, "application/javascript")

//************************************************************************************************
// Scripting::CodePiece
/** Describes a piece of script code. */
//************************************************************************************************

struct CodePiece
{
	const uchar* code;		///< the actual script code
	int length;				///< length in characters
	String fileName;		///< filename for debugging
	int lineNumber;			///< line number for debugging

	CodePiece (const uchar* code = nullptr, unsigned int length = 0,
			   StringRef fileName = nullptr, int lineNumber = 0)
	: code (code),
	  length (length),
	  fileName (fileName),
	  lineNumber (lineNumber)
	{}
};

//************************************************************************************************
// Scripting::IEngine
/** Runtime component for scripting support, can be implemented for different languages. */
//************************************************************************************************

interface IEngine: IUnknown
{
	/** Get scripting language description. */
	virtual const FileType& CCL_API getLanguage () const = 0;

	/** Set engine option, must be done before IComponent::initialize(). */
	virtual tresult CCL_API setOption (StringID id, VariantRef value) = 0;

	/** Create new context for script execution. */
	virtual IContext* CCL_API createContext () = 0;

	// Engine Options
	DECLARE_STRINGID_MEMBER (kGCThreshold)     ///< bytes before garbage collection
	DECLARE_STRINGID_MEMBER (kJITThreshold)    ///< number of calls before switch from interpreter to compiler
	DECLARE_STRINGID_MEMBER (kDebugProtocolID) ///< create debug contexts using the specified protocol

	DECLARE_IID (IEngine)
};

DEFINE_IID (IEngine, 0x24d03483, 0xac99, 0x41ea, 0x97, 0x8, 0xe6, 0xfa, 0xf0, 0x2f, 0x43, 0xb2)
DEFINE_STRINGID_MEMBER (IEngine, kGCThreshold, "gcThreshold")
DEFINE_STRINGID_MEMBER (IEngine, kJITThreshold, "jitThreshold")
DEFINE_STRINGID_MEMBER (IEngine, kDebugProtocolID, "debugProtocolId")

//************************************************************************************************
// Scripting::IContext
/** Context for script execution, usually one per thread. */
//************************************************************************************************

interface IContext: IUnknown
{
	/** Get engine owning this context. */
	virtual IEngine& CCL_API getEngine () const = 0;
	
	/** Set context option. */
	virtual tresult CCL_API setOption (StringID id, VariantRef value) = 0;

	/** Attach module to context.  */
	virtual void CCL_API attachModule (ModuleRef module) = 0;
	
	/** Detach module from context. */
	virtual void CCL_API detachModule (ModuleRef module) = 0;
	
	/** Set alert reporter. */
	virtual tresult CCL_API setReporter (Alert::IReporter* reporter) = 0;

	/** Register global object (shared by context). */
	virtual tresult CCL_API registerObject (CStringRef name, IObject* nativeObject) = 0;
	
	/** Create script object. For JavaScript, class name can be "Object", "Array", "Int8Array", etc. */
	virtual IObject* CCL_API createObject (CStringRef className, const Variant args[] = nullptr, int argCount = 0) = 0;

	/** Register object method as global function. */
	virtual tresult CCL_API registerGlobalFunction (CStringRef methodName, CCL::IObject* nativeObject) = 0;

	/** Execute script directly. */
	virtual tresult CCL_API executeScript (Variant& returnValue, const IScript& script) = 0;
	
	/** Compile script, can be executed later via IObject::invokeMethod. */
	virtual IObject* CCL_API compileScript (const IScript& script) = 0;

	/** Check garbage collection. */
	virtual void CCL_API garbageCollect (tbool force) = 0;

	/** Remove reference to given native object from context. */
	virtual tbool CCL_API removeReference (IUnknown* nativeObject) = 0;

	/** Dump context information to debug output. */
	virtual void CCL_API dump () = 0;

	// Context Options
	DECLARE_STRINGID_MEMBER (kStubObjectsEnabled)	///< enable stub object creation
	DECLARE_STRINGID_MEMBER (kHostStringsEnabled)	///< enable host strings, i.e. use CCL::String instead of IStringValue
	DECLARE_STRINGID_MEMBER (kLogMemoryAllocations)	///< enable memory allocation logging (debug build)

	DECLARE_IID (IContext)
};

DEFINE_IID (IContext, 0xd0ec6a01, 0x3eeb, 0x4ca6, 0xb9, 0x3d, 0x7c, 0x51, 0xb8, 0x1c, 0x81, 0x19)
DEFINE_STRINGID_MEMBER (IContext, kStubObjectsEnabled, "stubsEnabled")
DEFINE_STRINGID_MEMBER (IContext, kHostStringsEnabled, "stringsEnabled")
DEFINE_STRINGID_MEMBER (IContext, kLogMemoryAllocations, "logMallocs")

//************************************************************************************************
// Scripting::IStringValue
/** Script string value interface. */
//************************************************************************************************

interface IStringValue: IUnknown
{
	/** Get string data as uchar. */
	virtual const uchar* CCL_API getUCharData () const = 0;

	/** Get string length. */
	virtual int CCL_API getLength () const = 0;

	/** Get string data as char. */
	virtual const char* CCL_API getCharData () const = 0;

	/** Get character encoding. */
	virtual TextEncoding CCL_API getEncoding () const = 0;

	DECLARE_IID (IStringValue)
};

DEFINE_IID (IStringValue, 0x1e223bce, 0x9116, 0x4959, 0xad, 0x95, 0x59, 0x4e, 0xcb, 0xf1, 0xd5, 0xe)

//************************************************************************************************
// Scripting::IFunction
/** Script function interface. */
//************************************************************************************************

interface IFunction: IUnknown
{
	/** Call function. */
	virtual tbool CCL_API call (Variant& returnValue, IObject* This = nullptr, const Variant args[] = nullptr, int argCount = 0) = 0;

	DECLARE_IID (IFunction)
};

DEFINE_IID (IFunction, 0x83665693, 0xdf61, 0x40f8, 0xbd, 0x85, 0x18, 0xb4, 0x3f, 0x59, 0x8b, 0xf1)

//************************************************************************************************
// Scripting::IEngineHost
/** Host interface for script engine, passed via IComponent::initialize. */
//************************************************************************************************

interface IEngineHost: IUnknown
{
	/** Create native stub object for script object. */
	virtual IObject* CCL_API createStubObject (IObject* scriptObject) = 0;

	/** Resolve script with given file name relative to including script. */
	virtual IScript* CCL_API resolveIncludeFile (StringRef fileName, const IScript* includingScript) = 0;

	DECLARE_IID (IEngineHost)
};

DEFINE_IID (IEngineHost, 0x2ddb7bf9, 0x89a9, 0x494d, 0x85, 0x25, 0x67, 0xcc, 0x6b, 0x5c, 0x50, 0xbf)

//************************************************************************************************
// IScript
/** Script interface. */
//************************************************************************************************

interface IScript: IUnknown
{
	/** Get path to script file. */
	virtual UrlRef CCL_API getPath () const = 0;

	/** Get script package identifier (optional). */
	virtual StringRef CCL_API getPackageID () const = 0;

	/** Get plain script code. */
	virtual tbool CCL_API getCode (CodePiece& codePiece) const = 0;

	DECLARE_IID (IScript)
};

DEFINE_IID (IScript, 0x8283afee, 0x712, 0x4f07, 0x86, 0xc4, 0xcd, 0x3a, 0xd8, 0xb, 0x83, 0xa6)

} // namespace Scripting
} // namespace CCL

#endif // _ccl_iscriptengine_h
