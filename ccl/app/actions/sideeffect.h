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
// Filename    : ccl/app/actions/sideeffect.h
// Description : Side Effects for Actions
//
//************************************************************************************************

#ifndef _ccl_sideeffect_h
#define _ccl_sideeffect_h

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/text/cstring.h"

namespace CCL {

class Action;

//************************************************************************************************
// SideEffect
/** A side effect can extend an undoable action. */
//************************************************************************************************

class SideEffect: public Object
{
public:
	SideEffect ();

	PROPERTY_MUTABLE_CSTRING (name, Name)
	
	/** Creates an additional action that should be executed after the original action.
		Returns 0 if the original action is not relevant. */
	virtual Action* createAction (Action* originalAction) = 0;
	virtual Action* createActionInContext (Action* originalAction, StringID context) {return nullptr;}

	bool isSuspended () const;
	class Suspender;

private:
	bool suspended;
};

//************************************************************************************************
// SideEffectRegistry
/** Registry for side effect to be automatically applied to actions. */
//************************************************************************************************

class SideEffectRegistry: public Object,
						  public Singleton<SideEffectRegistry>
{
public:
	/** Returns true if subActions were added. */
	bool extendAction (Action* originalAction, StringID context = nullptr);

	void registerSideEffect (SideEffect* sideEffect);
	void registerSideEffectBefore (SideEffect* sideEffect, StringID otherEffectName);

	SideEffect* getSideEffect (StringID name);

protected:
	SideEffectRegistry ();
	friend class Singleton<SideEffectRegistry>;

private:
	ObjectList sideEffects;
};

//************************************************************************************************
// SideEffect::Suspender
/** Temporarily suspends a specific side effects (by name). */
//************************************************************************************************

class SideEffect::Suspender: public Object
{
public:
	Suspender (StringID sideEffectName);
	~Suspender ();

private:
	SideEffect* sideEffect;
	bool wasSuspended;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// SideEffect macros.
// Use IMPLEMENT_SIDEEFFECT to declare and register the SideEffect class;
// Then add the implementaton of the createAction method.
//////////////////////////////////////////////////////////////////////////////////////////////////

struct RegisterSideEffect
{
	RegisterSideEffect (SideEffect* sideEffect, StringID name)
	{
		sideEffect->setName (name);
		SideEffectRegistry::instance ().registerSideEffect (sideEffect);
	}

	RegisterSideEffect (SideEffect* sideEffect, StringID name, StringID otherEffectName)
	{
		sideEffect->setName (name);
		SideEffectRegistry::instance ().registerSideEffectBefore (sideEffect, otherEffectName);
	}
};

#define DECLARE_SIDEEFFECT(SideEffectClass) class SideEffectClass: public CCL::SideEffect {\
	CCL::Action* createAction (CCL::Action* originalAction) override; };

#define REGISTER_SIDEEFFECT(SideEffectClass) static RegisterSideEffect UNIQUE_IDENT (rSE) (NEW SideEffectClass, #SideEffectClass);
	
#define REGISTER_SIDEEFFECT_BEFORE(SideEffectClass, otherEffectName) static RegisterSideEffect UNIQUE_IDENT (rSE) (NEW SideEffectClass, #SideEffectClass, otherEffectName);

#define IMPLEMENT_SIDEEFFECT(SideEffectClass)\
	DECLARE_SIDEEFFECT (SideEffectClass)\
	REGISTER_SIDEEFFECT (SideEffectClass)

//////////////////////////////////////////////////////////////////////////////////////////////////
// SideEffect inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline SideEffect::SideEffect ()
: suspended (false)
{}

inline bool SideEffect::isSuspended () const
{ return suspended; }

} // namespace CCL

#endif // _ccl_sideeffect_h
