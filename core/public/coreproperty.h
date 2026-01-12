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
// Filename    : core/public/coreproperty.h
// Description : Core Property Handler
//
//************************************************************************************************

#ifndef _coreproperty_h
#define _coreproperty_h

#include "core/public/coretypes.h"

namespace Core {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Four-character types
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Four-character identifier. */
typedef int32 FourCharID;

/** Property type. */
typedef FourCharID PropertyType;

/** Interface identifier. */
typedef FourCharID InterfaceID;

/** Define four-character identifier. Some compilers don't support multi-character constants. */
#define FOUR_CHAR_ID(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))

//////////////////////////////////////////////////////////////////////////////////////////////////
/** General Error Codes */
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Errors 
{
	enum ErrorCodes
	{
		kError_NoError = 0,			///< no error		
		kError_ItemNotFound	= 100,	///< item could not be found
		kError_InvalidArgument,		///< invalid argument passed to function
		kError_InvalidThread,		///< function cannot be called on the current thread
		kError_OutOfMemory,			///< out of memory
		kError_InvalidState,        ///< object in wrong state
		kError_NotReady,            ///< function called too early
		kError_NotImplemented,      ///< function not implemented
		kError_Failed				///< function failed (unspecified cause)
	
		// ATTENTION: Please do not place context specific error codes here!
	};
}

/** Error code. */
typedef int ErrorCode;

//************************************************************************************************
// Property
/** Basic property definition. */
//************************************************************************************************

struct Property
{
	PropertyType type;	///< property type
	int size;			///< size in bytes
		
	Property (PropertyType type, int size)
	: type (type),
	  size (size)
	{}
};

//************************************************************************************************
// InterfaceProperty
/** Property to get additional interface via IPropertyHandler. */
//************************************************************************************************

struct InterfaceProperty: Property
{
	static const PropertyType kID = FOUR_CHAR_ID ('I', 'F', 'a', 'c');
	
	InterfaceID iid;
	void** ptr;
	
	InterfaceProperty (InterfaceID iid, void** ptr)
	: Property (kID, sizeof(InterfaceProperty)),
	  iid (iid),
	  ptr (ptr)
	{}
};

//************************************************************************************************
// IPropertyHandler
/** Basic interface to get/set properties. */
//************************************************************************************************

struct IPropertyHandler
{
	/** Set property value. */
	virtual void setProperty (const Property& value) = 0;

	/** Get property value. Type and size need to be initialized by caller. */
	virtual void getProperty (Property& value) = 0;

	/** Release this instance. */
	virtual void release () = 0;
	
	static const InterfaceID kIID = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Helper to get additional interface. */
template <class Interface>
Interface* GetInterface (IPropertyHandler* handler, InterfaceID iid)
{
	void* ptr = nullptr;
	InterfaceProperty p (iid, &ptr);
	if(handler) handler->getProperty (p);
	return reinterpret_cast<Interface*> (ptr);
}

template <class Interface>
Interface* GetInterface (IPropertyHandler* handler)
{
	return GetInterface<Interface> (handler, Interface::kIID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Helper to implement query for interface.  */
template <class Class, class Interface>
bool ImplementGetInterface (Class* This, Property& value, InterfaceID iid)
{
	if(value.type == InterfaceProperty::kID)
	{
		InterfaceProperty& interfaceProp = reinterpret_cast<InterfaceProperty&> (value);
		if(interfaceProp.iid == iid)
		{
			*interfaceProp.ptr = static_cast<Interface*> (This);
			return true;
		}
	}
	return false;
}

template <class Class, class Interface>
bool ImplementGetInterface (Class* This, Property& value)
{
	return ImplementGetInterface<Class, Interface> (This, value, Interface::kIID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Core

#endif // _coreproperty_h
