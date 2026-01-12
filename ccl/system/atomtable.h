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
// Filename    : ccl/system/atomtable.h
// Description : Atom Table
//
//************************************************************************************************

#ifndef _ccl_atomtable_h
#define _ccl_atomtable_h

#include "ccl/base/collections/objecthashtable.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/system/iatomtable.h"
#include "ccl/public/system/threadsync.h"

namespace CCL {

//************************************************************************************************
// Atom
//************************************************************************************************

class Atom: public Object,
			public IAtom
{
public:
	DECLARE_CLASS (Atom, Object)

	Atom (StringID name = nullptr);

	PROPERTY_MUTABLE_CSTRING (name, Name)

	// IAtom
	StringID CCL_API getAtomName () const override;

	// Object
	bool equals (const Object& obj) const override;
	int getHashCode (int size) const override;

	CLASS_INTERFACE (IAtom, Object)
};

//************************************************************************************************
// AtomTable
//************************************************************************************************

class AtomTable: public Object,
				 public IAtomTable
{
public:
	DECLARE_CLASS (AtomTable, Object)

	AtomTable ();
	~AtomTable ();

	static AtomTable& instance ();

	// IAtomTable
	IAtom* CCL_API createAtom (StringID name) override;

	CLASS_INTERFACE (IAtomTable, Object)

protected:
	Threading::CriticalSection lock;
	ObjectList atoms;
	ObjectHashTable hashTable;
};

} // namespace CCL

#endif // _ccl_atomtable_h
