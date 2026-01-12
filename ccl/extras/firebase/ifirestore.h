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
// Filename    : ccl/extras/firebase/ifirestore.h
// Description : Firestore Interfaces
//
//************************************************************************************************

#ifndef _ccl_ifirestore_h
#define _ccl_ifirestore_h

#include "ccl/public/base/variant.h"
#include "ccl/public/base/iasyncoperation.h"

#include "ccl/extras/firebase/timestamp.h"
#include "ccl/extras/firebase/errorcodes.h"

namespace CCL {
interface IAttributeList; 
interface IAttributeQueue;
interface IArrayObject;

namespace Firebase {
interface IApp;

namespace Firestore {

interface IDocumentReference;
interface ICollectionReference;
interface IWriteBatch;

//************************************************************************************************
// Firebase::Firestore::IFirestore
//************************************************************************************************

interface IFirestore: IUnknown
{
	/** Get app this object belongs to. */
	virtual IApp& CCL_API getApp () const = 0;

	/** Return reference to document at given path in the database. */
	virtual IDocumentReference& CCL_API getDocument (StringRef documentPath) = 0;
	
	/** Return reference to collection at given path in the database. */
	virtual ICollectionReference& CCL_API getCollection (StringRef collectionPath) = 0;

	/** Return reference to a WriteBatch which can accumulate multiple writes and deletes and commit them in one go. */
	virtual IWriteBatch* CCL_API createBatch () = 0;

	DECLARE_IID (IFirestore)
};

DEFINE_IID (IFirestore, 0x43c42f06, 0x78a0, 0x4586, 0xbd, 0x21, 0x4a, 0xf9, 0xe3, 0xf2, 0x8a, 0xda)

//************************************************************************************************
// Firebase::Firestore::SetOptions
//************************************************************************************************

struct SetOptions
{
	DEFINE_ENUM (Type)
	{
		//kOverwrite,
		kMergeAll,
		kMergeSpecific
	};
	
	Type type = kMergeAll;
	const IArrayObject* fields = nullptr;
};

//************************************************************************************************
// Firebase::Firestore::IDocumentReference
//************************************************************************************************

interface IDocumentReference: IUnknown
{
	/** Get ID of this document location. */
	virtual StringRef CCL_API getID () const = 0;

	/** Get relative path of this document. */
	virtual StringRef CCL_API getPath () const = 0;

	/** Read the document.
		Result: IDocumentSnapshot. */
	virtual IAsyncOperation* CCL_API get () = 0;
	
	/** Write to document.
		Result: (no data) */
	virtual IAsyncOperation* CCL_API set (const IAttributeList& data, const SetOptions& setOptions) = 0;
	
	/** Delete document in database. 
		Result: (no data) */
	virtual IAsyncOperation* CCL_API remove () = 0;

	DECLARE_IID (IDocumentReference)
};

DEFINE_IID (IDocumentReference, 0xfe71071d, 0xba1d, 0x4f10, 0x82, 0x35, 0xf8, 0x30, 0xc5, 0xe9, 0x13, 0x6)

//************************************************************************************************
// Firebase::Firestore::FieldValue
//************************************************************************************************

struct FieldValue: Variant
{
	enum Types
	{
		kStandard = 0,
		kTimestamp,
		kFirstSentinel,
		kSetToServerValue = kFirstSentinel,
		kIncrement,
		kMaximum,
		kMinimum,
		kAppendMissingElements,
		kRemoveAllFromArray
	};

	enum ServerValues
	{
		kRequestTime
	};

	FieldValue ();
	FieldValue (const Variant& v);
	
	void operator = (const Variant& v);

	Timestamp getTimestampValue () const;
	FieldValue& setTimestampValue (Timestamp t);
	bool isTimestamp () const;
	bool isSentinel () const;

	static FieldValue Increment (int byValue);
	static FieldValue Increment (float byValue);
	
	/**
	* Creates a sentinel to remove the given elements from a databases array if existent
	* @param elements the array elements to remove. The function takes ownership of the given Object
	*/
	static FieldValue ArrayRemove (IAttributeQueue* elements);
	
	/**
	* Creates a sentinel to add the given elements from the databases array if not yet existent
	* @param elements the array elements to add. The function takes ownership of the given Object
	*/
	static FieldValue ArrayUnion (IAttributeQueue* elements);
	
	static FieldValue ServerTimestamp ();
	static FieldValue IncreaseTo (int value);
	static FieldValue IncreaseTo (float value);
	static FieldValue DecreaseTo (int value);
	static FieldValue DecreaseTo (float value);
};

//************************************************************************************************
// Firebase::Firestore::ISnapshot
//************************************************************************************************

interface ISnapshot: IUnknown
{
	/** The time that snapshot was taken in the database (seconds precision) */
	virtual tresult CCL_API getDatabaseTimestamp (DateTime& date) const = 0;

	DECLARE_IID (ISnapshot)
};

DEFINE_IID (ISnapshot, 0xf296c31f, 0x2c93, 0x4c93, 0x23, 0x5a, 0xa8, 0x27, 0xf9, 0xf9, 0x2, 0xcc)

//************************************************************************************************
// Firebase::Firestore::IDocumentSnapshot
//************************************************************************************************

interface IDocumentSnapshot: ISnapshot
{
	/** Returns ID of document this snapshot contains data for. */
	virtual StringRef CCL_API getID () const = 0;

	/** Get a specific field value. */
	virtual FieldValue CCL_API get (StringID field) const = 0;
	
	/** Get all field values. */
	virtual void CCL_API getData (IAttributeList& data) const = 0;

