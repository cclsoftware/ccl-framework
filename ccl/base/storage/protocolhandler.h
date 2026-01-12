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
// Filename    : ccl/base/storage/protocolhandler.h
// Description : Protocol Handler classes
//
//************************************************************************************************

#ifndef _ccl_protocolhandler_h
#define _ccl_protocolhandler_h

#include "ccl/public/text/cclstring.h"
#include "ccl/public/system/iprotocolhandler.h"

#include "ccl/base/collections/objectlist.h"

namespace CCL {

//************************************************************************************************
// ProtocolHandler
/** Protocol handler base class. */
//************************************************************************************************

class ProtocolHandler: public Object,
					   public IProtocolHandler
{
public:
	DECLARE_CLASS_ABSTRACT (ProtocolHandler, Object)
	CLASS_INTERFACE (IProtocolHandler, Object)
};

//************************************************************************************************
// MountProtocolHandler
/** Mount point collection. */
//************************************************************************************************

class MountProtocolHandler: public ProtocolHandler
{
public:
	DECLARE_CLASS_ABSTRACT (MountProtocolHandler, ProtocolHandler)

	MountProtocolHandler ();

	virtual bool mount (StringRef name, IFileSystem* fileSys);
	virtual bool unmount (StringRef name);

	// ProtocolHandler
	IFileSystem* CCL_API getMountPoint (StringRef name) override;

	CLASS_INTERFACE (IProtocolHandler, Object)

protected:
	ObjectList mountPoints;
};

//************************************************************************************************
// MountPoint
/** A mounted file system. */
//************************************************************************************************

class MountPoint: public Object
{
public:
	DECLARE_CLASS (MountPoint, Object)

	MountPoint (StringRef name = nullptr, IFileSystem* fileSys = nullptr);
	~MountPoint ();

	IFileSystem* getFileSystem () const;
	StringRef getName () const;

	// Object
	bool equals (const Object& obj) const override;

protected:
	IFileSystem* fileSys;
	String name;
};

} // namespace CCL

#endif // _ccl_protocolhandler_h
