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
// Filename    : ccl/public/plugins/iscriptingmanager.h
// Description : Scripting Manager Interface
//
//************************************************************************************************

#ifndef _ccl_iscriptingmanager_h
#define _ccl_iscriptingmanager_h

#include "ccl/public/plugins/iscriptengine.h"

namespace CCL {

interface IAttributeList;

/** Script reference. */
typedef const Scripting::IScript& ScriptRef;

//************************************************************************************************
// IScriptingHost
//************************************************************************************************

interface IScriptingHost: IUnknown
{
	/** Register object with given name. */
	virtual void CCL_API registerObject (StringID name, IObject& object) = 0;

	/** Unregister object. */
	virtual void CCL_API unregisterObject (IObject& object) = 0;

	/** Get registered object by name. */
	virtual IObject* CCL_API getObject (StringID name) const = 0;

	DECLARE_IID (IScriptingHost)
};

DEFINE_IID (IScriptingHost, 0x4841bd1c, 0x3606, 0x4344, 0xad, 0x51, 0x37, 0x89, 0xaa, 0x84, 0x3a, 0x10)

//************************************************************************************************
// IScriptingEnvironment
//************************************************************************************************

interface IScriptingEnvironment: IUnknown
{
	/** Check if path points to a script file. */
	virtual tbool CCL_API isScriptFile (UrlRef path) = 0;

	/** Load script from file. */
	virtual Scripting::IScript* CCL_API loadScript (UrlRef path, StringRef packageID = nullptr) = 0;

	/** Create script object from stream and filename. */
	virtual Scripting::IScript* CCL_API createScript (IStream& stream, StringRef fileName, StringRef packageID = nullptr, IUnknown* package = nullptr) = 0;

	/** Execute script resource directly. */
	virtual tbool CCL_API executeScript (Variant& returnValue, ScriptRef script) = 0;

	/** Compile script resource. */
	virtual IObject* CCL_API compileScript (ScriptRef script) = 0;

	DECLARE_IID (IScriptingEnvironment)
};

DEFINE_IID (IScriptingEnvironment, 0x2d3300fd, 0x9, 0x4bc9, 0xb7, 0x5b, 0x9c, 0xb4, 0x1b, 0x5b, 0x4b, 0x4f)

//************************************************************************************************
// IScriptingManager
//************************************************************************************************

interface IScriptingManager: IScriptingEnvironment
{
	/** Startup scripting. */
	virtual void CCL_API startup (StringID moduleID, ModuleRef module, const ArgumentList* args = nullptr, tbool load = true) = 0;

	/** Shutdown scripting. */
	virtual void CCL_API shutdown (ModuleRef module, tbool unload = true) = 0;

	/** Set alert reporter. */
	virtual void CCL_API setReporter (Alert::IReporter* reporter) = 0;

	/** Returns the global scripting host instance. */
	virtual IScriptingHost& CCL_API getHost () = 0;

	/** Garbage collect in all global contexts. */
	virtual void CCL_API garbageCollect (tbool force = true) = 0;

	/** Remove reference to native object from all global contexts. */
	virtual tbool CCL_API removeReference (IUnknown* nativeObject) = 0;

	/** Dump global context information to debug output. */
	virtual void CCL_API dump () = 0;

	/** Create standalone scripting environment for given language. */
	virtual IScriptingEnvironment* CCL_API createEnvironment (StringRef language, ModuleRef module,
															  const IAttributeList* options = nullptr) = 0;

	DECLARE_IID (IScriptingManager)
};

DEFINE_IID (IScriptingManager, 0x4254f92c, 0xcf21, 0x4583, 0x9e, 0xad, 0x24, 0xcc, 0xea, 0x19, 0xe8, 0x91)

} // namespace CCL

#endif // _ccl_iscriptingmanager_h
