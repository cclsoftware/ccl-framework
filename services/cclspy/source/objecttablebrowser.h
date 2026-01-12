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
// Filename    : objecttablebrowser.h
// Description : Object table browser
//
//************************************************************************************************

#ifndef _objecttablebrowser_h
#define _objecttablebrowser_h

#include "ccl/base/objectnode.h"

#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/framework/iitemmodel.h"
#include "ccl/public/plugins/iobjecttable.h"

namespace CCL {
interface IDocumentManager; 
class Attributes; }

namespace Spy {

class ObjectTableBrowser;
class PropertyList;

//************************************************************************************************
// ObjectItem
//************************************************************************************************

class ObjectItem: public CCL::Object
{
public:
	DECLARE_CLASS_ABSTRACT (ObjectItem, Object)

	enum ObjectType
	{
		kTreeRoot,
		kObjectTable,
		kGeneric,
		kProperty,
		kTypeRegistry,
		kTypeLib,
		kDocumentManager
	};

	enum SubType
	{
		kNone,
		kModule,
		kDelegate
	};

	ObjectItem (ObjectType type, CCL::StringRef title = nullptr);

	PROPERTY_VARIABLE (ObjectType, type, Type)
	PROPERTY_VARIABLE (SubType, subType, SubType)
	PROPERTY_STRING (title, Title)
	PROPERTY_STRING (address, Address)

	PROPERTY_SHARED_AUTO (CCL::IImage, icon, Icon)

	void assign (IUnknown* obj);

	bool isStructuralItem () const;
	IUnknown* getAliveObject () const;

	void getProperties (PropertyList& propertyList);
};

//************************************************************************************************
// ObjectTableItemModel
//************************************************************************************************

class ObjectTableItemModel: public CCL::Object,
						    public CCL::AbstractItemModel
{
public:
	ObjectTableItemModel (ObjectTableBrowser& browser);
	~ObjectTableItemModel ();

	// IItemModel
	CCL::tbool CCL_API getRootItem (CCL::ItemIndex& index) override;
	CCL::tbool CCL_API getSubItems (CCL::IUnknownList& items, CCL::ItemIndexRef index) override;
	CCL::tbool CCL_API canExpandItem (CCL::ItemIndexRef index) override;
	CCL::tbool CCL_API getItemTitle (CCL::String& title, CCL::ItemIndexRef index) override;
	CCL::IImage* CCL_API getItemIcon (CCL::ItemIndexRef index) override;
	CCL::tbool CCL_API onItemFocused (CCL::ItemIndexRef index) override;
	void CCL_API viewAttached (CCL::IItemView* itemView) override;
	void CCL_API viewDetached (CCL::IItemView* itemView) override;
	CCL::tbool CCL_API appendItemMenu (CCL::IContextMenu& menu, CCL::ItemIndexRef index, const CCL::IItemSelection& selection) override;

	// Object
	void CCL_API notify (ISubject* subject, CCL::MessageRef msg) override;

	// Commands
	bool onItemCommand (CCL::CmdArgs args, CCL::VariantRef data);

	CLASS_INTERFACE (IItemModel, Object)

protected:
	CCL::IObjectTable& objectTable;
	ObjectTableBrowser& browser;
	CCL::IItemView* itemView;
	ObjectItem& rootItem;
	CCL::IDocumentManager* documentManager;

	void refreshAll ();

	ObjectItem* resolve (CCL::ItemIndexRef index) const;
};

//************************************************************************************************
// ObjectTableBrowser
//************************************************************************************************

class ObjectTableBrowser: public CCL::ObjectNode,
						  public CCL::AbstractController
{
public:
	ObjectTableBrowser ();

	DECLARE_CLASS (ObjectTableBrowser, ObjectNode)

	// IController
	IUnknown* CCL_API getObject (CCL::StringID name, CCL::UIDRef classID) override;

	CLASS_INTERFACE (IController, ObjectNode)

private:
	CCL::AutoPtr<ObjectTableItemModel> objectTableModel;
};

} // namespace Spy

#endif // _objecttablebrowser_h
