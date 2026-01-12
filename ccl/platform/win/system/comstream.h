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
// Filename    : ccl/platform/win/system/comstream.h
// Description : COM Stream class
//
//************************************************************************************************

#ifndef _ccl_comstream_h
#define _ccl_comstream_h

#include "ccl/base/object.h"

#include "ccl/platform/win/system/cclcom.h"

#include "ccl/public/base/istream.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// ComStream
/** COM IStream wrapper for CCL's IStream. */
//************************************************************************************************

class ComStream: public CCL::Object,
				 public ::IStream
{
public:
	ComStream (CCL::IStream* stream = nullptr);
	~ComStream ();

	CCL::IStream* detachStream ();

	operator ::IStream* () { return (IStream*)this; }

	// CCL::IUnknown
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;

	// IUnknown
	DELEGATE_COM_IUNKNOWN

	// ISequentialStream
	STDMETHODIMP Read (void* pv, ULONG cb, ULONG* pcbRead) override;
	STDMETHODIMP Write (const void* pv, ULONG cb, ULONG* pcbWritten) override;

	// IStream
	STDMETHODIMP Seek (LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) override;
	STDMETHODIMP SetSize (ULARGE_INTEGER libNewSize) override;
	STDMETHODIMP CopyTo (::IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten) override;
	STDMETHODIMP Commit (DWORD grfCommitFlags) override;
	STDMETHODIMP Revert () override;
	STDMETHODIMP LockRegion (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
	STDMETHODIMP UnlockRegion (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
	STDMETHODIMP Stat (STATSTG* pstatstg, DWORD grfStatFlag) override;
	STDMETHODIMP Clone (::IStream** ppstm) override;

protected:
	CCL::IStream* stream;
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_comstream_h
