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
// Filename    : ccl/gui/help/tutorialviewer.cpp
// Description : Tutorial Viewer
//
//************************************************************************************************

#include "ccl/gui/help/tutorialviewer.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/signalsource.h"

#include "ccl/gui/theme/theme.h"
#include "ccl/gui/theme/thememanager.h"
#include "ccl/gui/dialogs/dialogbuilder.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugins/stubobject.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/iskinmodel.h"
#include "ccl/public/gui/framework/controlsignals.h"

namespace CCL {

//************************************************************************************************
// HelpTutorialHandlerStub
//************************************************************************************************

class HelpTutorialHandlerStub: public StubObject,
							   public IHelpTutorialHandler
{
public:
	DECLARE_STUB_METHODS (IHelpTutorialHandler, HelpTutorialHandlerStub)

	// IHelpTutorialHandler
	void CCL_API onShowTutorialStep (IHelpTutorial& tutorial, StringRef stepId) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("onShowTutorialStep", &tutorial, stepId));
	}

	void CCL_API onTutorialClosed (IHelpTutorial& tutorial) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("onTutorialClosed", &tutorial));
	}
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Stub registration
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (HelpTutorialHandlerStub, kFirstRun)
{
	REGISTER_STUB_CLASS (IHelpTutorialHandler, HelpTutorialHandlerStub)
	return true;
}

//************************************************************************************************
// TutorialViewer
//************************************************************************************************

