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
// Filename    : ccl/extras/gadgets/gadgetmanager.cpp
// Description : Gadget Manager
//
//************************************************************************************************

#include "ccl/extras/gadgets/gadgetmanager.h"
#include "ccl/extras/gadgets/gadgetdashboard.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/storage.h"

#include "ccl/app/documents/documentmanager.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/plugins/iclassfactory.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/idatatarget.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/iwindowmanager.h"
#include "ccl/public/gui/framework/iworkspace.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/gui/framework/idragndrop.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"

using namespace CCL;
using namespace Install;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("GadgetManager")
	XSTRING (Gadgets, "Gadgets")
	XSTRING (NoGadgetsInstalled, "No Gadgets installed")
END_XSTRINGS

//************************************************************************************************
// GadgetDescription
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (GadgetDescription, StorableObject, "Gadget")

//////////////////////////////////////////////////////////////////////////////////////////////////

GadgetDescription::GadgetDescription ()
: usePerspective (false),
  resetActiveDocument (false),
  menuPriority (1000)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GadgetDescription::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	themeName = a.getCString ("themeName");
	formName = a.getCString ("formName");
	iconName = a.getCString ("iconName");
	menuIconName = a.getString ("menuIconName");
	usePerspective = a.getBool ("usePerspective");
	resetActiveDocument = a.getBool ("resetActiveDocument");
	a.getInt (menuPriority, "menuPriority");

	dashboardFormName = a.getCString ("Dashboard.formName");
	dashboardTitle = a.getString ("Dashboard.title");

	return !themeName.isEmpty () && (!formName.isEmpty () || !dashboardFormName.isEmpty ());
}

//************************************************************************************************
// GadgetItem
//************************************************************************************************

