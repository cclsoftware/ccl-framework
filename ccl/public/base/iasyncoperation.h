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
// Filename    : ccl/public/base/iasyncoperation.h
// Description : Asynchronous operation interfaces
//
//************************************************************************************************

#ifndef _ccl_iasyncoperation_h
#define _ccl_iasyncoperation_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IAsyncOperation;
interface IProgressNotify;

//************************************************************************************************
// IAsyncCompletionHandler
/**
	\ingroup ccl_base */
//************************************************************************************************

interface IAsyncCompletionHandler: IUnknown
{
	/** Called when an async operation has been completed, canceled or failed. */
	virtual void CCL_API onCompletion (IAsyncOperation& operation) = 0;

	DECLARE_IID (IAsyncCompletionHandler)
};

//************************************************************************************************
// IAsyncInfo
/**
	\ingroup ccl_base */
//************************************************************************************************

interface IAsyncInfo: IUnknown
{
	DEFINE_ENUM (State)
	{
		kNone,
		kStarted,
		kCompleted,
		kFailed,
		kCanceled
	};

	virtual State CCL_API getState () const = 0;

	DECLARE_IID (IAsyncInfo)
};

//************************************************************************************************
// IAsyncOperation
/**
	\ingroup ccl_base */
//************************************************************************************************

interface IAsyncOperation: IAsyncInfo
{
	virtual Variant CCL_API getResult () const = 0;

	virtual void CCL_API setResult (VariantRef value) = 0;

	virtual void CCL_API cancel () = 0;

	/** Call after result has been consumed. */
	virtual void CCL_API close () = 0;

	virtual void CCL_API setCompletionHandler (IAsyncCompletionHandler* handler) = 0;

	virtual void CCL_API setProgressHandler (IProgressNotify* handler) = 0;

	virtual IProgressNotify* CCL_API getProgressHandler () const = 0;

	DECLARE_IID (IAsyncOperation)
};

//************************************************************************************************
// ITypedAsyncOperation
/** Asynchronous operation interface with result type.
	\ingroup ccl_base */
//************************************************************************************************

template <typename ResultType>
interface ITypedAsyncOperation: IAsyncOperation
{
	static inline ITypedAsyncOperation* cast (IAsyncOperation* op) { return static_cast<ITypedAsyncOperation*> (op); }
};

//************************************************************************************************
// IAsyncCall
//************************************************************************************************

interface IAsyncCall: IUnknown
{
	virtual IAsyncOperation* CCL_API call () = 0;

	DECLARE_IID (IAsyncCall)
};

} // namespace CCL

#endif // _ccl_iasyncoperation_h
