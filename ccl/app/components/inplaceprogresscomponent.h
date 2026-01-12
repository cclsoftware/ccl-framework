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
// Filename    : ccl/app/components/inplaceprogresscomponent.h
// Description : Inplace Progress Component
//
//************************************************************************************************

#ifndef _ccl_inplaceprogresscomponent_h
#define _ccl_inplaceprogresscomponent_h

#include "ccl/app/component.h"

#include "ccl/public/gui/framework/iview.h"

#include "ccl/public/base/iprogress.h"

namespace CCL {

//************************************************************************************************
// InplaceProgressComponent
//************************************************************************************************

class InplaceProgressComponent: public Component,
                                public AbstractProgressNotify
{
public:
	DECLARE_CLASS_ABSTRACT (InplaceProgressComponent, Component)
	
	InplaceProgressComponent (StringRef name = nullptr);
	
	void setActivationDelay (double delaySeconds);
	void setParentView (IView* view);
	bool hasParentView () const;
	bool isInProgress () const;
	double getProgressValue () const;
	bool cancelProgress ();
	bool isCancelEnabled () const;

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;

	// IProgressNotify
	void CCL_API setTitle (StringRef title) override;
	void CCL_API setCancelEnabled (tbool state) override;
	void CCL_API beginProgress () override;
	void CCL_API endProgress () override;
	void CCL_API setProgressText (StringRef text) override;
	void CCL_API updateProgress (const State& state) override;
	tbool CCL_API isCanceled () override;
	IProgressNotify* CCL_API createSubProgress () override;

	CLASS_INTERFACE (IProgressNotify, Component)

private:
	int64 lastFlush;
	double startTime;
	double activationDelay;
	int recentTimeRemaining;
	ViewPtr parentView;
	bool canceled;
	int beginProgressCount;

	void flushUpdates (bool force = false);
};

} // namespace CCL 

#endif // _ccl_inplaceprogresscomponent_h
