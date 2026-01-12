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
// Filename    : core/platform/shared/posix/corediscovery.posix.h
// Description : DNS Service Discovery POSIX implementation
//
//************************************************************************************************

#ifndef _corediscovery_posix_h
#define _corediscovery_posix_h

#include "core/platform/corefeatures.h"

#if CORE_DISCOVERY_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
#include <dns_sd.h>
#endif

#include "core/platform/shared/coreplatformdiscovery.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

namespace Core {
namespace Platform {

typedef DNSServiceRef DiscoveryRef;
typedef TXTRecordRef TextRecordRef;

//************************************************************************************************
// PosixTextRecord
//************************************************************************************************

struct PosixTextRecord: ITextRecord
{
	const void* buffer;
	uint16 length;

	PosixTextRecord (const void* buffer, uint16 length);
	~PosixTextRecord ();

	// ITextRecord
	int getCount () const override;
	bool getItemAt (CString64& key, CString64& value, int index) const override;
	bool getValue (CString64& value, CStringPtr key) const override;
	bool getIntValue (int64& value, CStringPtr key) const override;
};

//************************************************************************************************
// PosixTextRecordBuilder
/** Note: Text record keys should be max. 8 characters long! */
//************************************************************************************************

class PosixTextRecordBuilder: public ITextRecordBuilder<PosixTextRecord>
{
public:
	PosixTextRecordBuilder ();
	~PosixTextRecordBuilder ();

	// ITextRecordBuilder
	void setValue (CStringPtr key, CStringPtr value) override;
	void setIntValue (CStringPtr key, int64 value) override;

	PosixTextRecord getTextRecord () const override;

protected:
	static const int kMaxBufferSize = 512;
	char textRecordBuffer[kMaxBufferSize];
	TextRecordRef textRecord;
};

//************************************************************************************************
// PosixDiscoveryHandler
//************************************************************************************************

typedef ServiceTargetDescriptorBase<PosixTextRecord> PosixServiceTargetDescriptor;
typedef BrowseReplyHandlerBase<PosixTextRecord, DiscoveryRef> PosixBrowseReplyHandler;
typedef RegisterReplyHandlerBase<DiscoveryRef> PosixRegisterReplyHandler;

class PosixDiscoveryHandler
{
public:
	/** Check if DNSSD is installed. */
	static bool isInstalled ();

	/** Register browser for given service type. */
	static bool registerBrowser (DiscoveryRef& sdRef, const TypeDescriptor& descriptor, PosixBrowseReplyHandler* replyHandler);

	/** Register service. */
	static bool registerService (DiscoveryRef& sdRef, const PosixServiceTargetDescriptor& descriptor, PosixRegisterReplyHandler* replyHandler);

	/** Unregister given reference (browser or service). */
	static void unregisterReference (DiscoveryRef sdRef);

	/** Process results for given reference (browser or service). */
	static bool processResults (const DiscoveryRef sdRefList[], int count);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

#if CORE_DISCOVERY_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
typedef PosixTextRecord TextRecord;
typedef PosixServiceTargetDescriptor ServiceTargetDescriptor;
typedef PosixTextRecordBuilder TextRecordBuilder;
typedef PosixBrowseReplyHandler BrowseReplyHandler;
typedef PosixRegisterReplyHandler RegisterReplyHandler;
#endif

} // namespace Platform
} // namespace Core

#endif // _corediscovery_posix_h
