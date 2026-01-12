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
// Filename    : ccl/network/netstream.h
// Description : Network Stream
//
//************************************************************************************************

#ifndef _ccl_netstream_h
#define _ccl_netstream_h

#include "ccl/base/object.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/network/isocket.h"

namespace CCL {
namespace Net {

interface ISocket;

//************************************************************************************************
// NetworkStream
//************************************************************************************************

class NetworkStream: public Object,
					 public IStream,
					 public INetworkStream
{
public:
	DECLARE_CLASS (NetworkStream, Object)

	NetworkStream (ISocket* socket = nullptr);
	~NetworkStream ();

	// IStream
	int CCL_API read (void* buffer, int size) override;
	int CCL_API write (const void* buffer, int size) override;
	int64 CCL_API tell () override;
	tbool CCL_API isSeekable () const override;
	int64 CCL_API seek (int64 pos, int mode) override;

	// INetworkStream
	ISocket* CCL_API getSocket () override;
	void CCL_API setPseudoBlocking (tbool state) override;
	void CCL_API setTimeout (int timeout) override;
	void CCL_API setCancelCallback (IProgressNotify* callback) override;

	CLASS_INTERFACE2 (IStream, INetworkStream, Object)

protected:
	ISocket* socket;
	int64 byteCount;
	bool pseudoBlocking;
	int timeout;
	IProgressNotify* cancelCallback;
};

} // namespace Net
} // namespace CCL

#endif // _ccl_netstream_h
