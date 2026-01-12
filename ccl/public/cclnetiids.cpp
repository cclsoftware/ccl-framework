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
// Filename    : ccl/public/cclnetiids.cpp
// Description : IID symbols
//
//************************************************************************************************

#define INIT_IID
#include "ccl/public/text/cstring.h"

// Network
#include "ccl/public/network/inetdiscovery.h"
#include "ccl/public/network/inetwork.h"

// Web
#include "ccl/public/network/web/iwebnewsreader.h"
#include "ccl/public/network/web/iwebclient.h"
#include "ccl/public/network/web/iwebcredentials.h"
#include "ccl/public/network/web/iwebprotocol.h"
#include "ccl/public/network/web/iwebrequest.h"
#include "ccl/public/network/web/iwebserver.h"
#include "ccl/public/network/web/iwebservice.h"
#include "ccl/public/network/web/iwebfileclient.h"
#include "ccl/public/network/web/iwebsocket.h"
#include "ccl/public/network/web/ixmlhttprequest.h"
#include "ccl/public/network/web/itransfermanager.h"
#include "ccl/public/network/web/iwebfileservice.h"
#include "ccl/public/network/web/iwebfiletask.h"

const CCL::String CCL::Web::IWebFileService::kProtocol ("webfs");

