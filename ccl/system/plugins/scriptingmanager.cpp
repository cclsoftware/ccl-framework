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
// Filename    : ccl/system/plugins/scriptingmanager.cpp
// Description : Scripting Manager
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/plugins/scriptingmanager.h"
#include "ccl/system/plugins/scriptinghost.h"

#include "ccl/base/storage/attributes.h"

#include "ccl/main/cclargs.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/base/streamer.h"
#include "ccl/public/storage/ifileresource.h"
#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/itextstreamer.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/plugins/icomponent.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

namespace CCL {

//************************************************************************************************
// ScriptEngine
//************************************************************************************************

class ScriptEngine: public Object
{
public:
	static ScriptEngine* createInstance (UIDRef cid, Scripting::IEngineHost* host, const IAttributeList* options = nullptr);

	PROPERTY_OBJECT (UID, cid, ClassID)

	const FileType& getFileType () const { return engine->getLanguage (); }
	Scripting::IContext* getContext () { return context; }

protected:
	Scripting::IEngine* engine;
	Scripting::IContext* context;

	ScriptEngine (UIDRef cid, Scripting::IEngine* engine, Scripting::IContext* context);
	~ScriptEngine ();
};

//************************************************************************************************
// StandaloneScriptEnvironment
//************************************************************************************************

class StandaloneScriptEnvironment: public ScriptingEnvironment
{
public:
	DECLARE_CLASS (StandaloneScriptEnvironment, ScriptingEnvironment)

	StandaloneScriptEnvironment ();
	~StandaloneScriptEnvironment ();

	bool construct (UIDRef cid, ModuleRef module, Alert::IReporter* reporter, const IAttributeList* options);

	CLASS_INTERFACES (ScriptingEnvironment)

protected:
	/** Helper to avoid circular reference when passed to IComponent::initialize(). */
	class HostDelegate: public Unknown,
						public IEngineHost
	{
	public:
		HostDelegate (IEngineHost& owner)
		: owner (owner)
		{}

		IObject* CCL_API createStubObject (IObject* scriptObject) override
		{
			return owner.createStubObject (scriptObject);
		}

		Scripting::IScript* CCL_API resolveIncludeFile (StringRef fileName, const Scripting::IScript* includingScript) override
		{
			return owner.resolveIncludeFile (fileName, includingScript);
		}

		CLASS_INTERFACE (IEngineHost, Unknown)

	protected:
		IEngineHost& owner;
	};

	ModuleRef module;
	ScriptEngine* engine;
	AutoPtr<HostDelegate> hostDelegate;

