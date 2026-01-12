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
// Filename    : ccl/public/system/istatistics.h
// Description : Statistics Interfaces
//
//************************************************************************************************

#ifndef _istatistics_h
#define _istatistics_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IStatisticsProvider;

//************************************************************************************************
// IStatisticsCollection
//************************************************************************************************

interface IStatisticsCollection: IUnknown
{
	struct Value
	{
		float normalized = 0;
		int64 timestamp = 0;
	};
	
	virtual int CCL_API countValues () const = 0;

	virtual Value CCL_API getValue (int index) const = 0;
	
	DECLARE_IID (IStatisticsCollection)
};

DEFINE_IID (IStatisticsCollection, 0xce4ed31d, 0x3884, 0x264b, 0xbe, 0xce, 0xd, 0x15, 0xaf, 0xdb, 0xc5, 0x82)

//************************************************************************************************
// IStatisticsProvider
//************************************************************************************************

interface IStatisticsProvider: IUnknown
{
	virtual IStatisticsCollection* CCL_API getData (StringID category = "") const = 0;
	
	// Signals which can be received via IObserver
	DECLARE_STRINGID_MEMBER (kStatsUpdated) ///< statistics have been updated.
	
	DECLARE_IID (IStatisticsProvider)
};

DEFINE_IID (IStatisticsProvider, 0xb6b4949b, 0x6df3, 0x4cf2, 0x8b, 0xec, 0xa, 0x24, 0x3f, 0x8f, 0x25, 0x79)
DEFINE_STRINGID_MEMBER (IStatisticsProvider, kStatsUpdated, "statsUpdated")

} // namespace CCL

#endif // _istatistics_h
