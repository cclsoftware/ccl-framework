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
// Filename    : ccl/app/components/startupprogresscomponent.h
// Description : Startup Progress Component
//
//************************************************************************************************

#ifndef _ccl_startupprogresscomponent_h
#define _ccl_startupprogresscomponent_h

#include "ccl/app/component.h"

#include "ccl/app/components/istartupprogress.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/base/iprogress.h"

namespace CCL {

interface IView;
class InplaceProgressComponent;

//************************************************************************************************
// StartupProgressComponent
//************************************************************************************************

class StartupProgressComponent: public Component,
								public IStartupProgress,
								public ComponentSingleton<StartupProgressComponent>
{
public:
	DECLARE_CLASS_ABSTRACT (StartupProgressComponent, Component)

	enum Flags
	{
		kIsComponentPhase = 1<<0 ///< starting this phase will collect startup information from component initialization
	};
	
	static void removeInstance ();
	
	StartupProgressComponent ();
	~StartupProgressComponent ();
	
	PROPERTY_BOOL (terminationAllowed, TerminationAllowed)
	
	bool addPhase (int id, double workUnits, StringRef title = nullptr, int flags = 0);
	IProgressNotify* startPhase (int id);
	void endProgress ();
	void setParentView (IView* view);

	// Component	
	tbool CCL_API canTerminate () const override;
	
	CLASS_INTERFACE (IStartupProgress, Component)
	
protected:
	class PhaseDescription;
	class Phase;
	
	InplaceProgressComponent* progressComponent;
	Phase* currentPhase;
	double workUnitsDone;
	ObjectArray phaseDescriptions;
	Vector<IComponent*> pendingStartupComponents;
	int totalStartupComponents;
	int startupComponentsDone;
	
	bool collectStartupComponents ();
	PhaseDescription* findPhaseDescription (int id) const;
	double getTotalWorkUnits () const;
	void setPhaseProgressText (StringRef text);
	void updateTotalProgress ();
	
	// IStartupProgress
	void CCL_API declareStartupComponent (IComponent* component) override;
	void CCL_API reportStartup (IComponent* component, StringRef title) override;
	void CCL_API reportStartupDone (IComponent* component) override;
};

} // namespace CCL

#endif // _ccl_startupprogresscomponent_h
