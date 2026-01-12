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
// Filename    : ccl/base/storage/storage.h
// Description : Storage class
//
//************************************************************************************************

#ifndef _ccl_storage_h
#define _ccl_storage_h

#include "ccl/base/storage/archive.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/text/iattributehandler.h"

namespace CCL {

//************************************************************************************************
// Storage
//************************************************************************************************

class Storage: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (Storage, Object)

	Storage (Attributes& attributes, Archive* archive = nullptr)
	: attributes (attributes),
	  archive (archive)
	{}

	Storage (Attributes& attributes, const Storage& storage)
	: attributes (attributes),
	  archive (storage.archive)
	{}

	Attributes& getAttributes () const { return attributes; }
	Archive* getArchive () const { return archive; }

	bool isAnonymous () const;
	StringID getSaveType () const;
	IUnknown* getContextUnknown (StringID id) const;

protected:
	Attributes& attributes;
	Archive* archive;
};

//************************************************************************************************
// OutputStorage
//************************************************************************************************

class OutputStorage: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (OutputStorage, Object)

	OutputStorage (IAttributeHandler& writer)
	: writer (writer)
	{}

	IAttributeHandler& getWriter () const { return writer; }
	IAttributeHandler* operator -> () const { return &writer; }

	bool writeArray (CStringPtr id, const Container& objects) const;

protected:
	IAttributeHandler& writer;
};

//************************************************************************************************
// AttributesBuilder
//************************************************************************************************

class AttributesBuilder: public Object,
						 public IAttributeHandler
{
public:
	AttributesBuilder (Attributes& root, bool initState = true, bool appendMode = false);

	PROPERTY_BOOL (appendMode, AppendMode) ///< append (instead of replace) attributes

	// IAttributeHandler
	tbool CCL_API startObject (StringRef id) override;
	tbool CCL_API endObject (StringRef id) override;
	tbool CCL_API startArray (StringRef id) override;
	tbool CCL_API endArray (StringRef id) override;
	tbool CCL_API setValue (StringRef id, VariantRef value) override;
	tbool CCL_API setValue (CStringPtr id, VariantRef value) override;

	CLASS_INTERFACE (IAttributeHandler, Object)

protected:
	struct State
	{
		bool isObject;
		union
		{
			Attributes* object;
			AttributeQueue* queue;
		};

		State (Attributes* object = nullptr): isObject (true), object (object) {}
		State (AttributeQueue* queue): isObject (false), queue (queue) {}

		void setObjectValue (StringID key, VariantRef value, int flags, bool appendMode)
		{
			if(appendMode)
				object->appendAttribute (key, value, flags);
			else
				object->setAttribute (key, value, flags);
		}
	};

	Vector<State> stateStack;
	State* currentState;
	Attributes& root;

	void pushState (const State& state);
	void popState ();
};

} // namespace CCL

#endif // _ccl_storage_h
