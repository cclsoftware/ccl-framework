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
// Filename    : ccl/extras/webfs/webfileaction.h
// Description : Web File Action
//
//************************************************************************************************

#ifndef _ccl_webfileaction_h
#define _ccl_webfileaction_h

#include "ccl/base/storage/url.h"

#include "ccl/public/network/web/iwebfiletask.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// FileAction
//************************************************************************************************

class FileAction: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (FileAction, Object)

	FileAction ();
	~FileAction ();

	PROPERTY_OBJECT (Url, webfsUrl, WebFSUrl)

	enum State
	{
		kNone,
		kPending,
		kCompleted,
		kFailed
	};

	State getState () const;
	bool isCompleted () const;

	virtual void start () = 0;

	void cancel ();
	void reset ();
	void restart ();

protected:
	State state;

	void setState (State state);
};

//************************************************************************************************
// GetDirectoryAction
//************************************************************************************************

class GetDirectoryAction: public FileAction
{
public:
	DECLARE_CLASS (GetDirectoryAction, FileAction)

	// FileAction
	void start () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

//************************************************************************************************
// FileTaskAction
//************************************************************************************************

class FileTaskAction: public FileAction
{
public:
	DECLARE_CLASS (FileTaskAction, FileAction)

	FileTaskAction ();

	PROPERTY_VARIABLE (int, tag, Tag)
	PROPERTY_SHARED_AUTO (IFileTask, task, Task)

	// FileAction
	void start () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

//************************************************************************************************
// FileTask
//************************************************************************************************

class FileTask: public Object,
				public IFileTask
{
public:
	DECLARE_CLASS_ABSTRACT (FileTask, Object)

	CLASS_INTERFACE (IFileTask, Object)
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webfileaction_h
