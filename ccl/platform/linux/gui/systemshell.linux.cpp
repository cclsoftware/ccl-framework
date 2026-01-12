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
// Filename    : ccl/platform/linux/gui/systemshell.linux.cpp
// Description : Linux System Shell
//
//************************************************************************************************

#include "ccl/gui/system/systemshell.h"

#include "ccl/base/storage/url.h"
#include "ccl/gui/gui.h"

#include "ccl/public/text/cstring.h"

#include "ccl/platform/linux/wayland/activationtoken.h"

#include <unistd.h>

namespace CCL {

//************************************************************************************************
// LinuxSystemShell
//************************************************************************************************

class LinuxSystemShell: public SystemShell
{
public:
	// SystemShell
	tresult openNativeUrl (UrlRef url, int flags) override;
	tresult showNativeFile (UrlRef url) override;
	tresult CCL_API addRecentFile (UrlRef url) override;
	tresult CCL_API setRunAtStartupEnabled (tbool state) override;
	tbool CCL_API isRunAtStartupEnabled () override;
	tbool CCL_API isRunAtStartupHidden (ArgsRef args) override;
};

} // namespace CCL

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// LinuxSystemShell
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (SystemShell, LinuxSystemShell)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult LinuxSystemShell::openNativeUrl (UrlRef url, int flags)
{
	String command;
	command.append ("xdg-open \"").append (UrlFullString (url, true)).append ("\"");
	MutableCString systemCommand (command, Text::kSystemEncoding);

	#if WAYLAND_USE_XDG_ACTIVATION
	ActivationToken activationToken;
	if(activationToken.request ())
		GUI.runModalLoop (nullptr, activationToken.done);

	::setenv ("XDG_ACTIVATION_TOKEN", activationToken.getTokenString ().str (), 1);
	#endif

	pid_t pid = 0;
	pid = ::fork ();
	if(pid == -1)
		return kResultFailed;
	else if(pid == 0)
	{
		// this is executed in the child process
		::system (systemCommand.str ());
		::_exit (0);
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult LinuxSystemShell::showNativeFile (UrlRef url)
{
	Url folderUrl (url);
	if(folderUrl.isFile ())
		folderUrl.ascend ();
	return openNativeUrl (folderUrl, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxSystemShell::addRecentFile (UrlRef url)
{
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxSystemShell::setRunAtStartupEnabled (tbool state)
{
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxSystemShell::isRunAtStartupEnabled ()
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxSystemShell::isRunAtStartupHidden (ArgsRef args)
{
	return false;
}