	DECLARE_IID (IDocumentSnapshot)
};

DEFINE_IID (IDocumentSnapshot, 0xfbaea31f, 0x2c9d, 0x4fa3, 0x83, 0x5d, 0xab, 0x58, 0xf9, 0x94, 0xf, 0xec)

//************************************************************************************************
// Firebase::Firestore::IQuery
//************************************************************************************************

interface IQuery: IUnknown
{
	/** Result: IQuerySnapshot. */
	virtual IAsyncOperation* CCL_API get () = 0;
	
	DECLARE_IID (IQuery)
};

DEFINE_IID (IQuery, 0xd4f79a4a, 0xf3ec, 0x4738, 0x96, 0x83, 0x59, 0x88, 0x8b, 0x7c, 0x18, 0xa6)

//************************************************************************************************
// Firebase::Firestore::ICollectionReference
//************************************************************************************************

interface ICollectionReference: IQuery
{
	/** Get ID of referenced collection. */
	virtual StringRef CCL_API getID () const = 0;

	/** Get relative path of this collection. */
	virtual StringRef CCL_API getPath () const = 0;

	/** Add new document to collection with specified data, document ID is assigned automatically.
		Result: IDocumentReference. */
	virtual IAsyncOperation* CCL_API add (const IAttributeList& data) = 0;

	DECLARE_IID (ICollectionReference)
};

DEFINE_IID (ICollectionReference, 0x49bd788c, 0x6a4d, 0x41ae, 0x88, 0x6c, 0xe8, 0x85, 0x9d, 0x49, 0xb7, 0x5f)

//************************************************************************************************
// Firebase::Firestore::IQuerySnapshot
//************************************************************************************************

interface IQuerySnapshot: ISnapshot
{
	/** Array of IDocumentSnapshot objects. */
	virtual IArrayObject& CCL_API getDocuments () = 0;

	DECLARE_IID (IQuerySnapshot)
};

DEFINE_IID (IQuerySnapshot, 0x28703dd5, 0x2ede, 0x41d7, 0x8f, 0xdb, 0x1f, 0x53, 0xcc, 0xee, 0xfb, 0x87)

//************************************************************************************************
// Firebase::Firestore::IWriteBatch
// Inspired by: https://firebase.google.com/docs/reference/cpp/class/firebase/firestore/write-batch
//************************************************************************************************

interface IWriteBatch: IUnknown
{
	/** Plan to delete a document. */
	virtual IWriteBatch& CCL_API deleteDocument (const IDocumentReference& document) = 0;

	/** Plan to write to the a document. */
	virtual IWriteBatch& CCL_API set (const IDocumentReference& document, const IAttributeList& data, const SetOptions& setOptions) = 0;

	/** Execute all planned deletes and updates of this batch. */
	virtual IAsyncOperation* CCL_API commit () = 0;

	DECLARE_IID (IWriteBatch)
};

DEFINE_IID (IWriteBatch, 0x82a6afb1, 0xe66c, 0x45b1, 0x0d, 0xea, 0x7a, 0x6c, 0x66, 0x9a, 0x8c, 0xab)

//////////////////////////////////////////////////////////////////////////////////////////////////
// FieldValue inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline FieldValue::FieldValue () {}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline FieldValue::FieldValue (const Variant& v)
: Variant (v)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void FieldValue::operator = (const Variant& v)
{
	operator = (FieldValue (v));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Timestamp FieldValue::getTimestampValue () const
{
	if(isInt ())
		return {asInt ()};
	else if(isFloat ())
		return Timestamp ().fromFractionalSeconds (asDouble ());
	else if(isString ())
		return TimestampFormat::scan (MutableCString (asString ()));
	else
		return Timestamp ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline FieldValue& FieldValue::setTimestampValue (Timestamp t)
{
	String string (TimestampFormat::print (t));
	static_cast<Variant&> (*this) = string;
	share ();
	setUserValue (kTimestamp);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool FieldValue::isTimestamp () const
{
	return getUserValue () == kTimestamp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool FieldValue::isSentinel () const
{
	return getUserValue () >= kFirstSentinel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline FieldValue FieldValue::Increment (int byValue)
{
	FieldValue increment;
	increment = byValue;
	increment.setUserValue (kIncrement);
	return increment;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline FieldValue FieldValue::Increment (float byValue)
{
	FieldValue increment;
	increment = byValue;
	increment.setUserValue (kIncrement);
	return increment;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline FieldValue FieldValue::ArrayRemove (IAttributeQueue* elements)
{
	FieldValue arrayRemove (Variant (elements, true));
	arrayRemove.setUserValue (kRemoveAllFromArray);
	return arrayRemove;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline FieldValue FieldValue::ArrayUnion (IAttributeQueue* elements)
{
	FieldValue arrayUnion (Variant (elements, true));
	arrayUnion.setUserValue (kAppendMissingElements);
	return arrayUnion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline FieldValue FieldValue::ServerTimestamp ()
{
	FieldValue serverTimestamp (kRequestTime);
	serverTimestamp.setUserValue (kSetToServerValue);
	return serverTimestamp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline FieldValue FieldValue::IncreaseTo (int value)
{
	FieldValue increase (value);
	increase.setUserValue (kMaximum);
	return increase;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline FieldValue FieldValue::IncreaseTo (float value)
{
	FieldValue increase (value);
	increase.setUserValue (kMaximum);
	return increase;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline FieldValue FieldValue::DecreaseTo (int value)
{
	FieldValue decrease (value);
	decrease.setUserValue (kMinimum);
	return decrease;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline FieldValue FieldValue::DecreaseTo (float value)
{
	FieldValue decrease (value);
	decrease.setUserValue (kMinimum);
	return decrease;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Firestore
} // namespace Firebase
} // namespace CCL

#endif // _ccl_ifirestore_h
