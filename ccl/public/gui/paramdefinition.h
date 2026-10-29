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
// Filename    : ccl/public/gui/paramdefinition.h
// Description : Parameter Definition
//
//************************************************************************************************

#ifndef _ccl_paramdefinition_h
#define _ccl_paramdefinition_h

#include "ccl/public/gui/paramtypes.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// ParameterDefinition
/** Parameter definition. */
//************************************************************************************************

struct ParameterDefinition
{
	ParameterType type;			///< parameter type, @see ParameterType
	MutableCString className;	///< parameter class (optional, must be registered)
	int flags;					///< parameter flags, @see ParameterFlags
	MutableCString name;		///< internal parameter name, e.g. for persistence
	String title;				///< user-facing parameter title
	int tag;					///< internal parameter identifier

	Variant minValue;			///< minimum value for numeric parameters (integer or floating-point)
	Variant maxValue;			///< maxiumun value for numeric parameters (integer or floating-point)
	Variant midValue;			///< additional value for interpolator (optional, independent of to min/max)
	Variant defaultValue;		///< default parameter value
	Variant precision;			///< parameter precision (optional)
	Variant data;				///< additional data (e.g. IArrayObject with list entries)
	
	MutableCString curveName;	///< parameter curve name (IParamCurve, optional)
	MutableCString unitName;	///< parameter unit name (IFormatter, optional)
	
	ParameterDefinition ()
	: type (kParamTypeInteger),
	  flags (0),
	  tag (0)
	{}

	/** Check if parameter has min/max range. */
	bool hasRange () const
	{
		return	type == kParamTypeToggle ||
				type == kParamTypeInteger ||
				type == kParamTypeFloat ||
				type == kParamTypeList;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Fluent initialization
	//////////////////////////////////////////////////////////////////////////////////////////////

	ParameterDefinition& setType (ParameterType _type);
	ParameterDefinition& setFlags (int _flags) { flags = _flags; return *this; }
	ParameterDefinition& setName (StringID _name) { name = _name; return *this; }
	ParameterDefinition& setTitle (StringRef _title) { title = _title; return *this; }
	ParameterDefinition& setTag (int _tag) { tag = _tag; return *this; }

	ParameterDefinition& setRange (VariantRef _min, VariantRef _max) { minValue = _min; maxValue = _max; return *this; }
	ParameterDefinition& setMid (VariantRef _mid) { midValue = _mid; return *this; }
	ParameterDefinition& setDefault (VariantRef _default) { defaultValue = _default; return *this; }
	ParameterDefinition& setPrecision (VariantRef _precision) { precision = _precision; return *this; }
	ParameterDefinition& setListValue (const ConstVector<String>& strings);
	ParameterDefinition& setListValue (const ConstVector<CStringPtr>& strings);

	ParameterDefinition& setCurve (StringID _curve) { curveName = _curve; return *this; }
	ParameterDefinition& setUnit (StringID _unit) { unitName = _unit; return *this; }
};

//************************************************************************************************
// ParameterDefinitionList
/** List of parameter definitions. */
//************************************************************************************************

class ParameterDefinitionList: public Vector<ParameterDefinition> 
{
public:
	static const ParameterDefinitionList kEmpty;

	ParameterDefinitionList& add (const ParameterDefinition& definition)
	{
		Vector<ParameterDefinition>::add (definition);
		return *this;
	}

	const ParameterDefinition* find (int tag) const
	{
		return findIf ([&] (const ParameterDefinition& definition) { return definition.tag == tag; });
	}

	const ParameterDefinition* find (StringID name) const
	{
		return findIf ([&] (const ParameterDefinition& definition) { return definition.name == name; });
	}
};

} // namespace CCL

#endif // _ccl_paramdefinition_h