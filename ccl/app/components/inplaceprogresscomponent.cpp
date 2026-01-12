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
// Filename    : ccl/app/components/inplaceprogresscomponent.cpp
// Description : Inplace Progress Component
//
//************************************************************************************************

#include "ccl/app/components/inplaceprogresscomponent.h"

#include "ccl/public/system/formatter.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum InplaceProgressTags
	{
		kTitle,
		kActive,
		kState,
		kInfinite,
		kText,
		kHasTime,
		kTime,
		kCancel
	};
}

//************************************************************************************************
// InplaceProgressComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (InplaceProgressComponent, Component)

/////////////////////////////////////////////////////////////////////////////////////////////////

InplaceProgressComponent::InplaceProgressComponent (StringRef _name)
: Component (_name),
  lastFlush (0),
  parentView (nullptr),
  startTime (0),
  recentTimeRemaining (0),
  activationDelay (0),
  beginProgressCount (0),
  canceled (false)
{
	if(getName ().isEmpty ())
		setName ("progress");
	
	paramList.addString (CSTR ("progressTitle"), Tag::kTitle);
	paramList.addParam ("active", Tag::kActive);
	paramList.addFloat (0.f, 100.f, CSTR ("progressState"), Tag::kState);
	paramList.addParam (CSTR ("progressInfinite"), Tag::kInfinite)->setValue (false);
	paramList.addString (CSTR ("progressText"), Tag::kText);
	paramList.addParam (CSTR ("hasProgressTime"), Tag::kHasTime)->setValue (false);
	paramList.addString (CSTR ("progressTime"), Tag::kTime);
	paramList.addParam (CSTR ("progressCancel"), Tag::kCancel);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void InplaceProgressComponent::setActivationDelay (double delay)
{
	activationDelay = delay;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void InplaceProgressComponent::setParentView (IView* _parentView)
{
	parentView = _parentView;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool InplaceProgressComponent::hasParentView () const 
{ 
	return parentView != nullptr; 
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool InplaceProgressComponent::isInProgress () const
{
	return beginProgressCount > 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

double InplaceProgressComponent::getProgressValue () const
{
	return paramList.byTag (Tag::kState)->getNormalized ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool InplaceProgressComponent::cancelProgress ()
{
	if(canceled == false && isInProgress () == true)
	{
		IParameter* cancelParam = paramList.byTag (Tag::kCancel);
	
		cancelParam->enable (false);
		canceled = true;
		flushUpdates ();
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API InplaceProgressComponent::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kCancel)
		cancelProgress ();
	
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API InplaceProgressComponent::setTitle (StringRef title)
{
	paramList.byTag (Tag::kTitle)->setValue (title);

	flushUpdates (true);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool InplaceProgressComponent::isCancelEnabled () const
{
	return paramList.byTag (Tag::kCancel)->isEnabled ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API InplaceProgressComponent::setCancelEnabled (tbool state)
{
	paramList.byTag (Tag::kCancel)->enable (state);

	flushUpdates ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API InplaceProgressComponent::beginProgress ()
{
	if(beginProgressCount++ == 0)
	{
		canceled = false;
		startTime = System::GetProfileTime ();
		lastFlush = 0;
		recentTimeRemaining = 0;

		if(activationDelay <= 0)
			paramList.byTag (Tag::kActive)->setValue (true);

		paramList.byTag (Tag::kHasTime)->setValue (false);

		flushUpdates ();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API InplaceProgressComponent::endProgress ()
{
	if(--beginProgressCount == 0)
	{
		paramList.byTag (Tag::kActive)->setValue (false);
		paramList.byTag (Tag::kCancel)->enable (true);
		paramList.byTag (Tag::kInfinite)->setValue (false);
		paramList.byTag (Tag::kText)->setValue (String::kEmpty);

		activationDelay = 0;
		startTime = 0;

		flushUpdates ();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API InplaceProgressComponent::setProgressText (StringRef text)
{
	paramList.byTag (Tag::kText)->setValue (text);

	flushUpdates (true);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API InplaceProgressComponent::updateProgress (const State& state)
{
	bool animated = (state.flags & kIndeterminate) != 0;

	IParameter* p = paramList.byTag (Tag::kInfinite);
	bool wasAnimated = p->getValue ().asBool ();
	p->setValue (animated);

	if(animated != wasAnimated)
	{
		// reset for timing estimation
		startTime = System::GetProfileTime ();
		recentTimeRemaining = 0;
		paramList.byTag (Tag::kHasTime)->setValue (false);
	}

	p = paramList.byTag (Tag::kState);
	if(animated)
		p->setNormalized (1.f);
	else
	{
		p->setNormalized ((float)state.value);

		double delta = System::GetProfileTime () - startTime;
		if(delta > 3. && state.value >= 0.001)
		{
			String timeString;
			int seconds = (int)((delta / state.value) - delta);
			if(seconds >= 0)
			{
				if(recentTimeRemaining > 0 && seconds > recentTimeRemaining && (seconds - recentTimeRemaining) < 20)
					seconds = recentTimeRemaining;				
				recentTimeRemaining = seconds;
				if(seconds > 60)
					seconds /= 10, seconds *= 10;

				timeString = Format::Duration::print (seconds);
			}

			paramList.byTag (Tag::kTime)->fromString (timeString);
			paramList.byTag (Tag::kHasTime)->setValue (!animated && !timeString.isEmpty ());
		}
	}

	flushUpdates ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API InplaceProgressComponent::isCanceled ()
{
	return canceled;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

IProgressNotify* CCL_API InplaceProgressComponent::createSubProgress ()
{
	return NEW InplaceProgressComponent;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void InplaceProgressComponent::flushUpdates (bool force)
{	
	if(isInProgress () && activationDelay > 0 && (System::GetProfileTime () - startTime) > activationDelay)
	{
		paramList.byTag (Tag::kActive)->setValue (true);
		activationDelay = 0;
	}

	int64 now = System::GetSystemTicks ();
	if(force == false)
	{
		if(now - lastFlush < 20)
			return;
	}
	lastFlush = now;

	if(force)
		System::GetSignalHandler ().flush ();
		
	SharedPtr<IView> viewGuard (parentView);
	System::GetDesktop ().flushUpdatesWithProgressWindows (parentView);
}
