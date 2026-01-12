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
// Filename    : ccl/public/base/iprogress.h
// Description : Progress Notification Interface
//
//************************************************************************************************

#ifndef _ccl_iprogress_h
#define _ccl_iprogress_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IProgressNotify
/** Progress notification interface.
	\ingroup base_io  */
//************************************************************************************************

interface IProgressNotify: IUnknown
{
	/** Progress flags. */
	enum Flags
	{
		kIndeterminate = 1<<0,	///< total duration is unknown yet
		kImportant = 1<<1		///< important update for user
	};

	/** Progress state. */
	struct State
	{
		double value;	///< normalized progress (0..1)
		int flags;		///< @see Flags

		State (double value = 0, int flags = 0)
		: value (value),
		  flags (flags)
		{}
	};

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Installer side
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	/** Set progress title. */
	virtual void CCL_API setTitle (StringRef title) = 0;

	/** Enable/disable cancellation by user. */
	virtual void CCL_API setCancelEnabled (tbool state) = 0;

	/** Begin progression. */
	virtual void CCL_API beginProgress () = 0;

	/** End progression. */
	virtual void CCL_API endProgress () = 0;
	
	/** Creates a sub-step progress, must be released by caller. */
	virtual IProgressNotify* CCL_API createSubProgress () = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Processor side
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Update progress text. */
	virtual void CCL_API setProgressText (StringRef text) = 0;

	/** Update progress. */
	virtual void CCL_API updateProgress (const State& state) = 0;

	/** Check if canceled by user. */
	virtual tbool CCL_API isCanceled () = 0;

	DECLARE_IID (IProgressNotify)

	//////////////////////////////////////////////////////////////////////////////////////////////
	
	void updateAnimated () { updateProgress (State (-1., kIndeterminate)); }
	void updateAnimated (StringRef text, bool important = false) { setProgressText (text); updateProgress (State (-1., kIndeterminate | (important ? kImportant : 0))); }
};

DEFINE_IID (IProgressNotify, 0x9fb64ad1, 0x6465, 0x4a59, 0xad, 0x63, 0x69, 0x49, 0x46, 0xd9, 0xa7, 0x4)

//************************************************************************************************
// IProgressDetails
/** Progress details interface.
	\ingroup base_io  */
//************************************************************************************************

interface IProgressDetails: IUnknown
{
	/** Set text of given detail row. */
	virtual tbool CCL_API setDetailText (int index, StringRef text) = 0;
	
	/** Report a warning. */
	virtual tbool CCL_API reportWarning (StringRef text) = 0;

	DECLARE_IID (IProgressDetails)
};

DEFINE_IID (IProgressDetails, 0xddc015d0, 0x1776, 0x4725, 0x82, 0x1, 0x89, 0xc, 0xd8, 0x55, 0x65, 0xcc)

//************************************************************************************************
// IProgressProvider
/** Progress provider interface.
	\ingroup base_io  */
//************************************************************************************************

interface IProgressProvider: IUnknown
{
	/** Create new progress notification interface. */
	virtual IProgressNotify* CCL_API createProgressNotify () = 0;
		
	DECLARE_IID (IProgressProvider)
};

DEFINE_IID (IProgressProvider, 0x87981649, 0xfb22, 0x4ceb, 0x89, 0x3f, 0x7, 0x4c, 0xd, 0x8, 0xb8, 0x21)

//************************************************************************************************
// AbstractProgressNotify
/** \ingroup base_io  */
//************************************************************************************************

class AbstractProgressNotify: public IProgressNotify
{
public:
	//IProgressNotify
	void CCL_API setTitle (StringRef title) override {}
	void CCL_API setCancelEnabled (tbool state) override {}
	void CCL_API beginProgress () override {}
	void CCL_API endProgress () override {}
	IProgressNotify* CCL_API createSubProgress () override { return nullptr; }
	void CCL_API setProgressText (StringRef text) override {}
	void CCL_API updateProgress (const State& state) override {}
	tbool CCL_API isCanceled () override { return false; }
};

//************************************************************************************************
// ProgressNotifyScope
/** \ingroup base_io  */
//************************************************************************************************

struct ProgressNotifyScope
{
	IProgressNotify* progress;

	ProgressNotifyScope (IProgressNotify* progress)
	: progress (progress)
	{
		if(progress)
			progress->retain (),
			progress->beginProgress ();
	}

	ProgressNotifyScope (IProgressProvider* progressProvider, StringRef title, bool canCancel = true)
	: progress (nullptr)
	{
		if(progressProvider)
		{
			progress = progressProvider->createProgressNotify ();
			if(progress)
			{
				progress->setTitle (title);
				progress->setCancelEnabled (canCancel);
				progress->beginProgress ();
			}
		}
	}

	ProgressNotifyScope (ProgressNotifyScope& parent)
	: progress (parent.progress ? parent.progress->createSubProgress () : nullptr)
	{
		if(progress)
			progress->beginProgress ();
	}

	~ProgressNotifyScope ()
	{
		finish ();
	}

	void finish ()
	{
		if(progress)
		{
			progress->endProgress ();
			progress->release ();
			progress = nullptr;
		}
	}
	
	IProgressNotify* operator -> ()
	{
		return progress;
	}
	
	operator IProgressNotify* ()
	{
		return progress;
	}
};

} // namespace CCL

#endif // _ccl_iprogress_h
