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
// Filename    : ccl/app/fileinfo/pluginfileinfo.h
// Description : Plug-in File Info
//
//************************************************************************************************

#ifndef _ccl_pluginfileinfo_h
#define _ccl_pluginfileinfo_h

#include "ccl/app/fileinfo/fileinfocomponent.h"

#include "ccl/base/signalsource.h"

namespace CCL {

//************************************************************************************************
// PlugInFileInfo
//************************************************************************************************

class PlugInFileInfo: public FileInfoComponent
{
public:
	DECLARE_CLASS (PlugInFileInfo, FileInfoComponent)

	PlugInFileInfo ();
	~PlugInFileInfo ();

	static void registerInfo ();
	static bool canHandleFile (UrlRef path);

	// FileInfoComponent
	tbool CCL_API setFile (UrlRef path) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	SignalSink pluginsSignalSink;
	UIDBytes cid;
};

} // namespace CCL

#endif // _ccl_pluginfileinfo_h
