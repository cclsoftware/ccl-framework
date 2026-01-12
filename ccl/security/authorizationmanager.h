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
// Filename    : ccl/security/authorizationmanager.h
// Description : Authorization Manager
//
//************************************************************************************************

#ifndef _ccl_authorizationmanager_h
#define _ccl_authorizationmanager_h

#include "ccl/base/singleton.h"

#include "ccl/security/authorizationpolicy.h"

#include "ccl/public/security/iauthorizationmanager.h"

namespace CCL {
namespace Security {

//************************************************************************************************
// AuthorizationManager
//************************************************************************************************

class AuthorizationManager: public Object,
							public IAuthorizationManager,
							public Singleton<AuthorizationManager>
{
public:
	DECLARE_CLASS (AuthorizationManager, Object)

	AuthorizationManager ();

	// IAuthorizationManager
	tresult CCL_API setAppIdentity (StringRef appSid, StringRef appSubSid = nullptr) override;
	StringRef CCL_API getAppIdentity () const override;
	StringRef CCL_API getAppSubIdentity () const override;
	tresult CCL_API getComputerIdentity (String& computerSid, ComputerIDType type = kDefaultComputerID) override;
	tresult CCL_API loadPolicy (IStream& stream, int mode = 0) override;
	Authorization::IPolicyItem* CCL_API getPrivileges (StringRef resourceSid) override;
	IUnknownIterator* CCL_API createIterator (StringRef dataSid) override;
	tresult CCL_API revokePolicy (IStream& stream, int mode = 0) override;

	CLASS_INTERFACE (IAuthorizationManager, Object)

protected:
	String appSid;
	String appSubSid;
	ComputerIDType platformComputerIdType;
	String savedComputerSidV1;
	String savedComputerSidV2;
	AutoPtr<Authorization::Policy> policy;

	struct BasicComputerInformation
	{
		int cpuClockSpeed;
		int64 physicalMemoryAmount;
		String cpuIdentifier;
        String cpuModel;
		String diskIdentifier;
		String diskSerialNumber;
		String volumeSerialNumber;
		String macAddress;
		String adapterName;
        String deviceModelSubtype;
        bool processIsTranslated;

		BasicComputerInformation ()
		: cpuClockSpeed (0),
		  physicalMemoryAmount (0),
          processIsTranslated (false)
		{}
	};

	struct ComputerFingerprint
	{
		enum Metrics { kSize = 10 };
		union
		{
			uint8 bytes[kSize];
			struct
			{
				uint32 partHash1;
				uint32 partHash2;
				uint8 partByte1;
				uint8 checkByte;
			} fields;
		} data;

		ComputerFingerprint ()
		{ ::memset (data.bytes, 0, kSize); }
	};

	Authorization::Policy* parsePolicy (IStream& stream, int mode) const;

	void updateHardwareHashes ();
	String finish (ComputerFingerprint& fingerprint) const;
	bool verify (const ComputerFingerprint& fingerprint) const;
	void getBasicComputerInformation (BasicComputerInformation& info) const;
};

} // namespace Security
} // namespace CCL

#endif // _ccl_authorizationmanager_h
