//************************************************************************************************
//
// Firebase Service
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
// Filename    : restfirestore.cpp
// Description : Firestore class using REST API
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "restfirestore.h"
#include "restapp.h"
#include "restauth.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/jsonarchive.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/text/istringdict.h"
#include "ccl/public/network/web/httpstatus.h"
#include "ccl/public/network/web/iwebrequest.h"
#include "ccl/public/network/web/iwebcredentials.h"
#include "ccl/public/plugservices.h"

/*
	Firestore REST API Documentation
 
	- Value Encoding: https://firebase.google.com/docs/firestore/reference/rest/v1/Value
	- Get document: https://firebase.google.com/docs/firestore/reference/rest/v1/projects.databases.documents/get
	- Update or insert document: https://firebase.google.com/docs/firestore/reference/rest/v1/projects.databases.documents/patch
	- Update, insert or transform document: https://firebase.google.com/docs/firestore/reference/rest/v1/projects.databases.documents/commit
											https://firebase.google.com/docs/firestore/reference/rest/v1/Write
	- Create document: https://firebase.google.com/docs/firestore/reference/rest/v1/projects.databases.documents/createDocument
	- Remove document: https://firebase.google.com/docs/firestore/reference/rest/v1/projects.databases.documents/delete

	Firestore C++ SDK Documentation

	- CollectionReference class: https://firebase.google.com/docs/reference/cpp/class/firebase/firestore/collection-reference
	- DocumentReference class: https://firebase.google.com/docs/reference/cpp/class/firebase/firestore/document-reference
	- FieldValue class: https://firebase.google.com/docs/reference/cpp/class/firebase/firestore/field-value
*/

#define FIRESTORE_ENDPOINT "https://firestore.googleapis.com/v1/"

namespace CCL {
namespace Firebase {
namespace Firestore {

//************************************************************************************************
// Firebase::Firestore::RESTValueEncoding
//************************************************************************************************

class RESTValueEncoding
{
public:
	static bool unpackFieldValue (Variant& dstValue, const Attributes& fieldAttr, IClassAllocator& allocator);
	static bool packFieldValue (Attributes& fieldAttr, const FieldValue& srcValue, bool skipSentinels = true);
	static bool packTransformValue (Attributes& transformAttr, CStringRef name, const FieldValue& srcValue);
	static void unpackFields (IAttributeList& dst, const Attributes& src);
	static void packFields (Attributes& dst, const IAttributeList& src);
	static void packFieldTransforms (AttributeQueue& dst, const IAttributeList& src);
};

} // namespace Firestore
} // namespace Firebase
} // namespace CCL

using namespace CCL;
using namespace Web;
using namespace Firebase;
using namespace Firestore;

//************************************************************************************************
// Firebase::Firestore::RESTCollectionReference::AddOperation
//************************************************************************************************

class RESTCollectionReference::AddOperation: public RESTOperation
{
public:
	AddOperation (RESTCollectionReference& collection, IXMLHttpRequest* httpRequest)
	: RESTOperation (httpRequest),
	  collection (collection)
	{}

	// RESTOperation
	void onHttpRequestFinished () override
	{
		RESTOperation::onHttpRequestFinished ();
		if(!hasError ())
		{
			AutoPtr<IUnknown> newResult = collection.onAddCompleted (getJsonResult ());
			setResult (Variant ().takeShared (newResult));
		}
	}

protected:
	RESTCollectionReference& collection;
};

//************************************************************************************************
// Firebase::Firestore::RESTFirestore
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RESTFirestore, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

