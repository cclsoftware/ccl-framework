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
// Filename    : ccl/app/utilities/multiprogress.cpp
// Description : Multistep progress
//
//************************************************************************************************

#include "ccl/app/utilities/multiprogress.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/gui/framework/iprogressdialog.h"

namespace CCL {

//************************************************************************************************
// MultiProgress::StepProxy
/** Passes calls to another IProgressNotify and notifies it's parent MultiProgress. */
//************************************************************************************************

class MultiProgress::StepProxy: public ProgressProxy
{
public:
	StepProxy (MultiProgress& multiProgress, IProgressNotify& progress)
	: ProgressProxy (progress),
	  multiProgress (&multiProgress)
	{}
	
	// ProgressProxy
	void CCL_API updateProgress (const State& state) override { SharedPtr<MultiProgress::StepProxy> saver (this); progress->updateProgress (state); multiProgress->updateStep (state);}

private:
	SharedPtr<MultiProgress> multiProgress;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("MultiProgress")
	XSTRING (StepXofY, "Step %(1) of %(2)")
END_XSTRINGS

//************************************************************************************************
// ProgressProxy
//************************************************************************************************

ProgressProxy::ProgressProxy (IProgressNotify& progress)
: progress (&progress)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ProgressProxy::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IProgressNotify)
	
	if(iid == ccl_iid<IProgressDetails> () || iid == ccl_iid<IProgressDialog> ())
		return progress->queryInterface (iid, ptr);

	return Unknown::queryInterface (iid, ptr);
}

//************************************************************************************************
// MultiProgress::Step
//************************************************************************************************

MultiProgress::Step::Step (MultiProgress& multiProgress, double stepWorkUnits)
: scope (multiProgress.createStep (stepWorkUnits))
{
	if(scope.progress)
		scope.progress->release (); // created for us
}

//************************************************************************************************
// MultiProgress
//************************************************************************************************

MultiProgress::MultiProgress (IProgressNotify* progress)
: progress (progress),
  stepCountPattern (XSTR (StepXofY)),
  numSteps (1),
  step (-1),
  totalWork (0),
  stepWork (0),
  workDone (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiProgress::setNumSteps (int steps)
{
	numSteps = ccl_max (1, steps);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiProgress::setTotalWork (double totalWorkUnits)
{
	totalWork = totalWorkUnits;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IProgressNotify* MultiProgress::createStep (double stepWorkUnits)
{
	if(progress)
	{
		if(totalWork <= 0)
			totalWork = numSteps;
		if(stepWorkUnits < 0)
			stepWorkUnits = 1;

		step++;
		ccl_lower_limit (numSteps, step + 1); // check if exceeded

		workDone += stepWork;
		stepWork = stepWorkUnits;

		if(numSteps > 1)
		{
			// update main progress text & value
			Variant args[] = { step + 1, numSteps };
			String text;
			text.appendFormat (stepCountPattern, args, ARRAY_COUNT (args));
			progress->setProgressText (text);
			progress->updateProgress (IProgressNotify::State (workDone / totalWork));

			// create sub progress for step
			IProgressNotify* subProgress = progress->createSubProgress ();
			if(subProgress)
				return NEW StepProxy (*this, *subProgress);
		}
		else
		{
			progress->retain ();
			return progress;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiProgress::updateStep (const IProgressNotify::State& state)
{
	if(!progress)
		return;
		
	if(state.flags & IProgressNotify::kIndeterminate)
	{
		if(numSteps == 1)
			progress->updateProgress (state);
		else // show 50% of step done if indeterminate
			progress->updateProgress (IProgressNotify::State ((workDone + .5 * stepWork) / totalWork));
	}
	else
		progress->updateProgress (IProgressNotify::State ((workDone + state.value * stepWork) / totalWork));
}
