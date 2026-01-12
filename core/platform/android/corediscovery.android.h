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
// Filename    : core/platform/android/corediscovery.android.h
// Description : DNS Service Discovery Android implementation
//
//************************************************************************************************

#ifndef _corediscovery_android_h
#define _corediscovery_android_h

#include "core/network/corenetwork.h"

#include "core/platform/shared/coreplatformdiscovery.h"

namespace Core {
namespace Platform {

class DiscoveryContext;

typedef const DiscoveryContext* DiscoveryRef;

//************************************************************************************************
// AndroidTextRecordData
//************************************************************************************************

struct AndroidTextRecordData
{
	CString64 key;
	CString64 value;
};

//************************************************************************************************
// AndroidTextRecord
//************************************************************************************************

class AndroidTextRecord: ITextRecord
{
public:
	AndroidTextRecord (const ConstVector<AndroidTextRecordData*>& records);

	// ITextRecord
	int getCount () const override;
	bool getItemAt (CString64& key, CString64& value, int index) const override;
	bool getValue (CString64& value, CStringPtr key) const override;
	bool getIntValue (int64& value, CStringPtr key) const override;

private:
	ConstVector<AndroidTextRecordData*> records;
};

//************************************************************************************************
// AndroidTextRecordBuilder
//************************************************************************************************

class AndroidTextRecordBuilder: public ITextRecordBuilder<AndroidTextRecord>
{
public:
	~AndroidTextRecordBuilder ();

	// ITextRecordBuilder
	void setValue (CStringPtr key, CStringPtr value) override;
	void setIntValue (CStringPtr key, int64 value) override;

	AndroidTextRecord getTextRecord () const override;

protected:
	Vector<AndroidTextRecordData*> records;
};

//************************************************************************************************
// Android type definitions
//************************************************************************************************

typedef AndroidTextRecord TextRecord;
typedef AndroidTextRecordBuilder TextRecordBuilder;
typedef ServiceTargetDescriptorBase<AndroidTextRecord> ServiceTargetDescriptor;
typedef BrowseReplyHandlerBase<AndroidTextRecord, DiscoveryRef> BrowseReplyHandler;
typedef RegisterReplyHandlerBase<DiscoveryRef> RegisterReplyHandler;

} // namespace Platform
} // namespace Core

#endif // _corediscovery_android_h
