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
// Filename    : ccl/platform/shared/interfaces/platformfileselector.h
// Description : CCL GUI Platform Integration
//
//************************************************************************************************

#ifndef _ccl_platformfileselector_h
#define _ccl_platformfileselector_h

#include "ccl/platform/shared/interfaces/platformdialog.h"

namespace CCL {
namespace PlatformIntegration {

//************************************************************************************************
// IPlatformFileSelectorObserver
//************************************************************************************************

struct IPlatformFileSelectorObserver: IPlatformDialogObserver
{
    virtual void addResult (CStringPtr path) = 0;
};

//************************************************************************************************
// IPlatformFileSelector
//************************************************************************************************

struct IPlatformFileSelector: Core::IPropertyHandler,
							  IPlatformDialog
{
    enum Mode
    {
        kOpen,
        kSave
    };
    
    enum FileMode
    {
        kFile,
        kMultipleFiles,
        kDirectory
    };
    
	virtual void addFilter (CStringPtr description, CStringPtr filter) = 0;
	virtual tbool open (IPlatformFileSelectorObserver& observer, int mode, int fileMode, CStringPtr title, CStringPtr defaultSuffix = "", CStringPtr initialDirectory = "", CStringPtr initialFileName = "") = 0;
    virtual tbool close () = 0;
    
	static const Core::InterfaceID kIID = FOUR_CHAR_ID ('F','i','l','S');
};

} // namespace PlatformIntegration
} // namespace CCL

#endif // _ccl_platformfileselector_h
