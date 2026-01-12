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
// Filename    : ccl/public/gui/framework/usertooltip.cpp
// Description : User Tooltip Popup
//
//************************************************************************************************

#include "ccl/public/gui/framework/usertooltip.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/itooltip.h"
#include "ccl/public/gui/framework/controlsignals.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/system/iatomtable.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//************************************************************************************************
// UserTooltipPopup
//************************************************************************************************

UserTooltipPopup::UserTooltipPopup (IView* view, bool followTooltipSignals)
: view (view),
  tooltipPopup (nullptr),
  followTooltipSignals (followTooltipSignals)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserTooltipPopup::~UserTooltipPopup ()
{
	hideTooltip ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserTooltipPopup::setTooltip (StringRef text, const Point* position)
{
	bool mustCreate = (tooltipPopup == nullptr);
	if(mustCreate)
	{
		tooltipPopup = ccl_new<ITooltipPopup> (ClassID::TooltipPopup);
		if(!tooltipPopup)
			return;
		if(tooltipPopup->isReserved ())
		{	
			tooltipPopup = nullptr;
			return;
		}
		System::GetGUI ().hideTooltip ();
		
		tooltipPopup->construct (view);

		if(followTooltipSignals)
		{
			AutoPtr<IAtom> atom = System::GetAtomTable ().createAtom (Signals::kControls);
			UnknownPtr<ISubject> subject (atom);
			subject->addObserver (this);
		}
	}

	if(position)
		tooltipPopup->setPosition (*position, view);
	else
		tooltipPopup->moveToMouse ();

	tooltipPopup->setText (text);

	if(mustCreate)
		tooltipPopup->show ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserTooltipPopup::hideTooltip ()
{
	if(tooltipPopup)
	{
		tooltipPopup->reserve (false);
		tooltipPopup->hide ();
		tooltipPopup->release ();
		tooltipPopup = nullptr;

		if(followTooltipSignals)
		{
			AutoPtr<IAtom> atom = System::GetAtomTable ().createAtom (Signals::kControls);
			UnknownPtr<ISubject> subject (atom);
			subject->removeObserver (this);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserTooltipPopup::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kHideTooltip)
		hideTooltip ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserTooltipPopup::setPosition (const Point& position)
{
	if(tooltipPopup)
		tooltipPopup->setPosition (position, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserTooltipPopup::moveToMouse ()
{
	if(tooltipPopup)
		tooltipPopup->moveToMouse ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserTooltipPopup::reserve (bool state)
{
	if(tooltipPopup)
		tooltipPopup->reserve (state);
}
	
