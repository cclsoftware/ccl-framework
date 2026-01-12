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
// Filename    : ccl/app/components/helpcomponent.cpp
// Description : Help Component
//
//************************************************************************************************

#include "ccl/app/components/helpcomponent.h"

#include "ccl/app/controls/usercontrol.h"

#include "ccl/public/gui/framework/ipresentable.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/guiservices.h"

namespace CCL {

//************************************************************************************************
// HelpInfoControl
//************************************************************************************************

class HelpInfoControl: public UserControl
{
public:
	DECLARE_CLASS (HelpInfoControl, UserControl)

	HelpInfoControl (HelpInfoViewComponent* component = nullptr, RectRef size = Rect ());

	// UserControl
	void attached (IView* parent) override;
	void removed (IView* parent) override;
	bool onContextMenu (const ContextMenuEvent& event) override;

protected:
	SharedPtr<HelpInfoViewComponent> component;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// HelpCatalogComponent
//************************************************************************************************

const CString HelpCatalogComponent::kCatalogPrefix ("Show Catalog ");
const CString HelpCatalogComponent::kLocationPrefix ("Show Location ");
const int HelpCatalogComponent::kCommandIndexStart = 1;

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpCatalogComponent::makeMainMenu (IMenu& menu, StringID category)
{
	makeMenu (menu, nullptr, category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpCatalogComponent::makeMenu (IMenu& menu, ICommandHandler* handler, StringID category)
{
	IterForEachUnknown (System::GetHelpManager ().newCatalogIterator (), unk)
		if(UnknownPtr<IHelpCatalog> catalog = unk)
		{
			if(category != catalog->getCategory ())
				continue;

			MutableCString commandName (kCatalogPrefix);
			commandName.appendFormat ("%d", commandIndex++);

			if(!catalog->getTitle ().isEmpty ()) // ignore hidden catalogs used for overwrite
				menu.addCommandItem (catalog->getTitle (), "Help", commandName, handler);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpCatalogComponent::HelpCatalogComponent ()
: Component (CCLSTR ("HelpCatalogs")),
  commandIndex (kCommandIndexStart)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpCatalogComponent::appendCatalogMenu (IMenu& menu, StringID category)
{
	makeMenu (menu, this, category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HelpCatalogComponent::checkCommandCategory (CStringRef category) const
{
	return category == "Help";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HelpCatalogComponent::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == "Help")
	{
		// Help Catalog
		if(msg.name.startsWith (kCatalogPrefix))
		{
			int64 index = -1;
			if(!msg.name.subString (kCatalogPrefix.length ()).getIntValue (index))
				return false;

			if(!msg.checkOnly ())
			{
				int i = kCommandIndexStart;
				IterForEachUnknown (System::GetHelpManager ().newCatalogIterator (), unk)
					if(UnknownPtr<IHelpCatalog> catalog = unk)
						if(i++ == index)
						{
							System::GetHelpManager ().showHelpCatalog (catalog);
							break;
						}
				EndFor
			}
			return true;
		}
		// Help Location
		else if(msg.name.startsWith (kLocationPrefix))
		{
			if(!msg.checkOnly ())
			{
				MutableCString location = msg.name.subString (kLocationPrefix.length ());
				System::GetHelpManager ().showLocation (String (location));
			}
			return true;
		}
	}
	return false;
}

//************************************************************************************************
// HelpTutorialComponent
//************************************************************************************************

const CString HelpTutorialComponent::kTutorialPrefix ("Show Tutorial ");
const int HelpTutorialComponent::kCommandIndexStart = 1;

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpTutorialComponent::makeMenu (IMenu& menu, CStringRef categoryFilter)
{
	String category (categoryFilter);

	int i = kCommandIndexStart;
	IterForEachUnknown (System::GetHelpManager ().newTutorialIterator (), unk)
		if(UnknownPtr<IHelpTutorial> tutorial = unk)
		{
			MutableCString commandName (kTutorialPrefix);
			commandName.appendFormat ("%d", i++);

			if(tutorial->getTitle ().isEmpty ())
				continue;

			if(!category.isEmpty () && tutorial->getCategory () != category)
				continue;

			menu.addCommandItem (tutorial->getTitle (), "Help", commandName);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpTutorialComponent::HelpTutorialComponent ()
: Component (CCLSTR ("HelpTutorials"))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HelpTutorialComponent::checkCommandCategory (CStringRef category) const
{
	return category == "Help";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HelpTutorialComponent::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == "Help")
	{
		if(msg.name.startsWith (kTutorialPrefix))
		{
			int64 index = -1;
			if(!msg.name.subString (kTutorialPrefix.length ()).getIntValue (index))
				return false;

			if(!msg.checkOnly ())
			{
				int i = kCommandIndexStart;
				IterForEachUnknown (System::GetHelpManager ().newTutorialIterator (), unk)
					if(UnknownPtr<IHelpTutorial> tutorial = unk)
						if(i++ == index)
						{
							System::GetHelpManager ().showTutorial (tutorial->getID ());
							break;
						}
				EndFor
			}
			return true;
		}
	}
	return false;
}

//************************************************************************************************
// HelpInfoViewComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (HelpInfoViewComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpInfoViewComponent::HelpInfoViewComponent ()
: Component (CCLSTR ("InfoView")),
  viewCount (0),
  active (false),
  lastModifiers (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpInfoViewComponent::~HelpInfoViewComponent ()
{
	setActive (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpInfoViewComponent::setActive (bool state)
{
	if(active != state)
	{
		if(active)
		{
			System::GetHelpManager ().removeInfoViewer (this);
			//System::GetGUI ().removeHandler (this);
			System::GetGUI ().removeIdleTask (this);
		}
		active = state;
		if(active)
		{
			System::GetHelpManager ().addInfoViewer (this);
			//System::GetGUI ().addHandler (this);
			System::GetGUI ().addIdleTask (this);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpInfoViewComponent::viewAttached ()
{
	if(viewCount == 0)
		setActive (true);
	viewCount++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpInfoViewComponent::viewDetached ()
{
	viewCount--;
	if(viewCount == 0)
		setActive (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API HelpInfoViewComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "InfoView")
	{
		infoView = *NEW HelpInfoControl (this, bounds);
		updateInfoView ();
		return infoView;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API HelpInfoViewComponent::updateHelpInfo (IPresentable* info)
{
	currentInfo = info;
	updateInfoView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpInfoViewComponent::updateInfoView ()
{
	if(infoView)
	{
		ViewBox vb (infoView);
		vb.getChildren ().removeAll ();

		if(currentInfo)
		{
			// Note: Help info might be shared among different viewers,
			// modifiers are only valid for our own representation!
			UnknownPtr<IHelpInfoBuilder> builder (currentInfo);
			if(builder)
				builder->setActiveOption (lastModifiers);

			Rect bounds;
			vb.getClientRect (bounds);
			IView* content = currentInfo->createView (bounds, vb.getVisualStyle ());
			if(content)
				vb.getChildren ().add (content);

			if(builder)
				builder->setActiveOption (0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HelpInfoViewComponent::handleEvent (IWindow* window, const GUIEvent& event)
{
	if(event.eventClass == GUIEvent::kKeyEvent)
	{
		uint32 modifiers = System::GetGUI ().getLastKeyState ().getModifiers ();
		if(modifiers != lastModifiers)
		{
			lastModifiers = modifiers;
			updateInfoView ();
		}
	}
	return false; // do not swallow event
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API HelpInfoViewComponent::onTimer (ITimer* timer)
{
	KeyState state;
	System::GetGUI ().getKeyState (state);
	uint32 modifiers = state.getModifiers ();
	if(modifiers != lastModifiers)
	{
		lastModifiers = modifiers;
		updateInfoView ();
	}
}

//************************************************************************************************
// HelpInfoControl
//************************************************************************************************

DEFINE_CLASS_HIDDEN (HelpInfoControl, UserControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpInfoControl::HelpInfoControl (HelpInfoViewComponent* component, RectRef size)
: UserControl (size),
  component (component)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpInfoControl::attached (IView* parent)
{
	SuperClass::attached (parent);

	if(component)
		component->viewAttached ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HelpInfoControl::removed (IView* parent)
{
	if(component)
		component->viewDetached ();

	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpInfoControl::onContextMenu (const ContextMenuEvent& event)
{
	if(component)
	{
		// delegate to current presentable
		UnknownPtr<IContextMenuHandler> handler (component->currentInfo);
		if(handler)
			handler->appendContextMenu (event.contextMenu);
	}
	return SuperClass::onContextMenu (event);
}
