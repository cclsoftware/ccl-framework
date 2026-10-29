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
// Filename    : ccl/public/gui/iparamcurve.h
// Description : Parameter Curve Interface
//
//************************************************************************************************

#ifndef _ccl_iparamcurve_h
#define _ccl_iparamcurve_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IParamCurve
/** Non-linear parameter curve.
	\ingroup gui_param */
//************************************************************************************************

interface IParamCurve: IUnknown
{
	/** Get value modified by parameter curve (non-linear), i.e. from display domain to internal. */
	virtual double CCL_API displayToNormalized (double displayValue) const = 0;

	/** Get inverse value from parameter curve (linear), i.e. to display domain. */
	virtual double CCL_API normalizedToDisplay (double normalized) const = 0;

	/** Calculate value for relative parameter editing (in/out values are linear). */
	virtual double CCL_API getRelativeValue (double startValue, double endValue, double linearValue) const = 0;

	/** Get name of factory which created this instance. */
	virtual CStringPtr CCL_API getFactoryName () const = 0;

	DECLARE_IID (IParamCurve)
};

DEFINE_IID (IParamCurve, 0xfcd72f78, 0x6aa7, 0x402f, 0x95, 0x45, 0xf, 0xad, 0x30, 0x4, 0xd9, 0x2c)

} // namespace CCL

#endif // _ccl_iparamcurve_h
