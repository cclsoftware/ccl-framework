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
// Filename    : ccl/base/storage/protocolhandler.cpp
// Description : Protocol Handler classes
//
//************************************************************************************************

#include "ccl/base/storage/protocolhandler.h"

#include "ccl/public/system/ifilesystem.h"

using namespace CCL;

//************************************************************************************************
// ProtocolHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ProtocolHandler, Object)

//************************************************************************************************
// MountProtocolHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MountProtocolHandler, ProtocolHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

MountProtocolHandler::MountProtocolHandler ()
{
	mountPoints.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MountProtocolHandler::mount (StringRef name, IFileSystem* fileSys)
{
	mountPoints.add (NEW MountPoint (name, fileSys));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MountProtocolHandler::unmount (StringRef name)
{
	MountPoint* m = (MountPoint*)mountPoints.findEqual (MountPoint (name));
	if(m)
	{
		mountPoints.remove (m);
		m->release ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileSystem* MountProtocolHandler::getMountPoint (StringRef name)
{
	MountPoint* m = (MountPoint*)mountPoints.findEqual (MountPoint (name));
	if(m)
		return m->getFileSystem ();
	return nullptr;
}

//************************************************************************************************
// MountPoint
//************************************************************************************************

DEFINE_CLASS (MountPoint, Object)
DEFINE_CLASS_NAMESPACE (MountPoint, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

MountPoint::MountPoint (StringRef name, IFileSystem* fileSys)
: name (name),
  fileSys (fileSys)
{
	if(fileSys)
		fileSys->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MountPoint::~MountPoint ()
{
	if(fileSys)
		fileSys->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileSystem* MountPoint::getFileSystem () const 
{ 
	return fileSys; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef MountPoint::getName () const
{ 
	return name; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MountPoint::equals (const Object& obj) const
{
	const MountPoint* m = ccl_cast<MountPoint> (&obj);
	return m && m->name == name;
}
