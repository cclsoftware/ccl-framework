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
// Filename    : ccl/app/editing/addins/editaddincollection.h
// Description : Edit Add-in Collection
//
//************************************************************************************************

#ifndef _ccl_editaddincollection_h
#define _ccl_editaddincollection_h

#include "ccl/app/component.h"

#include "ccl/base/storage/isettings.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/graphics/iimage.h"

namespace CCL {

interface IMenu;
interface IEditEnvironment;
interface IClassDescription;

//************************************************************************************************
// EditAddInCollection
//************************************************************************************************

class EditAddInCollection: public Component,
						   public ISettingsSaver
{
public:
	DECLARE_CLASS (EditAddInCollection, Component)

	EditAddInCollection (StringRef name = nullptr);
	~EditAddInCollection ();

	static void makeMainMenu (IMenu& menu, StringRef subCategory); // for all available add-ins
	static void removeFromMenu (IMenu& menu);

	void collectAddIns (StringRef subCategory, IEditEnvironment* environment);
	void makeMainMenu (IMenu& menu); // for add-ins of this collection

	// ISettingsSaver
	void restore (Settings&) override;
	void flush (Settings&) override;

	// Component
	tresult CCL_API terminate () override;
	IParameter* CCL_API findParameter (StringID name) const override;
	IObjectNode* CCL_API findChild (StringRef id) const override;
	tbool CCL_API getChildDelegates (IMutableArray& delegates) const override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	CLASS_INTERFACE (ISettingsSaver, Component)

protected:
	class AddInItem: public Object
	{
	public:
		AddInItem (IUnknown* unknown, const IClassDescription& classInfo);

		PROPERTY_MUTABLE_CSTRING (id, ID)
		PROPERTY_STRING (name, Name)
		PROPERTY_STRING (title, Title)
		PROPERTY_VARIABLE (int, menuPriority, MenuPriority)
		PROPERTY_POINTER (IUnknown, unknown, PlugInUnknown)

		PROPERTY_SHARED_AUTO (IImage, icon, Icon)
		PROPERTY_MUTABLE_CSTRING (windowClassId, WindowClassID)
		PROPERTY_MUTABLE_CSTRING (commandCategory, CommandCategory)
		PROPERTY_MUTABLE_CSTRING (commandName, CommandName)
		PROPERTY_STRING (groupID, GroupID)

		// Object
		int compare (const Object& obj) const override;
	};

	ObjectArray addIns;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

} // namespace CCL

#endif // _ccl_editaddincollection_h
