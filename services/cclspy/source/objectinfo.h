//************************************************************************************************
//
// CCL Spy
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
// Filename    : objectinfo.h
// Description : Object info & item model
//
//************************************************************************************************

#ifndef _objectinfo_h
#define _objectinfo_h

#include "ccl/base/storage/attributes.h"

#include "ccl/app/controls/itemviewmodel.h"

#include "ccl/public/gui/commanddispatch.h"

using namespace CCL;

namespace Spy {

//************************************************************************************************
// PropertyHandler
//************************************************************************************************

class PropertyHandler: public Object
{
public:
	typedef IItemModel::DrawInfo DrawInfo;

	struct EditContext
	{
		const IItemModel::EditInfo& editInfo;	///< in
		AutoPtr<IUnknown> objectToInspect;		///< out, optional - edit capability is kObjectLink

		EditContext (const IItemModel::EditInfo& editInfo)
		: editInfo (editInfo)
		{}
	};
	
	enum EditType 
	{ 
		kNoEdit,
		kObjectLink,
		kCustomLink,
		kStringEdit,
		kNumericEdit,
		kColorEdit,
		kCustomEdit
	};

	virtual void toString (String& string, VariantRef value)	{ value.toString (string); }
	virtual bool draw  (VariantRef value, const DrawInfo& info)	{ return false; }
	virtual int getEditCapability (VariantRef value)			{ return kNoEdit; }
	virtual bool edit (VariantRef value, EditContext& context)	{ return false; }

	static PropertyHandler* getNumericHandler () ///< default handler for numeric editing
	{
		class NumericHandler: public PropertyHandler
		{
		public:
			int getEditCapability (VariantRef value) override { return kNumericEdit; }
		};
		static AutoPtr<PropertyHandler> theHandler = NEW NumericHandler;
		return theHandler;
	}

	static PropertyHandler* getStringHandler () ///< default handler for string editing
	{
		class StringHandler: public PropertyHandler
		{
		public:
			int getEditCapability (VariantRef value) override { return kStringEdit; }
		};
		static AutoPtr<PropertyHandler> theHandler = NEW StringHandler;
		return theHandler;
	}
};

//************************************************************************************************
// PropertyList
//************************************************************************************************

class PropertyList: public Object
{
public:
	PropertyList (StringID name): name (name) { properties.objectCleanup (true); }

	PROPERTY_MUTABLE_CSTRING (name, Name)

	class Property;

	void setProperty (StringID id, VariantRef value, PropertyHandler* handler = nullptr);
	Property* getProperty (StringID id) const;
	Property* getPropertyAt (int index) const { return (Property*)properties.at (index); }
	int countProperties () const { return properties.count (); }

private:
	ObjectArray properties;
};

//************************************************************************************************
// PropertyList::Property
//************************************************************************************************

class PropertyList::Property: public Attribute
{
public:
	DECLARE_CLASS_ABSTRACT (PropertyList::Property, Attribute)

	Property (StringID id) : Attribute (id) {}

	PROPERTY_SHARED_AUTO (PropertyHandler, handler, Handler)
};

//************************************************************************************************
// ObjectInfo
//************************************************************************************************

class ObjectInfo: public Object
{
public:
	ObjectInfo (IUnknown* object);
	~ObjectInfo ();

	IUnknown* getObject () { return object; }

	void addProperty (StringID path, VariantRef value, PropertyHandler* handler = nullptr); ///< path like "group/attribute"
	void addObjectProperty (IUnknown* object, MemberID propertyId, StringID path = nullptr, PropertyHandler* handler = nullptr);
	void addObjectProperty (MemberID propertyId, PropertyHandler* handler = nullptr);
	String getPropertyString (StringID path);

	PropertyList* getGroup (StringID name, bool create = true);
	PropertyList* getGroupAt (int index);

	// Object
	void CCL_API notify (ISubject* s, MessageRef msg) override;

private:
	IUnknown* object;
	ISubject* subject;
	ObjectArray groups;
};

//************************************************************************************************
// PropertiesItemModel
//************************************************************************************************

class PropertiesItemModel: public CCL::ItemModel
{
public:
	PropertiesItemModel ();

	PROPERTY_SHARED_AUTO (PropertyList, properties, Properties)

	enum Columns
	{
		kKey,
		kValue
	};

	// ItemModel
	int CCL_API countFlatItems () override;
	tbool CCL_API getItemTitle (String& title, CCL::ItemIndexRef index) override;
	tbool CCL_API createColumnHeaders (CCL::IColumnHeaderList& list) override;
	tbool CCL_API editCell (CCL::ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API drawCell (CCL::ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API appendItemMenu (CCL::IContextMenu& menu, CCL::ItemIndexRef index, const CCL::IItemSelection& selection) override;

	// Commands
	bool onPropertyCommand (CCL::CmdArgs args, CCL::VariantRef data);
};

} // namespace Spy

#endif // _objectinfo_h
