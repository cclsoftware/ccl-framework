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
// Filename    : core/platform/win/corediscovery.win.h
// Description : DNS Service Discovery Windows implementation
//
//************************************************************************************************

#ifndef _corediscovery_win_h
#define _corediscovery_win_h

#include "core/network/corenetwork.h"

#include "core/platform/shared/coreplatformdiscovery.h"

#include <windns.h>

namespace Core {
namespace Platform {

class DiscoveryContext;

typedef const DiscoveryContext* DiscoveryRef;

//************************************************************************************************
// WindowsTextRecord
//************************************************************************************************

class WindowsTextRecord: ITextRecord
{
public:
	WindowsTextRecord (const ConstVector<DNS_TXT_DATA*>& records);

	// ITextRecord
	int getCount () const override;
	bool getItemAt (CString64& key, CString64& value, int index) const override;
	bool getValue (CString64& value, CStringPtr key) const override;
	bool getIntValue (int64& value, CStringPtr key) const override;

private:
	ConstVector<DNS_TXT_DATA*> records;
};

//************************************************************************************************
// WindowsTextRecordBuilder
//************************************************************************************************

class WindowsTextRecordBuilder: public ITextRecordBuilder<WindowsTextRecord>
{
public:
	~WindowsTextRecordBuilder ();

	// ITextRecordBuilder
	void setValue (CStringPtr key, CStringPtr value) override;
	void setIntValue (CStringPtr key, int64 value) override;

	WindowsTextRecord getTextRecord () const override;

protected:
	Vector<DNS_TXT_DATA*> records;
};

//************************************************************************************************
// Windows type definitions
//************************************************************************************************

typedef WindowsTextRecord TextRecord;
typedef WindowsTextRecordBuilder TextRecordBuilder;
typedef ServiceTargetDescriptorBase<WindowsTextRecord> ServiceTargetDescriptor;
typedef BrowseReplyHandlerBase<WindowsTextRecord, DiscoveryRef> BrowseReplyHandler;
typedef RegisterReplyHandlerBase<DiscoveryRef> RegisterReplyHandler;

} // namespace Platform
} // namespace Core

#endif // _corediscovery_win_h
