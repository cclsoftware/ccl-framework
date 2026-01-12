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
// Filename    : ccl/base/security/policybuilder.h
// Description : Authorization Policy Builder
//
//************************************************************************************************

#ifndef _ccl_policybuilder_h
#define _ccl_policybuilder_h

#include "ccl/base/security/cryptomaterial.h"

namespace CCL {
namespace Security {

//************************************************************************************************
// PolicyBuilder
//************************************************************************************************
/*
	<AuthorizationPolicy>
		<AuthAssociatedData sid="...">
			<AuthData sid="key=XXXX..."/>
		</AuthAssociatedData>

		<AuthResource sid="...">	
			<AuthClient sid="*">
				<AccessAllowed sid="*"/>
			</AuthClient>
		</AuthResource>
	</AuthorizationPolicy>
*/

class PolicyBuilder
{
public:
	PolicyBuilder& begin ();
	PolicyBuilder& addEncryptionKey (StringRef id, StringRef key);
	PolicyBuilder& addResourceAccessAllowed (StringRef resourceSid, StringRef clientSid = "*");
	PolicyBuilder& end ();

	String toBase64 () const;
	String asString () const;

	static String encodeKey (StringRef id, StringRef key)
	{
		return PolicyBuilder ().begin ().addEncryptionKey (id, key).end ().toBase64 ();
	}

protected:
	String policy;
};

//************************************************************************************************
// PolicyBuilder inline
//************************************************************************************************

inline PolicyBuilder& PolicyBuilder::begin ()
{
	policy.empty ();
	policy << "<AuthorizationPolicy>\n";
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline PolicyBuilder& PolicyBuilder::addEncryptionKey (StringRef id, StringRef key)
{
	policy << "\t<AuthAssociatedData sid=\"" << id << "\">\n";
	policy << "\t\t<AuthData sid=\"key=" << key << "\"/>\n";
	policy << "\t</AuthAssociatedData>\n";
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline PolicyBuilder& PolicyBuilder::addResourceAccessAllowed (StringRef resourceSid, StringRef clientSid)
{
	policy << "\t<AuthResource sid=\"" << resourceSid << "\">\n";
	policy << "\t\t<AuthClient sid=\"" << clientSid << "\">\n";
	policy << "\t\t\t<AccessAllowed sid=\"*\"/>\n";
	policy << "\t\t</AuthClient>\n";
	policy << "\t</AuthResource>\n";
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline PolicyBuilder& PolicyBuilder::end ()
{
	policy << "</AuthorizationPolicy>";
	return *this;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

inline String PolicyBuilder::toBase64 () const
{
	return Crypto::Material ().copyFrom (MutableCString (policy)).toBase64 ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline String PolicyBuilder::asString () const
{
	return policy;
}

} // namespace Security
} // namespace CCL

#endif // _ccl_policybuilder_h
