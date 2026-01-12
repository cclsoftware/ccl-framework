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
// Filename    : ccl/public/base/variant.h
// Description : Variant type
//
//************************************************************************************************

#ifndef _ccl_variant_h
#define _ccl_variant_h

#include "ccl/public/text/cclstring.h"

namespace CCL {

class MutableCString;

//************************************************************************************************
// Variant
/** \ingroup ccl_base */
//************************************************************************************************

struct Variant
{
	enum Types
	{
		kInt = 1,
		kFloat,
		kString,
		kObject,

		kTypeMask = 0x000F,
		kFlagMask = 0x00F0,
		kUserMask = 0xFF00,

		kShared = 1<<4,
		kBoolFormat = 1<<5,
		kLastFlag = 7
	};
	
	short type;
	union
	{
		int64 lValue;
		double fValue;
		IString* string;
		IUnknown* object;
	};

	// Variant ctor
	Variant (): type (0), lValue (0) {}
	Variant (tresult v): type (kInt), lValue (static_cast<int32> (v)) {}
	Variant (int64 v): type (kInt), lValue (v) {}
	Variant (uint64 v): type (kInt), lValue (v) {}
	Variant (int v): type (kInt), lValue (v) {}
	Variant (uint32 v): type (kInt), lValue (v) {}
	Variant (int16 v): type (kInt), lValue (v) {}
	Variant (uint16 v): type (kInt), lValue (v) {}
	Variant (int8 v): type (kInt), lValue (v) {}
	Variant (uint8 v): type (kInt), lValue (v) {}
	Variant (bool b, int flags = 0): type (kInt|(flags & kBoolFormat)), lValue (b ? 1 : 0) {}
	Variant (double v): type (kFloat), fValue (v) {}
	Variant (float v): type (kFloat), fValue (v) {}
	Variant (const char* s): type (kString|kShared) { String str (s); (string = str.theString)->retain (); }
	Variant (StringRef s, bool shared = false): type (kString), string (s.theString) { if(shared) share (); }
	Variant (const IUnknown* o, bool shared = false): type (kObject), object (const_cast<IUnknown*> (o)) { if(shared) share (); }
	Variant (const Variant& v): type (v.getCopyType ()), lValue (v.lValue) { if(v.isShared ()) share (); }
	template<class T> Variant (const UnknownPtr<T>& p): type (kObject), object (p.as_plain ()) { share (); }
	template<class T> Variant (const AutoPtr<T>& p): type (kObject), object (p.as_plain ()) { share (); }
	template<class T> Variant (const SharedPtr<T>& p): type (kObject), object (p.as_plain ()) { share (); }

	~Variant () { clear (); }

	short getType () const;
	bool isValid () const;
	bool isInt () const;
	bool isFloat () const;
	bool isNumeric () const;
	bool isString () const;
	bool isObject () const;
	bool isNil () const;
	bool isShared () const;
	bool isBoolFormat () const;
	void setBoolFormat (bool state = true);

	short getUserFlags () const;
	void setUserFlags (short flags);
	short getUserValue () const;
	void setUserValue (short value);

	void clear ();
	void share ();

	// String conversion
	bool toString (String& string) const;
	String toString () const;
	Variant& fromString (StringRef string);
	bool toCString (MutableCString& string, TextEncoding encoding = Text::kASCII) const;
	int64 parseLargeInt () const;
	int64 parseLargeInt (int64 errorValue) const;
	int parseInt () const;
	int parseInt (int errorValue ) const;
	double parseDouble () const;
	double parseDouble (double errorValue) const;
	float parseFloat () const;
	float parseFloat (float errorValue) const;
	bool parseBool () const;
	
	// Assignment
	Variant& operator = (const Variant& v);
	Variant& operator = (tresult);
	Variant& operator = (int64);
	Variant& operator = (uint64);
	Variant& operator = (int);
	Variant& operator = (uint32);
	Variant& operator = (int16);
	Variant& operator = (uint16);
	Variant& operator = (int8);
	Variant& operator = (uint8);
	Variant& operator = (bool);
	Variant& operator = (double);
	Variant& operator = (float);
	Variant& operator = (StringRef);
	Variant& operator = (const char*);
	Variant& operator = (IUnknown*);
	template<class T> Variant& operator = (const UnknownPtr<T>&);
	template<class T> Variant& operator = (const AutoPtr<T>&);
	template<class T> Variant& operator = (const SharedPtr<T>&);
	
