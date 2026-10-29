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
// Filename    : ccl/public/gui/iparamprovider.h
// Description : Parameter Provider Interfaces
//
//************************************************************************************************

#ifndef _ccl_iparamprovider_h
#define _ccl_iparamprovider_h

#include "ccl/public/gui/paramdefinition.h"

namespace CCL {

//************************************************************************************************
// IParameterDefinitionProvider
/** Interface to provide parameter definitions. */
//************************************************************************************************

interface IParameterDefinitionProvider: IUnknown
{
	/** Get number of parameters. */
	virtual int CCL_API countParameters () const = 0;

	/** Get parameter definition at index. */
	virtual tresult CCL_API getParameterAt (ParameterDefinition& definition, int index) const = 0;

	DECLARE_IID (IParameterDefinitionProvider)
};

DEFINE_IID (IParameterDefinitionProvider, 0x83d73cba, 0x199e, 0x43dd, 0x8a, 0x72, 0x90, 0x2f, 0xe5, 0x73, 0x91, 0xe7)

//************************************************************************************************
// IParameterProvider
/** Interface to provide parameter definitions and values. */
//************************************************************************************************

interface IParameterProvider: IParameterDefinitionProvider
{
	/**	Set parameter value by tag. */
	virtual tresult CCL_API setParameterValue (int tag, VariantRef value) = 0;

	/**	Set parameter value by name. */
	virtual tresult CCL_API setParameterValue (StringID name, VariantRef value) = 0;

	/** Get parameter value by tag. */
	virtual tresult CCL_API getParameterValue (Variant& value, int tag) const = 0;

	/** Get parameter value by name. */
	virtual tresult CCL_API getParameterValue (Variant& value, StringID name) const = 0;

	DECLARE_IID (IParameterProvider)
};

DEFINE_IID (IParameterProvider, 0xd856fc2f, 0x7ff, 0x48ff, 0xb2, 0xa4, 0xea, 0x53, 0x66, 0xbc, 0x84, 0x1)

} // namespace CCL

#endif // _ccl_iparamprovider_h