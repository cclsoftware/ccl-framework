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
// Filename    : ccl/system/atomtable.cpp
// Description : Atom Table
//
//************************************************************************************************

#include "ccl/system/atomtable.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IAtomTable& CCL_API System::CCL_ISOLATED (GetAtomTable) ()
{
	return AtomTable::instance ();
}

//************************************************************************************************
// Atom
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Atom, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Atom::Atom (StringID name)
: name (name)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API Atom::getAtomName () const
{
	return getName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Atom::equals (const Object& obj) const
{
	const Atom* a = ccl_cast<Atom> (&obj);
	return a ? name == a->name : SuperClass::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Atom::getHashCode (int size) const
{
	return (name.getHashCode () & 0x7FFFFFFF) % size;
}

//************************************************************************************************
// AtomTable
//************************************************************************************************

AtomTable& AtomTable::instance ()
{
	static AtomTable* theAtomTable = nullptr;
	if(theAtomTable == nullptr)
	{
		theAtomTable = NEW AtomTable;
		addGarbageCollected (theAtomTable, true);
	}
	return *theAtomTable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (AtomTable, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AtomTable::AtomTable ()
{
	atoms.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AtomTable::~AtomTable ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAtom* CCL_API AtomTable::createAtom (StringID _name)
{
	MutableCString name (_name);
	name.toLowercase ();

	Threading::ScopedLock scopedLock (lock);

	Atom* atom = (Atom*)hashTable.lookup (Atom (name));
	if(!atom)
	{
		atom = NEW Atom (name);
		atoms.add (atom);
		hashTable.add (atom);
	}

	if(atom)
		atom->retain ();
	return atom;
}
