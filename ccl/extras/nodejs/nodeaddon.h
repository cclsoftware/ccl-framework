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
// Filename    : ccl/extras/nodejs/nodeaddon.h
// Description : Node addon base class
//
//************************************************************************************************

#ifndef _ccl_nodeaddon_h
#define _ccl_nodeaddon_h

#include "ccl/extras/nodejs/napihelpers.h"

#include "ccl/public/text/cstring.h"

namespace CCL {
namespace NodeJS {

//************************************************************************************************
// NodeAddon
//************************************************************************************************

class NodeAddon
{
public:
	NodeAddon (CStringPtr moduleId = 0);
	virtual ~NodeAddon ();

	static NodeAddon& getInstance ();

	ModuleRef initPlatformModule ();
	virtual bool startup (napi_env environment);
	virtual void shutdown ();
	virtual napi_value createExportsObject () = 0;

protected:
	napi_env environment;
	CString moduleId;

	void initApp (StringID appID, StringRef companyName, StringRef appName, StringRef appVersion, int versionInt);

	void getPlatformPluginsFolder (CCL::IUrl& url) const;

private:
	static NodeAddon* theInstance;
};

} // namespace NodeJS
} // namespace CCL

#endif // _ccl_nodeaddon_h
