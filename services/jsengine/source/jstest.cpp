//************************************************************************************************
//
// JavaScript Engine
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
// Filename    : jstest.cpp
// Description : Test class
//
//************************************************************************************************

#include "jstest.h"
#include "jsengine.h"

#include "ccl/base/unittest.h"
#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"

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

constexpr int kNumberOfCycles = 100;
constexpr int kNumberOfProcesses = 4;

class WorkItem: public Work
{
public:
	WorkItem (Engine* engine)
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

bool TestClass::runTest (Engine* engine)
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
		returnValue = (IObject*)child;
		return true;
	}
	else
		return Object::invokeMethod (returnValue, msg);
}
