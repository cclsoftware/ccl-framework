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
// Filename    : ccl/platform/win/gui/taskbar.cpp
// Description : Task Bar
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/win/gui/taskbar.h"

#include "ccl/public/gui/framework/iwindow.h"

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// TaskBarDelegate
//************************************************************************************************

DEFINE_CLASS (TaskBarDelegate, Object)
DEFINE_CLASS_UID (TaskBarDelegate, 0x6c0c1c5b, 0x6a4f, 0x46d0, 0x91, 0xe8, 0x9b, 0x78, 0x6a, 0x2f, 0x57, 0x68)
DEFINE_CLASS_NAMESPACE (TaskBarDelegate, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TaskBarDelegate::queryInterface (UIDRef iid, void** ptr)
{
	if(iid == ccl_iid<ITaskBar> ())
	{
		AutoPtr<TaskBar> taskBar = TaskBar::instance ();
		return taskBar->queryInterface (iid, ptr);
	}
	else
		return SuperClass::queryInterface (iid, ptr);
}

//************************************************************************************************
// TaskBar
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TaskBar, Object)
DEFINE_CLASS_UID (TaskBar, 0x6c0c1c5b, 0x6a4f, 0x46d0, 0x91, 0xe8, 0x9b, 0x78, 0x6a, 0x2f, 0x57, 0x68) // keep it here for instance association!
DEFINE_SHARED_SINGLETON (TaskBar)

//////////////////////////////////////////////////////////////////////////////////////////////////

TaskBar::TaskBar ()
: hwndApp (nullptr),
  savedProgressState (TBPF_NOPROGRESS)
{
	HRESULT hr = ::CoCreateInstance (CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskbarList, taskBarList);
	ASSERT (SUCCEEDED (hr))
	if(SUCCEEDED (hr))
	{
		hr = taskBarList->HrInit ();
		ASSERT (SUCCEEDED (hr))

		taskBarList->QueryInterface (IID_ITaskbarList4, taskBarList4);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TaskBar::~TaskBar ()
{
	taskBarList.release (); // place for breakpoint here
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IProgressNotify* CCL_API TaskBar::getProgressBar (IWindow* window)
{
	hwndApp = window ? (HWND)window->getSystemWindow () : nullptr;
	ASSERT (hwndApp)

	// Windows 7 and above
	return taskBarList4 && hwndApp ? this : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TaskBar::beginProgress ()
{
	CCL_PRINTLN ("TaskBar begin progress")
	ASSERT (taskBarList4 && hwndApp)

	if(savedProgressState == TBPF_INDETERMINATE)
	{
		// updateProgress() has been called before beginProgress()
		HRESULT hr = taskBarList4->SetProgressState (hwndApp, TBPF_INDETERMINATE);
		ASSERT (SUCCEEDED (hr))
	}
	else
	{
		savedProgressState = TBPF_NORMAL;
		HRESULT hr = taskBarList4->SetProgressValue (hwndApp, 0, 100);
		ASSERT (SUCCEEDED (hr))
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TaskBar::endProgress ()
{
	CCL_PRINTLN ("TaskBar end progress")
	ASSERT (taskBarList4 && hwndApp)

	savedProgressState = TBPF_NOPROGRESS;
	HRESULT hr = taskBarList4->SetProgressState (hwndApp, TBPF_NOPROGRESS);
	ASSERT (SUCCEEDED (hr))
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TaskBar::updateProgress (const State& state)
{
	ASSERT (taskBarList4 && hwndApp)

	bool indeterminate = (state.flags & kIndeterminate) != 0;
	if(indeterminate)
	{
		savedProgressState = TBPF_INDETERMINATE;
		HRESULT hr = taskBarList4->SetProgressState (hwndApp, TBPF_INDETERMINATE);
		ASSERT (SUCCEEDED (hr))
	}
	else
	{
		savedProgressState = TBPF_NORMAL;
		ULONGLONG value = (int)(state.value * 100.);
		HRESULT hr = taskBarList4->SetProgressValue (hwndApp, value, 100);
		ASSERT (SUCCEEDED (hr))
	}
}
