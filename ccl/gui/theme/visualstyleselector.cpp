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
// Filename    : ccl/gui/theme/visualstyleselector.cpp
// Description : VisualStyleSelector class
//
//************************************************************************************************

#include "ccl/gui/theme/visualstyleselector.h"

#include "ccl/base/message.h"
#include "ccl/base/trigger.h" // Property class

#include "ccl/public/gui/iparameter.h"

using namespace CCL;

StringID VisualStyleAlias::kStyleChanged = CSTR ("styleChanged");

//************************************************************************************************
// VisualStyleSelector
//************************************************************************************************

DEFINE_CLASS_HIDDEN (VisualStyleSelector, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyleSelector::VisualStyleSelector (VisualStyleAlias* styleAlias)
: styleAlias (styleAlias),
  param (nullptr)
{
	// the VisualStyle alias is shared (only) by the client views, we are owned by the style alias
	styleAlias->setStyleSelector (this);

	styles.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyleSelector::~VisualStyleSelector ()
{
	setParameter (nullptr);

	share_and_observe_unknown<IUnknown> (this, controller, nullptr);

	for(auto* style : styles)
		if(ccl_cast<VisualStyleAlias> (style))
			style->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyleSelector::initialize ()
{
	updateSelectedStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyleSelector::setParameter (IParameter* p)
{
	if(param)
	{
		ISubject::removeObserver (param, this);
		param->release ();
	}

	param = p;

	if(param)
	{
		param->retain ();
		ISubject::addObserver (param, this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyleSelector::setSelectorProperty (CStringRef propertyId, IUnknown* controller)
{
	if(!propertyId.isEmpty ())
	{
		share_and_observe_unknown<IUnknown> (this, this->controller, controller);
		this->propertyId = propertyId;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyleSelector::addStyle (VisualStyle* style)
{
	if(ccl_cast<VisualStyleAlias> (style))
		style->addObserver (this); // we have to forward the change to our clients

	styles.add (return_shared (style));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
void CCL_API VisualStyleSelector::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		UnknownPtr<IParameter> p (subject);
		if(p && p == param)
			updateSelectedStyle ();
	}
	else if(msg == kPropertyChanged && isEqualUnknown (subject, controller))
	{
		if(isPropertyMode () && msg.getArgCount () > 0) // filter other properties
			if(msg.getArg (0).asString () != String (propertyId))
				return;

		updateSelectedStyle ();
	}
	else if(msg == VisualStyleAlias::kStyleChanged)
	{
		// a style we depend on has changed: forward the notification to our clients
		styleAlias->signalStyleChanged ();
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VisualStyleSelector::isPropertyMode () const
{
	return !propertyId.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyleSelector::updateSelectedStyle ()
{
	if(param)
	{
		selectStyle (ccl_bound (param->getValue ().asInt (), 0, styles.count () - 1));
	}
	else if(isPropertyMode ())
	{
		UnknownPtr<IObject> iObject (controller);
		Variant value;
		Property (iObject, propertyId).get (value);
		selectStyle (ccl_bound (value.asInt (), 0, styles.count () - 1));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyleSelector::selectStyle (int index)
{
	auto* style = static_cast<VisualStyle*> (styles.at (index));
	if(style != styleAlias->getInherited ())
	{
		// indirectly switch styles by changing the inherited style of the style alias
		// (style alias must stay assigned to client views)
		styleAlias->setInherited (style);

		styleAlias->signalStyleChanged ();
	}
}

//************************************************************************************************
// VisualStyleAlias
//************************************************************************************************

DEFINE_CLASS_HIDDEN (VisualStyleAlias, VisualStyle)

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyleAlias::VisualStyleAlias (StringID name)
: VisualStyle (name),
  wasObserved (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API VisualStyleAlias::addObserver (IObserver* observer)
{
	wasObserved = true;
	SuperClass::addObserver (observer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyleAlias::setStyleSelector (VisualStyleSelector* selector)
{
	styleSelector = selector;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyle* CCL_API VisualStyleAlias::getOriginal () const
{
	return getInherited ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyleAlias::use (IVisualStyleClient* client)
{
	ASSERT (client)
	ASSERT (!clients.contains (client))
	if(clients.contains (client))
		return;
			
	clients.append (client);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyleAlias::unuse (IVisualStyleClient* client)
{
	bool result = clients.remove (client);
	ASSERT (result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyleAlias::signalStyleChanged ()
{
	// inform the clients that their style has changed
	for(auto client : clients)
		client->onVisualStyleChanged ();

	// inform other VisualStyleSelector that depend on this
	if(wasObserved)
		signal (Message (kStyleChanged));
}
