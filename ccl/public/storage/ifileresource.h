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
// Filename    : ccl/public/storage/ifileresource.h
// Description : File Resource Interface
//
//************************************************************************************************

#ifndef _ccl_ifileresource_h
#define _ccl_ifileresource_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IFileResource
/**	Interface for file-based resource identified by an URL. It can be opened, created, deleted, etc.
	Opening calls are counted. For each successful open, a close call must follow.
	Please note that the file resource itself is not safe for simultaneous access by multiple
	threads. If thread-safety is required, separate instances sharing the same URL should be created.
	\ingroup base_io  */
//************************************************************************************************

interface IFileResource: IUnknown
{
	/** Helper class closing resource in dtor. */
	struct Closer
	{
		Closer (IFileResource& res): res (res) {}
		~Closer () { if(res.isOpen ()) res.close (); }
		IFileResource& res;
	};

	/** Helper class to open/close resource. */
	struct Opener
	{
		Opener (IFileResource& res, int mode = 0): res (res) { success = res.open (mode); }
		~Opener () { if(success) res.close (); }
		IFileResource& res;
		tbool success;
	};

	/** Call once to init file location. */
	virtual tbool CCL_API setPath (UrlRef path) = 0;
	
	/** Get file location. */
	virtual UrlRef CCL_API getPath () const = 0;

	/** Open existing file. */
	virtual tbool CCL_API open (int mode = 0) = 0;

	/** Create new file. */
	virtual tbool CCL_API create (int mode = 0) = 0;

	/** Close file. */
	virtual tbool CCL_API close () = 0;
	
	/** Check if file exists. */
	virtual tbool CCL_API isExisting () const = 0;

	/** Check if file is currently open. */
	virtual tbool CCL_API isOpen () const = 0;

	/** Try to delete file physically. */
	virtual tbool CCL_API deletePhysical (int mode = 0) = 0;

	DECLARE_IID (IFileResource)
};

DEFINE_IID (IFileResource, 0xff69b3b7, 0x4bae, 0x4cff, 0x9a, 0x14, 0xe8, 0x9e, 0x11, 0xaf, 0x26, 0x97)

} // namespace CCL

#endif // _ccl_ifileresource_h
