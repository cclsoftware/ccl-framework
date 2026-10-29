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
// Filename    : ccl/public/gui/paramdefinition.cpp
// Description : Parameter Definition
//
//************************************************************************************************

#include "ccl/public/gui/paramdefinition.h"

#include "ccl/public/collections/variantvector.h"

using namespace CCL;

//************************************************************************************************
// ParameterDefinition
//************************************************************************************************

ParameterDefinition& ParameterDefinition::setType (ParameterType _type)
{
	type = _type; 
	if(type == kParamTypeToggle)
		setRange (0, 1);
	return *this; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParameterDefinition& ParameterDefinition::setListValue (const ConstVector<String>& strings)
{
	AutoPtr<IMutableArray> arrayObject = NEW VariantStringVector;
	for(String s : strings)
		arrayObject->addArrayElement (s);
	data.takeShared (arrayObject);	
	return setRange (0, arrayObject->getArrayLength ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParameterDefinition& ParameterDefinition::setListValue (const ConstVector<CStringPtr>& strings)
{
	AutoPtr<IMutableArray> arrayObject = NEW VariantStringVector;
	for(CStringPtr s : strings)
		arrayObject->addArrayElement (String (s));
	data.takeShared (arrayObject);
	return setRange (0, arrayObject->getArrayLength ());
}

//************************************************************************************************
// ParameterDefinitionList
//************************************************************************************************

const ParameterDefinitionList ParameterDefinitionList::kEmpty;