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
// Filename    : ccl/public/system/idiagnosticstore.h
// Description : Diagnostic Store Interfaces
//
//************************************************************************************************

#ifndef _ccl_idiagnosticstore_h
#define _ccl_idiagnosticstore_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/collections/iunknownlist.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Diagnostic Categories and Context IDs
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace DiagnosticID
{
	// keys
	const CStringPtr kSaveDuration = "SaveDuration";
	const CStringPtr kSaveSize = "SaveSize";
	const CStringPtr kLoadDuration = "LoadDuration";
	const CStringPtr kExceptionEvent = "Exception";
	const CStringPtr kScanDuration = "ScanDuration";

	// context IDs
	static StringID kClassIDPrefix = CSTR ("cid/");
	static StringID kFileTypePrefix = CSTR ("filetype/");
}

//************************************************************************************************
// IDiagnosticResult
//************************************************************************************************

interface IDiagnosticResult: IUnknown
{
	virtual StringID CCL_API getContext () const = 0;
	virtual StringRef CCL_API getLabel () const = 0;

	virtual double CCL_API getMinimum () const = 0;
	virtual double CCL_API getMaximum () const = 0;
	virtual double CCL_API getAverage () const = 0;
	virtual double CCL_API getSum () const = 0;
	virtual int CCL_API getCount () const = 0;

	// short-term mode only
	virtual tbool CCL_API hasValues () const = 0;
	virtual tbool CCL_API getValue (Variant& value, int index) const = 0;
	virtual int64 CCL_API getTimestamp (int index) const = 0;
};

//************************************************************************************************
// IDiagnosticResultSet
//************************************************************************************************

interface IDiagnosticResultSet: IContainer
{
	virtual IDiagnosticResult* CCL_API at (int index) const = 0;
	virtual int CCL_API getCount () const = 0;

	virtual void CCL_API sortByMinimum () = 0;
	virtual void CCL_API sortByMaximum () = 0;
	virtual void CCL_API sortByAverage () = 0;
	virtual void CCL_API sortBySum () = 0;
	virtual void CCL_API sortByCount () = 0;
};

//************************************************************************************************
// IDiagnosticStore
//************************************************************************************************

interface IDiagnosticStore: IUnknown
{
	DEFINE_ENUM (DiagnosticMode)
	{
		kLongTerm,	//< Calculate statistics on submit. Provide long-term statistics.
		kShortTerm	//< Calculate statistics and keep submitted values. Provide statistics for recently submitted data only.
	};

	/** Variant user flags for value in submitValue. */
	enum Flags
	{
		kNoStatistics = 1 << (Variant::kLastFlag + 1),	///< no calculation of statistics (count, average, min, max, sum)
	};

	/** 
	* Set diagnostics mode. 
	* @return the old mode
	*/
	virtual DiagnosticMode CCL_API setMode (DiagnosticMode mode) = 0;

	/**
	* Submit diagnostics information
	* 
	* @param context a /-delimited path which describes the context. Each path segment denotes a context parameter.
	* @param value the actual value, e.g. a duration or a file size.
	*/
	virtual tresult CCL_API submitValue (StringID context, StringID key, VariantRef value, StringRef label = nullptr) = 0;

	/**
	* Query diagnostics statistics
	* 
	* @param context a path used to filter the result. Use * to mark "don't care" parameters.
	*/
	virtual IDiagnosticResultSet* CCL_API queryResults (StringID context, StringID key) const = 0;
	
	/**
	* Query diagnostics statistics
	* 
	* @param context a path used to filter the result. Use * to mark "don't care" parameters.
	* @return the first result item which matches the context or a nullptr
	*/
	virtual IDiagnosticResult* CCL_API queryResult (StringID context, StringID key) const = 0;
	
	/**
	* Query diagnostics statistics
	* 
	* @param context a path used to filter the result.
	* @return for each key in \a keys, this function returns the first result item which matches the context or a nullptr
	* The result set will contain an entry for each queried key (in the given order). A result entry can be a nullptr for keys that don't have a result.
	*/
	virtual IDiagnosticResultSet* CCL_API queryMultipleResults (StringID context, CString keys[], int keyCount) const = 0;

	/**
	* Clear data for a specific key or all keys of a context
	* 
	* @param context a /-delimited path which describes the context. Each path segment denotes a context parameter.
	* @param key specifices the key to be removed; An empty key will remove all data of the context.
	*/
	virtual tresult CCL_API clearData (StringID context, StringID key) = 0;

	DECLARE_IID (IDiagnosticStore)
};

DEFINE_IID (IDiagnosticStore, 0xc053ba63, 0x61fe, 0x42b3, 0xb2, 0x12, 0x73, 0x5d, 0xf, 0x8e, 0x36, 0xc7)

//************************************************************************************************
// DiagnosticStoreAccessor
/** Helper for storing & retrieving values. */
//************************************************************************************************

class DiagnosticStoreAccessor
{
public:
	DiagnosticStoreAccessor (IDiagnosticStore& store);

	/// store / retreive plain values without statistics
	void setPlainValue (StringID context, StringID key, VariantRef value);
	bool getPlainValue (Variant& value, StringID context, StringID key) const;
	Variant getPlainValue (StringID context, StringID key) const;

private:
	IDiagnosticStore& store;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline DiagnosticStoreAccessor::DiagnosticStoreAccessor (IDiagnosticStore& store)
: store (store)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void DiagnosticStoreAccessor::setPlainValue (StringID context, StringID key, VariantRef value)
{
	Variant var (value);
	var.setUserFlags (IDiagnosticStore::kNoStatistics);
	store.submitValue (context, key, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool DiagnosticStoreAccessor::getPlainValue (Variant& value, StringID context, StringID key) const
{
	AutoPtr<IDiagnosticResult> result (store.queryResult (context, key));
	return result && result->getValue (value, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Variant DiagnosticStoreAccessor::getPlainValue (StringID context, StringID key) const
{
	Variant value;
	getPlainValue (value, context, key);
	return value;
}

} // namespace CCL

#endif // _ccl_idiagnosticstore_h
