//************************************************************************************************
//
// JavaScript Engine
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
// Filename    : jstest.cpp
// Description : Test class
//
//************************************************************************************************

#include "jstest.h"
#include "jsengine.h"

#include "ccl/base/unittest.h"
#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/base/ibuffer.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/imultiworker.h"
#include "ccl/public/systemservices.h"

using namespace JScript;
using namespace CCL;
using namespace Threading;

//************************************************************************************************
// JsTest Suite
//************************************************************************************************

class JsTest: public Test
{
public:
	void setUp () override
	{
		engine = NEW Engine;
		engine->initialize (nullptr);
	}

	void tearDown () override
	{
		engine->terminate ();
		engine->release ();
	}

protected:
	Engine* engine;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (JsTest, TestJavaScript)
{
	TestClass testClass;
	testClass.runTest (engine);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

class TestScript: public Object,
				  public Scripting::IScript
{
public:
	TestScript (Scripting::CodePiece& code)
	: code (code)
	{}

	// IScript
	UrlRef CCL_API getPath () const override { return Url::kEmpty; }
	StringRef CCL_API getPackageID () const override { return String::kEmpty; }
	tbool CCL_API getCode (Scripting::CodePiece& codePiece) const override { codePiece = this->code; return true; }

	CLASS_INTERFACE (IScript, Object)

protected:
	Scripting::CodePiece& code;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

class TestEngineHost: public Object,
					  public Scripting::IEngineHost
{
public:
	TestEngineHost ()
	: includedCodeString ("var includedValue = 42;"),
	  includedCodeChars (includedCodeString),
	  includedCodePiece (includedCodeChars, includedCodeString.length (), CCLSTR ("Included")),
	  includedScript (includedCodePiece),
	  resolveIncludeFileCalled (false),
	  createStubObjectCalled (false)
	{}

	bool resolveIncludeFileCalled;
	bool createStubObjectCalled;

	// IEngineHost
	IObject* CCL_API createStubObject (IObject* /*scriptObject*/) override
	{
		createStubObjectCalled = true;
		return NEW TestClass;
	}

	Scripting::IScript* CCL_API resolveIncludeFile (StringRef /*fileName*/, const Scripting::IScript* /*includingScript*/) override
	{
		resolveIncludeFileCalled = true;
		includedScript.retain (); // caller adopts via AutoPtr without retaining again
		return &includedScript;
	}

	CLASS_INTERFACE (Scripting::IEngineHost, Object)

protected:
	String includedCodeString;
	StringChars includedCodeChars;
	Scripting::CodePiece includedCodePiece;
	TestScript includedScript;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (JsTest, TestScriptObjectInterfaces)
{
	AutoPtr<Scripting::IContext> context = engine->createContext ();

	static CStringPtr code =
		"function add (a, b) { return a + b; }"
		"var arr = [1, 2, 3];"
		"var buf = new Int32Array (4);"
		"buf[0] = 42;";

	String codeString (code);
	StringChars codeChars (codeString);
	Scripting::CodePiece codePiece (codeChars, codeString.length (), CCLSTR ("InterfaceTest"));
	TestScript script (codePiece);

	AutoPtr<IObject> scriptObject = context->compileScript (script);
	CCL_TEST_ASSERT (scriptObject.isValid ());

	// IFunction: call a plain script function value.
	Variant funcVar;
	CCL_TEST_ASSERT (scriptObject->getProperty (funcVar, "add"));
	UnknownPtr<Scripting::IFunction> func (funcVar.asUnknown ());
	CCL_TEST_ASSERT (func.isValid ());

	Variant args[2];
	args[0] = 3;
	args[1] = 4;
	Variant result;
	CCL_TEST_ASSERT (func->call (result, nullptr, args, 2));
	CCL_TEST_ASSERT_EQUAL (7, result.asInt ());

	// IMutableArray: get/add/set elements on a plain script Array.
	Variant arrVar;
	CCL_TEST_ASSERT (scriptObject->getProperty (arrVar, "arr"));
	UnknownPtr<IMutableArray> arr (arrVar.asUnknown ());
	CCL_TEST_ASSERT (arr.isValid ());
	CCL_TEST_ASSERT_EQUAL (3, arr->getArrayLength ());

	Variant elem;
	CCL_TEST_ASSERT (arr->getArrayElement (elem, 1));
	CCL_TEST_ASSERT_EQUAL (2, elem.asInt ());

	Variant newElem (99);
	CCL_TEST_ASSERT (arr->addArrayElement (newElem));
	CCL_TEST_ASSERT_EQUAL (4, arr->getArrayLength ());

	// IBuffer: raw memory access to a script-side typed array.
	Variant bufVar;
	CCL_TEST_ASSERT (scriptObject->getProperty (bufVar, "buf"));
	UnknownPtr<IBuffer> buf (bufVar.asUnknown ());
	CCL_TEST_ASSERT (buf.isValid ());
	CCL_TEST_ASSERT_EQUAL (16, int(buf->getBufferSize ())); // 4 * sizeof(int32)

	int32* bufData = static_cast<int32*> (buf->getBufferAddress ());
	CCL_TEST_ASSERT (bufData != nullptr);
	CCL_TEST_ASSERT_EQUAL (42, bufData[0]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (JsTest, TestStringMarshaling)
{
	static CStringPtr code = "function getString () { return 'hello'; }";

	// Default (kHostStringsEnabled off): strings marshal as IStringValue-wrapped objects, not
	// plain CCL::String Variants - callers must query for IStringValue explicitly.
	{
		AutoPtr<Scripting::IContext> context = engine->createContext ();

		String codeString (code);
		StringChars codeChars (codeString);
		Scripting::CodePiece codePiece (codeChars, codeString.length (), CCLSTR ("StringTestDefault"));
		TestScript script (codePiece);

		AutoPtr<IObject> scriptObject = context->compileScript (script);
		CCL_TEST_ASSERT (scriptObject.isValid ());

		Variant funcVar;
		CCL_TEST_ASSERT (scriptObject->getProperty (funcVar, "getString"));
		UnknownPtr<Scripting::IFunction> func (funcVar.asUnknown ());
		CCL_TEST_ASSERT (func.isValid ());

		Variant result;
		CCL_TEST_ASSERT (func->call (result, nullptr, nullptr, 0));
		CCL_TEST_ASSERT_EQUAL (int(Variant::kObject), int(result.getType ()));

		UnknownPtr<Scripting::IStringValue> stringValue (result.asUnknown ());
		CCL_TEST_ASSERT (stringValue.isValid ());
		CCL_TEST_ASSERT_EQUAL (5, stringValue->getLength ());
		CCL_TEST_ASSERT (stringValue->getCharData () != nullptr);
		CCL_TEST_ASSERT_EQUAL (0, CString (stringValue->getCharData ()).compare ("hello"));
	}

	// kHostStringsEnabled on: strings marshal directly as plain CCL::String Variants.
	{
		AutoPtr<Scripting::IContext> context = engine->createContext ();
		context->setOption (Scripting::IContext::kHostStringsEnabled, true);

		String codeString (code);
		StringChars codeChars (codeString);
		Scripting::CodePiece codePiece (codeChars, codeString.length (), CCLSTR ("StringTestHostEnabled"));
		TestScript script (codePiece);

		AutoPtr<IObject> scriptObject = context->compileScript (script);
		CCL_TEST_ASSERT (scriptObject.isValid ());

		Variant funcVar;
		CCL_TEST_ASSERT (scriptObject->getProperty (funcVar, "getString"));
		UnknownPtr<Scripting::IFunction> func (funcVar.asUnknown ());
		CCL_TEST_ASSERT (func.isValid ());

		Variant result;
		CCL_TEST_ASSERT (func->call (result, nullptr, nullptr, 0));
		CCL_TEST_ASSERT_EQUAL (int(Variant::kString), int(result.getType ()));

		String resultString = result;
		CCL_TEST_ASSERT (resultString == CCLSTR ("hello"));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (JsTest, TestIncludeFile)
{
	AutoPtr<TestEngineHost> host = NEW TestEngineHost;

	AutoPtr<Engine> includeEngine = NEW Engine;
	includeEngine->initialize (static_cast<Unknown*> (host.as_plain ()));

	{
		AutoPtr<Scripting::IContext> context = includeEngine->createContext ();

		static CStringPtr code = "include_file ('whatever.js');";
		String codeString (code);
		StringChars codeChars (codeString);
		Scripting::CodePiece codePiece (codeChars, codeString.length (), CCLSTR ("IncludeTest"));
		TestScript script (codePiece);

		AutoPtr<IObject> scriptObject = context->compileScript (script);
		CCL_TEST_ASSERT (scriptObject.isValid ());
		CCL_TEST_ASSERT (host->resolveIncludeFileCalled);

		Variant includedValue;
		CCL_TEST_ASSERT (scriptObject->getProperty (includedValue, "includedValue"));
		CCL_TEST_ASSERT_EQUAL (42, includedValue.asInt ());
	} // context (and its JSRuntime) must be destroyed before the engine terminates

	includeEngine->terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (JsTest, TestStubObjects)
{
	AutoPtr<TestEngineHost> host = NEW TestEngineHost;

	AutoPtr<Engine> stubEngine = NEW Engine;
	stubEngine->initialize (static_cast<Unknown*> (host.as_plain ()));

	{
		AutoPtr<Scripting::IContext> context = stubEngine->createContext ();
		context->setOption (Scripting::IContext::kStubObjectsEnabled, true);

		// Note: functions are not excluded from stubbing - so this deliberately fetches
		// plain data values directly rather than via a function call, which would itself get
		// replaced by the (non-callable) host stub before it could ever be invoked.
		static CStringPtr code =
			"var obj = {foo: 1};"
			"var arr = [1, 2, 3];";
		String codeString (code);
		StringChars codeChars (codeString);
		Scripting::CodePiece codePiece (codeChars, codeString.length (), CCLSTR ("StubTest"));
		TestScript script (codePiece);

		AutoPtr<IObject> scriptObject = context->compileScript (script);
		CCL_TEST_ASSERT (scriptObject.isValid ());

		// Plain objects: the host is asked for a stub instead of getting a plain ScriptObject.
		Variant objVar;
		CCL_TEST_ASSERT (scriptObject->getProperty (objVar, "obj"));
		CCL_TEST_ASSERT (host->createStubObjectCalled);
		CCL_TEST_ASSERT (unknown_cast<TestClass> (objVar.asUnknown ()) != nullptr);

		// Arrays are never stubbed, even with kStubObjectsEnabled on.
		host->createStubObjectCalled = false;

		Variant arrVar;
		CCL_TEST_ASSERT (scriptObject->getProperty (arrVar, "arr"));
		CCL_TEST_ASSERT (!host->createStubObjectCalled);

		UnknownPtr<IArrayObject> arr (arrVar.asUnknown ());
		CCL_TEST_ASSERT (arr.isValid ());
		CCL_TEST_ASSERT_EQUAL (3, arr->getArrayLength ());
	} // context (and its JSRuntime) must be destroyed before the engine terminates

	stubEngine->terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

constexpr int kNumberOfCycles = 100;
constexpr int kNumberOfProcesses = 4;

class WorkItem: public Work
{
public:
	WorkItem (EngineBase* engine)
	: context (engine->createContext ()),
	  scriptObject (nullptr)
	{
		context->attachModule (System::GetCurrentModuleRef ());

		AutoPtr<TestClass> gTest = NEW TestClass;
		context->registerObject ("gTest", gTest);

		static CStringPtr code =
			"function test ()"
			"{"
			"  gTest.width = 100;"
			"  gTest.sayHello (\"Hello world!\");"
			"  var x2 = gTest.getChild ();"
			"  gTest.sayHello (x2);"
			"  return 101; "
			"}";

		String codeString (code);
		StringChars codeChars (codeString);

		Scripting::CodePiece codePiece (codeChars, codeString.length (), CCLSTR ("Test"));
		TestScript script (codePiece);

		scriptObject = context->compileScript (script);
	}

	~WorkItem ()
	{
		context->detachModule (System::GetCurrentModuleRef ());
	}

	void work () override
	{
		Variant returnValue;
		scriptObject->invokeMethod (returnValue, Message ("test"));
	}

protected:
	AutoPtr<Scripting::IContext> context;
	AutoPtr<IObject> scriptObject;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TestClass::runTest (EngineBase* engine)
{
	CCL::Vector<WorkItem*> workItems (kNumberOfProcesses);
	for(int i = 0; i < kNumberOfProcesses; i++)
		workItems.add (NEW WorkItem (engine));

	// Process threads
	AutoPtr<IMultiWorker> processor = System::CreateMultiThreadWorker ({System::GetSystem ().getNumberOfCPUs (), 0, Threading::kPriorityHigh, false, "TestWorker"});
	for(int n = 0; n < kNumberOfCycles; n++)
	{
		for(int i = 0; i < kNumberOfProcesses; i++)
			processor->push (workItems[i]);

		processor->work ();
	}

	processor->terminate ();

	for(int i = 0; i < kNumberOfProcesses; i++)
		delete workItems[i];

	return true;
}

//************************************************************************************************
// TestClass
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TestClass, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (TestClass)
	DEFINE_METHOD_NAME ("sayHello")
	DEFINE_METHOD_NAME ("getChild")
END_METHOD_NAMES (TestClass)

//////////////////////////////////////////////////////////////////////////////////////////////////

TestClass::TestClass ()
: width (0),
  child (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TestClass::~TestClass ()
{
	if(child)
		child->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TestClass::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "width")
	{
		var = width;
		return true;
	}
	return Object::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TestClass::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "width")
	{
		width = var;
		return true;
	}
	return Object::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TestClass::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "TestClass") // ctor!
	{
		for(int i = 0; i < msg.getArgCount (); i++)
		{
			short type = msg[i].getType ();
			type = type;
		}
		return true;
	}
	else if(msg == "sayHello")
	{
		String string = msg[0];
		if(!string.isEmpty ())
			Debugger::print (string);
		return true;
	}
	else if(msg == "getChild")
	{
		if(!child)
			child = NEW TestClass;
		returnValue = static_cast<IObject*> (child);
		return true;
	}
	else
		return Object::invokeMethod (returnValue, msg);
}
