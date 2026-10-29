//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/public/gui/paramtypes.h
// Description : Parameter Types
//
//************************************************************************************************

#ifndef _ccl_paramtypes_h
#define _ccl_paramtypes_h

#include "ccl/public/base/platform.h"

namespace CCL {

//************************************************************************************************
// ParameterType
/** Parameter types. */
//************************************************************************************************

DEFINE_ENUM (ParameterType)
{
	kParamTypeToggle = 0,	///< toggle value (on/off)
	kParamTypeInteger,		///< integer value
	kParamTypeFloat,		///< floating-point value
	kParamTypeString,		///< string value
	kParamTypeList,			///< list of values
	kParamTypeCommand,		///< command (category/name)
	kParamTypeColor,		///< color value
	kParamTypeImage,		///< image
	kParamTypeSegments,		///< multiple numeric segments
	kParamTypeScroll,		///< integer value with page size
	kParamTypeTextModel,	///< text model
	kNumParamTypes
};

//************************************************************************************************
// ParameterFlags
/** Parameter flags. */
//************************************************************************************************

DEFINE_ENUM (ParameterFlags)
{
	// behavioral flags
	kParamBehaviorSignalFirst	= 1<<1,		///< parameter signals with old and with new value
	kParamBehaviorSignalAlways	= 1<<2,		///< parameter signals even if current value is set again
	kParamBehaviorBipolar		= 1<<3,		///< parameter is bipolar, the center is defined by (max-min)/2
	kParamBehaviorReverse		= 1<<4,		///< parameter is reverse, display should be upside down
	kParamBehaviorOutOfRange	= 1<<5,		///< parameter value is out of range
	kParamBehaviorWrapAround	= 1<<6,		///< parameter increment/decrement does not stop at min/max
		
	// semantic flags
	kParamSemanticStorable		= 1<<7,		///< parameter is storable
	kParamSemanticListValue		= 1<<8,		///< parameter stores selected list value instead of list index
	kParamSemanticGroupable		= 1<<9,		///< parameter can be grouped
	kParamSemanticPublic		= 1<<10,	///< parameter is public
	kParamSemanticMutable		= 1<<11,	///< parameter is mutable (min/max or list entries can change)
	kParamSemanticReadOnly		= 1<<12,	///< parameter is used for display purposes only, no editing

	// usage-specific flags
	kParamUserFlag1				= 1<<13,	///< usage-specific
	kParamUserFlag2				= 1<<14,	///< usage-specific
	kParamUserFlag3				= 1<<15,	///< usage-specific

	kLastParameterFlag			= 15
};

} // namespace CCL

#endif // _ccl_paramtypes_h