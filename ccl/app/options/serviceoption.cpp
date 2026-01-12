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
// Filename    : ccl/app/options/serviceoption.cpp
// Description : Service Option
//
//************************************************************************************************

#include "ccl/app/options/serviceoption.h"

#include "ccl/app/utilities/pluginclass.h"
#include "ccl/app/controls/itemviewmodel.h"
#include "ccl/public/app/signals.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/plugins/iservicemanager.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// ServiceListModel
//************************************************************************************************

class ServiceListModel: public ItemModel
{
public:
	ServiceListModel ();

	enum Columns
	{
		kIcon,
		kName,
		kCheck,
		kNumColumns
	};

	PROPERTY_BOOL (warned, Warned)

	void updateList ();
	void removeAll ();

	// IItemModel
	int CCL_API countFlatItems () override;
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override;
	tbool CCL_API createColumnHeaders (IColumnHeaderList& list) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API measureCellContent (Rect& size, ItemIndexRef index, int column, const StyleInfo& info) override;

	CLASS_INTERFACE (IItemModel, Object)

protected:
	Vector<const IServiceDescription*> services;

	Rect& getButtonRect (Rect& buttonRect, RectRef itemRect) const;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("UserOption")
	XSTRING (Services, "Services")
	XSTRING (ServiceWarning, "Your changes will be applied next time you start $APPNAME.")
	XSTRING (Enable, "Enable")
	XSTRING (Disable, "Disable")
END_XSTRINGS

//************************************************************************************************
// ServiceOption
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ServiceOption, UserOption)

//////////////////////////////////////////////////////////////////////////////////////////////////

ServiceOption::ServiceOption ()
: UserOption (CCLSTR ("ServiceOption")),
  serviceList (NEW ServiceListModel)
{
	setTitle (String () << Advanced () << strSeparator << XSTR (Services));

	setFormName ("CCL/ServiceOption");

	paramList.addParam ("showServices");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ServiceOption::~ServiceOption ()
{
	serviceList->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ServiceOption::opened ()
{
	SuperClass::opened ();

	paramList.lookup ("showServices")->setValue (0);

	serviceList->setWarned (false);
	serviceList->updateList ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ServiceOption::closed ()
{
	serviceList->removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ServiceOption::getObject (StringID name, UIDRef classID)
{
	if(name == "serviceList")
		return (IItemModel*)serviceList;
	return nullptr;
}

//************************************************************************************************
// ServiceListModel
//************************************************************************************************

ServiceListModel::ServiceListModel ()
: warned (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ServiceListModel::updateList ()
{
	services.removeAll ();
	int count = System::GetServiceManager ().countServices ();
	for(int i = 0; i < count; i++)
	{
		const IServiceDescription* description = System::GetServiceManager ().getService (i);
		if(description->isUserService ())
			services.add (description);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ServiceListModel::removeAll ()
{
	services.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ServiceListModel::countFlatItems ()
{
	return services.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ServiceListModel::getItemTitle (String& title, ItemIndexRef index)
{
	const IServiceDescription* description = services.at (index.getIndex ());
	if(!description)
		return false;

	title = description->getServiceTitle ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ServiceListModel::createColumnHeaders (IColumnHeaderList& list)
{
	list.addColumn (40);  // kIcon
	list.addColumn (IColumnHeaderList::kAutoWidth, nullptr, nullptr, 320); // kName
	list.addColumn (80);  // kCheck
	list.addColumn (1);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& ServiceListModel::getButtonRect (Rect& buttonRect, RectRef itemRect) const
{
	buttonRect = itemRect;
	if(buttonRect.getHeight () > 22)
	{
		buttonRect.setHeight (22);
		buttonRect.center (itemRect);
	}
	return buttonRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ServiceListModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	const IServiceDescription* description = services.at (index.getIndex ());
	if(!description)
		return false;

	PlugInClass serviceClass;
	const IClassDescription* classInfo = System::GetPlugInManager ().getClassDescription (description->getServiceID ());
	ASSERT (classInfo != nullptr)
	if(classInfo)
		serviceClass.assign (*classInfo);

	bool running = description->getServiceInstance () != nullptr;
	bool enabled = description->isUserEnabled () != 0;
	bool selected = getItemView ()->getSelection ().isSelected (index) != 0;;

	ITheme& theme = ViewBox (info.view).getTheme ();

	switch(column)
	{
	case kIcon :
		{
			IImage* icon = serviceClass.getIcon ();

			IImage* overlay = nullptr;
			if(running != enabled)
				overlay = theme.getImage ("OverlayIcon:Warning");

			if(icon)
				drawIconWithOverlay (info, icon, overlay, enabled, false);
		}
		break;

	case kCheck :
		{
			if(selected)
			{
				Rect rect;
				getButtonRect (rect, info.rect);
				const DrawInfo info2 = {info.view, info.graphics, rect, info.style, info.state};

				drawButton (info2, enabled ? XSTR (Disable) : XSTR (Enable));
			}
		}
		break;

	case kName :
		{
			// TODO: use ItemModelPainter::drawTitleWithSubtitle()!
			Font font (info.style.font);
			font.isBold (true);
			if(running != enabled)
				font.isItalic (true);

			SolidBrush brush = info.style.getTextBrush (enabled);
			String title (description->getServiceTitle ());
			String desc (description->getServiceDescription ());

			Rect textRect (info.rect);
			textRect.top += 2;
			textRect.bottom -= 2;

			if(!desc.isEmpty ())
				textRect.setHeight (textRect.getHeight ()/2);

			info.graphics.drawString (textRect, title, font, brush, Alignment::kLeft|Alignment::kVCenter);

			if(!desc.isEmpty ())
			{
				font.isBold (false);
				textRect.offset (0, textRect.getHeight ());
				String desc2 (desc);
				Font::collapseString (desc2, textRect.getWidth (), font);
				info.graphics.drawString (textRect, desc2, font, brush, Alignment::kLeft|Alignment::kVCenter);
			}
		}
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ServiceListModel::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	const IServiceDescription* description = services.at (index.getIndex ());
	if(!description)
		return false;

	if(column == kCheck && getItemView ()->getSelection ().isSelected (index))
	{
		// check if button clicked
		if(auto mouseEvent = info.editEvent.as<MouseEvent> ())
		{
			Rect rect;
			getButtonRect (rect, info.rect);
			if(!rect.pointInside (mouseEvent->where))
				return true;
		}

		bool running = description->getServiceInstance () != nullptr;
		bool enabled = description->isUserEnabled () != 0;

		enabled = !enabled;

		System::GetServiceManager ().enableService (*description, enabled);

		if(enabled != running)
		{
			if(!warned)
			{
				//Alert::warn (XSTR (ServiceWarning));
				SignalSource (Signals::kApplication).deferSignal (NEW Message (Signals::kRequestRestart, XSTR (ServiceWarning)));
				warned = true;
			}
		}

		getItemView ()->invalidateItem (index);
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ServiceListModel::measureCellContent (Rect& size, ItemIndexRef index, int column, const StyleInfo& info)
{
	const IServiceDescription* description = services.at (index.getIndex ());
	if(!description)
		return false;

	if(column == kName)
	{
		Font font (info.font);
		font.isBold (true);
		Font::measureString (size, description->getServiceTitle (), font);

		if(!description->getServiceDescription ().isEmpty ())
		{
			Rect size2;
			Font::measureString (size2, description->getServiceDescription (), info.font);
			size.join (size2);
		}
		return true;
	}
	return false;
}
