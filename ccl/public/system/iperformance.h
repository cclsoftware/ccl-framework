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
// Filename    : ccl/public/system/iperformance.h
// Description : Performance Measurement
//
//************************************************************************************************

#ifndef _ccl_iperformance_h
#define _ccl_iperformance_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IPerformanceMeter
/**	\ingroup ccl_system */
//************************************************************************************************

interface IPerformanceMeter: IUnknown
{
	virtual void CCL_API setMaxPeriod (double seconds) = 0;

	virtual void CCL_API beginPeriod () = 0;

	virtual void CCL_API endPeriod   () = 0;

	/** Get performance (1.0 means 100 per cent). */
	virtual double CCL_API getPerformance () = 0;

	virtual tbool CCL_API isOverLoad () = 0;

	virtual void CCL_API setOverLoad (tbool state) = 0;

	virtual void CCL_API reset () = 0;

	DECLARE_IID (IPerformanceMeter)
};

DEFINE_IID (IPerformanceMeter, 0xe8657202, 0xd789, 0x4eee, 0x91, 0x95, 0xf4, 0xc3, 0xfa, 0xd7, 0x8a, 0x82)

//************************************************************************************************
// IPerformanceProvider
/**	\ingroup ccl_system */
//************************************************************************************************

interface IPerformanceProvider: IUnknown
{
	virtual IPerformanceMeter& CCL_API getPerformanceMeter () = 0;

	virtual void CCL_API setPerformanceMeterActive (tbool state) = 0;

	DECLARE_IID (IPerformanceProvider)
};

DEFINE_IID (IPerformanceProvider, 0xdc78dbf3, 0x933e, 0x4059, 0x9b, 0xe4, 0x32, 0xb2, 0x27, 0xf0, 0xf1, 0x64)

//************************************************************************************************
// IPerformanceValueProvider
/**	\ingroup ccl_system */
//************************************************************************************************

interface IPerformanceValueProvider: IUnknown
{
	virtual void CCL_API getPerformanceValue (Variant& value, IntPtr token) = 0;

	virtual void CCL_API printPerformanceValue (String& string, VariantRef value, IntPtr token) = 0;

	DECLARE_IID (IPerformanceValueProvider)
};

DEFINE_IID (IPerformanceValueProvider, 0x1075de6b, 0xc89a, 0x4fd3, 0x87, 0xdc, 0x30, 0x27, 0x4b, 0x9f, 0xb6, 0x85)

//************************************************************************************************
// IPerformanceViewer
/**	\ingroup ccl_system */
//************************************************************************************************

interface IPerformanceViewer: IUnknown
{
	virtual void CCL_API addPerformanceMeter (StringID name, IPerformanceMeter* meter) = 0;

	virtual void CCL_API removePerformanceMeter (IPerformanceMeter* meter) = 0;

	virtual void CCL_API addPerformanceProvider (IPerformanceProvider* provider, IUnknown* context) = 0;

	virtual void CCL_API removePerformanceProvider (IPerformanceProvider* provider) = 0;

	virtual void CCL_API addValueProvider (StringID name, IPerformanceValueProvider* valueProvider, IntPtr token = 0) = 0;

	virtual void CCL_API removeValueProvider (IPerformanceValueProvider* valueProvider, IntPtr token = 0) = 0;

	virtual void CCL_API addSubComponent (IUnknown* component) = 0;

	virtual void CCL_API removeSubComponent (IUnknown* component) = 0;

	DECLARE_IID (IPerformanceViewer)
};

DEFINE_IID (IPerformanceViewer, 0x8eafb216, 0x4403, 0x4073, 0xa8, 0xcc, 0xe6, 0xfb, 0xad, 0xdb, 0xb7, 0xdd)

//************************************************************************************************
// PerformanceGuard
/**	\ingroup ccl_system */
//************************************************************************************************

struct PerformanceGuard
{
	PerformanceGuard (IPerformanceMeter* meter)
	: meter (meter)
	{ if(meter) meter->beginPeriod (); }

	~PerformanceGuard ()
	{ if(meter) meter->endPeriod (); }

	IPerformanceMeter* meter;
};

} // namespace CCL

#endif // _ccl_iperformance_h
