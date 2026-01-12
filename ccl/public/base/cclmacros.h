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
// Filename    : ccl/public/base/cclmacros.h
// Description : Macros
//
//************************************************************************************************

#ifndef _ccl_cclmacros_h
#define _ccl_cclmacros_h

#include "core/public/coremacros.h"

//************************************************************************************************
// Interface macros
//************************************************************************************************

/** Declare IUnknown methods in derived class. */
#define DECLARE_UNKNOWN \
CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override; \
unsigned int CCL_API retain () override; \
unsigned int CCL_API release () override;

/** Query for specified interface, used to implement IUnknown::queryInterface. */
#define QUERY_INTERFACE(Interface) \
if(CCL::ccl_iid<Interface> ().equals (iid)) \
{ *ptr = static_cast<Interface*>(this); retain (); return CCL::kResultOk; }

/** Query for IUnknown unambiguously, used to implement IUnknown::queryInterface. */
#define QUERY_UNKNOWN(Interface) \
if(CCL::ccl_iid<IUnknown> ().equals (iid)) \
{ *ptr = static_cast<Interface*>(this); retain (); return CCL::kResultOk; }

/** Delegate reference counting to super class. */
#define PARENT_REFCOUNT(Parent) \
unsigned int CCL_API retain () override { return Parent::retain (); } \
unsigned int CCL_API release () override { return Parent::release (); }

/** Delegate reference counting to 'Unknown' class. */
#define UNKNOWN_REFCOUNT PARENT_REFCOUNT (CCL::Unknown)

/** Delegate IUnknown to super class. */
#define DELEGATE_UNKNOWN(Parent) \
CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override \
{ return Parent::queryInterface (iid, ptr); } \
PARENT_REFCOUNT (Parent)

/** Implement IUnknown (+ 1 interface). */
#define CLASS_INTERFACE(Interface, Parent) \
UNKNOWN_REFCOUNT \
CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override \
{ QUERY_INTERFACE (Interface) return Parent::queryInterface (iid, ptr); }

/** Implement IUnknown (+ 2 interfaces). */
#define CLASS_INTERFACE2(Interface1, Interface2, Parent) \
UNKNOWN_REFCOUNT \
CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override \
{ QUERY_INTERFACE (Interface1) \
  QUERY_INTERFACE (Interface2) \
  return Parent::queryInterface (iid, ptr); }

/** Implement IUnknown (+ 3 interfaces). */
#define CLASS_INTERFACE3(Interface1, Interface2, Interface3, Parent) \
UNKNOWN_REFCOUNT \
CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override \
{ QUERY_INTERFACE (Interface1) \
  QUERY_INTERFACE (Interface2) \
  QUERY_INTERFACE (Interface3) \
  return Parent::queryInterface (iid, ptr); }

/** Implement IUnknown (multiple interfaces). */
#define CLASS_INTERFACES(Parent) \
UNKNOWN_REFCOUNT \
CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override;

/** Implement IUnknown (multiple interfaces in header). 
    BEGIN_CLASS_INTERFACES
		QUERY_INTERFACE (Interface1)
		QUERY_INTERFACE (Interface2)
		...
	END_CLASS_INTERFACES (ParentClass)
*/
#define BEGIN_CLASS_INTERFACES \
UNKNOWN_REFCOUNT \
CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override {

#define END_CLASS_INTERFACES(Parent) \
	return Parent::queryInterface (iid, ptr); }

//************************************************************************************************
// Interface macros (without reference counting)
//************************************************************************************************

/** Implement IUnknown without reference count. */
#define IMPLEMENT_UNKNOWN_NO_REFCOUNT \
unsigned int CCL_API retain () override  { return 1; } \
unsigned int CCL_API release () override { return 1; }

