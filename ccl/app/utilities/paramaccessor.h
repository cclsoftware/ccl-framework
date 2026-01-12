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
// Filename    : ccl/app/utilities/paramaccessor.h
// Description : Parameter Accessor
//
//************************************************************************************************

#ifndef _ccl_paramaccessor_h
#define _ccl_paramaccessor_h

#include "ccl/public/base/variant.h"

namespace CCL {

interface IParameter;

//************************************************************************************************
// ParamAccessor
/** Access parameter via absolute or relative path (e.g. "controller/controller/paramName"). */
//************************************************************************************************

class ParamAccessor
{
public:
	ParamAccessor (IUnknown* controller, StringID paramPath);
	ParamAccessor (StringID paramPath);

	Variant get () const;
	bool get (Variant& value) const;
	bool set (VariantRef value, tbool update = false);

	IParameter* operator -> () const;
	IParameter* getParam () const;

private:
	IParameter* parameter;

	void resolve (IUnknown* anchor, StringID paramPath);
	IUnknown* lookupController (IUnknown* anchor, StringID path);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Variant ParamAccessor::get () const
{ Variant v; get (v); return v; }

inline IParameter* ParamAccessor::operator -> () const
{ return parameter; }

inline IParameter* ParamAccessor::getParam () const
{ return parameter; }

} // namespace CCL

#endif // _ccl_paramaccessor_h
