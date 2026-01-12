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
// Filename    : ccl/platform/win/gui/taskbar.h
// Description : Task Bar
//
//************************************************************************************************

#ifndef _ccl_taskbar_h
#define _ccl_taskbar_h

#include "ccl/base/singleton.h"

#include "ccl/platform/win/system/cclcom.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/gui/framework/iwin32specifics.h"

#include <shobjidl.h>

namespace CCL {
namespace Win32 {

//************************************************************************************************
// TaskBar
//************************************************************************************************

class TaskBar: public Object,
			   public ITaskBar,
			   public AbstractProgressNotify,
			   public SharedSingleton<TaskBar>
{
public:
	DECLARE_CLASS (TaskBar, Object)

	TaskBar ();
	~TaskBar ();

	// ITaskBar
	CCL::IProgressNotify* CCL_API getProgressBar (IWindow* window) override;

	// IProgressNotify
	void CCL_API beginProgress () override;
	void CCL_API endProgress () override;
	void CCL_API updateProgress (const State& state) override;

	CLASS_INTERFACE (ITaskBar, Object)

protected:
	ComPtr<ITaskbarList> taskBarList;
	ComPtr<ITaskbarList4> taskBarList4;
	HWND hwndApp;
	TBPFLAG savedProgressState;
};

//************************************************************************************************
// TaskBarDelegate
//************************************************************************************************

class TaskBarDelegate: public Object
{
public:
	DECLARE_CLASS (TaskBarDelegate, Object)

	CLASS_INTERFACES (Object)
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_taskbar_h
