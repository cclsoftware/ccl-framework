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
// Filename    : ccl/base/storage/archive.cpp
// Description : Archive base class
//
//************************************************************************************************

#include "ccl/base/storage/storage.h"

using namespace CCL;

//************************************************************************************************
// Storage
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Storage, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Storage::isAnonymous () const
{
	ASSERT (archive != nullptr)
	return archive && archive->isAnonymous ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID Storage::getSaveType () const
{	
	return archive ? archive->getSaveType () : CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* Storage::getContextUnknown (StringID id) const
{
	//ASSERT (archive != 0)
	return archive && archive->getContext () ? archive->getContext ()->getUnknown (id) : nullptr;
}

//************************************************************************************************
// OutputStorage
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (OutputStorage, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OutputStorage::writeArray (CStringPtr id, const Container& objects) const
{
	writer.startArray (id);

	ForEach (objects, Object, obj)
		writer.startObject (nullptr);
		if(!obj->save (writer))
			return false;
		writer.endObject (nullptr);
	EndFor

	writer.endArray (id);
	return true;
}

//************************************************************************************************
// Archive
//************************************************************************************************

StringID Archive::kSaveTypeUndo ("undo");
StringID Archive::kSaveTypeCopy ("copy");
StringID Archive::kSaveTypePreview ("preview");

//////////////////////////////////////////////////////////////////////////////////////////////////

Archive::Archive (IStream& stream, Attributes* _context, StringID _saveType)
: stream (stream),
  context (nullptr),
  saveType (_saveType),
  flags (0)
{
	if(_context)
		setContext (_context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Archive::~Archive ()
{
	if(context)
		context->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream& Archive::getStream ()
{ 
	return stream; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Archive::setContext (Attributes* _context)
{
	take_shared<Attributes> (context, _context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes* Archive::getContext ()
{
	return context;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Archive::saveObject (ObjectID name, const Object& object)
{
	Attributes attributes;
	Storage storage (attributes, this);
	if(!object.save (storage))
		return false;

	return saveAttributes (name, attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Archive::loadObject (ObjectID name, Object& object)
{
	Attributes attributes;
	if(!loadAttributes (name, attributes))
		return false;

	Storage storage (attributes, this);
	return object.load (storage) ? true : false;
}
