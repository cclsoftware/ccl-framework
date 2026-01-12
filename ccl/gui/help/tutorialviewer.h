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
// Filename    : ccl/gui/help/tutorialviewer.h
// Description : Tutorial Viewer
//
//************************************************************************************************

#ifndef _ccl_tutorialviewer_h
#define _ccl_tutorialviewer_h

#include "ccl/gui/help/helptutorial.h"
#include "ccl/gui/views/view.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/paramlist.h"
#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/gui/icommandhandler.h"

namespace CCL {

interface ITheme;

//************************************************************************************************
// TutorialViewer
//************************************************************************************************

class TutorialViewer: public Object,
					  public AbstractController,
					  public IParamObserver,
					  public IViewFactory,
					  public ICommandHandler
{
public:
	DECLARE_CLASS_ABSTRACT (TutorialViewer, Object)

	TutorialViewer (HelpTutorial& tutorial);
	~TutorialViewer ();

	static TutorialViewer* createViewerForTutorial (HelpTutorial& tutorial);

	virtual IAsyncOperation* runAsync ();

	// IController
	DECLARE_PARAMETER_LOOKUP (paramList)

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override {}

	// IViewFactory
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACES (Object)

protected:
	enum ParamTags
	{
		kPrevStepTag = 100,
		kNextStepTag,
		kFirstStepTag,
		kLastStepTag,
		kStepInfoTag,
		kCloseTag
	};

	ParamList paramList;
	HelpTutorial& tutorial;
	IHelpTutorialHandler* tutorialHandler;
	int currentStep;
	ObservedPtr<View> contentView;

	void makeHandler ();
	void releaseHandler ();
	int getStepCount () const;
	const HelpTutorial::Step* getStep (int index) const;
	void navigateTo (int stepIndex);
	void updateNavigation ();	

	virtual void showStep (const HelpTutorial::Step& step);
};

//************************************************************************************************
// SkinTutorialViewer
//************************************************************************************************

class SkinTutorialViewer: public TutorialViewer
{
public:
	DECLARE_CLASS_ABSTRACT (SkinTutorialViewer, TutorialViewer)

	SkinTutorialViewer (HelpTutorial& tutorial);
	~SkinTutorialViewer ();

	static bool canView (const HelpTutorial& tutorial);

	// TutorialViewer
	IAsyncOperation* runAsync () override;
	CCL::tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const override;

protected:
	enum ExtendedParamTags
	{
		kPrimaryTextTag = 200,
		kHeadingTextTag,
		kCoverImageTag,
		kHorizontalContentImageTag,
		kVerticalContentImageTag,
		kLinkTitleTag,
		kLinkUrlTag
	};

	ITranslationTable* stringTable;
	ITheme* theme;

	bool loadContent ();
	void unloadContent ();
	bool getStepContent (HelpTutorial::StepContent& content, const HelpTutorial::Step& step);

	// TutorialViewer
	void showStep (const HelpTutorial::Step& step) override;
};

} // namespace CCL

#endif // _ccl_tutorialviewer_h
