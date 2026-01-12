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
// Filename    : ccl/public/base/iunknown.h
// Description : Basic Interface
//
//************************************************************************************************

#ifndef _ccl_iunknown_h
#define _ccl_iunknown_h

#include "ccl/public/base/uiddef.h"

namespace CCL {

/** \addtogroup ccl_base 
@{ */

//************************************************************************************************
// IID macros
//************************************************************************************************

/** Declare interface ID with class name. */
#define DECLARE_IID(Interface) DECLARE_IID_

/** Declare interface ID without class name. */
#define DECLARE_IID_ static const CCL::UIDBytes __iid;

/** Define interface ID. */
#define DEFINE_IID_(Interface, data1, data2, data3, a, b, c, d, e, f, g, h) \
const CCL::UIDBytes Interface::__iid = INLINE_UID (data1, data2, data3, a, b, c, d, e, f, g, h);

/** Define class ID. */
#define DEFINE_CID_(Name, data1, data2, data3, a, b, c, d, e, f, g, h) \
extern const CCL::UIDBytes Name; \
const CCL::UIDBytes Name = INLINE_UID (data1, data2, data3, a, b, c, d, e, f, g, h);

/** Define string identifier. */
#define DEFINE_STRINGID_(Name, text) \
extern const CCL::CString Name; \
const CCL::CString Name = CSTR (text);

/** Declare string identifier in class scope. */
#define DECLARE_STRINGID_MEMBER(Name) \
static const CCL::CString Name;

/** Define string identifier in class scope. */
#define DEFINE_STRINGID_MEMBER_(Class, Name, text) \
const CCL::CString Class::Name = CSTR (text);

#ifdef INIT_IID
	/** Create symbol for interface ID. */
	#define DEFINE_IID(Interface, data1, data2, data3, a, b, c, d, e, f, g, h) \
	DEFINE_IID_ (Interface, data1, data2, data3, a, b, c, d, e, f, g, h)

	/** Create symbol for class ID. */
	#define DEFINE_CID(Name, data1, data2, data3, a, b, c, d, e, f, g, h) \
	DEFINE_CID_ (Name, data1, data2, data3, a, b, c, d, e, f, g, h)

	/** Create symbol for string identifier. */
	#define DEFINE_STRINGID(Name, text) \
	DEFINE_STRINGID_ (Name, text)

	/** Create symbol for string identifier in class scope. */
	#define DEFINE_STRINGID_MEMBER(Class, Name, text) \
	DEFINE_STRINGID_MEMBER_ (Class, Name, text)
#else
	/** Placeholder for interface ID symbol. */
	#define DEFINE_IID(Interface, data1, data2, data3, a, b, c, d, e, f, g, h)
	
	/** Placeholder for class ID symbol. */
	#define DEFINE_CID(Name, data1, data2, data3, a, b, c, d, e, f, g, h) \
	extern const CCL::UIDBytes Name;

	/** Placeholder for string identifier symbol. */
	#define DEFINE_STRINGID(Name, text) \
	extern const CCL::CString Name;

	/** Placeholder for string identifier symbol in class scope. */
	#define DEFINE_STRINGID_MEMBER(Class, Name, text)
#endif

//************************************************************************************************
// Result types
//************************************************************************************************
	
/** Result type compatible to HRESULT. */
enum class tresult: int32;

/** Result codes for tresult. */
constexpr tresult kResultOk = static_cast<tresult> (0x00000000L); ///< true (S_OK)
constexpr tresult kResultTrue = kResultOk; ///< same as kResultOk
constexpr tresult kResultFalse = static_cast<tresult> (0x00000001L); ///< false (S_FALSE)
constexpr tresult kResultNotImplemented = static_cast<tresult> (0x80004001L); ///< Not implemented (E_NOTIMPL)
constexpr tresult kResultNoInterface = static_cast<tresult> (0x80004002L); ///< Interface not supported (E_NOINTERFACE)
constexpr tresult kResultInvalidPointer = static_cast<tresult> (0x80004003L); ///< Invalid pointer (E_POINTER)
constexpr tresult kResultAborted = static_cast<tresult> (0x80004004L); ///< Operation aborted (E_ABORT)
constexpr tresult kResultAccessDenied = static_cast<tresult> (0x80070005L); 	///< General access denied error (E_ACCESSDENIED)
constexpr tresult kResultFailed = static_cast<tresult> (0x80004005L); ///< Unspeficied error (E_FAIL)
constexpr tresult kResultUnexpected = static_cast<tresult> (0x8000FFFFL); ///< Unexpected failure (E_UNEXPECTED)
constexpr tresult kResultClassNotFound = static_cast<tresult> (0x80040154L); ///< Class not found (REGDB_E_CLASSNOTREG)
constexpr tresult kResultOutOfMemory = static_cast<tresult> (0x8007000EL); ///< Out of memory (E_OUTOFMEMORY)
constexpr tresult kResultInvalidArgument = static_cast<tresult> (0x80070057L); ///< Invalid argument(s) (E_INVALIDARG)
constexpr tresult kResultWrongThread = static_cast<tresult> (0x8001010EL); ///< Interface called by wrong thread (RPC_E_WRONG_THREAD)
constexpr tresult kResultAlreadyExists = static_cast<tresult> (0x80075010L); ///< The object already exists (ERROR_OBJECT_ALREADY_EXISTS)

//************************************************************************************************
// IUnknown
/** Basic interface to manage object lifetime and to obtain other interface pointers.
	The vtable is binary equivalent to IUnknown of COM.  */
//************************************************************************************************

interface CCL_NOVTABLE IUnknown
{
	/** Obtain pointer to another interface supported by this object. 
		In case of success, the caller holds a reference to this interface, 
		which must be released afterwards. 
		
		\param iid ID of requested interface (e.g. "ccl_iid<ISomething> ()")
		\param ptr receives pointer to requested interface

		\return kResultOk for success, kResultNoInterface if unsupported. */
	virtual tresult CCL_API queryInterface (UIDRef iid, void** ptr) = 0;

	/** Increments the object's reference count. */
	virtual unsigned int CCL_API retain  () = 0;

	/** Decrement the object's reference count. 
		If the reference count reaches zero, the object is freed in memory. */
	virtual unsigned int CCL_API release () = 0;

	DECLARE_IID (IUnknown)
};

//************************************************************************************************
// IClassAllocator
/** Basic interface to create new class instances with known identifier.  */
//************************************************************************************************

interface CCL_NOVTABLE IClassAllocator: IUnknown
{
	/** Create new class instance. */
	virtual tresult CCL_API createInstance (UIDRef cid, UIDRef iid, void** obj) = 0;

	DECLARE_IID (IClassAllocator)
};

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Returns id of specified interface. */
template <class Interface> 
UIDRef ccl_iid ()
{
	return Interface::__iid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** @} */

} // namespace CCL

#include "ccl/public/base/smartptr.h" // include smart pointer classes

#endif // _ccl_iunknown_h