	Variant& takeShared (IUnknown* o);

	// Comparison
	bool operator == (const Variant&) const;
	bool operator != (const Variant&) const;
	bool operator > (const Variant&) const;
	bool operator < (const Variant&) const;
	int compare (const Variant&) const;

	// Cast operators
	operator String () const;
	operator IUnknown* () const;
	operator int64 () const;
	operator uint64 () const;
	operator int () const;
	operator uint32 () const;
	operator int16 () const;
	operator uint16 () const;
	operator int8 () const;
	operator uint8 () const;
	operator double () const;
	operator float () const;
	operator bool () const;

	// Cast methods
	String asString () const;
	IUnknown* asUnknown () const;
	int64 asLargeInt () const;
	uint64 asLargeUInt () const;
	int asInt () const;
	uint32 asUInt () const;
	double asDouble () const;
	float asFloat () const;
	bool asBool () const;
	tresult asResult () const;

	UIntPtr asIntPointer () const;
	Variant& setIntPointer (UIntPtr);

private:
	short getCopyType () const { return (type &~kShared); }
};

//************************************************************************************************
// VariantString
//************************************************************************************************

class VariantString: public String
{
public:
	VariantString (VariantRef var) { var.toString (*this); }
};

//************************************************************************************************
// IVariant
//************************************************************************************************

interface IVariant: IUnknown
{	
	virtual void CCL_API assign (VariantRef variant) = 0;

	virtual void CCL_API copyTo (Variant& variant) const = 0;

