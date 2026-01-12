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
// Filename    : cpptest.cpp
// Description : C++ features Unit test
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/objectlist.h"
#include "ccl/base/collections/linkablelist.h"
#include "ccl/base/collections/arraybox.h"
#include "ccl/base/collections/stringlist.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/boxedtypes.h"

#include "ccl/public/collections/variantvector.h"
#include "ccl/public/collections/hashmap.h"
#include "ccl/public/system/logging.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
String logElement (T& v)
{
	String s;
	s << v;
	return s;
}

template<typename T>
String logElement (T* v)
{
	String s;
	s << *v;
	return s;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class C>
String logContainer (C& c)
{
	String result;
	for(auto v : c)
	{
		if(!result.isEmpty ())
			result << ", ";

		result << logElement (v);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

class LogScope
{
public:
	LogScope (ITestContext* testContext, StringRef title)
	: testContext (testContext),
	  title (title)
	{}

	~LogScope ()
	{
		Logging::debug (title << ": " << log.concat (", "));
	}

	template<class T> LogScope& operator << (T& t)
	{
		log.add (String () << t);
		return *this;
	}

	LogScope& operator << (StringRef s)
	{
		log.add (s);
		return *this;
	}

private:
	ITestContext* testContext;
	String title;
	StringList log;
};

//************************************************************************************************
// CppTest
//************************************************************************************************

class CppTest: public Test
{
protected:
	static int hashFunc (const int& key, int size)
	{
		return key % size;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (CppTest, TestRangeForCore)
{
	{
		LogScope log (testContext, "Vector<int>");

		Vector<int> v;
		v.add (1);
		v.add (2);
		v.add (3);

		for(auto i : v)
			log << i;
	}

	{
		LogScope log (testContext, "LinkedList<int>");

		LinkedList<int> list;
		list.append (1);
		list.append (2);
		list.append (3);

		for(auto i : list)
			log << i;
	}

	{
		LogScope log (testContext, "HashMap<int, int>");

		HashMap<int, int> hashMap (16, hashFunc);
		hashMap.add (1, 100);
		hashMap.add (3, 300);
		hashMap.add (5, 500);

		for(auto i : hashMap)
			log << i;
	}
}

CCL_TEST_F (CppTest, TestRangeForCCL)
{
	class MyArray: public ObjectArray
	{};

	#if 1
	MyArray stringArray;
	#else
	ObjectArray stringArray;
	#endif

	{
		LogScope log (testContext, "ObjectArray");

		stringArray.objectCleanup (true);
		stringArray.add (NEW Boxed::String ("A"));
		stringArray.add (NEW Boxed::String ("B"));
		stringArray.add (NEW Boxed::String ("C"));

		// iterator returns Object*
		for(auto obj : stringArray)
		{
			Boxed::String* s = static_cast<Boxed::String*> (obj);
			log << *s;
		}
	}

	{
		LogScope log (testContext, "ObjectArray (cast)");

		// iterate_as: internal static_cast to pointer to given class
		for(auto s : iterate_as<Boxed::String> (stringArray))
			log << *s;
	}

	{
		LogScope log (testContext, "ObjectList");

		ObjectList urlList;
		urlList.objectCleanup (true);
		urlList.add (NEW Url ("http://ccl.dev/a"));
		urlList.add (NEW Url ("http://ccl.dev/b"));
		urlList.add (NEW Url ("http://ccl.dev/c"));

		for(auto url : iterate_as<Url> (urlList))
			log << UrlFullString (*url);
	}

	{
		LogScope log (testContext, "LinkableList");

		class StringLink: public Linkable, public String
		{
		public:
			StringLink (StringRef s) : String (s) {}
		};

		LinkableList stringLinkableList;
		stringLinkableList.objectCleanup (true);
		stringLinkableList.add (NEW StringLink ("A"));
		stringLinkableList.add (NEW StringLink ("B"));
		stringLinkableList.add (NEW StringLink ("C"));

		for(auto s : iterate_as<StringLink> (stringLinkableList))
			log << *s;
	}

	{
		LogScope log (testContext, "ArrayBox");

		AutoPtr<VariantVector> variantVector (NEW VariantVector);
		variantVector->add (Variant (AutoPtr<Unknown> (NEW Boxed::String ("A")), true)); // must add as Variant of type Object, not String!
		variantVector->add (Variant (AutoPtr<Unknown> (NEW Boxed::String ("B")), true));
		variantVector->add (Variant (AutoPtr<Unknown> (NEW Boxed::String ("C")), true));
		ArrayBox arrayBox (variantVector);

		for(auto s : iterate_as<Boxed::String> (arrayBox))
			log << *s;
	}

	{
		LogScope log (testContext, "Container&");

		// iterate via abstract Container reference (creates iterator on heap)
		const Container& container (stringArray);

		for(auto s : iterate_as<Boxed::String> (container))
			log << *s;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (CppTest, TestInitializerList)
{
	Vector<int> ints = { 1, 2, 3 };
	StringList strings1 = { String ("A"), String ("B"), String ("C") };
	StringList strings2 = { "a", "b", "c" };	// automatic conversion to String

	Logging::debug (logContainer (ints));
	Logging::debug (logContainer (strings1));
	Logging::debug (logContainer (strings2));
}
