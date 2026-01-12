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
// Filename    : core/public/coreclassmacros.h
// Description : Class Macros
//
//************************************************************************************************

#ifndef _coreclassmacros_h
#define _coreclassmacros_h

//************************************************************************************************
// Class Setter/Getter Method Macros
//************************************************************************************************

/** Setter/getter for already defined member (by value). */
#define PROPERTY_BY_VALUE(type, member, Method) \
void set##Method (type __value) { member = __value; } \
type get##Method () const { return member; }

/** Setter/getter for already defined member (by reference). */
#define PROPERTY_BY_REFERENCE(type, member, Method) \
void set##Method (const type& __value) { member = __value; } \
const type& get##Method () const { return member; }

/** Define object member (by reference). */
#define PROPERTY_OBJECT(Class, member, Method) \
protected: Class member; public: \
PROPERTY_BY_REFERENCE (Class, member, Method)

/** Define simple member variable (by value). */
#define PROPERTY_VARIABLE(type, member, Method) \
protected: type member; public: \
PROPERTY_BY_VALUE (type, member, Method)

/** Define pointer member (by address). */
#define PROPERTY_POINTER(type, member, Method) \
PROPERTY_VARIABLE (type*, member, Method)

/** Define read-only flag member. */
#define PROPERTY_READONLY_FLAG(var, value, method) \
bool method () const { return (var & (value)) != 0; }

/** Define flag member. */
#define PROPERTY_FLAG(var, value, method) \
void method (bool __state) { if(__state) var |= (value); else var &= ~(value); } \
PROPERTY_READONLY_FLAG (var, value, method)

/** Define indexed flag member (32 bit integer). */
#define PROPERTY_INDEX_FLAG32(var,method) \
bool method (int __index) const { return (var & (1<<__index)) != 0; } \
void method (int __index, bool __state) { if(__state) var |= (1<<__index); else var &= ~(1<<__index); }

/** Define indexed flag member (64 bit integer). */
#define PROPERTY_INDEX_FLAG64(var,method) \
bool method (int __index) const { return (var & (1LL<<__index)) != 0; } \
void method (int __index, bool __state) { if(__state) var |= (1LL<<__index); else var &= ~(1LL<<__index); }

/** Define boolean member. */
#define PROPERTY_BOOL(member, Method) \
protected: bool member; public: \
bool is##Method () const { return member; } \
void set##Method (bool __state = true) { member = __state; }

/** Define C-string buffer member (by reference). */
#define PROPERTY_CSTRING_BUFFER(size, member, Method) \
protected: Core::CStringBuffer<size> member; public: \
void set##Method (const char* _cstr) { member = _cstr; } \
const Core::CStringBuffer<size>& get##Method () const { return member; }

#endif // _coreclassmacros_h