TutorialViewer* TutorialViewer::createViewerForTutorial (HelpTutorial& tutorial)
{
	if(SkinTutorialViewer::canView (tutorial))
		return NEW SkinTutorialViewer (tutorial);
	else
	{
		CCL_WARN ("Tutorial type not supported: %s\n", MutableCString (tutorial.getContentType ()).str ());
		return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (TutorialViewer, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

TutorialViewer::TutorialViewer (HelpTutorial& tutorial)
: tutorial (tutorial),
  tutorialHandler (nullptr),
  currentStep (-1)
{
	paramList.setController (this);
	paramList.addParam ("prevStep", kPrevStepTag)->enable (false);
	paramList.addParam ("nextStep", kNextStepTag)->enable (false);
	paramList.addParam ("firstStep", kFirstStepTag)->enable (false);
	paramList.addParam ("lastStep", kLastStepTag)->enable (false);
	paramList.addString ("stepInfo", kStepInfoTag)->setReadOnly (true);
	paramList.addParam ("close", kCloseTag);

	SignalSource::addObserver (Signals::kGUI, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TutorialViewer::~TutorialViewer ()
{
	cancelSignals ();
	releaseHandler ();
	SignalSource::removeObserver (Signals::kGUI, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TutorialViewer::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IController)
	QUERY_INTERFACE (IParamObserver)
	QUERY_INTERFACE (IViewFactory)
	QUERY_INTERFACE (ICommandHandler)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* TutorialViewer::runAsync ()
{
	Theme& theme = FrameworkTheme::instance ();
	View* view = unknown_cast<View> (theme.createView ("TutorialViewer", this->asUnknown ()));
	ASSERT (view)
	if(!view)
		return AsyncOperation::createFailed ();

	view->setTitle (tutorial.getTitle ());

	AutoPtr<DialogBuilder> builder = NEW DialogBuilder;
	builder->setTheme (theme);
	Promise promise (builder->runDialogAsync (view));

	return return_shared<IAsyncOperation> (promise.then ([&] (IAsyncOperation& op)
	{
		if(tutorialHandler)
			tutorialHandler->onTutorialClosed (tutorial);
	}));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TutorialViewer::makeHandler ()
{
	if(!tutorialHandler && tutorial.getEventHandlerClassUID ().isValid ())
		tutorialHandler = ccl_new<IHelpTutorialHandler> (tutorial.getEventHandlerClassUID ());
	ASSERT (tutorialHandler || !tutorial.getEventHandlerClassUID ().isValid ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TutorialViewer::releaseHandler ()
{
	if(tutorialHandler)
		ccl_release (tutorialHandler);
	tutorialHandler = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TutorialViewer::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case kPrevStepTag :
		navigateTo (currentStep - 1);
		break;
	case kNextStepTag :
		navigateTo (currentStep + 1);
		break;
	case kFirstStepTag :
		navigateTo (0);
		break;
	case kLastStepTag :
		navigateTo (getStepCount () - 1);
		break;
	case kCloseTag:
		if(contentView)
			if(auto w = contentView->getWindow ())
				w->close ();
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API TutorialViewer::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "content")
	{
		contentView = NEW View (bounds);
		navigateTo (0);
		return contentView;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TutorialViewer::checkCommandCategory (CStringRef category) const
{
	return category == "Navigation";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TutorialViewer::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == "Navigation")
	{
		if(msg.name == "Page Down" || msg.name == "Down" || msg.name == "Right")
		{
			if(!msg.checkOnly ())
				navigateTo (currentStep + 1);
			return true;
		}
		else if(msg.name == "End")
		{
			if(!msg.checkOnly ())
				navigateTo (getStepCount () - 1);
			return true;
		}
		else if(msg.name == "Page Up" || msg.name == "Up" || msg.name == "Left")
		{
			if(!msg.checkOnly ())
				navigateTo (currentStep - 1);
			return true;
		}
		else if(msg.name == "Start")
		{
			if(!msg.checkOnly ())
				navigateTo (0);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TutorialViewer::getStepCount () const
{
	return tutorial.getSteps ().count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const HelpTutorial::Step* TutorialViewer::getStep (int index) const
{
	return static_cast<HelpTutorial::Step*> (tutorial.getSteps ().at (index));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TutorialViewer::navigateTo (int stepIndex)
{
	int stepCount = getStepCount ();
	int newStep = ccl_bound (stepIndex, 0, stepCount-1);
	if(newStep != currentStep)
	{
		currentStep = newStep;
		updateNavigation ();

		const HelpTutorial::Step* step = getStep (currentStep);
		ASSERT (step)
		if(step)
			showStep (*step);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TutorialViewer::updateNavigation ()
{
	int stepCount = getStepCount ();
	String stepInfo;
	stepInfo.appendFormat ("%(1) / %(2)", currentStep+1, stepCount);

	paramList.byTag (kPrevStepTag)->enable (currentStep > 0 && stepCount > 1);
	paramList.byTag (kNextStepTag)->enable (currentStep < stepCount-1 && stepCount > 1);
	paramList.byTag (kFirstStepTag)->enable (currentStep > 0 && stepCount > 1);
	paramList.byTag (kLastStepTag)->enable (currentStep < stepCount-1 && stepCount > 1);
	paramList.byTag (kStepInfoTag)->fromString (stepInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TutorialViewer::showStep (const HelpTutorial::Step& step)
{
	if(tutorialHandler)
		tutorialHandler->onShowTutorialStep (tutorial, step.getID ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TutorialViewer::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kOrientationChanged)
	{
		(NEW Message ("AfterOrientationChanged"))->post (this);
	}
	else if(msg == "AfterOrientationChanged")
	{
		const HelpTutorial::Step* step = getStep (currentStep);
		ASSERT (step)
		if(step)
			showStep (*step);
	}
}

//************************************************************************************************
// SkinTutorialViewer
//************************************************************************************************

bool SkinTutorialViewer::canView (const HelpTutorial& tutorial)
{
	// LATER TODO: Add support for JSON skins
	return tutorial.getContentType () == ThemeManager::instance ().getThemeFileType ().getMimeType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (SkinTutorialViewer, TutorialViewer)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinTutorialViewer::SkinTutorialViewer (HelpTutorial& tutorial)
: TutorialViewer (tutorial),
  stringTable (nullptr),
  theme (nullptr)
{
	// Text
	paramList.addString (HelpTutorial::StepContent::kPrimaryText, kPrimaryTextTag);
	paramList.addString (HelpTutorial::StepContent::kHeadingText, kHeadingTextTag);

	// Images
	paramList.addImage (HelpTutorial::StepContent::kCoverImage, kCoverImageTag);
	paramList.addImage (HelpTutorial::StepContent::kHorizontalContentImage, kHorizontalContentImageTag);
	paramList.addImage (HelpTutorial::StepContent::kVerticalContentImage, kVerticalContentImageTag);

	// Weblink
	paramList.addString (HelpTutorial::StepContent::kLinkTitle, kLinkTitleTag);
	paramList.addString (HelpTutorial::StepContent::kLinkUrl, kLinkUrlTag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinTutorialViewer::~SkinTutorialViewer ()
{
	unloadContent ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* SkinTutorialViewer::runAsync ()
{
	if(!loadContent ())
		return AsyncOperation::createFailed ();

	return SuperClass::runAsync ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SkinTutorialViewer::getProperty (Variant& var, MemberID propertyId) const
{
	auto hasText = [this] (ExtendedParamTags tag) -> bool
	{
		if(auto param = paramList.byTag (tag))
			return !param->getValue ().asString ().isEmpty ();

		return false;
	};

	auto hasImage = [this] (ExtendedParamTags tag) -> bool
	{
		if(UnknownPtr<IImageProvider> provider = paramList.byTag (tag))
			return provider->getImage () != nullptr;

		return false;
	};

	if(propertyId == "hasCover")
	{
		var = hasImage (kCoverImageTag);
		return true;
	}
	else if(propertyId == "hasHorizontalContentImage")
	{
		var = hasImage (kHorizontalContentImageTag);
		return true;
	}
	else if(propertyId == "hasVerticalContentImage")
	{
		var = hasImage (kVerticalContentImageTag);
		return true;
	}
	else if(propertyId == "hasPrimaryText")
	{
		var = hasText (kPrimaryTextTag);
		return true;
	}
	else if(propertyId == "hasLink")
	{
		var = hasText (kLinkUrlTag);
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinTutorialViewer::loadContent ()
{
	ASSERT (!theme) // expected once
	if(theme)
		return true;
	
	// Content package (or folder) needs to contain everything: 
	// - meta info
	// - strings
	// - skin
	
	Url contentPath;
	if(!tutorial.detectContentPath (contentPath))
		return false;

	PackageInfo packageInfo;
	if(!packageInfo.loadFromPackage (contentPath))
		return false;

	MutableCString packageId = packageInfo.getPackageID ();
	if(packageId.isEmpty ())
		return false;
	
	// strings are optional
	ITranslationTable* table = nullptr;
	MutableCString sharedTableId = packageInfo.getString (Meta::kTranslationSharedTableID);
	if(sharedTableId.isEmpty ())
	{
		MutableCString customTableId = packageInfo.getString (Meta::kTranslationTableID);
		MutableCString tableId = customTableId.isEmpty () ? packageId : customTableId;
		System::GetLocaleManager ().loadStrings (table, contentPath, tableId);
		stringTable = table;
	}
	else
		table = System::GetLocaleManager ().getStrings (sharedTableId);

	if(ThemeManager::instance ().loadTheme (theme, contentPath, packageId, table) != kResultOk)
		return false;

	makeHandler ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinTutorialViewer::unloadContent ()
{
	releaseHandler ();
	
	if(theme)
	{
		ThemeManager::instance ().unloadTheme (theme);
		theme = nullptr;
	}

	if(stringTable)
	{
		System::GetLocaleManager ().unloadStrings (stringTable);
		stringTable = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinTutorialViewer::getStepContent (HelpTutorial::StepContent& content, const HelpTutorial::Step& step)
{
	// PLEASE NOTE: Use skin model interfaces instead of implementation classes
	// to keep compatibility with both formats, JSON and XML-based skins.
	
	UnknownPtr<ISkinModel> skinModel (theme);
	ASSERT (skinModel)
	if(!skinModel)
		return false;
	
	auto findChildElement = [] (IContainer* c, StringID name) -> ISkinElement*
	{
		if(c) ForEachUnknown (*c, unk)
			if(UnknownPtr<ISkinElement> e = unk)
			{
				if(e->getName () == name)
					return e;
			}
		EndFor
		return nullptr;
	};

	/**
	 * Lookup image resource element by name attribute.
	 * @param elementName  name of the element to lookup
	 */
	auto findImageElement = [findChildElement] (IContainer* form, ISkinModel* model, StringID elementName) -> ISkinElement*
	{
		auto imageViewElement = findChildElement (form, elementName);
		if(imageViewElement == nullptr)
			return nullptr;

		Variant v;
		imageViewElement->getAttributeValue (v, CanonicalSkinAttributes::kImage);
		MutableCString imageName = v.asString ();

		// Lookup image resource.
		auto* elements = model->getContainerForType (ISkinModel::kImagesElement);
		return findChildElement (elements, imageName);
	};

	MutableCString formName (step.getContentReference ());
	if(UnknownPtr<IContainer> form = findChildElement (skinModel->getContainerForType (ISkinModel::kFormsElement), formName))
	{
		if(auto textElement = findChildElement (form, HelpTutorial::StepContent::kPrimaryText))
		{
			Variant v;
			textElement->getAttributeValue (v, CanonicalSkinAttributes::kTitle);
			content.setPrimaryText (v);
		}

		if(auto textElement = findChildElement (form, HelpTutorial::StepContent::kHeadingText))
		{
			Variant v;
			textElement->getAttributeValue (v, CanonicalSkinAttributes::kTitle);
			content.setHeadingText (v);
		}

		if(UnknownPtr<ISkinImageElement> e = findImageElement (form, skinModel, HelpTutorial::StepContent::kCoverImage))
			content.setCoverImage (e->getImage ());

		if(UnknownPtr<ISkinImageElement> e = findImageElement (form, skinModel, HelpTutorial::StepContent::kHorizontalContentImage))
			content.setHorizontalContentImage (e->getImage ());

		if(UnknownPtr<ISkinImageElement> e = findImageElement (form, skinModel, HelpTutorial::StepContent::kVerticalContentImage))
			content.setVerticalContentImage (e->getImage ());

		if(auto textElement = findChildElement (form, HelpTutorial::StepContent::kLinkUrl))
		{
			Variant v;
			textElement->getAttributeValue (v, CanonicalSkinAttributes::kUrl);
			content.setLinkUrl (v);
		}

		if(auto textElement = findChildElement (form, HelpTutorial::StepContent::kLinkTitle))
		{
			Variant v;
			textElement->getAttributeValue (v, CanonicalSkinAttributes::kTitle);
			content.setLinkTitle (v);

			// Fallback to URL as title.
			if(content.getLinkTitle ().isEmpty ())
				content.setLinkTitle (content.getLinkUrl ());
		}

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinTutorialViewer::showStep (const HelpTutorial::Step& step)
{
	HelpTutorial::StepContent content;
	getStepContent (content, step);

	paramList.byTag (kPrimaryTextTag)->fromString (content.getPrimaryText ());
	paramList.byTag (kHeadingTextTag)->fromString (content.getHeadingText ());

	if(UnknownPtr<IImageProvider> provider = paramList.byTag (kCoverImageTag))
		provider->setImage (content.getCoverImage ());

	if(UnknownPtr<IImageProvider> provider = paramList.byTag (kHorizontalContentImageTag))
		provider->setImage (content.getHorizontalContentImage ());

	if(UnknownPtr<IImageProvider> provider = paramList.byTag (kVerticalContentImageTag))
		provider->setImage (content.getVerticalContentImage ());

	paramList.byTag (kLinkTitleTag)->fromString (content.getLinkTitle ());
	paramList.byTag (kLinkUrlTag)->fromString (content.getLinkUrl ());

	if(contentView)
	{
		contentView->removeAll ();
		auto childView = unknown_cast<View> (FrameworkTheme::instance ().createView ("TutorialViewer.SkinContent", this->asUnknown ()));
		ASSERT (childView)
		if(childView)
			contentView->addView (childView);
	}

	SuperClass::showStep (step);
}
