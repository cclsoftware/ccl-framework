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
// Filename    : core/portable/coresingleton.cpp
// Description : Singleton class
//
//************************************************************************************************

#include "coresingleton.h"

#include "core/public/corevector.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// DeletableList
//************************************************************************************************

class DeletableList: public Vector<Deletable*>,
					 public StaticSingleton<DeletableList>
{
public:
	~DeletableList ()
	{
		VectorForEach (*this, Deletable*, instance)
			delete instance;
		EndFor
	}
};

} // namespace Portable
} // namespace Core

using namespace Core;
using namespace Portable;

DEFINE_STATIC_SINGLETON (DeletableList)

//************************************************************************************************
// Deletable
//************************************************************************************************

void Deletable::addInstance (Deletable* instance)
{
	DeletableList::instance ().add (instance);
}
