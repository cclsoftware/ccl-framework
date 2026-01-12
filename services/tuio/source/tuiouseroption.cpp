//************************************************************************************************
//
// TUIO Support
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
// Filename    : tuioservice.cpp
// Description : TUIO Service
//
//************************************************************************************************

#include "tuioservice.h"
#include "tuiouseroption.h"

#include "ccl/app/params.h"
#include "ccl/app/options/useroptionelement.h"

#include "ccl/public/gui/framework/iwindow.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("TUIO")
	XSTRING (TUIOOption, "Touch Input")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum TUIOUserOptionTags
	{
		kMatchMonitorTag = 100
	};
}

//************************************************************************************************
// TUIOUserOption
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TUIOUserOption, UserOption)

//////////////////////////////////////////////////////////////////////////////////////////////////

TUIOUserOption::TUIOUserOption ()
: monitorNumber (*NEW IntParam (0, 99)),
  monitorCount (*NEW IntParam (0, 99))
{
	setTitle (String () << General () << strSeparator << XSTR (TUIOOption));
	setFormName ("TUIOUserOption");

	ConfigurationElement* e = NEW ConfigurationElement ("TUIO", "clientEnabled_2", NEW Parameter);
	e->setApplyCallback (TUIOService::applyConfiguration);
	addElement (e);

	e = NEW ConfigurationElement ("TUIO", "clientPort", NEW IntParam (1, 65535));
	e->setApplyCallback (TUIOService::applyConfiguration);
	addElement (e);

	monitorNumber.setFormatter ((AutoPtr<IFormatter> (NEW Format::Offset (1))));
	e = NEW ConfigurationElement ("TUIO", "monitorNumber", &monitorNumber);
	e->setApplyCallback (TUIOService::applyConfiguration);
	addElement (e);

	AutoPtr<Parameter> matchMonitorParam = NEW Parameter ("TUIO.matchMonitor");
	paramList.addShared (matchMonitorParam);
	matchMonitorParam->connect (this, Tag::kMatchMonitorTag);

	e = NEW ConfigurationElement ("TUIO", "monitorCount", &monitorCount);
	e->setApplyCallback (TUIOService::applyConfiguration);
	addElement (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TUIOUserOption::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kMatchMonitorTag :
		if(optionView)
		{
			Point origin;
			monitorNumber.setValue (System::GetDesktop ().findMonitor (optionView->clientToScreen (origin), true));
		}
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API TUIOUserOption::createView (StringID name, VariantRef data, const Rect& bounds)
{
	optionView = SuperClass::createView (name, data, bounds);
	return optionView;
}


