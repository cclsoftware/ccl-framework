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
// Filename    : ccl/base/performance.h
// Description : Performance Measurement
//
//************************************************************************************************

#ifndef _ccl_performance_h
#define _ccl_performance_h

#include "ccl/public/base/unknown.h"

#include "ccl/public/system/iperformance.h"

namespace CCL {

//************************************************************************************************
// PerformanceMeter
//************************************************************************************************

class PerformanceMeter: public Unknown,
						public IPerformanceMeter
{
public:
	PerformanceMeter ();

	struct ExtraDurationGuard;
	void addExtraDuration (double duration);

	// IPerformanceMeter
	void CCL_API setMaxPeriod (double seconds) override;
	void CCL_API beginPeriod () override;
	void CCL_API endPeriod () override;
	double CCL_API getPerformance () override;
	void CCL_API reset () override;
	tbool CCL_API isOverLoad () override;
	void CCL_API setOverLoad (tbool state) override;

	CLASS_INTERFACE (IPerformanceMeter, Unknown)

protected:
	double startTime;
	double minFreq;
	double performance;
	double decay;
	double overAccumulated;
	int extraDurationInt;
	bool overLoad;

	virtual void setPerformance (double p);
};

//************************************************************************************************
// PerformanceMeter::ExtraDurationGuard
//************************************************************************************************

struct PerformanceMeter::ExtraDurationGuard
{
	PerformanceMeter* meter;
	double startTime;

	ExtraDurationGuard (PerformanceMeter* meter);
	~ExtraDurationGuard ();
	double stop ();
};

//************************************************************************************************
// ProcessMemoryMeter
//************************************************************************************************

class ProcessMemoryMeter: public PerformanceMeter,
						  public IPerformanceValueProvider
{
public:
	ProcessMemoryMeter ();

	enum Identifiers { kMemoryUsed, kMemoryTotal };

	// IPerformanceValueProvider
	void CCL_API getPerformanceValue (Variant& value, IntPtr token) override;
	void CCL_API printPerformanceValue (String& string, VariantRef value, IntPtr token) override;

	// PerformanceMeter
	double CCL_API getPerformance () override;

	CLASS_INTERFACE (IPerformanceValueProvider, PerformanceMeter)

protected:
	int64 lastUpdateTime;
	double memoryUsed;
	double memoryTotal;
};

} // namespace CCL

#endif // _ccl_performance_h