RESTFirestore::RESTFirestore (RESTApp& app)
: app (app)
{
	documentReferences.objectCleanup (true);
	collectionReferences.objectCleanup (true);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

IApp& CCL_API RESTFirestore::getApp () const
{
	return app;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDocumentReference& CCL_API RESTFirestore::getDocument (StringRef documentPath)
{	
	auto dr = static_cast<RESTDocumentReference*> (documentReferences.search (RESTDocumentReference (*this, documentPath)));
	if(!dr)
		documentReferences.add (dr = NEW RESTDocumentReference (*this, documentPath));
	return *dr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICollectionReference& CCL_API RESTFirestore::getCollection (StringRef collectionPath)
{
	auto cr = static_cast<RESTCollectionReference*> (collectionReferences.search (RESTCollectionReference (*this, collectionPath)));
	if(!cr)
		collectionReferences.add (cr = NEW RESTCollectionReference (*this, collectionPath));
	return *cr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWriteBatch* CCL_API RESTFirestore::createBatch ()
{
	return NEW RESTWriteBatch (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String RESTFirestore::getDbRootPath () const
{
	String projectId = getApp ().getOptions ().projectId;
	return String () << "projects/" << projectId << "/databases/(default)/documents";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String RESTFirestore::getDbRootUrl () const
{
	return String () << FIRESTORE_ENDPOINT << getDbRootPath ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IXMLHttpRequest* RESTFirestore::sendRequest (StringID method, UrlRef url, StringID contentType, IStream* data)
{
	ASSERT (!data || !contentType.isEmpty ())
	auto futureId = getAppInternal ().getAuth ().getCurrentUser ()->getToken ();
	SharedPtr<IXMLHttpRequest> request = ccl_new<IXMLHttpRequest> (ClassID::XMLHttpRequest);

	if(data)
		data->retain ();
	Url localUrl (url);

	Promise (futureId).then ([=] (IAsyncOperation& op)
	{
		request->open (method, localUrl, true, nullptr, op.getResult ().asString (), String (Meta::kBearer));
		if(!contentType.isEmpty ())
			request->setRequestHeader (Meta::kContentType, contentType);
		request->send (data);
		if(data)
			data->release ();
	});

	return request;
}


//************************************************************************************************
// Firebase::Firestore::RESTFirestoreObject
//************************************************************************************************

String RESTFirestoreObject::extractID (StringRef path)
{
	return path.subString (path.lastIndex (Url::strPathChar)+1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String RESTFirestoreObject::makePath (StringRef parent, StringRef id)
{
	return String () << parent << Url::strPathChar << id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (RESTFirestoreObject, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

RESTFirestoreObject::RESTFirestoreObject (RESTFirestore& store, StringRef objectPath)
: store (store),
  objectPath (objectPath),
  objectId (extractID (objectPath))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int RESTFirestoreObject::compare (const Object& obj) const
{
	return objectPath.compare (static_cast<const RESTFirestoreObject&> (obj).objectPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String RESTFirestoreObject::getRequestURL () const
{
	return String () << FIRESTORE_ENDPOINT << getFullObjectPath ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String RESTFirestoreObject::getFullObjectPath () const
{
	return String () << store.getDbRootPath () << "/" << objectPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* RESTFirestoreObject::get ()
{
	AutoPtr<RESTSnapshot> snapshot = createSnapshot ();
	AutoPtr<AsyncSequence> sequence = NEW AsyncSequence;
	sequence->setCancelOnError (true);

	sequence->add ([this] () { return sendGET (); });
	sequence->then ([this, sequence, snapshot] (IAsyncOperation& op)
	{
		handleGetResponse (op, sequence, snapshot);
	});

	return return_shared<IAsyncOperation> (sequence->start ().then ([snapshot] (IAsyncOperation& op)
	{
		op.setResult (Variant ().takeShared (snapshot->asUnknown ()));
	}));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* RESTFirestoreObject::sendGET (StringRef nextPageToken)
{
	Url url (getRequestURL ());
	if(!nextPageToken.isEmpty ())
		url.getParameters ().appendEntry ("pageToken", nextPageToken);
	auto request = store.sendRequest (HTTP::kGET, url);
	auto operation = NEW RESTOperation (request);
	operation->setState (IAsyncInfo::kStarted);
	return operation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RESTFirestoreObject::handleGetResponse (IAsyncOperation& op, AsyncSequence* sequence, RESTSnapshot* snapshot)
{
	if(op.getState () == IAsyncOperation::kCompleted)
	{
		auto operation = unknown_cast<RESTOperation> (&op);
		ASSERT (operation)
		auto jsonResult = operation->getJsonResult ();
		auto& allocator = store.getAppInternal ().getAllocator ();
		snapshot->assign (jsonResult, allocator);

		DateTime snapshotTimestamp;
		if(snapshot->getDatabaseTimestamp (snapshotTimestamp) != kResultOk)
			snapshot->setDatabaseTimestamp (operation->getResponseTimestamp ());

		if(jsonResult.contains ("nextPageToken"))
		{
			String nextPageToken = jsonResult.getString ("nextPageToken");
			sequence->add ([this, nextPageToken] ()
			{
				return sendGET (nextPageToken);
			});
			sequence->then ([this, sequence, snapshot] (IAsyncOperation& op) { handleGetResponse (op, sequence, snapshot); });
		}
	}
}

//************************************************************************************************
// Firebase::Firestore::RESTSnapshot
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RESTSnapshot, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void RESTSnapshot::setDatabaseTimestamp (DateTime date)
{
	this->date = date;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API RESTSnapshot::getDatabaseTimestamp (DateTime& date) const
{
	if(this->date == DateTime ())
		return kResultFailed;
	date = this->date;
	return kResultOk;
}

//************************************************************************************************
// Firebase::Firestore::RESTDocumentReference
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RESTDocumentReference, RESTFirestoreObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API RESTDocumentReference::getID () const
{
	return objectId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API RESTDocumentReference::getPath () const
{
	return objectPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API RESTDocumentReference::get ()
{
	return SuperClass::get ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RESTSnapshot* RESTDocumentReference::createSnapshot ()
{
	return NEW RESTDocumentSnapshot (getID ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API RESTDocumentReference::set (const IAttributeList& data, const SetOptions& setOptions)
{
	AutoPtr<RESTWriteBatch> batch = NEW RESTWriteBatch (store);
	batch->set (*this, data, setOptions);
	return batch->commit ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API RESTDocumentReference::remove ()
{
	Url url (getRequestURL ());
	auto request = store.sendRequest ("DELETE", url);

	// TODO: delete reference object in store???
	auto operation = NEW RESTVoidOperation (request); // void like in Firebase SDK
	operation->setState (IAsyncInfo::kStarted);
	return operation;
}

//************************************************************************************************
// Firebase::Firestore::RESTValueEncoding
//************************************************************************************************

bool RESTValueEncoding::unpackFieldValue (Variant& dstValue, const Attributes& fieldAttr, IClassAllocator& allocator)
{
	MutableCString fieldType;
	if(!fieldAttr.getAttributeName (fieldType, 0))
		return false;
				
	Variant fieldValue;
	fieldAttr.getAttributeValue (fieldValue, 0);

	if(fieldType == "arrayValue")
	{
		if(auto arraySrc = unknown_cast<Attributes> (fieldValue.asUnknown ()))
		{
			AutoPtr<IAttributeQueue> dstQueue = AttributeClassFactory (allocator).newAttributeQueue ();
			IterForEach (arraySrc->newQueueIterator ("values", ccl_typeid<Attributes> ()), Attributes, valueAttr)
				Variant v;
				unpackFieldValue (v, *valueAttr, allocator);
				dstQueue->addValue (v);
			EndFor
			dstValue.takeShared (dstQueue);
		}
	}
	else if(fieldType == "mapValue")
	{
		if(auto mapSrc = unknown_cast<Attributes> (fieldValue.asUnknown ()))
		{
			AutoPtr<IAttributeList> mapDst = AttributeClassFactory (allocator).newAttributes ();
			unpackFields (*mapDst, *mapSrc);
			dstValue.takeShared (mapDst);
		}
	}
	else
	{
		// could be "stringValue", etc.
		dstValue = fieldValue;
		dstValue.share ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RESTValueEncoding::packFieldValue (Attributes& fieldAttr, const FieldValue& srcValue, bool skipSentinels)
{
	bool result = true;
	if(srcValue.isSentinel () && skipSentinels)
	{
		result = false;
	}
	else if(UnknownPtr<IAttributeQueue> srcQueue = srcValue.asUnknown ())
	{
		Attributes* arrayValue = NEW Attributes;
		fieldAttr.set ("arrayValue", arrayValue, Attributes::kOwns);
		AttributeQueue* valuesQueue = NEW AttributeQueue;
		arrayValue->set ("values", valuesQueue, Attributes::kOwns);
		
		if(UnknownPtr<IContainer> c = srcValue.asUnknown ())
			ForEachUnknown (*c, unk)
				if(UnknownPtr<IAttribute> attr = unk)
				{
					AutoPtr<Attributes> fieldAttr2 = NEW Attributes;
					if(packFieldValue (*fieldAttr2, attr->getValue ()))
						valuesQueue->addAttributes (fieldAttr2.detach (), Attributes::kOwns);
				}
			EndFor
	}
	else if(UnknownPtr<IAttributeList> srcList = srcValue.asUnknown ())
	{
		Attributes* mapValue = NEW Attributes;
		packFields (*mapValue, *srcList);
		fieldAttr.set ("mapValue", mapValue, Attributes::kOwns);
	}
	else
	{
		ASSERT (!srcValue.isObject ())
		// TODO: geo points, null values???

		switch(srcValue.getType ())
		{
		case Variant::kInt :
			if(srcValue.isBoolFormat ())
				fieldAttr.setAttribute ("booleanValue", srcValue);
			else
				fieldAttr.setAttribute ("integerValue", srcValue);
			break;
		case Variant::kFloat :
			fieldAttr.setAttribute ("doubleValue", srcValue);
			break;
		case Variant::kString :
			if(srcValue.isTimestamp ())
				fieldAttr.setAttribute ("timestampValue", srcValue);
			else
				fieldAttr.setAttribute ("stringValue", srcValue);
			break;
		default :
			result = false;
			break;
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CCL::Firebase::Firestore::RESTValueEncoding::packTransformValue (Attributes& transformAttr, CStringRef name, const FieldValue& srcValue)
{
	if(!srcValue.isSentinel ())
	{
		return false;
	}
	AutoPtr<Attributes> transform = NEW Attributes;
	if(!packFieldValue (*transform, srcValue, false))
	{
		return false;
	}
	switch(srcValue.getUserValue ())
	{
	case FieldValue::kIncrement:
		transformAttr.set ("increment", transform.detach (), Attributes::kOwns);
		break;
	case FieldValue::kSetToServerValue:
		switch(srcValue.asInt ())
		{
		case FieldValue::kRequestTime:
			transformAttr.set ("setToServerValue", "REQUEST_TIME");
			break;
		}
		break;
	case FieldValue::kMaximum:
		transformAttr.set ("maximum", transform.detach (), Attributes::kOwns);
		break;
	case FieldValue::kMinimum:
		transformAttr.set ("minimum", transform.detach (), Attributes::kOwns);
		break;
	case FieldValue::kAppendMissingElements:
		transformAttr.set ("appendMissingElements", transform.detach (), Attributes::kOwns);
		break;
	case FieldValue::kRemoveAllFromArray:
		transformAttr.set ("removeAllFromArray", transform.detach (), Attributes::kOwns);
		break;
	default:
		return false;
	}
	transformAttr.set ("fieldPath", name);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RESTValueEncoding::unpackFields (IAttributeList& dst, const Attributes& src)
{
	if(auto fields = src.getAttributes ("fields"))
		ForEachAttribute (*fields, fieldName, var)
			if(auto fieldAttr = unknown_cast<Attributes> (var.asUnknown ()))
			{					
				Variant plainValue;
				if(unpackFieldValue (plainValue, *fieldAttr, dst))
					dst.setAttribute (fieldName, plainValue);
			}
		EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RESTValueEncoding::packFields (Attributes& dst, const IAttributeList& src)
{
	Attributes* fields = NEW Attributes;
	dst.set ("fields", fields, Attributes::kOwns);
	ForEachAttribute (src, fieldName, var)
		AutoPtr<Attributes> fieldAttr = NEW Attributes;
		if(packFieldValue (*fieldAttr, var))
			fields->set (fieldName, fieldAttr.detach (), Attributes::kOwns);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RESTValueEncoding::packFieldTransforms (AttributeQueue& dst, const IAttributeList& src)
{
	ForEachAttribute (src, fieldName, var)
		AutoPtr<Attributes> transformAttr = NEW Attributes;
		if(packTransformValue (*transformAttr, fieldName, var))
			dst.addAttributes (transformAttr.detach (), Attributes::kOwns);
	EndFor
}

//************************************************************************************************
// Firebase::Firestore::RESTDocumentSnapshot
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RESTDocumentSnapshot, RESTSnapshot)

//////////////////////////////////////////////////////////////////////////////////////////////////

RESTDocumentSnapshot::RESTDocumentSnapshot (StringRef documentId)
: documentId (documentId)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RESTDocumentSnapshot::assign (const Attributes& jsonResult, IClassAllocator& allocator)
{
	data = AttributeClassFactory (allocator).newAttributes ();
	RESTValueEncoding::unpackFields (*data, jsonResult);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API RESTDocumentSnapshot::getID () const
{
	return documentId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FieldValue CCL_API RESTDocumentSnapshot::get (StringID field) const
{
	FieldValue value;
	if(data)
		data->getAttribute (value, field);
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API RESTDocumentSnapshot::getData (IAttributeList& _data) const
{
	if(data)
		_data.copyFrom (*data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API RESTDocumentSnapshot::getDatabaseTimestamp (DateTime& date) const
{
	return SuperClass::getDatabaseTimestamp (date);
}

//************************************************************************************************
// Firebase::Firestore::RESTCollectionReference
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RESTCollectionReference, RESTFirestoreObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API RESTCollectionReference::getID () const
{
	return objectId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API RESTCollectionReference::getPath () const
{
	return objectPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API RESTCollectionReference::get ()
{
	return SuperClass::get ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RESTSnapshot* RESTCollectionReference::createSnapshot ()
{
	return NEW RESTQuerySnapshot;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API RESTCollectionReference::add (const IAttributeList& data)
{
	Attributes fields;
	RESTValueEncoding::packFields (fields, data);
	AutoPtr<IStream> jsonData = JsonUtils::serialize (fields);

	Url url (getRequestURL ());	
	auto request = store.sendRequest (HTTP::kPOST, url, JsonArchive::kMimeType, jsonData);
	auto operation = NEW AddOperation (*this, request);
	operation->setState (IAsyncInfo::kStarted);
	return operation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* RESTCollectionReference::onAddCompleted (const Attributes& jsonResult)
{
	String name = jsonResult.getString ("name");
	String newId = extractID (name);
	ASSERT (!newId.isEmpty ())
	if(newId.isEmpty ())
		return nullptr;
	
	String documentPath = makePath (objectPath, newId);
	auto& document = store.getDocument (documentPath);
	return return_shared (&document);
}

//************************************************************************************************
// Firebase::Firestore::RESTQuerySnapshot
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RESTQuerySnapshot, RESTSnapshot)

//////////////////////////////////////////////////////////////////////////////////////////////////

void RESTQuerySnapshot::assign (const Attributes& jsonResult, IClassAllocator& allocator)
{
	IterForEach (jsonResult.newQueueIterator ("documents", ccl_typeid<Attributes> ()), Attributes, documentAttr)
		String name = documentAttr->getString ("name");
		String documentId = RESTFirestoreObject::extractID (name);

		AutoPtr<RESTDocumentSnapshot> ds = NEW RESTDocumentSnapshot (documentId);
		ds->assign (*documentAttr, allocator);
		documentSnapshots.add (Variant ().takeShared (ds->asUnknown ()));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IArrayObject& CCL_API RESTQuerySnapshot::getDocuments ()
{
	return documentSnapshots;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API RESTQuerySnapshot::getDatabaseTimestamp (DateTime& date) const
{
	return SuperClass::getDatabaseTimestamp (date);
}

//************************************************************************************************
// Firebase::Firestore::RESTWriteBatch
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RESTWriteBatch, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

RESTWriteBatch::RESTWriteBatch (RESTFirestore& store)
: store (store),
  writes (NEW AttributeQueue)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API RESTWriteBatch::commit ()
{
	Attributes jsonStructure;
	jsonStructure.set ("writes", writes);
	AutoPtr<IStream> jsonData = JsonUtils::serialize (jsonStructure);
	writes = NEW AttributeQueue; // reset
	String endpoint = store.getDbRootUrl () << ":commit";
	Url url (endpoint);
	auto request = store.sendRequest (HTTP::kPOST, url, JsonArchive::kMimeType, jsonData);

	auto operation = NEW RESTVoidOperation (request); // void like in Firebase SDK
	operation->setState (IAsyncInfo::kStarted);
	return operation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWriteBatch& CCL_API RESTWriteBatch::deleteDocument (const IDocumentReference& document)
{
	Attributes* updateWrapper = NEW Attributes;
	writes->addAttributes (updateWrapper, Attributes::kOwns);

	String documentPath = unknown_cast<RESTDocumentReference> (&document)->getFullObjectPath ();
	updateWrapper->set ("delete", documentPath);

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWriteBatch& CCL_API RESTWriteBatch::set (const IDocumentReference& document, const IAttributeList& data, const SetOptions& setOptions)
{
	Attributes* updateWrapper = NEW Attributes;
	writes->addAttributes (updateWrapper, Attributes::kOwns);

	Attributes* update = NEW Attributes;
	updateWrapper->set ("update", update, Attributes::kOwns);
	AttributeQueue* updateTransforms = NEW AttributeQueue;
	updateWrapper->set ("updateTransforms", updateTransforms, Attributes::kOwns);

	String documentPath = unknown_cast<RESTDocumentReference> (&document)->getFullObjectPath ();
	update->set ("name", documentPath);
	
	RESTValueEncoding::packFields (*update, data);

	RESTValueEncoding::packFieldTransforms (*updateTransforms, data);

	// set fields to update
	if(setOptions.type == SetOptions::kMergeSpecific)
	{
		Attributes* updateMask = NEW Attributes;
		updateWrapper->set ("updateMask", updateMask, Attributes::kOwns);

		AttributeQueue* fieldPaths = NEW AttributeQueue;
		updateMask->set ("fieldPaths", fieldPaths, Attributes::kOwns);

		if(setOptions.fields)
		{
			auto& fieldNames = *(setOptions.fields);
			for(int i = 0; i < fieldNames.getArrayLength (); i++)
			{
				auto a = fieldNames[i];
				fieldPaths->addValue (a);
			}
		}
	}
	else if(setOptions.type == SetOptions::kMergeAll)
	{
		Attributes* updateMask = NEW Attributes;
		updateWrapper->set ("updateMask", updateMask, Attributes::kOwns);

		AttributeQueue* fieldPaths = NEW AttributeQueue;
		updateMask->set ("fieldPaths", fieldPaths, Attributes::kOwns);

		ForEachAttribute (data, fieldName, var)
			if(!FieldValue (var).isSentinel ())
			{
				fieldPaths->addValue (fieldName.str (), Attributes::kOwns);
			}
		EndFor
	}

	return *this;
}
