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
// Filename    : ccl/public/app/ifileinforegistry.h
// Description : File Info Registry Interfaces
//
//************************************************************************************************

#ifndef _ccl_ifileinforegistry_h
#define _ccl_ifileinforegistry_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IImage;
interface IFileInfoComponent;

//************************************************************************************************
// IFileInfoFactory
/** Factory interface that can create an info component for a file. 
	\ingroup app_inter */
//************************************************************************************************

interface IFileInfoFactory: IUnknown
{
	/** Create an info component for a file if possible. */
	virtual IFileInfoComponent* CCL_API createComponent (UrlRef path) const = 0;

	DECLARE_IID (IFileInfoFactory)
};

DEFINE_IID (IFileInfoFactory, 0x6fffaca7, 0x36fe, 0x41e2, 0xba, 0x74, 0xed, 0x38, 0xd7, 0xcc, 0x5f, 0xda)

//************************************************************************************************
// IFileInfoComponent
/** Component that can provide information about a file. Should implement IViewFactory. 
	\ingroup app_inter */
//************************************************************************************************

interface IFileInfoComponent: IUnknown
{
	/** Set path of a file to inspect. Returns true if it can handle the file. */
	virtual tbool CCL_API setFile (UrlRef path) = 0;

	/** Returns true if this is the default component for unknown files. */
	virtual tbool CCL_API isDefault () = 0;

	/** Override icon and title to be displayed. */
	virtual tbool CCL_API setDisplayAttributes (IImage* icon, StringRef title) = 0;

	// file information identifiers
	DECLARE_STRINGID_MEMBER (kFileInfo1)
	DECLARE_STRINGID_MEMBER (kFileInfo2)

	/** Get file information string. */
	virtual tbool CCL_API getFileInfoString (String& result, StringID id) const = 0;

	DECLARE_IID (IFileInfoComponent)
};

DEFINE_IID (IFileInfoComponent, 0xB9D20C13, 0x6EDC, 0x4E10, 0x96, 0x98, 0xDE, 0x74, 0x70, 0xE3, 0x0C, 0xB2)
DEFINE_STRINGID_MEMBER (IFileInfoComponent, kFileInfo1, "fileInfo1")
DEFINE_STRINGID_MEMBER (IFileInfoComponent, kFileInfo2, "fileInfo2")

} // namespace CCL

#endif // _ccl_ifileinforegistry_h
