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
// Filename    : ccl/platform/win/system/cclwinrt.h
// Description : Windows Runtime (WinRT) Integration
//
//************************************************************************************************

#ifndef _cclwinrt_h
#define _cclwinrt_h

#include "ccl/platform/win/interfaces/iwinrtplatform.h"

#include <windows.foundation.h>

#include "ccl/public/base/variant.h"
#include "ccl/public/base/unknown.h"
#include "ccl/public/base/iasyncoperation.h"

namespace CCL {
namespace WinRT {

// include shared implementation
#include "ccl/platform/win/system/cclcom.impl.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Create WinRT object using the same syntax as ccl_new<>().  */
template <class T> inline T* winrt_new (UStringPtr activatableClassId)
{
	T* obj = 0;
	System::GetWinRTPlatform ().getActivationFactory (activatableClassId, __uuidof(T), (void**)&obj);
	return obj;
}

//************************************************************************************************
// PlatformString
//************************************************************************************************

struct PlatformString
{
	HSTRING hString;

	PlatformString ()
	: hString (nullptr)
	{}

	PlatformString (UStringPtr string)
	: hString (System::GetWinRTPlatform ().createString (string))
	{}

	PlatformString (StringRef string)
	: hString (System::GetWinRTPlatform ().createString (StringChars (string)))
	{}

	~PlatformString ()
	{
		release ();
	}

	void release ()
	{
		System::GetWinRTPlatform ().deleteString (hString);
		hString = nullptr;
	}

	String asString () const
	{
		uint32 length = 0;
		UStringPtr buffer = System::GetWinRTPlatform ().getStringBuffer (hString, length);
		return String ().append (buffer, length);
	}

	operator HSTRING () const	{ return hString; }
	operator HSTRING* ()		{ ASSERT (hString == nullptr) return &hString; }
};

//************************************************************************************************
// DumpRuntimeObject
//************************************************************************************************

#if DEBUG
inline void DumpRuntimeObject (IInspectable* object)
{
	if(object == nullptr)
	{
		Debugger::println ("null");
		return;
	}

	PlatformString className;
	object->GetRuntimeClassName (className);
	Debugger::println (className.asString ());

	ULONG count = 0;
	IID* iids = nullptr;
	if(SUCCEEDED (object->GetIids (&count, &iids)))
	{
		for(ULONG i = 0; i < count; i++)
		{
			String iidString;
			CCL::UID (com_uid_cast (iids[i])).toString (iidString);
			Debugger::println (iidString);
		}
		::CoTaskMemFree (iids);
	}
}
#endif

//************************************************************************************************
// TypedEventHandler
//************************************************************************************************

template <typename TBase, typename TSender, typename TArg, typename TReceiver>
class TypedEventHandler: public Unknown,
						 public TBase
{
public:
	typedef HRESULT (TReceiver::*ReceiverMethod) (TSender sender, TArg arg);

	TypedEventHandler (TReceiver* receiver, ReceiverMethod method)
	: receiver (receiver),
	  method (method)
	{}

	#if (1 && DEBUG)
	~TypedEventHandler ()
	{
		int breakPointHere = 0;
	}
	#endif

	static AutoPtr<TypedEventHandler> make (TReceiver* receiver, ReceiverMethod method)
	{
		return AutoPtr<TypedEventHandler> (NEW TypedEventHandler (receiver, method));
	}

	DELEGATE_COM_IUNKNOWN
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override
	{
		QUERY_COM_INTERFACE (TBase)
		return Unknown::queryInterface (iid, ptr);
	}

	STDMETHODIMP Invoke (TSender sender, TArg arg) override
	{
		return (receiver->*method) (sender, arg);
	}

protected:
	TReceiver* receiver;
	ReceiverMethod method;
};

//************************************************************************************************
// AsyncOperationWrapper
//************************************************************************************************

template<typename TOperation, typename TCompletionHandler, typename TResult>
class AsyncOperationWrapper: public Unknown,
							 public CCL::IAsyncOperation
{
public:
	AsyncOperationWrapper (TOperation* op = nullptr)
	: op (op)
	{}

	#if (1 && DEBUG)
	~AsyncOperationWrapper ()
	{
		int breakPointHere = 0;
	}
	#endif

	operator TOperation** () { ASSERT (op == 0) return op; }

	class CompletionHandler: public Unknown,
							 public TCompletionHandler
	{
	public:
		CompletionHandler (IAsyncOperation& outerOperation, IAsyncCompletionHandler* outerHandler)
		: outerOperation (outerOperation),
		  outerHandler (outerHandler)
		{}

		#if (1 && DEBUG)
		~CompletionHandler ()
		{
			int breakPointHere = 0;
		}
		#endif

		DELEGATE_COM_IUNKNOWN
		tresult CCL_API queryInterface (UIDRef iid, void** ptr) override
		{
			QUERY_COM_INTERFACE (TCompletionHandler)
			return Unknown::queryInterface (iid, ptr);
		}

		HRESULT STDMETHODCALLTYPE Invoke (TOperation* op, ABI::Windows::Foundation::AsyncStatus status) override
		{
			outerHandler->onCompletion (outerOperation);
			return S_OK;
		}

	protected:
		IAsyncOperation& outerOperation;
		SharedPtr<IAsyncCompletionHandler> outerHandler;
	};

	typedef ABI::Windows::Foundation::IAsyncInfo WinRT_IAsyncInfo;
	typedef ABI::Windows::Foundation::AsyncStatus WinRT_Status;

	static inline State fromWinRTStatus (WinRT_Status status)
	{
		switch(status)
		{
		case ABI::Windows::Foundation::AsyncStatus::Started : return kStarted;
		case ABI::Windows::Foundation::AsyncStatus::Completed : return kCompleted;
		case ABI::Windows::Foundation::AsyncStatus::Canceled : return kCanceled;
		default : return kFailed; // ABI::Windows::Foundation::AsyncStatus::Error
		}
	}

	// IAsyncOperation
	State CCL_API getState () const override
	{
		ABI::Windows::Foundation::AsyncStatus status = ABI::Windows::Foundation::AsyncStatus::Error;
		ComPtr<WinRT_IAsyncInfo> asyncInfo;
		op.as (asyncInfo);
		if(asyncInfo)
			asyncInfo->get_Status (&status);
		return fromWinRTStatus (status);
	}

	Variant CCL_API getResult () const override
	{
		TResult* result = 0;
		op->GetResults (&result);
		Variant v;
		v.takeShared (reinterpret_cast<CCL::IUnknown*> (static_cast<::IUnknown*> (result)));
		return v;
	}

	void CCL_API setResult (VariantRef value) override
	{
		CCL_NOT_IMPL ("Can't set the result generically.")
	}

	void CCL_API cancel () override
	{
		ComPtr<WinRT_IAsyncInfo> asyncInfo;
		op.as (asyncInfo);
		if(asyncInfo)
			asyncInfo->Cancel ();
	}

	void CCL_API close () override
	{
		ComPtr<WinRT_IAsyncInfo> asyncInfo;
		op.as (asyncInfo);
		if(asyncInfo)
			asyncInfo->Close ();
	}

	void CCL_API setCompletionHandler (IAsyncCompletionHandler* handler) override
	{
		ComPtr<TCompletionHandler> innerHandler = NEW CompletionHandler (*this, handler);
		HRESULT hr = op->put_Completed (innerHandler);
		ASSERT (SUCCEEDED (hr))
	}

	void CCL_API setProgressHandler (IProgressNotify* handler) override
	{
		CCL_NOT_IMPL ("Can't set progress handler!")
	}

	IProgressNotify* CCL_API getProgressHandler () const override
	{
		return 0;
	}

	CLASS_INTERFACE2 (IAsyncInfo, IAsyncOperation, Unknown)

protected:
	mutable ComPtr<TOperation> op;
};

//************************************************************************************************
// IterationHelper
//************************************************************************************************

template <typename TIterable, typename TIterator, typename TType>
struct IterationHelper
{
	ComPtr<TIterable> iterable;
	ComPtr<TIterator> iterator;
	ComPtr<TType> current;
	boolean hasCurrent;

