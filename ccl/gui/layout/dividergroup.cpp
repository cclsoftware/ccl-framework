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
// Filename    : ccl/gui/layout/dividergroup.cpp
// Description : Groups of synchronized Dividers
//
//************************************************************************************************

#include "ccl/gui/layout/dividergroup.h"
#include "ccl/gui/layout/divider.h"
#include "ccl/gui/gui.h"

#include "ccl/public/gui/framework/controlsignals.h"

#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// DividerGroups
//************************************************************************************************

DividerGroups::DividerGroups ()
: dirtySink (nullptr)
{
	groups.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DividerGroups::Group* DividerGroups::getGroup (StringID name)
{
	ForEach (groups, Group, group)
		if(group->getName () == name)
			return group;
	EndFor

	Group* group = NEW Group (name);
	group->setDirtySink (getDirtySink ());
	groups.add (group);
	return group;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API DividerGroups::findParameter (StringID name) const
{
	return const_cast<DividerGroups*> (this)->getGroup (name)->newParameter ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DividerGroups::flush ()
{
	ForEach (groups, Group, group)
		group->flush ();
	EndFor
}

//************************************************************************************************
// DividerGroups::Group
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DividerGroups::Group, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

DividerGroups::Group::Group (StringID name)
: name (name),
  dirtySink (nullptr),
  editParam (nullptr),
  lastValue (0)
{
	params.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* DividerGroups::Group::newParameter ()
{
	IntParam* param = NEW DividerParam (name);
	
	if(lastValue == 0)
		if(IntParam* lastParam = (IntParam*)params.getLast ())
			lastValue = lastParam->getValue ().asInt ();
	
	(NEW Message ("initValue", ccl_as_unknown (getAlignmentParam (false))))->post (param);

	param->connect (this, 23);
	params.add (param);

	CCL_PRINTF ("DividerGroup %s: param added, now %d\n", name.str (), params.count ());
	return param;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* DividerGroups::Group::getAlignmentParam (bool needsConnectedParameter) const
{	
	struct AlignmentPosition
	{
		AlignmentPosition (int value = 0, Parameter* param = nullptr) : value (value), param (param) {}
		bool operator > (const AlignmentPosition& other) const { return value > other.value; }

		int value;
		Parameter* param;
	};
	
	Vector<AlignmentPosition> positions;	
	ForEach (params, Parameter, p)
		if(auto dp = ccl_cast<DividerParam> (p))
		{
			if(dp->isInitialized () && dp->isDividerConnected ())
				positions.addSorted (AlignmentPosition (p->getValue ().asInt (), p));
		}
	EndFor
	
	
	// check if preferred alignment of initialized dividers is detectable ( >= 3 )
	int count = positions.count ();
	if(count == 0)
	{
		if(needsConnectedParameter)
			return nullptr;
			
		DividerParam* firstParam = ccl_cast<DividerParam> (params.getFirst ());
		return (firstParam && firstParam->isInitialized ()) ? firstParam : nullptr;
	}

	// select alignment parameter for dominant divider position
	Parameter* alignmentParam = positions[0].param;
	
	if(count < 3) // no preference detectable yet 
		return alignmentParam; // return first connected divider parameter
		
	int occurences = 1;
	int mostOccurences = 1;
	for (int i = 1; i < count; i++)
	{
		if(positions[i - 1].value == positions[i].value)
		{
			occurences++;
		}
		else
		{
			if(occurences > mostOccurences)
			{
				mostOccurences = occurences;
				alignmentParam = positions[i - 1].param;
			}
			occurences = 1;
		}
	}

	if(occurences > mostOccurences)
		alignmentParam = positions[count - 1].param;
	
	return alignmentParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DividerGroups::Group::removeParameter (IParameter* param)
{
	if(Object* p = unknown_cast<Object> (param))
		if(params.remove (p))
			param->release ();

	CCL_PRINTF ("DividerGroup %s: param removed, now %d\n", name.str (), params.count ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DividerGroups::Group::paramEdit (IParameter* param, tbool begin)
{
	if(begin)
	{
		editParam = param;
		lastValue = param->getValue ().asInt ();

		KeyState keys;
		GUI.getKeyState (keys);
		if(keys.isSet (KeyState::kCommand))
			paramChanged (param);	// do absolute sync immediately after click
	}
	else
	{
		ASSERT (editParam)
		if(param == editParam)
		{
			ForEach (params, Parameter, p)
				if(p != editParam)
					p->endEdit ();
			EndFor

			editParam = nullptr;
		}
		if(dirtySink)
			dirtySink->signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DividerGroups::Group::paramChanged (IParameter* param)
{
	if(param == editParam)
	{
		KeyState keys;
		GUI.getKeyState (keys);

		bool sync = !keys.isSet (KeyState::kShift);
		bool absolute = keys.isSet (KeyState::kCommand);
		if(sync)
			synchronize (param, absolute);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DividerGroups::Group::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kDividerDoubleClick)
	{
		UnknownPtr<IParameter> param (msg[0]);
		if(param)
			synchronize (param, true);

		if(msg.getArgCount () > 1)
		{
			UnknownPtr<IVariant> result (msg[1]);
			if(result)
				result->assign (true); // return value: indicate that we handled the gesture
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DividerGroups::Group::synchronize (IParameter* param, bool absolute)
{
	if(absolute)
	{
		// set all others to the same value
		int value = param->getValue ().asInt ();
		lastValue = value;

		ForEach (params, Parameter, p)
			if(p != param)
			{
				p->signal (Message (IParameter::kBeginEdit));
				p->setValue (value);
				p->signal (Message (IParameter::kEndEdit)); // Divider: onManipulationDone -> save layout state
			}
		EndFor
	}
	else
	{
		static const int kSameValueThreshold = 10;

		// apply the same difference to all others (above the kSameValueThreshold)
		int value = param->getValue ().asInt ();
		int diff = value - lastValue;
		lastValue = value;

		ForEach (params, Parameter, p)
			if(p != param)
			{
				int paramValue = p->getValue ().asInt () + diff;
				p->signal (Message (IParameter::kBeginEdit));
				p->setValue (ccl_abs (paramValue - value) < kSameValueThreshold ? value : paramValue);
				p->signal (Message (IParameter::kEndEdit)); // Divider: onManipulationDone -> save layout state
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DividerGroups::Group::flush ()
{
	ForEach (params, Parameter, p)
		p->Object::signal (Message (IParameter::kEndEdit)); // Divider: onManipulationDone -> save layout state
	EndFor
}

//************************************************************************************************
// DividerParam
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DividerGroups::DividerParam, IntParam)

//////////////////////////////////////////////////////////////////////////////////////////////////

DividerGroups::DividerParam::DividerParam (StringID name)
: IntParam (0, kMaxCoord, name),
  initialized (false),
  dividerConnected (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API DividerGroups::DividerParam::release ()
{
	int rc = IntParam::release ();

	if(rc == 1 && controller) // last refCount is hold by group
		static_cast<DividerGroups::Group*> (controller)->removeParameter (this);
	return rc;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DividerGroups::DividerParam::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "initValue")
	{
		// ask divider if it's parent layout has a saved layout state
		bool hasLayoutState = false;
		Variant var;
		var.setIntPointer (reinterpret_cast<UIntPtr> (&hasLayoutState));
		Object::signal (Message (Divider::kHasLayoutState, var));

		// if no saved layout state was restored, init the new divider with the value from last param
		if(!hasLayoutState)
		{
			if(Parameter* reference = unknown_cast<Parameter> (msg[0]))
			{
				int value = reference->getValue ().asInt ();
					
				// try updated initial value 
				auto group = unknown_cast<DividerGroups::Group> (getController ());
				if(auto currentAlignmentParam = ccl_cast<DividerParam> (group->getAlignmentParam ()))
					if(value != currentAlignmentParam->getValue ().asInt ())
						value = currentAlignmentParam->getValue ().asInt ();

				// send messages synchronously
				Object::signal (Message (kBeginEdit));
				setValue (value, false);
				Object::signal (Message (kChanged));
				Object::signal (Message (kEndEdit)); // Divider: onManipulationDone -> save layout state
			}
		}
		initialized = true;
	}
	else
		IntParam::notify (subject, msg);
}