	// ScriptingEnvironment
	Scripting::IContext* getContext (const FileType& fileType) override;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Scripting APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IScriptingManager& CCL_API System::CCL_ISOLATED (GetScriptingManager) ()
{
	return ScriptingManager::instance ();
}

//************************************************************************************************
// Script
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Script, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Script::Script (UrlRef path, StringRef packageID)
: path (path),
  packageID (packageID),
  codeStream (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Script::~Script ()
{
	if(codeStream)
		codeStream->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MemoryStream& Script::getCodeStream ()
{
	if(!codeStream)
		codeStream = NEW MemoryStream;
	return *codeStream;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API Script::getPath () const
{
	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API Script::getPackageID () const
{
	return packageID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Script::getCode (Scripting::CodePiece& codePiece) const
{
	ASSERT (codeStream != nullptr)
	if(!codeStream)
		return false;

	int byteSize = (int)codeStream->getBytesWritten ();
	codePiece.code = (uchar*)codeStream->getMemoryAddress ();
	codePiece.length = byteSize / sizeof(uchar) - 1; // w/o null terminator!
	path.getName (codePiece.fileName);
	codePiece.lineNumber = 0;
	return codePiece.length > 0;
}

//************************************************************************************************
// ScriptingEnvironment
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ScriptingEnvironment, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingEnvironment::isScriptFile (UrlRef path)
{
	return getContext (path.getFileType ()) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Scripting::IScript* CCL_API ScriptingEnvironment::loadScript (UrlRef path, StringRef packageID)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kOpenMode);
	return stream ? loadInternal (*stream, path, packageID) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Scripting::IScript* CCL_API ScriptingEnvironment::createScript (IStream& stream, StringRef fileName, StringRef packageID, IUnknown* package)
{
	Url path;
	if(UnknownPtr<IFileResource> fileResource = package) // keep path to package for script debugging
	{
		path.assign (fileResource->getPath ());
		path.descend (fileName);
	}
	else
		path.setPath (fileName);

	Script* script = loadInternal (stream, path, packageID);
	if(script && package)
		script->setPackage (package);
	return script;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Script* ScriptingEnvironment::loadInternal (IStream& stream, UrlRef path, StringRef packageID)
{
	Script* script = NEW Script (path, packageID);

	AutoPtr<ITextStreamer> reader = System::CreateTextStreamer (stream);
	Streamer writer (script->getCodeStream ());
	String line;
	while(reader->readLine (line))
		writer.writeLine (line);
	writer.writeChar (0); // null terminator

	return script;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingEnvironment::executeScript (Variant& returnValue, ScriptRef script)
{
	if(Scripting::IContext* context = getContext (script.getPath ().getFileType ()))
		return context->executeScript (returnValue, script) == kResultOk;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* CCL_API ScriptingEnvironment::compileScript (ScriptRef script)
{
	if(Scripting::IContext* context = getContext (script.getPath ().getFileType ()))
		return context->compileScript (script);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Scripting::IScript* CCL_API ScriptingEnvironment::resolveIncludeFile (StringRef fileName, const Scripting::IScript* includingScript)
{
	// check which script is currently loading....
	Script* script = unknown_cast<Script> (includingScript);
	ASSERT (script != nullptr)
	if(!script)
		return nullptr;

	Url path;
	if(fileName.contains (CCLSTR ("//")))
		path.setUrl (fileName);
	else
		path.setPath (fileName);

	if(!path.getProtocol ().isEmpty ())
	{
		// resolve symbolic module name for resources
		if(path.getProtocol () == ResourceUrl::Protocol)
		{
			String resolvedId;
			MutableCString moduleId (path.getHostName ());
			ModuleRef module = ScriptingManager::instance ().resolveModule (moduleId);
			ASSERT (module != nullptr)
			if(module != nullptr)
				System::GetModuleIdentifier (resolvedId, module);
			path.setHostName (resolvedId);
		}

		return loadScript (path);
	}
	else
	{
		UnknownPtr<IFileSystem> fileSystem (script->getPackage ());
		if(fileSystem)
		{
			AutoPtr<IStream> stream = fileSystem->openStream (path);
			if(stream)
				return createScript (*stream, fileName, script->getPackageID (), fileSystem);
		}
		else
		{
			Url scriptFolder (script->getPath ());
			scriptFolder.ascend ();
			path.makeAbsolute (scriptFolder);
			return loadScript (path);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* CCL_API ScriptingEnvironment::createStubObject (IObject* scriptObject)
{
	IObject* stubObject = nullptr;
	System::GetPlugInManager ().createStubInstance (ccl_iid<IObject> (), scriptObject, (void**)&stubObject);
	return stubObject;
}

//************************************************************************************************
// ScriptEngine
//************************************************************************************************

ScriptEngine* ScriptEngine::createInstance (UIDRef cid, Scripting::IEngineHost* host, const IAttributeList* options)
{
	if(Scripting::IEngine* engine = ccl_new<Scripting::IEngine> (cid))
	{
		if(options != nullptr)
			ForEachAttribute (*options, name, value)
				engine->setOption (name, value);
			EndFor

		tresult result = kResultOk;
		{
			UnknownPtr<IComponent> iComponent (engine);
			if(iComponent)
				result = iComponent->initialize (host);
		}

		if(result == kResultOk)
			if(Scripting::IContext* context = engine->createContext ())
				return NEW ScriptEngine (cid, engine, context);

		ccl_release (engine);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptEngine::ScriptEngine (UIDRef cid, Scripting::IEngine* engine, Scripting::IContext* context)
: cid (cid),
  engine (engine),
  context (context)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptEngine::~ScriptEngine ()
{
	context->release ();

	{
		UnknownPtr<IComponent> iComponent (engine);
		if(iComponent)
			iComponent->terminate ();
	}

	ccl_release (engine);
}

//************************************************************************************************
// ScriptingManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ScriptingManager, ScriptingEnvironment)
DEFINE_SINGLETON (ScriptingManager)
int ScriptingManager::nextThreadId = 100;

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptingManager::ScriptingManager ()
: reporter (nullptr),
  startupCount (0),
  started (false),
  debugService (nullptr)
{
	engines.objectCleanup (true);

	#if !CCL_STATIC_LINKAGE
	modules.append (ModuleEntry (System::GetCurrentModuleRef (), CCLSYSTEM_PACKAGE_ID));
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptingManager::~ScriptingManager ()
{
	#if !CCL_STATIC_LINKAGE
	modules.remove (ModuleEntry (System::GetCurrentModuleRef ()));
	#endif

	ASSERT (debugService == nullptr)
	ASSERT (debuggables.isEmpty ())
	ASSERT (modules.isEmpty ())
	ASSERT (engines.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptingManager::isStarted () const
{
	return started;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ModuleRef ScriptingManager::resolveModule (StringID moduleID) const
{	
	if(moduleID == "{main}") // Use curly brackets borrowed from URI Templates (https://www.rfc-editor.org/rfc/rfc6570)
		return System::GetMainModuleRef ();
	
	ListForEach (modules, ModuleEntry, entry)
		if(entry.id == moduleID)
			return entry.module;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptingManager::startDebugService (StringRef debugProtocolId, StringRef startupArgs)
{
	ASSERT (debugService == nullptr)
	if(debugService)
		return false; // already started

	ForEachPlugInClass (PLUG_CATEGORY_DEBUGSERVICE, desc)
		Variant protocolId;
		desc.getClassAttribute (protocolId, IDebugService::kProtocolAttribute);
		if(protocolId.asString () == debugProtocolId)
		{
			debugService = ccl_new<IDebugService> (desc.getClassID ());
			break;
		}
	EndFor

	if(debugService && debugService->startup (startupArgs, this))
	{
		for(ScriptEngine* engine : iterate_as<ScriptEngine> (engines))
		{
			UnknownPtr<IDebuggable> debuggable (engine->getContext ());
			if(debuggable)
			{
				debuggable->setSender (debugService);
				debuggable->setThreadId (nextThreadId++);
				debuggables.add (debuggable);
			}
		}

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptingManager::startup (StringID moduleID, ModuleRef module, const ArgumentList* args, tbool load)
{
	ASSERT (!moduleID.isEmpty ())
	ASSERT (!modules.contains (ModuleEntry (module)))

	modules.append (ModuleEntry (module, moduleID));

	if(load && startupCount++ == 0)
	{
		// determine if script debugging should be enabled
		String debugProtocolId, debugProtocolValue;
		if(args)
		{
			String debugString;
			StringRef kDebugArg = CCLSTR ("-debug");
			for(int i = 0; i < args->count () - 1; i++)
				if(args->at (i) == kDebugArg)
				{
					debugString = args->at (i + 1);
					break;
				}

			if(!debugString.isEmpty ())
			{
				int separatorIndex = debugString.index (CCLSTR (":"));
				debugProtocolId = debugString.subString (0, separatorIndex);
				debugProtocolId.toLowercase ();
				debugProtocolValue = debugString.subString (separatorIndex + 1);
			}
		}

		// startup engines
		AutoPtr<Attributes> engineOptions;
		if(!debugProtocolId.isEmpty ())
		{
			engineOptions = NEW Attributes;
			engineOptions->set (Scripting::IEngine::kDebugProtocolID, debugProtocolId);
		}

		ForEachPlugInClass (PLUG_CATEGORY_SCRIPTENGINE, desc)
			if(ScriptEngine* engine = ScriptEngine::createInstance (desc.getClassID (), this, engineOptions))
				engines.add (engine);
		EndFor

		ListForEachObject (engines, ScriptEngine, engine)
			Scripting::IContext* context = engine->getContext ();
			context->setOption (Scripting::IContext::kStubObjectsEnabled, true);
			context->setOption (Scripting::IContext::kHostStringsEnabled, true);

			context->setReporter (this);

			ListForEach (modules, ModuleEntry, entry)
				context->attachModule (entry.module);
			EndFor

			context->registerObject (CSTR ("Host"), &ScriptingHost::instance ());
			System::GetFileTypeRegistry ().registerFileType (engine->getFileType ());
		EndFor

		started = true;

		// start debug service (optional)
		if(!debugProtocolId.isEmpty ())
			startDebugService (debugProtocolId, debugProtocolValue);
	}
	else
	{
		ASSERT (args == nullptr)

		// attach module
		ListForEachObject (engines, ScriptEngine, engine)
			engine->getContext ()->attachModule (module);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptingManager::shutdown (ModuleRef module, tbool unload)
{
	ASSERT (modules.contains (ModuleEntry (module)))

	// detach module
	ListForEachObject (engines, ScriptEngine, engine)
		engine->getContext ()->detachModule (module);
	EndFor

	modules.remove (ModuleEntry (module));

	if(unload && --startupCount == 0)
	{
		garbageCollect (); // ensure garbage collection was called before exit (i.e. ccl_forceGC ())

		// stop debug service
		if(debugService)
		{
			debugService->shutdown ();

			ccl_release (debugService);
			debugService = nullptr;
		}

		ListForEachObject (engines, ScriptEngine, engine)
			ListForEach (modules, ModuleEntry, entry)
				engine->getContext ()->detachModule (entry.module);
			EndFor

			if(!debuggables.isEmpty ())
				if(UnknownPtr<IDebuggable> debuggable = engine->getContext ())
					debuggables.remove (debuggable);
		EndFor

		engines.removeAll ();

		started = false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptingManager::setReporter (Alert::IReporter* _reporter)
{
	reporter = _reporter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptingManager::reportEvent (const Alert::Event& e)
{
	if(reporter)
		reporter->reportEvent (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptingManager::setReportOptions (Severity minSeverity, int eventFormat)
{
	if(reporter)
		reporter->setReportOptions (minSeverity, eventFormat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IScriptingHost& CCL_API ScriptingManager::getHost ()
{
	return ScriptingHost::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptingManager::garbageCollect (tbool force)
{
	ListForEachObject (engines, ScriptEngine, engine)
		engine->getContext ()->garbageCollect (force);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingManager::removeReference (IUnknown* nativeObject)
{
	bool result = false;
	ListForEachObject (engines, ScriptEngine, engine)
		if(engine->getContext ()->removeReference (nativeObject))
			result = true;
	EndFor
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptingManager::dump ()
{
	ListForEachObject (engines, ScriptEngine, engine)
		engine->getContext ()->dump ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IScriptingEnvironment* CCL_API ScriptingManager::createEnvironment (StringRef language, ModuleRef module, const IAttributeList* options)
{
	if(ScriptEngine* engine = getEngine (language))
	{
		AutoPtr<StandaloneScriptEnvironment> e = NEW StandaloneScriptEnvironment;
		if(e->construct (engine->getClassID (), module, this, options))
			return e.detach ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Scripting::IContext* ScriptingManager::getContext (const FileType& fileType)
{
	ListForEachObject (engines, ScriptEngine, engine)
		if(engine->getFileType () == fileType)
			return engine->getContext ();
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptEngine* ScriptingManager::getEngine (StringRef mimeType)
{
	ListForEachObject (engines, ScriptEngine, engine)
		if(engine->getFileType ().getMimeType () == mimeType)
			return engine;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingManager::isScriptFile (UrlRef path)
{
	return SuperClass::isScriptFile (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Scripting::IScript* CCL_API ScriptingManager::loadScript (UrlRef path, StringRef packageID)
{
	return SuperClass::loadScript (path, packageID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Scripting::IScript* CCL_API ScriptingManager::createScript (IStream& stream, StringRef fileName, StringRef packageID, IUnknown* package)
{
	return SuperClass::createScript (stream, fileName, packageID, package);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingManager::executeScript (Variant& returnValue, ScriptRef script)
{
	return SuperClass::executeScript (returnValue, script);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* CCL_API ScriptingManager::compileScript (ScriptRef script)
{
	return SuperClass::compileScript (script);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptingManager::receiveMessage (const IDebugMessage& request)
{
	bool broadcast = request.getThreadId () == IDebugMessage::kBroadcastThreadId;
	for(IDebuggable* dbg : iterate_as<IDebuggable> (debuggables))
	{
		if(broadcast || dbg->getThreadId () == request.getThreadId ())
		{
			dbg->receiveMessage (request);
			if(!broadcast)
				break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IContainer& CCL_API ScriptingManager::getDebuggables () const
{
	return debuggables;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptingManager::onDisconnected ()
{
	for(IDebuggable* dbg : iterate_as<IDebuggable> (debuggables))
		dbg->onDisconnected ();
}

//************************************************************************************************
// StandaloneScriptEnvironment
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StandaloneScriptEnvironment, ScriptingEnvironment)

//////////////////////////////////////////////////////////////////////////////////////////////////

StandaloneScriptEnvironment::StandaloneScriptEnvironment ()
: engine (nullptr),
  module (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StandaloneScriptEnvironment::~StandaloneScriptEnvironment ()
{
	if(engine)
	{
		engine->getContext ()->detachModule (module);
		engine->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StandaloneScriptEnvironment::construct (UIDRef cid, ModuleRef module, Alert::IReporter* reporter, const IAttributeList* options)
{
	hostDelegate = NEW HostDelegate (*this);
	if(engine = ScriptEngine::createInstance (cid, hostDelegate, options))
	{
		engine->getContext ()->attachModule (module);
		engine->getContext ()->setReporter (reporter);
		this->module = module;
	}
	return engine != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API StandaloneScriptEnvironment::queryInterface (UIDRef iid, void** ptr)
{
	if(iid == ccl_iid<Scripting::IContext> ())
		return engine->getContext ()->queryInterface (iid, ptr);

	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Scripting::IContext* StandaloneScriptEnvironment::getContext (const FileType&)
{
	return engine->getContext ();
}
