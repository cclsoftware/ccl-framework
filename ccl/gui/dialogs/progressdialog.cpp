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
// Filename    : ccl/gui/dialogs/progressdialog.cpp
// Description : Progress Dialog
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/dialogs/progressdialog.h"
#include "ccl/gui/dialogs/alert.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/skin/form.h"
#include "ccl/gui/layout/anchorlayout.h"

#include "ccl/app/params.h"

#include "ccl/base/message.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("ProgressDialog")
	XSTRING (Cancelling, "Cancelling...")
END_XSTRINGS

//************************************************************************************************
// ProgressStep
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ProgressStep, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ProgressStep::ProgressStep (ProgressStep* parent)
: parent (parent),
  beginProgressCount (0),
  startTime (0)
{
	paramList.setController (this);
	paramList.addFloat (0.f, 100.f, CSTR ("progressState"), kState);
	paramList.addParam (CSTR ("progressInfinite"), kInfinite)->setValue (false);
	paramList.addString (CSTR ("progressText"), kText);
	paramList.addParam (CSTR ("hasProgressTime"), kHasTime)->setValue (false);
	paramList.addString (CSTR ("progressTime"), kTime);

	for(int i = 0; i < kDetailCount; i++)
	{
		paramList.addIndexedParam (CSTR ("detailVisible"), NEW Parameter, kDetailVisible1 + i);
		paramList.addIndexedParam (CSTR ("detailText"), NEW StringParam, kDetailText1 + i);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ProgressStep::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IProgressNotify)
	QUERY_INTERFACE (IProgressDetails)
	QUERY_INTERFACE (IController)
	QUERY_INTERFACE (IParamObserver)
	
	if(iid == ccl_iid<IProgressDialog> ())
		if(ProgressDialog* progressDialog = getDialog ())
			return progressDialog->queryInterface (iid, ptr);
	
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ProgressDialog* ProgressStep::getDialog ()
{
	if(parent)
		return parent->getDialog ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ProgressStep::createView ()
{
	Theme& theme = FrameworkTheme::instance ();
	View* view = unknown_cast<View> (theme.createView ("ProgressStepView", this->asUnknown ()));
	ASSERT (view != nullptr)
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProgressStep::flushUpdates (bool force)
{
	ProgressDialog* dialog = getDialog ();
	if(dialog)
		dialog->flushUpdates (force);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressStep::setTitle (StringRef t)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressStep::beginProgress ()
{
	if(beginProgressCount++ == 0)
	{
		ProgressDialog* dialog = getDialog ();
		ASSERT (dialog != nullptr)
		if(dialog)
			dialog->addSubProgress (this);

		// reset time remaining
		startTime = System::GetProfileTime ();
		paramList.byTag (kTime)->fromString (String::kEmpty);
		paramList.byTag (kHasTime)->setValue (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressStep::endProgress ()
{
	if(--beginProgressCount == 0)
	{
		ProgressDialog* dialog = getDialog ();
		ASSERT (dialog != nullptr)
		if(dialog)
			dialog->removeSubProgress (this);
	}

	ASSERT (beginProgressCount >= 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IProgressNotify::State ProgressStep::getCurrentState () const
{
	State state;
	IParameter* p = paramList.byTag (kInfinite);
	bool animated = p->getValue ().asBool ();
	if(animated)
		state.flags = kIndeterminate;
	else
	{
		p = paramList.byTag (kState);
		state.value = p->getNormalized ();
	}
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressStep::updateProgress (const State& state)
{
	bool animated = (state.flags & kIndeterminate) != 0;

	IParameter* p = paramList.byTag (kInfinite);
	bool wasAnimated = p->getValue ().asBool ();
	p->setValue (animated);

	if(animated != wasAnimated)
	{
		// reset for timing estimation
		startTime = System::GetProfileTime ();
		paramList.byTag (kHasTime)->setValue (false);
	}

	p = paramList.byTag (kState);
	if(animated)
		p->setNormalized (1.f);
	else
	{
		p->setNormalized ((float)state.value);

		double delta = System::GetProfileTime () - startTime;
		if(delta > 3. && state.value >= 0.001)
		{
			String timeString;
			int seconds = (int)((delta / state.value) - delta);
			if(seconds >= 0)
				timeString = Format::Duration::print (seconds);

			paramList.byTag (kTime)->fromString (timeString);
			paramList.byTag (kHasTime)->setValue (!animated && !timeString.isEmpty ());
		}
	}

	bool important = (state.flags & kImportant) != 0;
	if(important)
	{
		// A deferred change message for the text parameter might still be pending (from setProgressText).
		// In this case, the GUI.flushUpdates in ProgressDialog::flushUpdates would not lead to a redraw with the expected text.
		// Signal the (potential) change synchronously to update a dependent control in time.
		UnknownPtr<ISubject> textParam (paramList.byTag (kText));
		System::GetSignalHandler ().performSignal (textParam, Message (kChanged));
	}

	flushUpdates (important);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressStep::setProgressText (StringRef text)
{
	IParameter* p = paramList.byTag (kText);
	p->fromString (text);

	flushUpdates (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProgressStep::isCanceled ()
{
	ProgressDialog* dialog = getDialog ();
	ASSERT (dialog != nullptr)
	if(dialog)
		return dialog->isCanceled ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressStep::setCancelEnabled (tbool state)
{
	ProgressDialog* dialog = getDialog ();
	ASSERT (dialog != nullptr)
	if(dialog)
		dialog->setCancelEnabled (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IProgressNotify* CCL_API ProgressStep::createSubProgress ()
{
	if(ProgressDialog* dlg = getDialog ())
		return dlg->createStep (this);

	// should not happen:
	ASSERT (getDialog ())
	return NEW ProgressStep (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProgressStep::setDetailText (int index, StringRef text)
{
	ASSERT (index >= 0 && index < kDetailCount)
	if(index < 0 || index >= kDetailCount)
		return false;

	IParameter* pt = paramList.byTag (kDetailText1 + index);
	ASSERT (pt != nullptr)
	pt->setValue (text);

	IParameter* pv = paramList.byTag (kDetailVisible1 + index);
	ASSERT (pv != nullptr)
	pv->setValue (!text.isEmpty ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProgressStep::reportWarning (StringRef text)
{
	if(ProgressDialog* dlg = getDialog ())
		dlg->reportWarning (text);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProgressStep::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "canceled")
	{
		var = const_cast<ProgressStep*> (this)->isCanceled ();
		return true;
	}
	if(propertyId == "detailCount")
	{
		var = kDetailCount;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProgressStep::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "value")
	{
		updateProgress (var.asFloat ());
		return true;
	}
	if(propertyId == "text")
	{
		setProgressText (var.asString ());
		return true;
	}
	if(propertyId == "title")
	{
		setTitle (var.asString ());
		return true;
	}
	if(propertyId == "cancelEnabled")
	{
		setCancelEnabled (var.asBool ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ProgressStep)
	DEFINE_METHOD_NAME ("updateAnimated")
	DEFINE_METHOD_NAME ("beginProgress")
	DEFINE_METHOD_NAME ("endProgress")
END_METHOD_NAMES (ProgressStep)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProgressStep::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "updateAnimated")
	{
		String text;
		if(msg.getArgCount () > 0)
			updateAnimated (msg[0].asString ());
		else
			updateAnimated ();
		return true;
	}
	else if(msg == "beginProgress")
	{
		beginProgress ();
		return true;
	}
	else if(msg == "endProgress")
	{
		endProgress ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ProgressDialog
//************************************************************************************************

IProgressNotify* ProgressDialog::getGlobalIndicator ()
{
	return AlertService::instance ().getProgressReporter ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ProgressDialog* ProgressDialog::firstInstance = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

ProgressDialog* ProgressDialog::getFirstInstance ()
{
	return firstInstance;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (ProgressDialog, ProgressStep)
DEFINE_CLASS_UID (ProgressDialog, 0x70346f66, 0x3984, 0x45b3, 0xa5, 0x7c, 0xa6, 0x10, 0x0, 0xf2, 0x39, 0xc0)

//////////////////////////////////////////////////////////////////////////////////////////////////

ProgressDialog::ProgressDialog ()
: window (nullptr),
  parentWindow (nullptr),
  progressListView (nullptr),
  flags (0),
  lastUpdateTime (0),
  openDelay (0),
  minLevels (1),
  maxLevels (-1)
{
	subProgressList.objectCleanup (true);
	paramList.addParam (CSTR ("progressCancel"), kCancel);
	paramList.addString ("warningMessage", kWarning);
	paramList.addParam ("hasWarning", kHasWarning);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ProgressDialog::~ProgressDialog ()
{
	subProgressList.objectCleanup (true);

	// hmm... can not happen if this is the window controller!
	ASSERT (window == nullptr)
	if(window)
		window->close ();

	ASSERT (firstInstance != this)
	if(firstInstance == this)
		firstInstance = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ProgressDialog* ProgressDialog::getDialog ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ProgressDialog::createView ()
{
	Theme& theme = FrameworkTheme::instance ();
	View* view = unknown_cast<View> (theme.createView ("ProgressDialogView", this->asUnknown ()));
	SOFT_ASSERT (view != nullptr, "cannot create view for progress")
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProgressDialog::flushUpdates (bool force)
{
	if(!window)
		return;

	int64 now = System::GetSystemTicks ();

	#define kUpdateDelay 50
	if(force || (now - lastUpdateTime >= kUpdateDelay))
	{
		lastUpdateTime = now;

		CCL_PROFILE_START (ProgressDialog_flushUpdates)
		GUI.flushUpdates (false);
		CCL_PROFILE_STOP (ProgressDialog_flushUpdates)
		CCL_PROFILE_START (ProgressDialog_flushWindowEvents)
		GUI.flushWindowEvents (window);
		CCL_PROFILE_STOP (ProgressDialog_flushWindowEvents)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ void ProgressDialog::flushAll (View* caller)
{
	GUI.flushUpdates (false);

	Window* window = firstInstance ? firstInstance->window : nullptr;

	#if 1 // TEST: use incoming window to avoid the system from treating us as unresponsive
	if(window == nullptr && caller)
		window = caller->getWindow ();
	#endif

	if(window)
		GUI.flushWindowEvents (window);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API ProgressDialog::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "ProgressStepList")
	{
		progressListView = NEW BoxLayoutView (bounds, StyleFlags (Styles::kVertical));
		progressListView->setSizeMode (View::kFitSize);
		((BoxLayoutView*)progressListView)->setMargin (0);
		((BoxLayoutView*)progressListView)->setSpacing (0);

		int numStepViews = 0;

		View* view = ProgressStep::createView ();
		if(view)
		{
			progressListView->addView (view);
			numStepViews++;
		}

		ForEach (subProgressList, ProgressStep, step)
			if(maxLevels >= 0 && numStepViews >= maxLevels)
				break;

			view = step->createView ();
			if(view)
			{
				progressListView->addView (view);
				numStepViews++;
			}
		EndFor

		return progressListView;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProgressDialog::reportWarning (StringRef text)
{
	paramList.byTag (kWarning)->setValue (text);
	paramList.byTag (kHasWarning)->setValue (true);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressDialog::constrainLevels (int min, int max)
{
	minLevels = ccl_max (min, 1);
	maxLevels = ccl_max (max, -1);

	// check if step views can be removed
	ForEachReverse (subProgressList, ProgressStep, step)
		if(step->getParent () == nullptr)
		{
			if(!canRemoveStepView ())
				break;

			removeSubProgress (step);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ProgressDialog::countStepViews () const
{
	int count = 0;
	if(progressListView)
		ForEachViewFast (*progressListView, v)
			count++;
		EndFor
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProgressDialog::canAddStepView () const
{
	return maxLevels < 0 || countStepViews () < maxLevels;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProgressDialog::canRemoveStepView () const
{
	return minLevels <= 1 || countStepViews () > minLevels;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IProgressNotify* ProgressDialog::createStep (ProgressStep* parent)
{
	ForEach (subProgressList, ProgressStep, step)
		if(step->getParent () == nullptr)
		{
			// reuse this unused step
			step->setParent (parent);
			step->retain ();
			ASSERT (subProgressList.index (step) == subProgressList.index (parent) + 1)
			CCL_PRINTF ("ProgressDialog::createStep: reuse step %d of %d\n", subProgressList.index (step), subProgressList.count ())
			return step;
		}
	EndFor

	// create a new step
	ProgressStep* step = NEW ProgressStep (parent);
	step->retain ();
	subProgressList.add (step);
	return step;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProgressDialog::addSubProgress (ProgressStep* step)
{
	if(progressListView && canAddStepView ())
	{
		View* view = step->createView ();
		if(view)
			progressListView->addView (view);
	}

	flushUpdates ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProgressDialog::removeSubProgress (ProgressStep* step)
{
	if(canRemoveStepView ())
	{
		if(progressListView)
		{
			ForEachView (*progressListView, view)
				if(isEqualUnknown (view->getController (), ccl_as_unknown (step)))
				{
					progressListView->removeView (view);
					view->release ();
					break;
				}
			EndFor
		}

		subProgressList.remove (step);
		step->release ();
	}
	else
	{
		// mark step as unused, keep step and view for later reuse
		step->setParent (nullptr);
	}

	flushUpdates ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressDialog::setTitle (StringRef t)
{
	title = t;

	if(window)
		window->setTitle (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressDialog::setOpenDelay (double seconds, tbool showWaitCursorBeforeOpen)
{
	openDelay = seconds;
	showWaitCursor (showWaitCursorBeforeOpen != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressDialog::setTranslucentAppearance (tbool state)
{
	translucent (state != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressDialog::setParentWindow (IWindow* window)
{
	parentWindow = unknown_cast<Window> (window);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProgressDialog::openWindow ()
{
	if(!window)
	{
		#if DEBUG
		if(DragSession::getActiveSession ())
			Debugger::println ("WARNING: Drag'n'Drop still active when opening ProgressDialog. Should be deferred!");
		#endif

		if(waitCursorShown ())
		{
			System::GetGUI ().setWaitCursor (false);
			waitCursorShown (false);
		}

		Form* form = ccl_cast<Form> (createView ());
		SOFT_ASSERT (form != nullptr, "cannot open window for progress")
		if(form != nullptr)
		{
			if(!title.isEmpty ())
				form->setTitle (title);

			StyleFlags style (form->getWindowStyle());
			if(translucent ())
				style.custom &= ~Styles::kWindowAppearanceTitleBar;

			style.custom |= Styles::kWindowAppearanceDropShadow;
			style.custom |= Styles::kWindowBehaviorProgressDialog;
			form->setWindowStyle (style);

			window = form->open (parentWindow);
			window->setCollectUpdates (true); // use invalidation for controls, otherwise it's too slow on Mac

			if(translucent ())
				window->setOpacity (.8f);

			window->addHandler (this);
			window->retain ();

			ASSERT (window == Desktop.getTopWindow (kPopupLayer)) // added in Form::open

			flushUpdates (true);
			#if CCL_PLATFORM_WINDOWS
			Win32Window::cast (window)->sendNCActivate ();
			#endif
		}

		// feed global
		if(window)
		{
			if(IProgressNotify* p = getGlobalIndicator ())
			{
				p->beginProgress ();
				p->updateProgress (getCurrentState ()); // assign initial state
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressDialog::beginProgress ()
{
	if(beginProgressCount++ == 0)
	{
		ASSERT (window == nullptr)

		if(openDelay <= 0)
			openWindow ();

		if(firstInstance == nullptr)
			firstInstance = this;

		startTime = System::GetProfileTime ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressDialog::endProgress ()
{
	if(--beginProgressCount == 0)
	{
		// feed global
		if(window)
			if(IProgressNotify* p = getGlobalIndicator ())
				p->endProgress ();

		if(firstInstance == this)
			firstInstance = nullptr;

		if(waitCursorShown ())
		{
			System::GetGUI ().setWaitCursor (false);
			waitCursorShown (false);
		}

		if(window)
		{
			Desktop.removeWindow (window);
			window->removeHandler (this);
			window->close ();
			window->release ();
			window = nullptr;
			progressListView = nullptr;
		}
	}
	ASSERT (beginProgressCount >= 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressDialog::updateProgress (const State& state)
{
	double startTimeOld = startTime;

	ProgressStep::updateProgress (state);

	if(!window)
	{
		startTime = startTimeOld; // is reset when animated state has changed
		double now = System::GetProfileTime ();
		double timePassed = now - startTime;

		if(timePassed >= openDelay)
			openWindow ();
		else if(showWaitCursor () && waitCursorShown () == false && (timePassed > 0.1 || (state.flags & kImportant) != 0))
		{
			System::GetGUI ().setWaitCursor (true);
			waitCursorShown (true);
		}
	}

	// feed global
	if(window)
		if(IProgressNotify* p = getGlobalIndicator ())
			p->updateProgress (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressDialog::tryCancel ()
{
	if(isCancelEnabled ())
		cancel (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressDialog::hideWindow (tbool state)
{
	if(window)
	{
		if(state)
			window->hide ();
		else
			window->show ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProgressDialog::isCanceled ()
{
	return canceled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProgressDialog::setCancelEnabled (tbool state)
{
	paramList.byTag (kCancel)->enable (state);
	signal (Message (kPropertyChanged));
	flushUpdates (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProgressDialog::isCancelEnabled () const
{
	return paramList.byTag (kCancel)->isEnabled () != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProgressDialog::cancel (bool flush)
{
	if(!canceled ())
	{
		setTitle (XSTR (Cancelling));
		paramList.byTag (kCancel)->enable (false);
		if(flush == true)
			flushUpdates (true);

		// set state after flushUpdates, to avoid accessing it too early (via IProgressNotify::isCanceled) during flushUpdates (e.g. idle)
		canceled (true);
		signal (Message (kCancelButtonHit));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProgressDialog::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isCancelEnabled")
	{
		var = isCancelEnabled ();
		return true;
	}

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProgressDialog::paramChanged (IParameter* param)
{
	if(param->getTag () == kCancel)
		cancel ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProgressDialog::onWindowEvent (WindowEvent& windowEvent)
{
	// avoid closing the window during progress mode
	if(windowEvent.eventType == WindowEvent::kClose)
	{
		if(isCancelEnabled ()) // same as pressing the cancel button
			cancel ();

		return false;
	}
	return true;
}

//************************************************************************************************
// ModalProgressDialog
//************************************************************************************************

DEFINE_CLASS (ModalProgressDialog, ProgressDialog)
DEFINE_CLASS_UID (ModalProgressDialog, 0x75bd62fa, 0xe314, 0x49a2, 0x87, 0xff, 0xfa, 0x7b, 0x3, 0xcd, 0xbd, 0x16)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ModalProgressDialog::run ()
{
	ASSERT (window == nullptr) // must not reenter!
	if(window)
		return;

	Form* form = ccl_cast<Form> (createView ());
	ASSERT (form != nullptr)
	if(!form)
		return;

	if(!title.isEmpty ())
		form->setTitle (title);

	StyleFlags windowStyle (form->getWindowStyle ());
	windowStyle.custom |= Styles::kWindowBehaviorCenter;
	windowStyle.custom |= Styles::kWindowBehaviorProgressDialog;

	Dialog dialog (form->getSize (), windowStyle, form->getTitle ());
	dialog.setName (CCLSTR ("ModalProgressDialog"));
	dialog.addView (form);
	dialog.setSizeMode (View::kAttachAll);
	dialog.addHandler (this);
	dialog.setCollectUpdates (true); // use invalidation for controls, otherwise it's too slow on Mac
	dialog.setController (this->asUnknown ());

	if(firstInstance == nullptr)
		firstInstance = this;

	startTime = System::GetProfileTime ();

	// feed global
	if(IProgressNotify* p = getGlobalIndicator ())
		p->beginProgress ();

	updateAnimated (); // set indeterminate state

	ScopedVar<Window*> dialogScope (this->window, &dialog);
	dialog.showModal ();

	// feed global
	if(IProgressNotify* p = getGlobalIndicator ())
		p->endProgress ();

	if(firstInstance == this)
		firstInstance = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ModalProgressDialog::close ()
{
	if(Dialog* dialog = ccl_cast<Dialog> (window))
	{
		dialog->removeHandler (this);
		dialog->close ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ModalProgressDialog::beginProgress ()
{ /** nothing here! */ }

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ModalProgressDialog::endProgress ()
{ /** nothing here! */ }

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ModalProgressDialog::updateProgress (const State& state)
{
	ProgressStep::updateProgress (state);

	// (avoid base class behavior here!)

	// feed global
	if(IProgressNotify* p = getGlobalIndicator ())
		p->updateProgress (state);
}
