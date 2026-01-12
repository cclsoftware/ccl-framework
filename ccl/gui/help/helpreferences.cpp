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
// Filename    : ccl/gui/help/helpreferences.cpp
// Description : Help References
//
//************************************************************************************************

#include "ccl/gui/help/helpreferences.h"

#include "ccl/base/storage/storage.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

static const String kDefaultFile (CCLSTR ("help.pdf"));

//************************************************************************************************
// HelpCatalog
//************************************************************************************************

DEFINE_CLASS (HelpCatalog, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpCatalog::HelpCatalog ()
: priority (0),
  primary (false),
  quickHelp (false)
{
	references.objectCleanup (true);
	defaultReference.setCatalog (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API HelpCatalog::getTitle () const
{
	if(title.isEmpty ())
		title = attributes.getStringWithAlternative ("Help:LocalizedCatalogName", "Help:CatalogName");
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API HelpCatalog::getCategory () const
{
	return category;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpCatalog::setCategory (StringID _category)
{
	category = _category;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpCatalog::addShared (const HelpCatalog& other)
{
	ArrayForEach (other.references, HelpReference, ref)
		if(HelpReference* existing = const_cast<HelpReference*> (lookup (ref->getHelpIdentifier ())))
		{
			if(priority >= other.getPriority ()) // keep existing
				continue;

			references.remove (existing); // replace existing
			existing->release ();
		}

		references.addSorted (ref);
		ref->retain ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpCatalog::setDefaultReference (const HelpReference& reference)
{
	defaultReference = reference;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const HelpReference* HelpCatalog::lookup (StringRef helpid) const
{
	return (HelpReference*)references.search (HelpReference (helpid));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const HelpReference& HelpCatalog::getDefaultReference () const
{
	return defaultReference;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpCatalog::equals (const Object& obj) const
{
	const HelpCatalog* other = ccl_cast<HelpCatalog> (&obj);
	if(other == nullptr)
		return false;

	return path->equals (*other->getPath ())
		&& contentLanguage == other->getContentLanguage ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpCatalog::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	priority = a.getInt ("priority");
	primary = a.getBool ("isPrimary");
	contentLanguage = a.getString ("contentlanguage");

	String defaultFile = a.getString ("defaultfile");
	if(defaultFile.isEmpty ())
		defaultFile = kDefaultFile;
	defaultReference.setFileName (defaultFile);

	attributes.removeAll ();
	references.removeAll ();

	AutoPtr<Object> obj;
	while((obj = a.unqueueObject (nullptr)) != nullptr)
	{
		if(HelpReference* r = ccl_cast<HelpReference> (obj))
		{
			if(r->getFileName ().isEmpty ())
				r->setFileName (defaultFile);
			r->setCatalog (this);
			references.addSorted (r);
			r->retain ();
		}
		else if(Attribute* attr = ccl_cast<Attribute> (obj))
		{
			attributes.setAttribute (attr->getID (), attr->getValue (), Attributes::kShare);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpCatalog::save (const Storage& storage) const
{
	CCL_NOT_IMPL ("HelpCatalog::save()")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
void HelpCatalog::dump ()
{
	ArrayForEach (references, HelpReference, ref)
		Debugger::printf ("Help reference \"%s\"\n", MutableCString (ref->getHelpIdentifier ()).str ());
	EndFor
}
#endif

//************************************************************************************************
// HelpReference
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (HelpReference, Object, "HelpReference")

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpReference::HelpReference (StringRef helpid)
: helpid (helpid),
  catalog (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int HelpReference::compare (const Object& obj) const
{
	const HelpReference& other = (const HelpReference&)obj;
	return helpid.compare (other.helpid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpReference::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	helpid = a.getString ("id");
	filename = a.getString ("file");
	destination = a.getString ("dest");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpReference::save (const Storage& storage) const
{
	CCL_NOT_IMPL ("HelpReference::save()")
	return false;
}
