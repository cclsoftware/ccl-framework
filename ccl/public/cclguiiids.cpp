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
// Filename    : ccl/public/cclguiiids.cpp
// Description : IID symbols
//
//************************************************************************************************

#define INIT_IID

#include "ccl/public/text/cstring.h"

// GUI
#include "ccl/public/gui/appanalytics.h"
#include "ccl/public/gui/iapplication.h"
#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/idatatarget.h"
#include "ccl/public/gui/inavigator.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/ipluginview.h"
#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/gui/iviewstate.h"
#include "ccl/public/gui/iuseroption.h"

// GUI / Framework
#include "ccl/public/gui/framework/controlproperties.h"
#include "ccl/public/gui/framework/controlsignals.h"
#include "ccl/public/gui/framework/iaccessibility.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/ianimation.h"
#include "ccl/public/gui/framework/icommandeditor.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/gui/framework/iclipboard.h"
#include "ccl/public/gui/framework/icolorscheme.h"
#include "ccl/public/gui/framework/idialogbuilder.h"
#include "ccl/public/gui/framework/idragndrop.h"
#include "ccl/public/gui/framework/idrawable.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/gui/framework/isystemsharing.h"
#include "ccl/public/gui/framework/iform.h"
#include "ccl/public/gui/framework/iguihelper.h"
#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/gui/framework/iitemmodel.h"
#include "ccl/public/gui/framework/idropbox.h"
#include "ccl/public/gui/framework/ieditbox.h"
#include "ccl/public/gui/framework/imacosspecifics.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/imousecursor.h"
#include "ccl/public/gui/framework/imousehandler.h"
#include "ccl/public/gui/framework/imultitouch.h"
#include "ccl/public/gui/framework/inamenavigator.h"
#include "ccl/public/gui/framework/inotificationcenter.h"
#include "ccl/public/gui/framework/inotifyicon.h"
#include "ccl/public/gui/framework/ipalette.h"
#include "ccl/public/gui/framework/ipresentable.h"
#include "ccl/public/gui/framework/iparametermenu.h"
#include "ccl/public/gui/framework/ipopupselector.h"
#include "ccl/public/gui/framework/iprintservice.h"
#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/gui/framework/iscrollview.h"
#include "ccl/public/gui/framework/iskinmodel.h"
#include "ccl/public/gui/framework/iskineditsupport.h"
#include "ccl/public/gui/framework/isprite.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/gui/framework/itimer.h"
#include "ccl/public/gui/framework/itooltip.h"
#include "ccl/public/gui/framework/itextmodel.h"
#include "ccl/public/gui/framework/iusercontrol.h"
#include "ccl/public/gui/framework/iembeddedviewhost.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/iviewanimation.h"
#include "ccl/public/gui/framework/iview3d.h"
#include "ccl/public/gui/framework/ivisualstyle.h"
#include "ccl/public/gui/framework/iwin32specifics.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/iwindowmanager.h"
#include "ccl/public/gui/framework/iworkspace.h"
#include "ccl/public/gui/framework/keycodes.h"
#include "ccl/public/gui/framework/ihandwriting.h"
#include "ccl/public/gui/framework/ilinuxspecifics.h"

const CCL::String CCL::IMenu::strSeparator = CCLSTR ("-");
const CCL::String CCL::IMenu::strLargeVariant = CCLSTR ("large");
const CCL::String CCL::IMenu::strFollowIndicator = CCLSTR ("...");
const CCL::String CCL::IUserOption::strSeparator = CCLSTR ("\n");
const CCL::String CCL::IThemeManager::kThemeProtocol = CCLSTR ("theme");

// GUI / Graphics
#include "ccl/public/gui/graphics/font.h"
#include "ccl/public/gui/graphics/ibitmap.h"
#include "ccl/public/gui/graphics/ibitmapfilter.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/igraphicshelper.h"
#include "ccl/public/gui/graphics/igraphicslayer.h"
#include "ccl/public/gui/graphics/igraphicspath.h"
#include "ccl/public/gui/graphics/igradient.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/graphics/iimagecache.h"
#include "ccl/public/gui/graphics/imarkuppainter.h"
#include "ccl/public/gui/graphics/itextlayout.h"
#include "ccl/public/gui/graphics/iuivalue.h"

// GUI / Graphics / 3D
#include "ccl/public/gui/graphics/3d/igraphics3d.h"
#include "ccl/public/gui/graphics/3d/imodel3d.h"
#include "ccl/public/gui/graphics/3d/iscene3d.h"
#include "ccl/public/gui/graphics/3d/itessellator3d.h"
#include "ccl/public/gui/graphics/3d/itransformconstraints3d.h"
#include "ccl/public/gui/graphics/3d/stockshader3d.h"
