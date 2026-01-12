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
// Filename    : ccl/system/diagnosticstore.cpp
// Description : Diagnostic Store
//
//************************************************************************************************

#include "ccl/system/diagnosticstore.h"

#include "ccl/base/storage/settings.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

namespace CCL {
	
//************************************************************************************************
// DiagnosticFilter
//************************************************************************************************

class DiagnosticFilter: public Unknown,
						public IObjectFilter
{
public:
	DiagnosticFilter (StringID context);

	// IObjectFilter
	tbool CCL_API matches (IUnknown* object) const override;

	CLASS_INTERFACE (IObjectFilter, Unknown)

protected:
	StringID context;
	String contextString;
};

//************************************************************************************************
// DiagnosticWildcardFilter
//************************************************************************************************

class DiagnosticWildcardFilter: public DiagnosticFilter
{
public:
	DiagnosticWildcardFilter (StringID context);

	// DiagnosticFilter
	tbool CCL_API matches (IUnknown* object) const override;
};

//************************************************************************************************
// DiagnosticResult
//************************************************************************************************

class DiagnosticResult: public Attributes,
						public IDiagnosticResult
{
public:
	DECLARE_CLASS (DiagnosticResult, Attributes)

	DECLARE_STRINGID_MEMBER (kContext)
	DECLARE_STRINGID_MEMBER (kLabel)
	DECLARE_STRINGID_MEMBER (kCount)
	DECLARE_STRINGID_MEMBER (kMinimum)
	DECLARE_STRINGID_MEMBER (kMaximum)
	DECLARE_STRINGID_MEMBER (kAverage)
	DECLARE_STRINGID_MEMBER (kSum)
	DECLARE_STRINGID_MEMBER (kItems)
	DECLARE_STRINGID_MEMBER (kValue)
	DECLARE_STRINGID_MEMBER (kTimestamp)

	// IDiagnosticResult
	StringID CCL_API getContext () const override;
	StringRef CCL_API getLabel () const override;

	double CCL_API getMinimum () const override;
	double CCL_API getMaximum () const override;
	double CCL_API getAverage () const override;
	double CCL_API getSum () const override;
	int CCL_API getCount () const override;

	tbool CCL_API hasValues () const override;
	tbool CCL_API getValue (Variant& value, int index) const override;
	int64 CCL_API getTimestamp (int index) const override;

	CLASS_INTERFACE (IDiagnosticResult, Attributes)

protected:
	mutable MutableCString context;
	mutable String label;
};

//************************************************************************************************
// DiagnosticResultSet
//************************************************************************************************

class DiagnosticResultSet: public Unknown,
						   public IDiagnosticResultSet
{
public:
	DiagnosticResultSet ();

	void add (DiagnosticResult* item);

	// IDiagnosticResultSet
	IDiagnosticResult* CCL_API at (int index) const override;
	int CCL_API getCount () const override;

	void CCL_API sortByMinimum () override;
	void CCL_API sortByMaximum () override;
	void CCL_API sortByAverage () override;
	void CCL_API sortBySum () override;
	void CCL_API sortByCount () override;

	IUnknownIterator* CCL_API createIterator () const override;

	CLASS_INTERFACE (IDiagnosticResultSet, Unknown)

protected:
	ObjectArray items;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IDiagnosticStore& CCL_API System::CCL_ISOLATED (GetDiagnosticStore) ()
{
	return DiagnosticStore::instance ();
}

//************************************************************************************************
// DiagnosticStore
//************************************************************************************************

DEFINE_SINGLETON (DiagnosticStore)

const String DiagnosticStore::kPersistentName = "Diagnostics";

////////////////////////////////////////////////////////////////////////////////////////////////////

DiagnosticStore::DiagnosticStore ()
{
	data.objectCleanup ();
	shortTermData.objectCleanup ();
	restore ();

	setMode (kLongTerm);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

DiagnosticStore::~DiagnosticStore ()
{
	ASSERT (shortTermData.isEmpty ())
	store ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DiagnosticStore::submitValue (StringID context, StringID key, VariantRef value, StringRef label)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	Attributes& keyData = getData (context, key, kLongTerm);
	
	AutoPtr<Attributes> item = (mode == kShortTerm) ? NEW Attributes : nullptr;

	if(value.isNumeric ()) //< Duration, size, etc.
	{
		if(item)
			item->set (DiagnosticResult::kValue, value.asDouble ());

		if(value.getUserFlags () & kNoStatistics)
			keyData.setAttribute (DiagnosticResult::kValue, value); // only keep the last submitted value
		else
		{
			int count = keyData.getInt (DiagnosticResult::kCount);
			double average = (keyData.getFloat (DiagnosticResult::kAverage) * count + value.asDouble ()) / (count + 1);
			double minimum = keyData.contains (DiagnosticResult::kMinimum) ? ccl_min (keyData.getFloat (DiagnosticResult::kMinimum), value.asDouble ()) : value.asDouble ();
			double maximum = ccl_max (keyData.getFloat (DiagnosticResult::kMaximum), value.asDouble ());
			double sum = keyData.getFloat (DiagnosticResult::kSum) + value.asDouble ();

			keyData.set (DiagnosticResult::kCount, count + 1);
			keyData.set (DiagnosticResult::kAverage, average);
			keyData.set (DiagnosticResult::kMinimum, minimum);
			keyData.set (DiagnosticResult::kMaximum, maximum);
			keyData.set (DiagnosticResult::kSum, sum);
		}
	}
	else if(!value.isValid ()) //< Events without values
	{
		int count = keyData.getInt (DiagnosticResult::kCount);
		keyData.set (DiagnosticResult::kCount, count + 1);
	}
	else
		return kResultInvalidArgument;
	
	if(!label.isEmpty ())
		keyData.set (DiagnosticResult::kLabel, label);

	if(item)
	{
		DateTime now;
		System::GetSystem ().getLocalTime (now);
		item->set (DiagnosticResult::kTimestamp, now.toOrdinal ());
		
		Attributes& shortTermData = getData (context, key, kShortTerm);
		
		if(!label.isEmpty ())
			shortTermData.set (DiagnosticResult::kLabel, label);

		shortTermData.queue (DiagnosticResult::kItems, item, Attributes::kShare);
	}

	return kResultOk;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DiagnosticStore::clearData (StringID context, StringID key)
{
	ObjectArray& results = (mode == kLongTerm) ? data : shortTermData;

	bool isWildcard = context.index ('*') != -1;
	AutoPtr<DiagnosticFilter> contextFilter = isWildcard ? NEW DiagnosticWildcardFilter (context) : NEW DiagnosticFilter (context);

	ObjectList contextsToRemove;
	for(auto contextData : iterate_as<Attributes> (results))
		if(contextFilter->matches (ccl_as_unknown (contextData)))
		{
			if(key.isEmpty ())
				contextsToRemove.add (contextData); // remove all keys of this context
			else
				contextData->remove (key); // remove the matching key

			if(!isWildcard)
				break;
		}

	for(auto contextData : iterate_as<Attributes> (contextsToRemove))
		if(results.remove (contextData))
			contextData->release ();

	return kResultOk;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IDiagnosticResultSet* CCL_API DiagnosticStore::queryResults (StringID context, StringID key) const
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return nullptr;

	AutoPtr<DiagnosticResultSet> resultSet = NEW DiagnosticResultSet;
	queryResults (*resultSet, context, key);
	return resultSet.detach ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IDiagnosticResult* CCL_API DiagnosticStore::queryResult (StringID context, StringID key) const
{
	DiagnosticResultSet results;
	queryResults (results, context, key, 1);
	return results.getCount () > 0 ? return_shared (results.at (0)) : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IDiagnosticResultSet* CCL_API DiagnosticStore::queryMultipleResults (StringID context, CString keys[], int keyCount) const
{
	AutoPtr<DiagnosticResultSet> resultSet = NEW DiagnosticResultSet;
	queryMultipleResults (*resultSet, context, keys, keyCount);
	return resultSet->getCount () == keyCount ? resultSet.detach () : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void DiagnosticStore::queryResults (DiagnosticResultSet& resultSet, StringID context, StringID key, int count) const
{
	AutoPtr<DiagnosticFilter> contextFilter = context.index ('*') != -1 ? NEW DiagnosticWildcardFilter (context) : NEW DiagnosticFilter (context);

	const ObjectArray* results = (mode == kLongTerm) ? &data : &shortTermData;

	for(Attributes* a : iterate_as<Attributes> (*results))
	{
		if(contextFilter->matches (ccl_as_unknown (a)) && a->contains (key))
		{
			Attributes* keyData = a->getAttributes (key);
			if(keyData)
			{
				AutoPtr<DiagnosticResult> resultAttributes = NEW DiagnosticResult;
				resultAttributes->copyFrom (*keyData);
				resultAttributes->set (DiagnosticResult::kContext, a->getCString (DiagnosticResult::kContext, Text::kASCII));
				resultSet.add (resultAttributes.detach ());
				if(count > 0 && resultSet.getCount () >= count)
					break;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void DiagnosticStore::queryMultipleResults (DiagnosticResultSet& resultSet, StringID context, CString keys[], int keyCount) const
{
	DiagnosticFilter contextFilter (context);

	const ObjectArray* results = (mode == kLongTerm) ? &data : &shortTermData;

	for(Attributes* a : iterate_as<Attributes> (*results))
	{
		if(contextFilter.matches (a->asUnknown ()))
		{
			for(int i = 0; i < keyCount; i++)
			{
				Attributes* keyData = a->getAttributes (keys[i]);
				if(keyData)
				{
					AutoPtr<DiagnosticResult> resultAttributes = NEW DiagnosticResult;
					resultAttributes->copyFrom (*keyData);
					resultAttributes->set (DiagnosticResult::kContext, a->getCString (DiagnosticResult::kContext, Text::kASCII));
					resultSet.add (resultAttributes.detach ());
				}
				else
					resultSet.add (nullptr);
			}
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IDiagnosticStore::DiagnosticMode CCL_API DiagnosticStore::setMode (IDiagnosticStore::DiagnosticMode newMode)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return mode;

	IDiagnosticStore::DiagnosticMode oldMode = mode;
	mode = newMode;	

	if(newMode == kLongTerm)
		cleanup ();

	return oldMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void DiagnosticStore::store ()
{
	XmlSettings settings (kPersistentName);
	settings.removeAll ();
	
	Attributes& a = settings.getAttributes (kPersistentName);
	a.queue (nullptr, data);

	settings.flush ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void DiagnosticStore::restore ()
{
	XmlSettings settings (kPersistentName);
	settings.restore ();
	
	Attributes& a = settings.getAttributes (kPersistentName);
	a.unqueue (data, nullptr, ccl_typeid<Attributes> ());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Attributes* DiagnosticStore::findContextData (const ObjectArray& contextContainer, StringID context) const
{
	for(Attributes* a : iterate_as<Attributes> (contextContainer))
		if(a->getCString (DiagnosticResult::kContext, Text::kASCII) == context)
			return a;

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& DiagnosticStore::getData (StringID context, StringID key, DiagnosticMode mode)
{
	const ObjectArray* results = (mode == kLongTerm) ? &data : &shortTermData;
	Attributes* attributes = findContextData (*results, context);

	if(attributes == nullptr)
	{
		attributes = NEW Attributes;
		attributes->set (DiagnosticResult::kContext, context, Text::kASCII);

		ObjectArray* results = (mode == kLongTerm) ? &data : &shortTermData;
		results->add (attributes);
	}

	if(!attributes->contains (key))
	{
		AutoPtr<Attributes> data = NEW Attributes;
		attributes->set (key, data, Attributes::kShare);
	}
	return *attributes->getAttributes (key);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void DiagnosticStore::cleanup ()
{
	shortTermData.removeAll ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DiagnosticStore::countDiagnosticData () const
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DiagnosticStore::getDiagnosticDescription (DiagnosticDescription& description, int index) const
{
	if(index == 0)
	{
		description.categoryFlags = DiagnosticDescription::kPlugInInformation | DiagnosticDescription::kApplicationLogs;
		description.fileName = kPersistentName;
		description.fileType = FileTypes::Xml ();
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API DiagnosticStore::createDiagnosticData (int index)
{
	if(index == 0)
	{
		XmlSettings settings (kPersistentName);
		return System::GetFileSystem ().openStream (settings.getPath ());
	}
	return nullptr;
}

//************************************************************************************************
// DiagnosticResult
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DiagnosticResult, Attributes)

DEFINE_STRINGID_MEMBER_ (DiagnosticResult, kContext, "context")
DEFINE_STRINGID_MEMBER_ (DiagnosticResult, kLabel, "label")
DEFINE_STRINGID_MEMBER_ (DiagnosticResult, kCount, "count")
DEFINE_STRINGID_MEMBER_ (DiagnosticResult, kMinimum, "min")
DEFINE_STRINGID_MEMBER_ (DiagnosticResult, kMaximum, "max")
DEFINE_STRINGID_MEMBER_ (DiagnosticResult, kAverage, "avg")
DEFINE_STRINGID_MEMBER_ (DiagnosticResult, kSum, "sum")
DEFINE_STRINGID_MEMBER_ (DiagnosticResult, kItems, "items")
DEFINE_STRINGID_MEMBER_ (DiagnosticResult, kValue, "value")
DEFINE_STRINGID_MEMBER_ (DiagnosticResult, kTimestamp, "timestamp")

////////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API DiagnosticResult::getContext () const
{
	Variant value;
	for(int i = 0; i < countAttributes (); i++)
	{
		if(getAttributeValue (value, i) && value.isString ())
			context = MutableCString (value.asString (), Text::kASCII);
	}
	return context;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API DiagnosticResult::getLabel () const
{
	label = getString (kLabel);
	return label;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API DiagnosticResult::getMinimum () const
{
	if(hasValues ())
	{
		double minimum = -1;
		IterForEach (newQueueIterator (kItems, ccl_typeid<Attributes> ()), Attributes, item)
			Variant value = 0.;
			item->getAttribute (value, kValue);
			if(minimum < 0)
				minimum = value.asDouble ();
			else
				minimum = ccl_min (minimum, value.asDouble ());
		EndFor
		return minimum;
	}
	else
		return getFloat (kMinimum);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API DiagnosticResult::getMaximum () const
{
	if(hasValues ())
	{
		double maximum = 0;
		IterForEach (newQueueIterator (kItems, ccl_typeid<Attributes> ()), Attributes, item)
			Variant value = 0.;
			item->getAttribute (value, kValue);
			maximum = ccl_max (maximum, value.asDouble ());
		EndFor
		return maximum;
	}
	else
		return getFloat (kMaximum);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API DiagnosticResult::getAverage () const
{
	if(hasValues ())
		return getSum () / getCount ();
	else
		return getFloat (kAverage);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API DiagnosticResult::getSum () const
{
	if(hasValues ())
	{
		double sum = 0;
		IterForEach (newQueueIterator (kItems, ccl_typeid<Attributes> ()), Attributes, item)
			Variant value = 0.;
			item->getAttribute (value, kValue);
			sum += value.asDouble ();
		EndFor
		return sum;
	}
	else
		return getFloat (kSum);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DiagnosticResult::getCount () const
{
	if(hasValues ())
	{
		int count = 0;
		IterForEach (newQueueIterator (kItems, ccl_typeid<Attributes> ()), Attributes, item)
			count++;
		EndFor
		return count;
	}
	else
		return getInt (kCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DiagnosticResult::hasValues () const
{
	return contains (kItems);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DiagnosticResult::getValue (Variant& value, int index) const
{
	if(hasValues ())
	{
		IterForEach (newQueueIterator (kItems, ccl_typeid<Attributes> ()), Attributes, item)
			if(index == 0)
			{
				item->getAttribute (value, kValue);
				return true;
			}
			--index;
		EndFor
		return false;
	}
	else
		return getAttribute (value, kValue);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int64 DiagnosticResult::getTimestamp (int index) const
{
	IterForEach (newQueueIterator (kItems, ccl_typeid<Attributes> ()), Attributes, item)
		if(index == 0)
		{
			return item->getInt64 (kTimestamp);
			return true;
		}
		--index;
	EndFor
	return 0;
}

//************************************************************************************************
// DiagnosticResultSet
//************************************************************************************************

DiagnosticResultSet::DiagnosticResultSet ()
{
	items.objectCleanup ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void DiagnosticResultSet::add (DiagnosticResult* item)
{
	items.add (item);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IDiagnosticResult* CCL_API DiagnosticResultSet::at (int index) const
{
	return static_cast<DiagnosticResult*> (items.at (index));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DiagnosticResultSet::getCount () const
{
	return items.count ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DiagnosticResultSet::sortByMinimum ()
{
	items.sort (LAMBDA_VECTOR_COMPARE_OBJECT (DiagnosticResult, item1, item2)
		if(item1 == nullptr || item2 == nullptr)
			return 0;
		return ccl_compare (item1->getMinimum (), item2->getMinimum ());
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DiagnosticResultSet::sortByMaximum ()
{
	items.sort (LAMBDA_VECTOR_COMPARE (DiagnosticResult, item1, item2)
		if(item1 == nullptr || item2 == nullptr)
			return 0;
		return ccl_compare (item2->getMaximum (), item1->getMaximum ());
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DiagnosticResultSet::sortByAverage ()
{
	items.sort (LAMBDA_VECTOR_COMPARE (DiagnosticResult, item1, item2)
		if(item1 == nullptr || item2 == nullptr)
			return 0;
		return ccl_compare (item2->getAverage (), item1->getAverage ());
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DiagnosticResultSet::sortBySum ()
{
	items.sort (LAMBDA_VECTOR_COMPARE (DiagnosticResult, item1, item2)
		if(item1 == nullptr || item2 == nullptr)
			return 0;
		return ccl_compare (item2->getSum (), item1->getSum ());
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DiagnosticResultSet::sortByCount ()
{
	items.sort (LAMBDA_VECTOR_COMPARE (DiagnosticResult, item1, item2)
		if(item1 == nullptr || item2 == nullptr)
			return 0;
		return ccl_compare (item2->getCount (), item1->getCount ());
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API DiagnosticResultSet::createIterator () const
{
	return items.newIterator ();
}

//************************************************************************************************
// DiagnosticFilter
//************************************************************************************************

DiagnosticFilter::DiagnosticFilter (StringID context)
: context (context),
  contextString (Text::kASCII, context)
{}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DiagnosticFilter::matches (IUnknown* object) const
{
	Attributes* result = unknown_cast<Attributes> (object);
	if(result == nullptr)
		return false;
	
	return result->getVariant (DiagnosticResult::kContext) == contextString;
}

//************************************************************************************************
// DiagnosticWildcardFilter
//************************************************************************************************

DiagnosticWildcardFilter::DiagnosticWildcardFilter (StringID context)
: DiagnosticFilter (context)
{}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DiagnosticWildcardFilter::matches (IUnknown* object) const
{
	Attributes* result = unknown_cast<Attributes> (object);
	if(result == nullptr)
		return false;
		
	MutableCString objectContext (result->getCString (DiagnosticResult::kContext, Text::kASCII));
	
	MutableCString filterContext (context);

	while(!filterContext.isEmpty ())
	{
		if(objectContext.isEmpty ())
			return false;

		int objectPosition = objectContext.lastIndex ('/');
		MutableCString objectParameter = objectContext.subString (objectPosition + 1);

		int filterPosition = filterContext.lastIndex ('/');
		MutableCString filterParameter = filterContext.subString (filterPosition + 1);

		if(objectParameter != filterParameter && filterParameter != "*")
			return false;

		objectContext.truncate (objectPosition > 0 ? objectPosition : 0);
		filterContext.truncate (filterPosition > 0 ? filterPosition : 0);
	}
	return true;
}
