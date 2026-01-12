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
// Filename    : core/portable/coreparaminfo.h
// Description : Parameter Info
//
//************************************************************************************************

#ifndef _coreparaminfo_h
#define _coreparaminfo_h

#include "core/public/coretypes.h"
#include "core/public/coremacros.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// ParamInfo macros
//************************************************************************************************

#define BEGIN_PARAMINFO(name) \
	static const Core::Portable::ParamInfo name [] = {

#define END_PARAMINFO \
	};

#define PARAM_TOGGLE_(tag, name, default, units, flags, title, shortTitle) \
	{Core::Portable::ParamInfo::kToggle, static_cast<int> (tag), name, 0, 1, 0, default, 1, flags, units, 0, 0, title, shortTitle}
#define PARAM_TOGGLE(tag, name, default, units, flags) PARAM_TOGGLE_(tag, name, default, units, flags, 0, 0)
	
#define PARAM_FLOAT_(tag, name, min, max, default, delta, units, curve, mid, flags, title, shortTitle) \
	{Core::Portable::ParamInfo::kFloat, static_cast<int> (tag), name, min, max, mid, default, delta, flags, units, curve, 0, title, shortTitle}
#define PARAM_FLOAT(tag, name, min, max, default, delta, units, curve, mid, flags) PARAM_FLOAT_(tag, name, min, max, default, delta, units, curve, mid, flags, 0, 0) 

#define PARAM_INT_(tag, name, min, max, default, units, curve, mid, flags, title, shortTitle) \
	{Core::Portable::ParamInfo::kInt, static_cast<int> (tag), name, min, max, mid, default, 1, flags, units, curve, 0, title, shortTitle}
#define PARAM_INT(tag, name, min, max, default, units, curve, mid, flags) PARAM_INT_(tag, name, min, max, default, units, curve, mid, flags, 0, 0)

#define PARAM_LIST_(tag, name, stringList, default, flags, title, shortTitle) \
	{Core::Portable::ParamInfo::kList, static_cast<int> (tag), name, 0, ARRAY_COUNT (stringList)-1, 0, default, 1, flags, 0, 0, stringList, title, shortTitle}
#define PARAM_LIST(tag, name, stringList, default, flags) PARAM_LIST_(tag, name, stringList, default, flags, 0, 0) 

#define PARAM_STRING_(tag, name, flags, title, shortTitle) \
	{Core::Portable::ParamInfo::kString, static_cast<int> (tag), name, 0, 0, 0, 0, 0, flags, 0, 0, 0, title, shortTitle}
#define PARAM_STRING(tag, name, flags) PARAM_STRING_(tag, name, flags, 0, 0) 
    
#define PARAM_ALIAS_(tag, name, flags, title, shortTitle) \
	{Core::Portable::ParamInfo::kAlias, static_cast<int> (tag), name, 0, 0, 0, 0, 0, flags, 0, 0, 0, title, shortTitle}
#define PARAM_ALIAS(tag, name, flags) PARAM_ALIAS_(tag, name, flags, 0, 0)
	
#define PARAM_COLOR_(tag, name, flags, title, shortTitle) \
	{Core::Portable::ParamInfo::kColor, static_cast<int> (tag), name, 0, 0, 0, 0, 0, flags, 0, 0, 0, title, shortTitle}
#define PARAM_COLOR(tag, name, flags) PARAM_COLOR_(tag, name, flags, 0, 0)

#define PARAM_DELTA		(1.f / 500.f) // default delta for float parameters

#define PARAM_BIPOLAR	Core::Portable::ParamInfo::kBipolar
#define PARAM_STORABLE	Core::Portable::ParamInfo::kStorable
#define PARAM_PRIVATE	Core::Portable::ParamInfo::kPrivate
#define PARAM_LINKABLE  Core::Portable::ParamInfo::kLinkable
#define PARAM_MUTABLE	Core::Portable::ParamInfo::kMutable
#define PARAM_READONLY	Core::Portable::ParamInfo::kReadOnly
#define PARAM_USERFLAG1	Core::Portable::ParamInfo::kUserFlag1
#define PARAM_USERFLAG2	Core::Portable::ParamInfo::kUserFlag2

//************************************************************************************************
// ParamType / ParamValue
//************************************************************************************************

typedef int ParamType;
typedef float ParamValue;

//************************************************************************************************
// ParamInfo
/**	Parameter information usually stored in POD-style C arrays
	and shared among multiple instances of a parameter object.
	\ingroup core_portable */
//************************************************************************************************

struct ParamInfo
{
	enum Types
	{
		kToggle,
		kInt,
		kList,
		kFloat,
		kString,
		kAlias,
		kColor
	};

	enum Flags 
	{
		// basic flags (compatible with CCL::IParameter)
		kBipolar	= 1<<0,		
		kStorable	= 1<<1,
		kPrivate	= 1<<2,
        kLinkable   = 1<<3,
		kMutable	= 1<<4,
		kReadOnly	= 1<<5,
		
		// user-defined flags
		kUserFlag1  = 1<<6,
		kUserFlag2  = 1<<7
	};

	static const int kMaxNameLength = 32;

	ParamType type;					///< parameter type
	int tag;						///< parameter tag
	char name[kMaxNameLength];		///< parameter name

	ParamValue minValue;			///< min range
	ParamValue maxValue;			///< max range
	ParamValue midValue;			///< additional value for interpolator, not related to min/max
	ParamValue defaultValue;		///< default value in min/max range
	ParamValue deltaValue;			///< delta value (normalized for kFloat, in min/max range otherwise)

	int flags;						///< parameter flags
	CStringPtr unitName;			///< formatter name
	CStringPtr curveName;			///< interpolator name
	CStringPtr* stringList;			///< list of static strings (optional)

	CStringPtr title;	            ///< parameter title (optional - only pointer)
	CStringPtr shortTitle;	        ///< parameter short title (optional - only pointer)

	void clear ();
	ParamValue makeValid (ParamValue v) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ParamInfo inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void ParamInfo::clear ()
{
	::memset (this, 0, sizeof(ParamInfo));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline ParamValue ParamInfo::makeValid (ParamValue v) const
{
	if(minValue < maxValue)
	{
		if(v < minValue)
			v = minValue;
		if(v > maxValue)
			v = maxValue;
	}
	else
	{
		// Linear reverse interpolator switches max and min param fields in order to function,
		// and must be clipped accordingly
		if(v < maxValue)
			v = maxValue;
		if(v > minValue)
			v = minValue;
	}

	switch(type)
	{
	case ParamInfo::kToggle : v = (ParamValue)(v != 0 ? 1 : 0); break;
	case ParamInfo::kInt    : // fall trough
	case ParamInfo::kList   : v = (ParamValue)(v < 0.f ? (int)(v - 0.5f) : (int)(v + 0.5f)); break;
	}

	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Portable
} // namespace Core

#endif // _coreparaminfo_h