GadgetItem* GadgetItem::createInstance (const IClassDescription& classInfo)
{
	AutoPtr<GadgetItem> gadget;
	IUnknown* unknown = ccl_new<IUnknown> (classInfo.getClassID ());
	ASSERT (unknown)
	if(unknown)
	{
		gadget = NEW GadgetItem (unknown, classInfo.getClassID ());
		gadget->setName (classInfo.getName ());
		gadget->setCommandName (MutableCString (classInfo.getName ()));

		String localizedName;
		classInfo.getLocalizedName (localizedName);
		gadget->setTitle (localizedName);

		// load gadget description
		bool loaded = false;
		GadgetDescription description;
		if(IPluginMetaClass* metaClass = System::GetPlugInManager ().createMetaClass (classInfo.getClassID ()))
		{
			StringID language = System::GetLocaleManager ().getLanguage ();

			Url resourcePath;
			if(metaClass->getResourceLocation (resourcePath, Meta::kClassGadgetResource, language) == kResultOk)
				loaded = description.loadFromFile (resourcePath);

			ccl_release (metaClass);
		}

		ASSERT (loaded == true)
		gadget->setDescription (description);

		// get icons
		ITheme* theme = System::GetThemeManager ().getTheme (description.getThemeName ());
		ASSERT (theme)
		if(theme)
		{
			gadget->setIcon (theme->getImage (description.getIconName ()));
			gadget->setMenuIcon (theme->getImage (description.getMenuIconName ()));
		}
	}
	return gadget.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (GadgetItem, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

GadgetItem::GadgetItem (IUnknown* theGadget, UIDRef cid)
: theGadget (theGadget),
  cid (cid),
  windowClass (nullptr),
  workspace (nullptr),
  perspective (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

GadgetItem::~GadgetItem ()
{
	ASSERT (windowClass == nullptr)
	ASSERT (perspective == nullptr)

	ccl_release (theGadget);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* GadgetItem::getPlugInUnknown ()
{
	return theGadget;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GadgetItem::registerCommand ()
{
	if(!description.hasPopupView ())
		return;

	CommandDescription command (GadgetManager::kCommandCategory, getCommandName ());
	command.displayCategory = XSTR (Gadgets);
	command.displayName = getTitle ();
	command.englishName = getCommandName ();
	command.classID = getClassID ();
	System::GetCommandTable ().registerCommand (command);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString GadgetItem::getWindowClassID () const
{
	MutableCString windowClassId;
	getClassID ().toCString (windowClassId);
	return windowClassId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GadgetItem::registerWindowClass ()
{
	if(!description.hasPopupView ())
		return;

	CString workspaceName (RootComponent::instance ().getApplicationID ());
	MutableCString windowClassId (getWindowClassID ());
	String groupName (CCLSTR ("Popups"));

	if(description.isUsePerspective ())
	{
		workspace = System::GetWorkspaceManager ().getWorkspace (workspaceName);
		ASSERT (workspace != nullptr)
		if(workspace)
		{
			perspective = workspace->clonePerspective ("Gadgets");
			ASSERT (perspective != nullptr)
			if(perspective)
			{
				perspective->retain (); // TODO: workspace manager keeps a reference to *all* perspectives!!!
				groupName = CCLSTR ("Gadgets");
			}
		}
	}

	Url url;
	RootComponent::instance ().makeUrl (url, GadgetManager::instance ().getName ());
	url.descend (getName ());

	String formName (description.getFormName ());
	CString themeName = description.getThemeName ();

	ASSERT (windowClass == nullptr)
	windowClass = System::GetWindowManager ().registerClass (windowClassId, formName, UrlFullString (url), groupName, workspaceName, themeName);
	ASSERT (windowClass)

	if(!description.isUsePerspective ())
		windowClass->setCommand (GadgetManager::kCommandCategory, getCommandName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GadgetItem::unregisterWindowClass ()
{
	if(windowClass)
	{
		System::GetWindowManager ().unregisterClass (windowClass);
		windowClass = nullptr;
	}

	safe_release (perspective);
	workspace = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* GadgetItem::getWindowParam ()
{
	if(!description.hasPopupView ())
		return nullptr;

	if(description.isUsePerspective ())
	{
		return System::GetCommandTable ().getCommandParam (GadgetManager::kCommandCategory, commandName);
	}
	else
	{
		MutableCString windowClassId (getWindowClassID ());
		UnknownPtr<IController> wm (&System::GetWindowManager ());
		return wm->findParameter (windowClassId);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GadgetItem::isViewOpen () const
{
	if(!description.hasPopupView ())
		return false;

	if(description.isUsePerspective ())
		return workspace && workspace->isViewOpen (MutableCString (getWindowClassID ()));
	else
		return System::GetWindowManager ().isWindowOpen (windowClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GadgetItem::openView (bool toggle)
{
	if(!description.hasPopupView ())
		return;

	if(description.isUsePerspective ())
	{
		ASSERT (workspace && perspective)
		if(workspace && perspective)
		{
			MutableCString windowClassId (getWindowClassID ());
			if(workspace->isViewOpen (windowClassId) && toggle)
			{
				// close: back to most recent non-gadget perspective
				AutoPtr<IObjectFilter> filter (ObjectFilter::create ([] (IUnknown* unk)
				{
					UnknownPtr<IPerspective> p (unk);
					return p && !p->getID ().startsWith ("Gadgets:");
				}));

				IPerspective* recentPerspective = workspace->getRecentIPerspective (*filter);
				ASSERT (recentPerspective)
				if(recentPerspective)
					workspace->selectPerspective (recentPerspective);
			}
			else
			{
				workspace->selectPerspective (perspective);
				workspace->openView (windowClassId);

				if(description.isResetActiveDocument ())
					DocumentManager::instance ().setActiveDocument (nullptr);
			}
		}
	}
	else
		System::GetWindowManager ().openWindow (windowClass, toggle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef GadgetItem::getDashboardTitle () const
{
	if(!description.getDashboardTitle ().isEmpty ())
		return description.getDashboardTitle ();
	return getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* GadgetItem::createDashboardView ()
{
	if(!description.hasDashboardView ())
		return nullptr;

	ITheme* theme = nullptr;
	if(description.getThemeName ().isEmpty ())
		theme = RootComponent::instance ().getTheme ();
	else
		theme = System::GetThemeManager ().getTheme (description.getThemeName ());

	return theme ? theme->createView (description.getDashboardFormName (), theGadget) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int GadgetItem::compare (const Object& obj) const
{
	const GadgetItem& other = (const GadgetItem&)obj;
	int prioDiff = description.getMenuPriority () - other.description.getMenuPriority ();
	if(prioDiff)
		return prioDiff;

	return name.compare (other.name); // keep order language-independent
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GadgetItem::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "title")
	{
		var = getTitle ();
		return true;
	}
	if(propertyId == "dashboardTitle")
	{
		var = getDashboardTitle ();
		return true;
	}
	return false;
}

//************************************************************************************************
// GadgetManager::Accessor
//************************************************************************************************

GadgetManager::Accessor::Accessor (const GadgetManager& manager, Mode mode)
{
	ForEach (manager.gadgets, GadgetItem, gadget)
		switch(mode)
		{
		case kPopup : if(gadget->getDescription ().hasPopupView ()) add (gadget); break;
		case kDashboard : if(gadget->getDescription ().hasDashboardView ()) add (gadget); break;
		}
	EndFor
}

//************************************************************************************************
// GadgetManager
//************************************************************************************************

const CString GadgetManager::kCommandCategory ("Gadgets");

DEFINE_CLASS_HIDDEN (GadgetManager, Component)
DEFINE_COMPONENT_SINGLETON (GadgetManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

GadgetManager::GadgetManager ()
: Component (String ("GadgetManager"))
{
	gadgets.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GadgetManager::~GadgetManager ()
{
	ASSERT (gadgets.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API GadgetManager::getObject (StringID name, UIDRef classID)
{
	if(name == "GadgetBox")
		return this->asUnknown ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GadgetManager::startup ()
{
	ForEachPlugInClass (PLUG_CATEGORY_GADGET, description)
		GadgetItem* gadget = GadgetItem::createInstance (description);
		if(gadget)
		{
			// register with command table
			gadget->registerCommand ();

			// sort by menu priority
			gadgets.addSorted (gadget);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GadgetManager::addDashboardGadget (StringRef name, StringRef title, StringID formName, int position)
{
	GadgetItem* gadget = NEW GadgetItem (nullptr, kNullUID);
	gadget->setName (name);
	gadget->setTitle (title);

	GadgetDescription description;
	description.setDashboardFormName (formName);
	gadget->setDescription (description);

	if(!gadgets.insertAt (position, gadget))
		gadgets.add (gadget);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GadgetManager::initialize (IUnknown* context)
{
	ForEach (gadgets, GadgetItem, gadget)
		gadget->registerWindowClass ();

		UnknownPtr<IComponent> iComponent (gadget->getPlugInUnknown ());
		if(iComponent)
			iComponent->initialize (this->asUnknown ());
	EndFor

	// add dashboard component
	GadgetDashboard* dashboard = NEW GadgetDashboard ();
	addComponent (dashboard);
	Accessor dashboardList (*this, Accessor::kDashboard);
	ForEach (dashboardList, GadgetItem, gadget)
		dashboard->addGadget (gadget);
	EndFor

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GadgetManager::terminate ()
{
	if(getContext () != nullptr) // otherwise it's an early program exit
	{
		// store selected tab
		paramList.storeSettings ("Gadgets");

		ForEach (gadgets, GadgetItem, gadget)
			UnknownPtr<IComponent> iComponent (gadget->getPlugInUnknown ());
			if(iComponent)
				iComponent->terminate ();

			gadget->unregisterWindowClass ();
		EndFor
	}

	gadgets.removeAll ();

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GadgetManager::extendMenu (IMenu& menu)
{
	int count = 0;
	ForEach (gadgets, GadgetItem, gadget)
		if(gadget->getDescription ().hasPopupView ())
		{
			IMenuItem* menuItem = menu.addCommandItem (gadget->getTitle (), kCommandCategory, gadget->getCommandName ());
			menuItem->setItemAttribute (IMenuItem::kItemIcon, gadget->getMenuIcon ());
			count++;
		}
	EndFor

	if(count == 0)
		menu.addCommandItem (XSTR (NoGadgetsInstalled));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API GadgetManager::findChild (StringRef id) const
{
	ForEach (gadgets, GadgetItem, gadget)
		if(gadget->getName () == id)
		{
			UnknownPtr<IObjectNode> iNode (gadget->getPlugInUnknown ());
			ASSERT (iNode)
			return iNode;
		}
	EndFor
	return SuperClass::findChild (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GadgetManager::getChildDelegates (IMutableArray& delegates) const
{
	ForEach (gadgets, GadgetItem, gadget)
		if(UnknownPtr<IObjectNode> (gadget->getPlugInUnknown ()).isValid ())
			delegates.addArrayElement (gadget->getName ());
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GadgetItem* GadgetManager::findGadgetItem (IUnknown* unknown) const
{
	ForEach (gadgets, GadgetItem, gadget)
		if(isEqualUnknown (gadget->getPlugInUnknown (), unknown))
			return gadget;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GadgetItem* GadgetManager::findGadgetWithCommand (StringID commandName) const
{
	ForEach (gadgets, GadgetItem, gadget)
		if(gadget->getCommandName () == commandName)
			return gadget;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API GadgetManager::openGadget (IUnknown* unknown)
{
	if(GadgetItem* gadget = findGadgetItem (unknown))
	{
		gadget->openView ();
		return kResultOk;
	}
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API GadgetManager::getGadgetWindowParam (IUnknown* unknown)
{
	if(GadgetItem* gadget = findGadgetItem (unknown))
		return gadget->getWindowParam ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GadgetManager::checkCommandCategory (CStringRef category) const
{
	return category == kCommandCategory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GadgetManager::interpretCommand (const CommandMsg& msg)
{
	if(msg.category != kCommandCategory)
		return false;

	GadgetItem* gadget = findGadgetWithCommand (msg.name);
	if(!gadget)
		return false;

	if(!gadget->getDescription ().isUsePerspective ())
		return false;

	if(msg.checkOnly ())
	{
		UnknownPtr<IMenuItem> menuItem (msg.invoker);
		if(menuItem)
		{
			bool checked = gadget->isViewOpen ();
			menuItem->setItemAttribute (IMenuItem::kItemChecked, checked);
		}
	}
	else
		gadget->openView ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API GadgetManager::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "GadgetBoxItem")
	{
		GadgetItem* gadget = unknown_cast<GadgetItem> (data.asUnknown ());
		ASSERT (gadget != nullptr)
		if(gadget == nullptr)
			return nullptr;

		IParameter* windowParam = gadget->getWindowParam ();
		ASSERT (windowParam != nullptr)

		Rect iconRect;
		IImage* icon = gadget->getIcon ();

		ASSERT (icon != nullptr)
		if(icon)
			iconRect (0, 0, icon->getWidth (), icon->getHeight ());
		else
			iconRect (0, 0, 32, 32);

		FormBox form (iconRect);
		form.setController (gadget->asUnknown ()); // controller is required by DropBox control!

		bool toggleNeeded = !gadget->getDescription ().isUsePerspective (); // perspective uses command parameter
		ControlBox button (toggleNeeded ? ClassID::Toggle : ClassID::Button, windowParam, iconRect);

		AutoPtr<IVisualStyle> visualStyle = ccl_new<IVisualStyle> (ClassID::VisualStyle);
		visualStyle->setImage (StyleID::kBackground, icon);
		button.setVisualStyle (visualStyle);

		button.setTooltip (String () << gadget->getTitle () << " @cmd[" << kCommandCategory << "|" << gadget->getCommandName () << "]");
		button.setSizeMode (IView::kVCenter);
		form.getChildren ().add (button);

		if(IView* divider = RootComponent::instance ().getTheme ()->createView ("CCL/GadgetDivider", this->asUnknown ()))
		{
			Rect dividerRect (divider->getSize ());
			dividerRect.offset (iconRect.right);
			divider->setSize (dividerRect);
			form.getChildren ().add (divider);

			Rect combinedRect (iconRect);
			combinedRect.right += dividerRect.getWidth ();
			form.setSize (combinedRect);
		}

		return form;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GadgetManager::getSubItems (IUnknownList& items, ItemIndexRef index)
{
	ASSERT (index.getObject () == nullptr)

	Accessor popupList (*this, Accessor::kPopup);
	ForEach (popupList, GadgetItem, gadget)
		items.add (gadget->asUnknown (), true);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GadgetManager::canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView)
{
	Accessor popupList (*this, Accessor::kPopup);
	GadgetItem* gadget = (GadgetItem*)popupList.at (index.getIndex ());
	if(gadget)
	{
		// TODO: we need a special drag handler here to check insert per gadget item!
		UnknownPtr<IDataTarget> dataTarget (gadget->getPlugInUnknown ());
		if(dataTarget)
		{
			SharedPtr<IDragHandler> oldHandler = session ? session->getDragHandler () : nullptr;
			if(dataTarget->canInsertData (data, session, targetView))
			{
				if(session->getResult () == IDragSession::kDropNone)
					session->setResult (IDragSession::kDropCopyReal);

				if(session->getDragHandler () == oldHandler)
					if(UnknownPtr<IItemView> itemView = targetView)
						session->setDragHandler (AutoPtr<IDragHandler> (itemView->createDragHandler (IItemView::kCanDragOnItem|IItemView::kDropInsertsData)));
				return true;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GadgetManager::insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session)
{
	Accessor popupList (*this, Accessor::kPopup);
	GadgetItem* gadget = (GadgetItem*)popupList.at (index.getIndex ());
	if(gadget)
	{
		UnknownPtr<IDataTarget> dataTarget (gadget->getPlugInUnknown ());
		if(dataTarget)
			if(dataTarget->insertData (data, session))
				return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GadgetManager::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "dashboardCount")
	{
		var = Accessor (*this, Accessor::kDashboard).count ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}