	IterationHelper (IInspectable* container)
	: hasCurrent (false)
	{
		iterable.fromUnknown (container);
		first ();
	}

	void first ()
	{
		iterator.release ();
		current.release ();
		hasCurrent = false;
		if(iterable)
			iterable->First (iterator);
		if(iterator)
			iterator->get_HasCurrent (&hasCurrent);
	}

	TType* next ()
	{
		current.release ();
		iterator->get_Current (current);
		iterator->MoveNext (&hasCurrent);
		return current;
	}

	bool done () const
	{
		return !hasCurrent;
	}
};

typedef IterationHelper<__FIIterable_1___FIKeyValuePair_2_HSTRING_IInspectable,
						__FIIterator_1___FIKeyValuePair_2_HSTRING_IInspectable,
						__FIKeyValuePair_2_HSTRING_IInspectable> KeyValuePair_HSTRING_IInspectable_Iterable;

#define IterableForEach(TIterationHelper, container, var) \
{ TIterationHelper __iter (container); \
  while(!__iter.done ()) \
  { auto var = __iter.next ();

//************************************************************************************************
// PropertyVariant
//************************************************************************************************

struct PropertyVariant: Variant
{
	PropertyVariant (ABI::Windows::Foundation::IPropertyValue* value)
	{
		assignValue (value);
	}

	PropertyVariant (IInspectable* value)
	{
		assignValue (value);
	}

	PropertyVariant& assignValue (ABI::Windows::Foundation::IPropertyValue* value)
	{
		ABI::Windows::Foundation::PropertyType type = ABI::Windows::Foundation::PropertyType_Empty;
		if(value)
			value->get_Type (&type);
		Variant* This = static_cast<Variant*> (this);
		switch(type)
		{
		case ABI::Windows::Foundation::PropertyType_String :
			{
				PlatformString string;
				value->GetString (string);
				String cclString (string.asString ());
				*This = cclString;
				share ();
			}
			break;
		case ABI::Windows::Foundation::PropertyType_Boolean :
			{
				boolean b = false;
				value->GetBoolean (&b);
				*This = b;
			}
			break;
		default :
			SOFT_ASSERT (type == ABI::Windows::Foundation::PropertyType_Empty, "Type not converted!\n")
			clear ();
			// to be continued...
			break;
		}
		return *this;
	}

	PropertyVariant& assignValue (IInspectable* value)
	{
		ComPtr<ABI::Windows::Foundation::IPropertyValue> propertyValue;
		propertyValue.fromUnknown (value);
		return assignValue (propertyValue);
	}
};

} // namespace WinRT
} // namespace CCL

#endif // _cclwinrt_h
