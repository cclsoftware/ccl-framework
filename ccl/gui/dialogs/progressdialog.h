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
// Filename    : ccl/gui/dialogs/progressdialog.h
// Description : Progress Dialog
//
//************************************************************************************************

#ifndef _ccl_progressdialog_h
#define _ccl_progressdialog_h

#include "ccl/base/collections/objectlist.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/gui/paramlist.h"
#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

class View;
class Window;
class ProgressDialog;

//************************************************************************************************
// ProgressStep
//************************************************************************************************

class ProgressStep: public Object,
					public IProgressNotify,
					public IProgressDetails,
					public AbstractController,
					public IParamObserver
{
public:
	DECLARE_CLASS (ProgressStep, Object)
	DECLARE_METHOD_NAMES (ProgressStep)

	ProgressStep (ProgressStep* parent = nullptr);

	PROPERTY_POINTER (ProgressStep, parent, Parent)

	virtual ProgressDialog* getDialog ();
	virtual View* createView ();
	virtual void flushUpdates (bool force = false);

	State getCurrentState () const;

	// IProgressNotify
	void CCL_API setTitle (StringRef title) override;
	void CCL_API beginProgress () override;
	void CCL_API endProgress () override;
	void CCL_API updateProgress (const State& state) override;
	void CCL_API setProgressText (StringRef text) override;
	tbool CCL_API isCanceled () override;
	void CCL_API setCancelEnabled (tbool state) override;
	IProgressNotify* CCL_API createSubProgress () override;

	// IProgressDetails
	tbool CCL_API setDetailText (int index, StringRef text) override;
	tbool CCL_API reportWarning (StringRef text) override;
	
	// IController
	DECLARE_PARAMETER_LOOKUP (paramList)

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override { return false; }
	void CCL_API paramEdit (IParameter* param, tbool begin) override {}

	CLASS_INTERFACES (Object)

protected:
	enum Tags
	{
		kState = 'Stat',
		kText = 'Text',
		kTime = 'Time',
		kHasTime = 'HasT',
		kInfinite = 'Infi',
		kDetailVisible1 = 100,
		kDetailText1 = 200,
		kDetailCount = 3
	};

	ParamList paramList;
	int beginProgressCount;
	double startTime;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// ProgressDialog
//************************************************************************************************

class ProgressDialog: public ProgressStep,
					  public IProgressDialog,
					  public IViewFactory,
					  public IWindowEventHandler
{
public:
	DECLARE_CLASS (ProgressDialog, ProgressStep)

	ProgressDialog ();
	~ProgressDialog ();

	static ProgressDialog* getFirstInstance ();
	static void flushAll (View* caller = nullptr);

	void addSubProgress (ProgressStep* step);
	void removeSubProgress (ProgressStep* step);
	IProgressNotify* createStep (ProgressStep* parent);
	bool isCancelEnabled () const;
	
	// ProgressStep
	ProgressDialog* getDialog () override;
	View* createView () override;
	void flushUpdates (bool force = false) override;
	void CCL_API setTitle (StringRef title) override;
	void CCL_API beginProgress () override;
	void CCL_API endProgress () override;
	void CCL_API updateProgress (const State& state) override;
	tbool CCL_API isCanceled () override;
	void CCL_API setCancelEnabled (tbool state) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override {}
	tbool CCL_API reportWarning (StringRef text) override;

	// IProgressDialog
	void CCL_API constrainLevels (int min, int max) override;
	void CCL_API setOpenDelay (double seconds, tbool showWaitCursorBeforeOpen) override;
	void CCL_API setTranslucentAppearance (tbool state) override;
	void CCL_API setParentWindow (IWindow* window) override;
	void CCL_API tryCancel () override;
	void CCL_API hideWindow (tbool state) override;

	// IViewFactory
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	// IWindowEventHandler
	tbool CCL_API onWindowEvent (WindowEvent& windowEvent) override;

	CLASS_INTERFACE3 (IProgressDialog, IViewFactory, IWindowEventHandler, ProgressStep)

protected:
	enum Tags
	{
		kCancel = 'Canc',
		kWarning = 'Warn',
		kHasWarning = 'HsWa'
	};

	Window* window;
	Window* parentWindow;
	View* progressListView;
	ObjectList subProgressList;
	int64 lastUpdateTime;
	double openDelay;
	String title;
	int minLevels;
	int maxLevels;
	int flags;

	PROPERTY_FLAG (flags, 1<<0, canceled)
	PROPERTY_FLAG (flags, 1<<1, translucent)
	PROPERTY_FLAG (flags, 1<<2, showWaitCursor)
	PROPERTY_FLAG (flags, 1<<3, waitCursorShown)

	void openWindow ();
	int countStepViews () const;
	bool canAddStepView () const;
	bool canRemoveStepView () const;
	void cancel (bool flush = true);

	static ProgressDialog* firstInstance;
	static IProgressNotify* getGlobalIndicator ();

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// ModalProgressDialog
//************************************************************************************************

class ModalProgressDialog: public ProgressDialog,
						   public IModalProgressDialog
{
public:
	DECLARE_CLASS (ModalProgressDialog, ProgressDialog)

	// IModalProgressDialog
	void CCL_API run () override;
	void CCL_API close () override;

	// ProgressDialog
	void CCL_API beginProgress () override;
	void CCL_API endProgress () override;
	void CCL_API updateProgress (const State& state) override;

	CLASS_INTERFACE (IModalProgressDialog, ProgressDialog)
};

} // namespace CCL

#endif // _ccl_progressdialog_h
