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
// Filename    : ccl/platform/win/system/cclcppwinrt.h
// Description : C++/WinRT (Windows Runtime) Integration
//
//************************************************************************************************

#ifndef _cclcppwinrt_h
#define _cclcppwinrt_h

#include "ccl/base/asyncoperation.h"
#include "ccl/platform/win/cclwindows.h"

#include <functional>

#include <winrt/Windows.Foundation.h>

namespace CCL {
namespace CppWinRT {

//************************************************************************************************
// AsyncOperationWrapper
//************************************************************************************************

template<class TResult>
class AsyncOperationWrapper: public AsyncOperation,
						     public winrt::Windows::Foundation::AsyncOperationCompletedHandler<TResult>
{
public:
	AsyncOperationWrapper (const winrt::Windows::Foundation::IAsyncOperation<TResult>& operation, const std::function< Variant (const TResult&) >& resultConverter = defaultConverter)
	: winrt::Windows::Foundation::AsyncOperationCompletedHandler<TResult> (this, &AsyncOperationWrapper::onCompletion),
	  wrappedOperation (operation),
	  resultConverter (resultConverter)
	{
		wrappedOperation.Completed (*this);
	}

	// AsyncOperation
	void CCL_API cancel () override
	{
		wrappedOperation.Cancel ();

		AsyncOperation::cancel ();
	}

	void CCL_API close () override
	{
		wrappedOperation.Close ();

		AsyncOperation::close ();
	}

private:
	winrt::Windows::Foundation::IAsyncOperation<TResult> wrappedOperation;
	std::function<Variant (const TResult&)> resultConverter;

	static Variant defaultConverter (const TResult& result)
	{
		return Variant ();
	}

	void onCompletion (winrt::Windows::Foundation::IAsyncOperation<TResult> op, winrt::Windows::Foundation::AsyncStatus status)
	{
		IAsyncInfo::State state = kFailed;
		switch(status)
		{
		case winrt::Windows::Foundation::AsyncStatus::Started:
			state = kStarted;
			break;
		case winrt::Windows::Foundation::AsyncStatus::Completed:
			state = kCompleted;
			break;
		case winrt::Windows::Foundation::AsyncStatus::Canceled:
			state = kCanceled;
			break;
		}

		setResult (resultConverter (wrappedOperation.GetResults ()));
		setStateDeferred (state);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AsyncOperationWrapper inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Variant AsyncOperationWrapper<bool>::defaultConverter (const bool& result) { return Variant (result); }

inline Variant AsyncOperationWrapper<int8_t>::defaultConverter (const int8_t& result) { return Variant (result); }
inline Variant AsyncOperationWrapper<int16_t>::defaultConverter (const int16_t& result) { return Variant (result); }
inline Variant AsyncOperationWrapper<int32_t>::defaultConverter (const int32_t& result) { return Variant (result); }
inline Variant AsyncOperationWrapper<int64_t>::defaultConverter (const int64_t& result) { return Variant (result); }

inline Variant AsyncOperationWrapper<uint8_t>::defaultConverter (const uint8_t& result) { return Variant (result); }
inline Variant AsyncOperationWrapper<uint16_t>::defaultConverter (const uint16_t& result) { return Variant (result); }
inline Variant AsyncOperationWrapper<uint32_t>::defaultConverter (const uint32_t& result) { return Variant (result); }
inline Variant AsyncOperationWrapper<uint64_t>::defaultConverter (const uint64_t& result) { return Variant (result); }

inline Variant AsyncOperationWrapper<float>::defaultConverter (const float& result) { return Variant (result); }
inline Variant AsyncOperationWrapper<double>::defaultConverter (const double& result) { return Variant (result); }

inline Variant AsyncOperationWrapper<winrt::hstring>::defaultConverter (const winrt::hstring& result) { return Variant (String (result.data ())); }

} // namespace CppWinRT
} // namespace CCL

#endif // _cclcppwinrt_h
