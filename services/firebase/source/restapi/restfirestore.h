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
// Filename    : restfirestore.h
// Description : Firestore class using REST API
//
//************************************************************************************************

#ifndef _firebase_firestore_h
#define _firebase_firestore_h

#include "ccl/base/storage/attributes.h"
#include "ccl/public/collections/variantvector.h"

#include "ccl/extras/firebase/ifirestore.h"

namespace CCL {
class AsyncSequence;

namespace Web {
interface IXMLHttpRequest; }

namespace Firebase {
class RESTApp;

namespace Firestore {
class RESTSnapshot;

//************************************************************************************************
// Firebase::Firestore::RESTFirestore
//************************************************************************************************

class RESTFirestore: public Object,
					 public IFirestore
{
public:
	DECLARE_CLASS_ABSTRACT (RESTFirestore, Object)

	RESTFirestore (RESTApp& app);

	RESTApp& getAppInternal () const { return app; }
	
	String getDbRootPath () const;
	String getDbRootUrl () const;

	Web::IXMLHttpRequest* sendRequest (StringID method, UrlRef url,
		StringID contentType = nullptr,
		IStream* data = nullptr);

	// IFirestore
	IApp& CCL_API getApp () const override;
	IDocumentReference& CCL_API getDocument (StringRef documentPath) override;
	ICollectionReference& CCL_API getCollection (StringRef collectionPath) override;
	IWriteBatch* CCL_API createBatch () override;

	CLASS_INTERFACE (IFirestore, Object)

protected:
	RESTApp& app;
	ObjectArray documentReferences;
	ObjectArray collectionReferences;
};

//************************************************************************************************
// Firebase::Firestore::RESTFirestoreObject
//************************************************************************************************

class RESTFirestoreObject: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (RESTFirestoreObject, Object)

	RESTFirestoreObject (RESTFirestore& store, StringRef objectPath);

	static String extractID (StringRef path);
	static String makePath (StringRef parent, StringRef id);

	PROPERTY_STRING (objectPath, ObjectPath)
	PROPERTY_STRING (objectId, ObjectID) // id is last part of path

	virtual IAsyncOperation* CCL_API get ();
	String getFullObjectPath () const;

	// Object
	int compare (const Object& obj) const override;


protected:
	RESTFirestore& store;

	String getRequestURL () const;

	virtual RESTSnapshot* createSnapshot () = 0;
	IAsyncOperation* CCL_API sendGET (StringRef nextPageToken = "");
	void handleGetResponse (IAsyncOperation& op, AsyncSequence* sequence, RESTSnapshot* snapshot);
};

//************************************************************************************************
// Firebase::Firestore::RESTSnapshot
//************************************************************************************************

class RESTSnapshot: public Object,
                    public ISnapshot
{
public:
	DECLARE_CLASS_ABSTRACT (RESTSnapshot, Object)

	void setDatabaseTimestamp (DateTime date);
	virtual void assign (const Attributes& jsonResult, IClassAllocator& allocator) = 0;

	// ISnapshot
	tresult CCL_API getDatabaseTimestamp (DateTime& date) const override;

	CLASS_INTERFACE (ISnapshot, Object)

protected:
	DateTime date;
};

//************************************************************************************************
// Firebase::Firestore::RESTDocumentReference
//************************************************************************************************

class RESTDocumentReference: public RESTFirestoreObject,
							 public IDocumentReference
{
public:
	DECLARE_CLASS_ABSTRACT (RESTDocumentReference, RESTFirestoreObject)

	using RESTFirestoreObject::RESTFirestoreObject;

	// IDocumentReference
	StringRef CCL_API getID () const override;
	StringRef CCL_API getPath () const override;
	IAsyncOperation* CCL_API get () override;
	IAsyncOperation* CCL_API set (const IAttributeList& data, const SetOptions& setOptions) override;
	IAsyncOperation* CCL_API remove () override;

	CLASS_INTERFACE (IDocumentReference, RESTFirestoreObject)

protected:
	// RESTFirestoreObject
	RESTSnapshot* createSnapshot () override;
};

//************************************************************************************************
// Firebase::Firestore::RESTDocumentSnapshot
//************************************************************************************************

class RESTDocumentSnapshot: public RESTSnapshot,
	                        public IDocumentSnapshot
{
public:
	DECLARE_CLASS_ABSTRACT (RESTDocumentSnapshot, RESTSnapshot)

	RESTDocumentSnapshot (StringRef documentId);

	// RESTSnapshot
	void assign (const Attributes& jsonResult, IClassAllocator& allocator) override;
	
	// IDocumentSnapshot
	StringRef CCL_API getID () const override;
	FieldValue CCL_API get (StringID field) const override;
	void CCL_API getData (IAttributeList& data) const override;
	tresult CCL_API getDatabaseTimestamp (DateTime& date) const override;

	CLASS_INTERFACE (IDocumentSnapshot, RESTSnapshot)
	
protected:
	String documentId;
	AutoPtr<IAttributeList> data;
};

//************************************************************************************************
// Firebase::Firestore::RESTCollectionReference
//************************************************************************************************

class RESTCollectionReference: public RESTFirestoreObject,
							   public ICollectionReference
{
public:
	DECLARE_CLASS_ABSTRACT (RESTCollectionReference, RESTFirestoreObject)

	using RESTFirestoreObject::RESTFirestoreObject;

	IUnknown* onAddCompleted (const Attributes& jsonResult);

	// ICollectionReference
	StringRef CCL_API getID () const override;
	StringRef CCL_API getPath () const override;
	IAsyncOperation* CCL_API get () override;
	IAsyncOperation* CCL_API add (const IAttributeList& data) override;

	CLASS_INTERFACE (ICollectionReference, RESTFirestoreObject)

protected:
	class AddOperation;

	// RESTFirestoreObject
	RESTSnapshot* createSnapshot () override;
};

//************************************************************************************************
// Firebase::Firestore::RESTQuerySnapshot
//************************************************************************************************

class RESTQuerySnapshot: public RESTSnapshot,
                         public IQuerySnapshot
{
public:
	DECLARE_CLASS_ABSTRACT (RESTQuerySnapshot, RESTSnapshot)

	// RESTSnapshot
	void assign (const Attributes& jsonResult, IClassAllocator& allocator) override;

	// IQuerySnapshot
	IArrayObject& CCL_API getDocuments () override;
	tresult CCL_API getDatabaseTimestamp (DateTime& date) const override;

	CLASS_INTERFACE (IQuerySnapshot, RESTSnapshot)

protected:
	VariantVector documentSnapshots;
};
//************************************************************************************************
// Firebase::Firestore::RESTWriteBatch
//************************************************************************************************

class RESTWriteBatch: public Object,
                      public IWriteBatch
{
public:
	DECLARE_CLASS_ABSTRACT (RESTWriteBatch, Object)

	RESTWriteBatch (RESTFirestore& store);

	// IWriteBatch
	virtual IAsyncOperation* CCL_API commit () override;
	virtual IWriteBatch& CCL_API deleteDocument (const IDocumentReference& document) override;
	virtual IWriteBatch& CCL_API set (const IDocumentReference& document, const IAttributeList& data, const SetOptions& setOptions) override;

	CLASS_INTERFACE (IWriteBatch, RESTWriteBatch)

protected:
	AutoPtr<AttributeQueue> writes;
	RESTFirestore& store;
};

} // namespace Firestore
} // namespace Firebase
} // namespace CCL

#endif // _firebase_firestore_h
