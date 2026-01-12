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
// Filename    : ccl/platform/win/system/comstream.cpp
// Description : COM Stream class
//
//************************************************************************************************

#define LOG_STREAM 0//DEBUG

#include "ccl/platform/win/system/comstream.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// ComStream
//************************************************************************************************

ComStream::ComStream (CCL::IStream* stream)
: stream (stream)
{
	if(stream)
		stream->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ComStream::~ComStream ()
{
	if(stream)
		stream->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IStream* ComStream::detachStream ()
{
	CCL::IStream* result = stream;
	stream = nullptr;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::tresult CCL_API ComStream::queryInterface (CCL::UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (ISequentialStream)
	QUERY_COM_INTERFACE (IStream)
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ComStream::Read (void* pv, ULONG cb, ULONG* pcbRead)
{
	ASSERT (stream != nullptr)
	if(!stream)
		return E_FAIL;

	int numRead = stream->read (pv, cb);
	if(pcbRead)
		*pcbRead = numRead;

	#if LOG_STREAM
	CCL::Debugger::printf ("IStream::Read %d Bytes -> %d read\n", cb, numRead);
	#endif
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ComStream::Write (const void* pv, ULONG cb, ULONG* pcbWritten)
{
	ASSERT (stream != nullptr)
	if(!stream)
		return E_FAIL;

	int numWritten = stream->write (pv, cb);
	if(pcbWritten)
		*pcbWritten = numWritten;

	#if LOG_STREAM
	CCL::Debugger::printf ("IStream::Write %d Bytes -> %d written\n", cb, numWritten);
	#endif
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ComStream::Seek (LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
	ASSERT (stream != nullptr)
	if(!stream)
		return E_FAIL;

	// STREAM_SEEK_xxx equals kSeekXXX constants!

	CCL::int64 result = stream->seek (dlibMove.QuadPart, dwOrigin);
	if(plibNewPosition)
		plibNewPosition->QuadPart = result;

	#if LOG_STREAM
	static const char* modes[3] = {"SEEK_SET", "SEEK_CUR", "SEEK_END"};
	CCL::Debugger::printf ("IStream::Seek to %I64d (Mode %s)\n", dlibMove.QuadPart, modes[dwOrigin]);
	#endif
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ComStream::SetSize (ULARGE_INTEGER libNewSize)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ComStream::CopyTo (::IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ComStream::Commit (DWORD grfCommitFlags)
{
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ComStream::Revert ()
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ComStream::LockRegion (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ComStream::UnlockRegion (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ComStream::Stat (STATSTG* pstatstg, DWORD grfStatFlag)
{
	ASSERT (stream != nullptr)
	if(!stream)
		return E_FAIL;

	ASSERT (grfStatFlag == STATFLAG_NONAME)

	if(!pstatstg)
		return E_INVALIDARG;

	ASSERT (stream->isSeekable ())
	CCL::int64 oldPos = stream->tell ();
	CCL::int64 size = stream->seek (0, CCL::IStream::kSeekEnd);
	stream->seek (oldPos, CCL::IStream::kSeekSet);

	SYSTEMTIME now;
	::GetSystemTime (&now);
	FILETIME time;
	::SystemTimeToFileTime (&now, &time);

	pstatstg->pwcsName = nullptr;
	pstatstg->type = STGTY_STREAM;
	pstatstg->cbSize.QuadPart = size;
	pstatstg->mtime = time;
	pstatstg->ctime = time;
	pstatstg->atime = time;
	pstatstg->grfMode = 0;
	pstatstg->grfLocksSupported = 0;
	pstatstg->clsid = CLSID_NULL;
	pstatstg->grfStateBits = 0;

	#if LOG_STREAM
	CCL::Debugger::print ("IStream::Stat called!\n");
	#endif
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ComStream::Clone (::IStream** ppstm)
{
	if(ppstm)
		*ppstm = nullptr;
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Win32
} // namespace CCL
