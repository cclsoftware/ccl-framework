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
// Filename    : ccl/app/components/startupprogresscomponent.cpp
// Description : Startup Progress Component
//
//************************************************************************************************

#include "ccl/app/components/startupprogresscomponent.h"

#include "ccl/app/components/inplaceprogresscomponent.h"

#include "ccl/base/message.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/iwindow.h"

namespace CCL {

//************************************************************************************************
// StartupProgressComponent::PhaseDescription
//************************************************************************************************

class StartupProgressComponent::PhaseDescription: public Object
{
public:
	PhaseDescription (int id, double workUnits, StringRef title, int flags)
	: id (id),
	  workUnits(workUnits),
	  title (title),
	  flags (flags)
	{}
	
	PROPERTY_VARIABLE (int, id, ID)
	PROPERTY_VARIABLE (double, workUnits, WorkUnits)
	PROPERTY_STRING (title, Title)
	PROPERTY_FLAG (flags, kIsComponentPhase, isComponentPhase)
	
protected:
	int flags;
};

//************************************************************************************************
// StartupProgressComponent::Phase
//************************************************************************************************

class StartupProgressComponent::Phase: public Object,
									   public AbstractProgressNotify
{
public:
	DECLARE_CLASS_ABSTRACT (Phase, Object)
	
	Phase (StartupProgressComponent& component, double totalWorkUnits = 0.);
	
	double getWorkUnits () const;
	double getWorkUnitsDone () const;
	
	// IProgressNotify
	void CCL_API setProgressText (StringRef text) override;
	void CCL_API updateProgress (const State& state) override;
		
	CLASS_INTERFACE (IProgressNotify, Object)
	
protected:
	StartupProgressComponent& component;
	double workUnits;
	double workUnitsDone;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum StartupProgressComponent
	{
		kPhaseTitle
	};
}

//************************************************************************************************
// IStartupProgress
//************************************************************************************************

DEFINE_IID_ (IStartupProgress, 0x6690a7e4, 0xb331, 0x2a45, 0xa0, 0x86, 0xf7, 0x71, 0x81, 0xf3, 0xe1, 0x93)
DEFINE_STRINGID_MEMBER_ (IStartupProgress, kCollectStartupComponents, "collectStartupComponents")

IStartupProgress* IStartupProgress::getInstance ()
{
	return RootComponent::instance ().findChildByInterface<IStartupProgress> ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void IStartupProgress::declareStartup (MessageRef msg, IComponent* component)
{
	UnknownPtr<IStartupProgress> progress (msg[0]);
	if(progress)
		progress->declareStartupComponent (component);
}

//************************************************************************************************
// StartupProgressComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StartupProgressComponent, Component)
DEFINE_COMPONENT_SINGLETON (StartupProgressComponent)

/////////////////////////////////////////////////////////////////////////////////////////////////

void StartupProgressComponent::removeInstance ()
{
	if(auto* startupProgress = StartupProgressComponent::peekInstance ())
	{
		startupProgress->terminate ();
		RootComponent::instance ().removeChild (startupProgress);
		startupProgress->release ();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

StartupProgressComponent::StartupProgressComponent ()
: Component ("StartupProgress"),
  terminationAllowed (false),
  workUnitsDone (0.),
  totalStartupComponents (0),
  startupComponentsDone (0),
  progressComponent (nullptr),
  currentPhase (nullptr)
{
	phaseDescriptions.objectCleanup (true);
	progressComponent = NEW InplaceProgressComponent ("Progress");
	progressComponent->setCancelEnabled (false);
	addChild (progressComponent);
	paramList.addString ("phaseTitle", Tag::kPhaseTitle);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

StartupProgressComponent::~StartupProgressComponent ()
{
	safe_release (currentPhase);
	pendingStartupComponents.removeAll ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool StartupProgressComponent::addPhase (int id, double workUnits, StringRef title, int flags)
{
	if(PhaseDescription* phase = findPhaseDescription (id))
		return false;
	
	phaseDescriptions.add (NEW PhaseDescription (id, workUnits, title, flags));
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

IProgressNotify* StartupProgressComponent::startPhase (int id)
{
	PhaseDescription* phaseDescription = findPhaseDescription (id);
	if(!phaseDescription)
		return nullptr;
	
	totalStartupComponents = 0;
	startupComponentsDone = 0;
	pendingStartupComponents.removeAll ();
	
	if(currentPhase)
	{
		workUnitsDone += currentPhase->getWorkUnits ();
		safe_release (currentPhase);
	}
	else
		progressComponent->beginProgress ();
	
	if(phaseDescription->isComponentPhase ())
		collectStartupComponents ();
	
	currentPhase = NEW Phase (*this, phaseDescription->getWorkUnits ());
	paramList.byTag (Tag::kPhaseTitle)->setValue (phaseDescription->getTitle ());
	setPhaseProgressText (String::kEmpty);

	return currentPhase;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void StartupProgressComponent::endProgress ()
{
	progressComponent->endProgress ();
	workUnitsDone = 0.;
	pendingStartupComponents.removeAll ();
	totalStartupComponents = 0;
	startupComponentsDone = 0;
	safe_release (currentPhase);
	paramList.byTag (Tag::kPhaseTitle)->setValue (String::kEmpty);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void StartupProgressComponent::setParentView (IView *view)
{
	progressComponent->setParentView (view);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StartupProgressComponent::canTerminate () const
{
	// prevent program termination during startup if not set otherwise
	return isTerminationAllowed ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool StartupProgressComponent::collectStartupComponents ()
{
	auto visitStartupSources = [&] (ObjectNode* node)
	{
		node->notify (this, Message (IStartupProgress::kCollectStartupComponents, this->asUnknown ()));
		return true;
	};
	
	RootComponent::instance ().visitChildren (visitStartupSources, true);
	
	totalStartupComponents = pendingStartupComponents.count ();
	
	return totalStartupComponents != 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

StartupProgressComponent::PhaseDescription* StartupProgressComponent::findPhaseDescription (int id) const
{
	for(auto* phaseDescription : iterate_as<PhaseDescription> (phaseDescriptions))
		if(phaseDescription->getID () == id)
			return phaseDescription;
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

double StartupProgressComponent::getTotalWorkUnits () const
{
	double totalWorkUnits = 0.;
	for(auto* phaseDescription : iterate_as<PhaseDescription> (phaseDescriptions))
		totalWorkUnits += phaseDescription->getWorkUnits ();
	return totalWorkUnits;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void StartupProgressComponent::setPhaseProgressText (StringRef text)
{
	progressComponent->setProgressText (text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void StartupProgressComponent::updateTotalProgress ()
{
	if(currentPhase)
		progressComponent->updateProgress ((workUnitsDone + currentPhase->getWorkUnitsDone ()) / getTotalWorkUnits ());
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StartupProgressComponent::declareStartupComponent (IComponent* component)
{
	pendingStartupComponents.addOnce (component);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StartupProgressComponent::reportStartup (IComponent* component, StringRef title)
{
	if(totalStartupComponents != 0 && pendingStartupComponents.contains (component))
	{
		if(currentPhase)
		{
			setPhaseProgressText (title);
			currentPhase->updateProgress ((double)startupComponentsDone / (double)totalStartupComponents);
		}
	}
	updateTotalProgress ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StartupProgressComponent::reportStartupDone (IComponent *component)
{
	if(totalStartupComponents != 0 && pendingStartupComponents.contains (component))
	{
		pendingStartupComponents.remove (component);
		startupComponentsDone++;
		if(currentPhase)
			currentPhase->updateProgress ((double)startupComponentsDone / (double)totalStartupComponents);
	}
}

//************************************************************************************************
// StartupProgressPhase
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StartupProgressComponent::Phase, Object)

/////////////////////////////////////////////////////////////////////////////////////////////////

StartupProgressComponent::Phase::Phase (StartupProgressComponent& component, double totalWorkUnits)
: component (component),
  workUnits (totalWorkUnits)
{
	if(totalWorkUnits <= 0)
		totalWorkUnits = 1.;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

double StartupProgressComponent::Phase::getWorkUnits () const
{
	return workUnits;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

double StartupProgressComponent::Phase::getWorkUnitsDone () const
{
	return workUnitsDone;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StartupProgressComponent::Phase::setProgressText (StringRef text)
{
	component.setPhaseProgressText (text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StartupProgressComponent::Phase::updateProgress (const State& state)
{
	workUnitsDone = workUnits * state.value;
	component.updateTotalProgress ();
}
