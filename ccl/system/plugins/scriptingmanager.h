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
// Filename    : ccl/system/plugins/scriptingmanager.h
// Description : Scripting Manager
//
//************************************************************************************************

#ifndef _ccl_scriptingmanager_h
#define _ccl_scriptingmanager_h

#include "ccl/base/singleton.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/stack.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/plugins/iscriptingmanager.h"
#include "ccl/public/plugins/idebugservice.h"

namespace CCL {

class MemoryStream;
class ScriptEngine;

//************************************************************************************************
// Script
//************************************************************************************************

class Script: public Object,
			  public Scripting::IScript
{
public:
	DECLARE_CLASS (Script, Object)

	Script (UrlRef path = Url (), StringRef packageID = nullptr);
	~Script ();

	MemoryStream& getCodeStream ();
	PROPERTY_SHARED_AUTO (IUnknown, package, Package)

	// IScript
	UrlRef CCL_API getPath () const override;
	StringRef CCL_API getPackageID () const override;
	tbool CCL_API getCode (Scripting::CodePiece& codePiece) const override;

	CLASS_INTERFACE (IScript, Object)

protected:
	Url path;
	String packageID;
	MemoryStream* codeStream;
};

//************************************************************************************************
// ScriptingEnvironment
//************************************************************************************************

class ScriptingEnvironment: public Object,
							public IScriptingEnvironment,
							public Scripting::IEngineHost
{
public:
	DECLARE_CLASS_ABSTRACT (ScriptingEnvironment, Object)

	// IScriptingEnvironment
	tbool CCL_API isScriptFile (UrlRef path) override;
	Scripting::IScript* CCL_API loadScript (UrlRef path, StringRef packageID = nullptr) override;
	Scripting::IScript* CCL_API createScript (IStream& stream, StringRef fileName, StringRef packageID = nullptr, IUnknown* package = nullptr) override;
	tbool CCL_API executeScript (Variant& returnValue, ScriptRef script) override;
	IObject* CCL_API compileScript (ScriptRef script) override;

	CLASS_INTERFACE2 (IScriptingEnvironment, IEngineHost, Object)

protected:
	virtual Scripting::IContext* getContext (const FileType& fileType) = 0;
	Script* loadInternal (IStream& stream, UrlRef path, StringRef packageID);

	// IEngineHost
	IObject* CCL_API createStubObject (IObject* scriptObject) override;
	Scripting::IScript* CCL_API resolveIncludeFile (StringRef fileName, const Scripting::IScript* includingScript) override;
};

//************************************************************************************************
// ScriptingManager
//************************************************************************************************

class ScriptingManager: public ScriptingEnvironment,
						public IScriptingManager,
						public Alert::IReporter,
						public Singleton<ScriptingManager>,
						public IDebuggableManager

{
public:
	DECLARE_CLASS (ScriptingManager, ScriptingEnvironment)

	ScriptingManager ();
	~ScriptingManager ();

	bool isStarted () const;

	ModuleRef resolveModule (StringID moduleID) const;

	// IScriptingManager
	void CCL_API startup (StringID moduleID, ModuleRef module, const ArgumentList* commandlineArgs = nullptr, tbool load = true) override;
	void CCL_API shutdown (ModuleRef module, tbool unload = true) override;
	void CCL_API setReporter (Alert::IReporter* reporter) override;
	IScriptingHost& CCL_API getHost () override;
	void CCL_API garbageCollect (tbool force = true) override;
	tbool CCL_API removeReference (IUnknown* nativeObject) override;
	void CCL_API dump () override;
	IScriptingEnvironment* CCL_API createEnvironment (StringRef language, ModuleRef module, const IAttributeList* options = nullptr) override;

	// IScriptingEnvironment (delegated to base class)
	tbool CCL_API isScriptFile (UrlRef path) override;
	Scripting::IScript* CCL_API loadScript (UrlRef path, StringRef packageID = nullptr) override;
	Scripting::IScript* CCL_API createScript (IStream& stream, StringRef fileName, StringRef packageID = nullptr, IUnknown* package = nullptr) override;
	tbool CCL_API executeScript (Variant& returnValue, ScriptRef script) override;
	IObject* CCL_API compileScript (ScriptRef script) override;

	// IDebuggableManager
	void CCL_API receiveMessage (const IDebugMessage& message) override;
	const IContainer& CCL_API getDebuggables () const override;
	void CCL_API onDisconnected () override;

	CLASS_INTERFACE3 (IScriptingManager, Alert::IReporter, IDebuggableManager, ScriptingEnvironment)

protected:
	static int nextThreadId;

	ObjectList engines;
	IDebugService* debugService;
	UnknownList debuggables;
	int startupCount;
	bool started;

	struct ModuleEntry
	{
		ModuleRef module;
		CString id;

		explicit ModuleEntry (ModuleRef module = nullptr, StringID id = nullptr)
		: module (module),
		  id (id)
		{}

		bool operator == (const ModuleEntry& other) const
		{
			return module == other.module;
		}
	};

	LinkedList<ModuleEntry> modules;
	Alert::IReporter* reporter;

	ScriptEngine* getEngine (StringRef mimeType);
	bool startDebugService (StringRef debugProtocolId, StringRef startupArgs);

	// ScriptingEnvironment
	Scripting::IContext* getContext (const FileType& fileType) override;

	// Alert::IReporter
	void CCL_API reportEvent (const Alert::Event& e) override;
	void CCL_API setReportOptions (Severity minSeverity, int eventFormat) override;
};

} // namespace CCL

#endif // _ccl_scriptingmanager_h
