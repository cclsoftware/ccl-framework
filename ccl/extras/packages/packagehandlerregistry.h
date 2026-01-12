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
// Filename    : ccl/extras/packages/packagehandlerregistry.h
// Description : Package Handler Registry
//
//************************************************************************************************

#ifndef _ccl_packagehandlerregistry_h
#define _ccl_packagehandlerregistry_h

#include "ccl/base/singleton.h"
#include "ccl/public/collections/vector.h"

namespace CCL {
namespace Packages {

class UnifiedPackage;
interface IUnifiedPackageHandler;

//************************************************************************************************
// PackageHandlerRegistry
/** Registry used to find available IUnifiedPackageHandler implementations. */
//************************************************************************************************

class PackageHandlerRegistry: public Object,
							  public Singleton<PackageHandlerRegistry>
{
public:
	PackageHandlerRegistry ();
	~PackageHandlerRegistry ();

	void registerHandler (IUnifiedPackageHandler* handler);
	void unregisterHandler (IUnifiedPackageHandler* handler);

	const Vector<IUnifiedPackageHandler*>& getHandlers () const;

private:
	Vector<IUnifiedPackageHandler*> handlers;
};

} // namespace Packages
} // namespace CCL

#endif // _ccl_packagehandlerregistry_h
