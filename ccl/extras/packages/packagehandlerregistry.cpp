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
// Filename    : ccl/extras/packages/packagehandlerregistry.cpp
// Description : Package Handler Registry
//
//************************************************************************************************

#include "ccl/extras/packages/packagehandlerregistry.h"

using namespace CCL;
using namespace Packages;

//************************************************************************************************
// PackageHandlerRegistry
//************************************************************************************************

DEFINE_SINGLETON (PackageHandlerRegistry)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageHandlerRegistry::PackageHandlerRegistry ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageHandlerRegistry::~PackageHandlerRegistry ()
{
	ASSERT (handlers.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageHandlerRegistry::registerHandler (IUnifiedPackageHandler* handler)
{
	ASSERT (handlers.contains (handler) == false)
	handlers.add (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageHandlerRegistry::unregisterHandler (IUnifiedPackageHandler* handler)
{
	ASSERT (handlers.contains (handler))
	handlers.remove (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<IUnifiedPackageHandler*>& PackageHandlerRegistry::getHandlers () const
{
	return handlers;
}