/** Implement IUnknown (+ 1 interface) without reference counting. */
#define IMPLEMENT_DUMMY_UNKNOWN(Interface) \
CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override \
{ QUERY_UNKNOWN (Interface) QUERY_INTERFACE (Interface) return (CCL::tresult)CCL::kResultNoInterface; } \
IMPLEMENT_UNKNOWN_NO_REFCOUNT

/** Implement IUnknown (+ 2 interfaces) without reference counting. */
#define IMPLEMENT_DUMMY_UNKNOWN2(Interface1, Interface2) \
CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override \
{ QUERY_UNKNOWN (Interface1) QUERY_INTERFACE (Interface1) \
  QUERY_INTERFACE (Interface2) return CCL::kResultNoInterface; } \
IMPLEMENT_UNKNOWN_NO_REFCOUNT

/** Implement IUnknown (+ 3 interfaces) without reference counting. */
#define IMPLEMENT_DUMMY_UNKNOWN3(Interface1, Interface2, Interface3) \
CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override \
{ QUERY_UNKNOWN (Interface1) QUERY_INTERFACE (Interface1) \
  QUERY_INTERFACE (Interface2) QUERY_INTERFACE (Interface3) return CCL::kResultNoInterface; } \
IMPLEMENT_UNKNOWN_NO_REFCOUNT

/** Implement IUnknown (+ 4 interfaces) without reference counting. */
#define IMPLEMENT_DUMMY_UNKNOWN4(Interface1, Interface2, Interface3, Interface4) \
CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override \
{ QUERY_UNKNOWN (Interface1) QUERY_INTERFACE (Interface1) \
  QUERY_INTERFACE (Interface2) QUERY_INTERFACE (Interface3) QUERY_INTERFACE (Interface4) return CCL::kResultNoInterface; } \
IMPLEMENT_UNKNOWN_NO_REFCOUNT

//************************************************************************************************
// Property macros (setter/getter methods)
//************************************************************************************************

/** Setter/getter for already defined string member (by reference). */
#define PROPERTY_STRING_METHODS(member, Method) \
void set##Method (CCL::StringRef _str) { member = _str; } \
CCL::StringRef get##Method () const { return member; }

/** Define string member (by reference). */
#define PROPERTY_STRING(member, Method) \
protected: CCL::String member; public: \
PROPERTY_STRING_METHODS (member, Method)

/** Define mutable C-string member (by reference). */
#define PROPERTY_MUTABLE_CSTRING(member, Method) \
protected: CCL::MutableCString member; public: \
void set##Method (const char* _cstr) { member = _cstr; } \
CCL::CStringRef get##Method () const { return member; }

/** Define smart auto pointer member (by address). Takes ownership in ctor and set method. */
#define PROPERTY_AUTO_POINTER(type, member, Method) \
protected: CCL::AutoPtr<type> member; public: \
void set##Method (type* _##member) { member = _##member; } \
type* get##Method () const { return member; }

/** Setter/getter for already defined plain pointer member (by address). */
#define PROPERTY_SHARED_METHODS(type, member, Method) \
void set##Method (type* _arg) { CCL::take_shared<type> (member, _arg); } \
type* get##Method () const { return member; }

/** Define plain shared pointer member (by address). */
#define PROPERTY_SHARED(type, member, Method) \
protected: type* member; public: \
PROPERTY_SHARED_METHODS (type, member, Method)

/** Define smart auto pointer member (by address). Takes ownership in ctor and shares in set method. */
#define PROPERTY_SHARED_AUTO(type, member, Method) \
protected: CCL::AutoPtr<type> member; public: \
void set##Method (type* _arg) { member.share (_arg); } \
type* get##Method () const { return member; }

/** Define smart shared pointer member (by address). Shares ownership in ctor and set method. */
#define PROPERTY_SHARED_POINTER(type, member, Method) \
protected: CCL::SharedPtr<type> member; public: \
void set##Method (type* _arg) { member = _arg; } \
type* get##Method () const { return member; }

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif // _ccl_cclmacros_h
