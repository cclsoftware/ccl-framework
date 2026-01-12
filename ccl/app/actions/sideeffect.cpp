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
// Filename    : ccl/app/actions/sideeffect.cpp
// Description : Side Effects for Actions
//
//************************************************************************************************

#include "ccl/app/actions/sideeffect.h"
#include "ccl/app/actions/action.h"

using namespace CCL;

//************************************************************************************************
// SideEffect::Suspender
//************************************************************************************************

SideEffect::Suspender::Suspender (StringID sideEffectName)
: sideEffect (SideEffectRegistry::instance ().getSideEffect (sideEffectName)),
  wasSuspended (false)
{
	if(sideEffect)
	{
		wasSuspended = sideEffect->suspended;
		sideEffect->suspended = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SideEffect::Suspender::~Suspender ()
{
	if(sideEffect)
		sideEffect->suspended = wasSuspended;
}

//************************************************************************************************
// SideEffectRegistry
//************************************************************************************************

DEFINE_SINGLETON (SideEffectRegistry)

//////////////////////////////////////////////////////////////////////////////////////////////////

SideEffectRegistry::SideEffectRegistry ()
{
	sideEffects.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SideEffectRegistry::registerSideEffect (SideEffect* sideEffect)
{
	CCL_PRINTF ("register %s\n", sideEffect->getName ().str ())
	sideEffects.add (sideEffect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SideEffectRegistry::registerSideEffectBefore (SideEffect* sideEffect, StringID otherEffectName)
{
	CCL_PRINTF ("register %s before %s\n", sideEffect->getName ().str (), otherEffectName.str ())

	ForEach (sideEffects, SideEffect, otherEffect)
		if(otherEffect->getName () == otherEffectName)
		{
			sideEffects.insertBefore (otherEffect, sideEffect);
			return;
		}
	EndFor
	
	sideEffects.add (sideEffect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SideEffect* SideEffectRegistry::getSideEffect (StringID name)
{
	for(auto sideEffect : iterate_as<SideEffect> (sideEffects))
		if(sideEffect->getName () == name)
			return sideEffect;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SideEffectRegistry::extendAction (Action* originalAction, StringID context)
{
	if(!originalAction->canHaveSideEffects ())
		return false;

	if(originalAction->isSideEffectsChecked ())
		return true;

	bool result = false;
	bool useContext = context.isEmpty () == false;
	ForEach (sideEffects, SideEffect, sideEffect)
		if(sideEffect->isSuspended ())
			continue;

		Action* newAction = useContext ? sideEffect->createActionInContext (originalAction, context) : sideEffect->createAction (originalAction);
		if(newAction)
		{
			result = true;
			CCL_PRINTF ("%s: %s %s %s ->  %s %s %s\n", sideEffect->getName ().str (), originalAction->myClass ().getPersistentName (), MutableCString (originalAction->getDescription ()).str (), MutableCString (originalAction->getDetailedDescription ()).str (), newAction->myClass ().getPersistentName (), MutableCString (newAction->getDescription ()).str (), MutableCString (newAction->getDetailedDescription ()).str ())
			if(originalAction->isExecuted () && newAction->isExecuted () == false)
				originalAction->addActionAndExecute (newAction);
			else
				originalAction->addAction (newAction);
		}
	EndFor
	originalAction->setSideEffectsChecked (true);
	return result;
}
