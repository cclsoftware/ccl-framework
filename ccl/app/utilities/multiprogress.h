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
// Filename    : ccl/app/utilities/multiprogress.h
// Description : Multistep progress
//
//************************************************************************************************

#ifndef _ccl_multiprogress_h
#define _ccl_multiprogress_h

#include "ccl/public/base/iprogress.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/base/unknown.h"

namespace CCL {

//************************************************************************************************
// ProgressProxy
/** Passes calls to another IProgressNotify. */
//************************************************************************************************

class ProgressProxy: public Unknown,
					 public AbstractProgressNotify
{
public:
	ProgressProxy (IProgressNotify& progress); ///< takes ownership!

	// IProgressNotify
	void CCL_API beginProgress () override						{ progress->beginProgress (); }
	void CCL_API endProgress () override						{ progress->endProgress (); }
	void CCL_API setProgressText (StringRef text) override		{ progress->setProgressText (text); }
	void CCL_API updateProgress (const State& state) override	{ progress->updateProgress (state); }
	tbool CCL_API isCanceled () override						{ return progress->isCanceled (); }

	CLASS_INTERFACES (Unknown)

protected:
	AutoPtr<IProgressNotify> progress;
};

//************************************************************************************************
// MultiProgress
/** Helper class for managing the progress info of multistep operations.
	usage:

		MultiProgress multiProgress (progress);
		multiProgress.setNumSteps (10);

		for(int i = 0; i < 10; i++)
		{
			MultiProgress::Step step (multiProgress);	// instantiate in the scope of current step
			stepWork[i]->process (step);				// pass step as IProgressNotify*
		}
*/
//************************************************************************************************

class MultiProgress: public Unknown
{
public:
	MultiProgress (IProgressNotify* progress);
	
	void setNumSteps (int steps);
	void reset ();

	/// pattern with 2 arguments: current step, numSteps; default: "Step %(1) of %(2)"
	PROPERTY_STRING (stepCountPattern, StepCountPattern)

	/// for finer calculation of the master progress, specify the total work in any unit
	/// and pass the work amount for each step to the Step constructor
	void setTotalWork (double totalWorkUnits);

	/// instantiate a Step object in the scope of the current step
	class Step
	{
	public:
		Step (MultiProgress& multiProgress, double stepWorkUnits = -1);

		// act as IProgressNotify* for one step
		operator IProgressNotify* ();
		IProgressNotify* operator -> ();

	private:
		ProgressNotifyScope scope;
	};

private:
	SharedPtr<IProgressNotify> progress;
	int numSteps;
	int step;
	double totalWork;
	double stepWork;
	double workDone;

	friend class Step;
	IProgressNotify* createStep (double stepWorkUnits);

	class StepProxy;
	friend class StepProxy;
	void updateStep (const IProgressNotify::State& state);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void MultiProgress::reset ()
{ step = -1; workDone = 0; stepWork = 0; }

inline MultiProgress::Step::operator IProgressNotify* ()
{ return scope.progress; }

inline IProgressNotify* MultiProgress::Step::operator -> ()
{ return scope.progress; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_multiprogress_h