	DECLARE_IID (IVariant)
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Variant inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Variant& Variant::operator = (const Variant& v)
{
	clear ();
	type = v.getCopyType ();
	lValue = v.lValue;
	if(v.isShared ())
		share ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Variant& Variant::operator = (int64 l)
{
	clear ();
	type = kInt;
	lValue = l;
	return *this; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Variant& Variant::operator = (double f)
{
	clear ();
	type = kFloat;
	fValue = f;
	return *this; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Variant& Variant::operator = (StringRef s) 
{ 
	clear ();
	type = kString;
	string = s.theString; 
	return *this; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Variant& Variant::operator = (const char* s) 
{ 
	clear ();
	type = kString|kShared;
	String str (s);
	string = str.theString;
	string->retain ();
	return *this; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Variant& Variant::operator = (IUnknown* obj)
{
	clear ();
	type = kObject;
	object = obj;
	return *this; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Variant& Variant::takeShared (IUnknown* obj)
{
	clear ();
	type = kObject;
	object = obj;
	share ();
	return *this; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T> inline Variant& Variant::operator = (const UnknownPtr<T>& ptr) { return takeShared (ptr); }
template<class T> inline Variant& Variant::operator = (const AutoPtr<T>& ptr)    { return takeShared (ptr); }
template<class T> inline Variant& Variant::operator = (const SharedPtr<T>& ptr)  { return takeShared (ptr); }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline short Variant::getType () const			{ return (type & kTypeMask); }
inline bool Variant::isValid () const			{ return getType () != 0; }
inline bool Variant::isInt () const				{ return getType () == kInt; }
inline bool Variant::isFloat () const			{ return getType () == kFloat; }
inline bool Variant::isNumeric () const			{ return isInt () || isFloat (); }
inline bool Variant::isString () const			{ return getType () == kString; }
inline bool Variant::isObject () const			{ return getType () == kObject; }
inline bool Variant::isNil () const				{ return object == nullptr; }
inline bool Variant::isShared () const			{ return (type & kShared) != 0; }
inline bool Variant::isBoolFormat () const		{ return (type & kBoolFormat) != 0; }
inline void Variant::setBoolFormat (bool state) { if(state) type |= kBoolFormat; else type &= ~kBoolFormat; }
inline short Variant::getUserFlags () const		{ return (type & kUserMask); }
inline void Variant::setUserFlags (short flags)	{ ASSERT ((flags & kUserMask) == flags) type = int16 ((type &~kUserMask)|(flags & kUserMask)); }
inline short Variant::getUserValue () const		{ return getUserFlags () >> (kLastFlag+1); }
inline void Variant::setUserValue (short value) { setUserFlags ((short)(value << (kLastFlag+1))); } 
inline Variant& Variant::operator = (tresult t) { return *this = static_cast<int64> (t); }
inline Variant& Variant::operator = (uint64 l)  { return *this = static_cast<int64> (l); }
inline Variant& Variant::operator = (int l)		{ return *this = static_cast<int64> (l); }
inline Variant& Variant::operator = (uint32 l)  { return *this = static_cast<int64> (l); }
inline Variant& Variant::operator = (int16 l)	{ return *this = static_cast<int64> (l); }
inline Variant& Variant::operator = (uint16 l)	{ return *this = static_cast<int64> (l); }
inline Variant& Variant::operator = (int8 l)	{ return *this = static_cast<int64> (l); }
inline Variant& Variant::operator = (uint8 l)	{ return *this = static_cast<int64> (l); }
inline Variant& Variant::operator = (float f)	{ return *this = static_cast<double> (f); }
inline Variant& Variant::operator = (bool b)	{ return *this = b ? 1 : 0; }
inline Variant::operator String () const		{ return getType () == kString ? String (string) : String (); }
inline Variant::operator IUnknown* () const		{ return getType () == kObject ? object : nullptr; };
inline Variant::operator int64 () const			{ return getType () == kInt ? lValue : getType () == kFloat ? static_cast<int64> (fValue) : 0; }
inline Variant::operator uint64 () const		{ return getType () == kInt ? static_cast<uint64> (lValue) : getType () == kFloat ? static_cast<uint64> (fValue) : 0; }
inline Variant::operator int () const			{ return static_cast<int> (static_cast<int64> (*this)); }
inline Variant::operator uint32 () const        { return static_cast<uint32> (static_cast<int64> (*this)); }
inline Variant::operator int16 () const			{ return static_cast<int16> (static_cast<int64> (*this)); }
inline Variant::operator uint16 () const        { return static_cast<uint16> (static_cast<int64> (*this)); }
inline Variant::operator int8 () const			{ return static_cast<int8> (static_cast<int64> (*this)); }
inline Variant::operator uint8 () const         { return static_cast<uint8> (static_cast<int64> (*this)); }
inline Variant::operator double () const		{ return getType () == kFloat ? fValue : getType () == kInt ? static_cast<double> (lValue) : 0.0; }
inline Variant::operator float () const			{ return static_cast<float> (static_cast<double> (*this)); }
inline Variant::operator bool () const			{ return static_cast<int64> (*this) ? true : false; }
inline String Variant::asString () const		{ return getType () == kString ? String (string) : String (); }
inline String Variant::toString () const		{ String str; toString (str); return str; }
inline IUnknown* Variant::asUnknown () const	{ return getType () == kObject ? object : nullptr; };
inline int64 Variant::asLargeInt () const		{ return getType () == kInt ? lValue : getType () == kFloat ? (int64)fValue : 0; }
inline uint64 Variant::asLargeUInt () const		{ return getType () == kInt ? static_cast<uint64> (lValue) : getType () == kFloat ? static_cast<uint64> (fValue) : 0; }
inline int Variant::asInt () const				{ return static_cast<int> (static_cast<int64> (*this)); }
inline uint32 Variant::asUInt () const			{ return static_cast<uint32> (static_cast<int64> (*this)); }
inline double Variant::asDouble () const		{ return getType () == kFloat ? fValue : getType () == kInt ? static_cast<double> (lValue) : 0.0; }
inline float Variant::asFloat () const			{ return static_cast<float> (static_cast<double> (*this)); }
inline bool Variant::asBool () const			{ return static_cast<int64> (*this) ? true : false; }
inline tresult Variant::asResult () const			{ return static_cast<tresult> (static_cast<int64> (*this)); }
inline bool Variant::operator != (const Variant& v) const { return !(*this == v); }
inline UIntPtr Variant::asIntPointer () const		{ return getType () == kInt ? static_cast<UIntPtr> (lValue) : 0; }
inline Variant& Variant::setIntPointer (UIntPtr p)	{ clear (); type = kInt; lValue = static_cast<int64> (p); return *this; }
inline int64 Variant::parseLargeInt () const   { return parseLargeInt (0); }
inline int Variant::parseInt () const          { return parseInt (0); }
inline double Variant::parseDouble () const    { return parseDouble (0.0); }
inline float Variant::parseFloat () const      { return parseFloat (0.f); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_variant_h
