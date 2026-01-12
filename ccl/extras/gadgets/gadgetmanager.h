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
// Filename    : ccl/extras/gadgets/gadgetmanager.h
// Description : Gadget Manager
//
//************************************************************************************************

#ifndef _ccl_gadgetmanager_h
#define _ccl_gadgetmanager_h

#include "ccl/app/component.h"

#include "ccl/base/storage/storableobject.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/extras/gadgets.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/framework/iitemmodel.h"

namespace CCL {

interface IMenu;
interface IWorkspace;
interface IPerspective;
interface IWindowClass;
interface IClassDescription;

namespace Install {

//************************************************************************************************
// GadgetDescription
//************************************************************************************************

class GadgetDescription: public StorableObject
{
public:
	DECLARE_CLASS (GadgetDescription, StorableObject)

	GadgetDescription ();

	PROPERTY_MUTABLE_CSTRING (themeName, ThemeName)
	PROPERTY_MUTABLE_CSTRING (formName, FormName)
	PROPERTY_MUTABLE_CSTRING (iconName, IconName)
	PROPERTY_MUTABLE_CSTRING (menuIconName, MenuIconName)
	PROPERTY_BOOL (usePerspective, UsePerspective)
	PROPERTY_BOOL (resetActiveDocument, ResetActiveDocument) ///< in combination with usePerspective: reset active document when perspective is selected
	PROPERTY_VARIABLE (int, menuPriority, MenuPriority) ///< lower values first

	PROPERTY_MUTABLE_CSTRING (dashboardFormName, DashboardFormName)
	PROPERTY_STRING (dashboardTitle, DashboardTitle)

	bool hasPopupView () const		{ return !formName.isEmpty (); }
	bool hasDashboardView () const	{ return !dashboardFormName.isEmpty (); }

	// StorableObject
	bool load (const Storage& storage) override;
};

//************************************************************************************************
// GadgetItem
//************************************************************************************************

class GadgetItem: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (GadgetItem, Object)

	static GadgetItem* createInstance (const IClassDescription&);

	IUnknown* getPlugInUnknown ();

	PROPERTY_STRING (name, Name)
	PROPERTY_STRING (title, Title)
	PROPERTY_OBJECT (UID, cid, ClassID)

	PROPERTY_OBJECT (GadgetDescription, description, Description)
	PROPERTY_MUTABLE_CSTRING (commandName, CommandName)

	PROPERTY_SHARED_AUTO (IImage, icon, Icon)
	PROPERTY_SHARED_AUTO (IImage, menuIcon, MenuIcon)
	PROPERTY_POINTER (IWorkspace, workspace, Workspace)
	PROPERTY_POINTER (IPerspective, perspective, Perspective)
	PROPERTY_POINTER (IWindowClass, windowClass, WindowClass)

	void registerCommand ();
	MutableCString getWindowClassID () const;
	void registerWindowClass ();
	void unregisterWindowClass ();
	IParameter* getWindowParam ();
	bool isViewOpen () const;
	void openView (bool toggle = true);

	StringRef getDashboardTitle () const;
	IView* createDashboardView ();

	// Object
	int compare (const Object& obj) const override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

protected:
	IUnknown* theGadget;

	friend class GadgetManager;
	GadgetItem (IUnknown* theGadget, UIDRef cid);
	~GadgetItem ();
};

//************************************************************************************************
// GadgetManager
//************************************************************************************************

class GadgetManager: public Component,
					 public IGadgetSite,
					 public ItemViewObserver<AbstractItemModel>,
					 public ComponentSingleton<GadgetManager>
{
public:
	DECLARE_CLASS (GadgetManager, Component)

	GadgetManager ();
	~GadgetManager ();

	static const CString kCommandCategory;

	void startup ();
	void addDashboardGadget (StringRef name, StringRef title, StringID formName, int position = -1);

	void extendMenu (IMenu& menu);

	// IGadgetSite
	tresult CCL_API openGadget (IUnknown* gadget) override;
	IParameter* CCL_API getGadgetWindowParam (IUnknown* gadget) override;

	// Component
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	IObjectNode* CCL_API findChild (StringRef id) const override;
	tbool CCL_API getChildDelegates (IMutableArray& delegates) const override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	CLASS_INTERFACE2 (IGadgetSite, IItemModel, Component)

protected:
	ObjectArray gadgets;

	class Accessor: public ObjectArray
	{
	public:
		enum Mode { kPopup, kDashboard };
		Accessor (const GadgetManager& manager, Mode mode);
	};

	GadgetItem* findGadgetItem (IUnknown* gadget) const;
	GadgetItem* findGadgetWithCommand (StringID commandName) const;

	// IItemModel
	tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) override;
	tbool CCL_API canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView = nullptr) override;
	tbool CCL_API insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session) override;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

} // namespace Install
} // namespace CCL

#endif // _ccl_gadgetmanager_h
