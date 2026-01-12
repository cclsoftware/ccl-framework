#************************************************************************************************
#
# This file is part of Crystal Class Library (R)
# Copyright (c) 2025 CCL Software Licensing GmbH.
# All Rights Reserved.
#
# Licensed for use under either:
#  1. a Commercial License provided by CCL Software Licensing GmbH, or
#  2. GNU Affero General Public License v3.0 (AGPLv3).
# 
# You must choose and comply with one of the above licensing options.
# For more information, please visit ccl.dev.
#
# Filename    : cclgui.cmake
# Description : CCL network library
#
#************************************************************************************************

find_package (webp REQUIRED)
find_package (yoga REQUIRED)

set (cclgui_exports
	CCLModuleMain
	${CCL_EXPORT_PREFIX}IsFrameworkHostProcess${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetGUI${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetCommandTable${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetHelpManager${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetSystemShell${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetThemeManager${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetGUIHelper${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetWindowManager${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetWorkspaceManager${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetGraphicsHelper${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetAlertService${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetClipboard${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetDesktop${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateFrameworkView${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}CreateChildWindow${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetFrameworkConfiguration${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetPrintService${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetNotificationCenter${CCL_EXPORT_POSTFIX}
	${CCL_EXPORT_PREFIX}GetAccessibilityManager${CCL_EXPORT_POSTFIX}
)

# collect resource files
ccl_list_append_once (cclgui_resources
	${CCL_DIR}/resource/commands.xml
	${CCL_DIR}/resource/skin
)

# collect source files
ccl_list_append_once (cclgui_main_sources
	${CCL_DIR}/main/cclmodmain.cpp
	${CCL_DIR}/main/cclmodmain.h
	${CCL_DIR}/main/cclmodmain.empty.cpp
)

ccl_list_append_once (cclgui_public_sources
	${CCL_DIR}/public/cclguiiids.cpp
)

ccl_list_append_once (cclgui_app_sources
	${CCL_DIR}/app/controls/itemviewmodel.cpp
	${CCL_DIR}/app/controls/itemviewmodel.h
	${CCL_DIR}/app/utilities/boxedguitypes.cpp
	${CCL_DIR}/app/utilities/boxedguitypes.h
	${CCL_DIR}/app/paramalias.cpp
	${CCL_DIR}/app/paramalias.h
	${CCL_DIR}/app/paramcontainer.cpp
	${CCL_DIR}/app/paramcontainer.h
	${CCL_DIR}/app/params.cpp
	${CCL_DIR}/app/params.h
)

ccl_list_append_once (cclgui_public_headers
	${CCL_DIR}/public/gui/appanalytics.h
	${CCL_DIR}/public/gui/commanddispatch.h
	${CCL_DIR}/public/gui/paramlist.h
	${CCL_DIR}/public/gui/iapplication.h
	${CCL_DIR}/public/gui/icommandhandler.h
	${CCL_DIR}/public/gui/icontextmenu.h
	${CCL_DIR}/public/gui/icontroller.h
	${CCL_DIR}/public/gui/idatatarget.h
	${CCL_DIR}/public/gui/inavigator.h
	${CCL_DIR}/public/gui/iparameter.h
	${CCL_DIR}/public/gui/iparamobserver.h
	${CCL_DIR}/public/gui/ipluginview.h
	${CCL_DIR}/public/gui/iuseroption.h
	${CCL_DIR}/public/gui/iviewfactory.h
	${CCL_DIR}/public/gui/iviewstate.h
	
	${CCL_DIR}/public/gui/framework/abstractdraghandler.h
	${CCL_DIR}/public/gui/framework/abstracttouchhandler.h
	${CCL_DIR}/public/gui/framework/controlclasses.h
	${CCL_DIR}/public/gui/framework/controlproperties.h
	${CCL_DIR}/public/gui/framework/controlscalepainter.h
	${CCL_DIR}/public/gui/framework/controlsignals.h
	${CCL_DIR}/public/gui/framework/controlstyles.h
	${CCL_DIR}/public/gui/framework/designsize.h
	${CCL_DIR}/public/gui/framework/dialogbox.h
	${CCL_DIR}/public/gui/framework/guievent.h

	${CCL_DIR}/public/gui/framework/iaccessibility.h
	${CCL_DIR}/public/gui/framework/ialert.h
	${CCL_DIR}/public/gui/framework/ianimation.h
	${CCL_DIR}/public/gui/framework/iclipboard.h
	${CCL_DIR}/public/gui/framework/icolorscheme.h
	${CCL_DIR}/public/gui/framework/icommandeditor.h
	${CCL_DIR}/public/gui/framework/icommandtable.h
	${CCL_DIR}/public/gui/framework/idialogbuilder.h
	${CCL_DIR}/public/gui/framework/idleclient.h
	${CCL_DIR}/public/gui/framework/idragndrop.h
	${CCL_DIR}/public/gui/framework/idrawable.h
	${CCL_DIR}/public/gui/framework/idropbox.h
	${CCL_DIR}/public/gui/framework/ieditbox.h
	${CCL_DIR}/public/gui/framework/iembeddedviewhost.h
	${CCL_DIR}/public/gui/framework/ifileselector.h
	${CCL_DIR}/public/gui/framework/iform.h
	${CCL_DIR}/public/gui/framework/iguihelper.h
	${CCL_DIR}/public/gui/framework/ihandwriting.h
	${CCL_DIR}/public/gui/framework/ihelpmanager.h
	${CCL_DIR}/public/gui/framework/iitemmodel.h
	${CCL_DIR}/public/gui/framework/ilistview.h
	${CCL_DIR}/public/gui/framework/ilinuxspecifics.h
	${CCL_DIR}/public/gui/framework/imacosspecifics.h
	${CCL_DIR}/public/gui/framework/imenu.h
	${CCL_DIR}/public/gui/framework/imousecursor.h
	${CCL_DIR}/public/gui/framework/imousehandler.h
	${CCL_DIR}/public/gui/framework/imultitouch.h
	${CCL_DIR}/public/gui/framework/inamenavigator.h
	${CCL_DIR}/public/gui/framework/inotificationcenter.h
	${CCL_DIR}/public/gui/framework/inotifyicon.h
	${CCL_DIR}/public/gui/framework/ipalette.h	
	${CCL_DIR}/public/gui/framework/iparametermenu.h
	${CCL_DIR}/public/gui/framework/ipopupselector.h
	${CCL_DIR}/public/gui/framework/ipresentable.h
	${CCL_DIR}/public/gui/framework/iprintservice.h
	${CCL_DIR}/public/gui/framework/iprogressdialog.h
	${CCL_DIR}/public/gui/framework/iscrollview.h
	${CCL_DIR}/public/gui/framework/iskineditsupport.h
	${CCL_DIR}/public/gui/framework/iskinmodel.h
	${CCL_DIR}/public/gui/framework/isprite.h
	${CCL_DIR}/public/gui/framework/isystemsharing.h
	${CCL_DIR}/public/gui/framework/isystemshell.h
	${CCL_DIR}/public/gui/framework/itemviewgeometry.h
	${CCL_DIR}/public/gui/framework/itextmodel.h
	${CCL_DIR}/public/gui/framework/itheme.h
	${CCL_DIR}/public/gui/framework/ithememanager.h
	${CCL_DIR}/public/gui/framework/itimer.h
	${CCL_DIR}/public/gui/framework/itooltip.h
	${CCL_DIR}/public/gui/framework/itreeview.h
	${CCL_DIR}/public/gui/framework/iusercontrol.h
	${CCL_DIR}/public/gui/framework/iuserinterface.h
	${CCL_DIR}/public/gui/framework/iview.h
	${CCL_DIR}/public/gui/framework/iview3d.h
	${CCL_DIR}/public/gui/framework/iviewanimation.h
	${CCL_DIR}/public/gui/framework/ivisualstyle.h
	${CCL_DIR}/public/gui/framework/iwin32specifics.h
	${CCL_DIR}/public/gui/framework/iwindow.h
	${CCL_DIR}/public/gui/framework/iwindowmanager.h
	${CCL_DIR}/public/gui/framework/iworkspace.h
	${CCL_DIR}/public/gui/framework/keycodes.h
	${CCL_DIR}/public/gui/framework/popupselectorclient.h
	${CCL_DIR}/public/gui/framework/skinxmldefs.h
	${CCL_DIR}/public/gui/framework/styleflags.h
	${CCL_DIR}/public/gui/framework/themeelements.h
	${CCL_DIR}/public/gui/framework/usercontrolbase.h
	${CCL_DIR}/public/gui/framework/usertooltip.h
	${CCL_DIR}/public/gui/framework/viewbox.h
	${CCL_DIR}/public/gui/framework/viewfinder.h
	
	${CCL_DIR}/public/gui/graphics/alignment.h
	${CCL_DIR}/public/gui/graphics/brush.h
	${CCL_DIR}/public/gui/graphics/color.h
	${CCL_DIR}/public/gui/graphics/dpiscale.h
	${CCL_DIR}/public/gui/graphics/font.h
	${CCL_DIR}/public/gui/graphics/graphicsfactory.h
	${CCL_DIR}/public/gui/graphics/ibitmap.h
	${CCL_DIR}/public/gui/graphics/ibitmapfilter.h
	${CCL_DIR}/public/gui/graphics/iconsetformat.h
	${CCL_DIR}/public/gui/graphics/igradient.h
	${CCL_DIR}/public/gui/graphics/igraphics.h
	${CCL_DIR}/public/gui/graphics/igraphicshelper.h
	${CCL_DIR}/public/gui/graphics/igraphicslayer.h
	${CCL_DIR}/public/gui/graphics/igraphicspath.h
	${CCL_DIR}/public/gui/graphics/iimage.h
	${CCL_DIR}/public/gui/graphics/iimagecache.h
	${CCL_DIR}/public/gui/graphics/imarkuppainter.h
	${CCL_DIR}/public/gui/graphics/itextlayout.h
	${CCL_DIR}/public/gui/graphics/iuivalue.h
	${CCL_DIR}/public/gui/graphics/markuptags.h
	${CCL_DIR}/public/gui/graphics/pen.h
	${CCL_DIR}/public/gui/graphics/point.h
	${CCL_DIR}/public/gui/graphics/rect.h
	${CCL_DIR}/public/gui/graphics/textformat.h
	${CCL_DIR}/public/gui/graphics/transform.h
	${CCL_DIR}/public/gui/graphics/types.h
	${CCL_DIR}/public/gui/graphics/updatergn.h
	
	${CCL_DIR}/public/gui/graphics/3d/doc3d.h
	${CCL_DIR}/public/gui/graphics/3d/ibufferallocator3d.h
	${CCL_DIR}/public/gui/graphics/3d/igeometrysource3d.h
	${CCL_DIR}/public/gui/graphics/3d/igraphics3d.h
	${CCL_DIR}/public/gui/graphics/3d/imodel3d.h
	${CCL_DIR}/public/gui/graphics/3d/iscene3d.h
	${CCL_DIR}/public/gui/graphics/3d/itessellator3d.h
	${CCL_DIR}/public/gui/graphics/3d/itransformconstraints3d.h
	${CCL_DIR}/public/gui/graphics/3d/modelfactory3d.h
	${CCL_DIR}/public/gui/graphics/3d/point3d.h
	${CCL_DIR}/public/gui/graphics/3d/ray3d.h
	${CCL_DIR}/public/gui/graphics/3d/shaderconstants3d.h
	${CCL_DIR}/public/gui/graphics/3d/stockshader3d.h
	${CCL_DIR}/public/gui/graphics/3d/transform3d.h
	${CCL_DIR}/public/gui/graphics/3d/vertex3d.h

	${CCL_DIR}/public/guiservices.h
)

ccl_list_append_once (cclgui_generated_headers
	${CCL_DIR}/meta/generated/cpp/controlstyles-generated.h
	${CCL_DIR}/meta/generated/cpp/graphics-constants-generated.h
	${CCL_DIR}/meta/generated/cpp/gui-constants-generated.h
	${CCL_DIR}/meta/generated/cpp/skinxmldefs-generated.h
)

ccl_list_append_once (cclgui_public_source_files
	${CCL_DIR}/public/gui/commanddispatch.cpp
	${CCL_DIR}/public/gui/paramlist.cpp
	
	${CCL_DIR}/public/gui/framework/controlscalepainter.cpp
	${CCL_DIR}/public/gui/framework/designsize.cpp
	${CCL_DIR}/public/gui/framework/dialogbox.cpp
	${CCL_DIR}/public/gui/framework/guievent.cpp
	${CCL_DIR}/public/gui/framework/ialert.cpp
	${CCL_DIR}/public/gui/framework/idleclient.cpp
	${CCL_DIR}/public/gui/framework/idrawable.cpp
	${CCL_DIR}/public/gui/framework/itemviewgeometry.cpp
	${CCL_DIR}/public/gui/framework/iskinmodel.cpp
	${CCL_DIR}/public/gui/framework/popupselectorclient.cpp
	${CCL_DIR}/public/gui/framework/usercontrolbase.cpp
	${CCL_DIR}/public/gui/framework/usertooltip.cpp
	${CCL_DIR}/public/gui/framework/viewbox.cpp
	
	${CCL_DIR}/public/gui/graphics/brush.cpp
	${CCL_DIR}/public/gui/graphics/color.cpp
	${CCL_DIR}/public/gui/graphics/font.cpp
	${CCL_DIR}/public/gui/graphics/graphicsfactory.cpp
	${CCL_DIR}/public/gui/graphics/rect.cpp
	${CCL_DIR}/public/gui/graphics/transform.cpp
	${CCL_DIR}/public/gui/graphics/updatergn.cpp
	${CCL_DIR}/public/gui/graphics/3d/modelfactory3d.cpp
	${CCL_DIR}/public/gui/graphics/3d/itransformconstraints3d.cpp
	${CCL_DIR}/public/gui/graphics/3d/point3d.cpp
	${CCL_DIR}/public/gui/graphics/3d/ray3d.cpp
	${CCL_DIR}/public/gui/graphics/3d/transform3d.cpp
	
	${CCL_DIR}/public/plugins/stubobject.cpp
	${CCL_DIR}/public/plugins/stubobject.h
)

ccl_list_append_once (cclgui_source_files
	${CCL_DIR}/gui/commands.cpp
	${CCL_DIR}/gui/commands.h

	${CCL_DIR}/gui/controls/autoscroller.cpp
	${CCL_DIR}/gui/controls/autoscroller.h
	${CCL_DIR}/gui/controls/button.cpp
	${CCL_DIR}/gui/controls/button.h
	${CCL_DIR}/gui/controls/colorbox.cpp
	${CCL_DIR}/gui/controls/colorbox.h
	${CCL_DIR}/gui/controls/commandbar/commandbarmodel.cpp
	${CCL_DIR}/gui/controls/commandbar/commandbarmodel.h
	${CCL_DIR}/gui/controls/commandbar/commandbarview.cpp
	${CCL_DIR}/gui/controls/commandbar/commandbarview.h
	${CCL_DIR}/gui/controls/control.cpp
	${CCL_DIR}/gui/controls/control.h
	${CCL_DIR}/gui/controls/controlaccessibility.cpp
	${CCL_DIR}/gui/controls/controlaccessibility.h
	${CCL_DIR}/gui/controls/controlxyhandler.cpp
	${CCL_DIR}/gui/controls/controlxyhandler.h
	${CCL_DIR}/gui/controls/editbox.cpp
	${CCL_DIR}/gui/controls/editbox.h
	${CCL_DIR}/gui/controls/knob.cpp
	${CCL_DIR}/gui/controls/knob.h
	${CCL_DIR}/gui/controls/label.cpp
	${CCL_DIR}/gui/controls/label.h
	${CCL_DIR}/gui/controls/linkview.cpp
	${CCL_DIR}/gui/controls/linkview.h
	${CCL_DIR}/gui/controls/pictureviewer.cpp
	${CCL_DIR}/gui/controls/pictureviewer.h
	${CCL_DIR}/gui/controls/pluginview.cpp
	${CCL_DIR}/gui/controls/pluginview.h
	${CCL_DIR}/gui/controls/popupbox.cpp
	${CCL_DIR}/gui/controls/popupbox.h
	${CCL_DIR}/gui/controls/scrollbar.cpp
	${CCL_DIR}/gui/controls/scrollbar.h
	${CCL_DIR}/gui/controls/scrollpicker.cpp
	${CCL_DIR}/gui/controls/scrollpicker.h
	${CCL_DIR}/gui/controls/segmentbox.cpp
	${CCL_DIR}/gui/controls/segmentbox.h
	${CCL_DIR}/gui/controls/selectbox.cpp
	${CCL_DIR}/gui/controls/selectbox.h
	${CCL_DIR}/gui/controls/slider.cpp
	${CCL_DIR}/gui/controls/slider.h
	${CCL_DIR}/gui/controls/swipehandler.cpp
	${CCL_DIR}/gui/controls/swipehandler.h
	${CCL_DIR}/gui/controls/tabview.cpp
	${CCL_DIR}/gui/controls/tabview.h
	${CCL_DIR}/gui/controls/textbox.cpp
	${CCL_DIR}/gui/controls/textbox.h
	${CCL_DIR}/gui/controls/texteditor.cpp
	${CCL_DIR}/gui/controls/texteditor.h
	${CCL_DIR}/gui/controls/updownbox.cpp
	${CCL_DIR}/gui/controls/updownbox.h
	${CCL_DIR}/gui/controls/usercontrolhost.cpp
	${CCL_DIR}/gui/controls/usercontrolhost.h
	${CCL_DIR}/gui/controls/valuebar.cpp
	${CCL_DIR}/gui/controls/valuebar.h
	${CCL_DIR}/gui/controls/valuebox.cpp
	${CCL_DIR}/gui/controls/valuebox.h
	${CCL_DIR}/gui/controls/variantview.cpp
	${CCL_DIR}/gui/controls/variantview.h
	${CCL_DIR}/gui/controls/vectorpad.cpp
	${CCL_DIR}/gui/controls/vectorpad.h
	${CCL_DIR}/gui/controls/trivectorpad.cpp
	${CCL_DIR}/gui/controls/trivectorpad.h

	${CCL_DIR}/gui/dialogs/alert.cpp
	${CCL_DIR}/gui/dialogs/alert.h
	${CCL_DIR}/gui/dialogs/commandeditor.cpp
	${CCL_DIR}/gui/dialogs/commandeditor.h
	${CCL_DIR}/gui/dialogs/commandselector.cpp
	${CCL_DIR}/gui/dialogs/commandselector.h
	${CCL_DIR}/gui/dialogs/dialogbuilder.cpp
	${CCL_DIR}/gui/dialogs/dialogbuilder.h
	${CCL_DIR}/gui/dialogs/fileselector.cpp
	${CCL_DIR}/gui/dialogs/fileselector.h
	${CCL_DIR}/gui/dialogs/progressdialog.cpp
	${CCL_DIR}/gui/dialogs/progressdialog.h
	${CCL_DIR}/gui/dialogs/useroptiondialog.cpp
	${CCL_DIR}/gui/dialogs/useroptionmodel.cpp
	${CCL_DIR}/gui/dialogs/useroptionmodel.h

	${CCL_DIR}/gui/graphics/3d/bufferallocator3d.cpp
	${CCL_DIR}/gui/graphics/3d/bufferallocator3d.h
	${CCL_DIR}/gui/graphics/3d/nativegraphics3d.cpp
	${CCL_DIR}/gui/graphics/3d/nativegraphics3d.h
	${CCL_DIR}/gui/graphics/3d/model/model3d.cpp
	${CCL_DIR}/gui/graphics/3d/model/model3d.h
	${CCL_DIR}/gui/graphics/3d/model/tessellator3d.cpp
	${CCL_DIR}/gui/graphics/3d/model/tessellator3d.h
	${CCL_DIR}/gui/graphics/3d/scene/scene3d.cpp
	${CCL_DIR}/gui/graphics/3d/scene/scene3d.h
	${CCL_DIR}/gui/graphics/3d/scene/scenerenderer3d.cpp
	${CCL_DIR}/gui/graphics/3d/scene/scenerenderer3d.h
	${CCL_DIR}/gui/graphics/3d/shader/shaderreflection3d.h
	${CCL_DIR}/gui/graphics/3d/shader/shaderreflection3d.cpp

	${CCL_DIR}/gui/graphics/colorgradient.cpp
	${CCL_DIR}/gui/graphics/colorgradient.h
	${CCL_DIR}/gui/graphics/graphicsdevice.cpp
	${CCL_DIR}/gui/graphics/graphicsdevice.h
	${CCL_DIR}/gui/graphics/graphicshelper.cpp
	${CCL_DIR}/gui/graphics/graphicshelper.h
	${CCL_DIR}/gui/graphics/graphicslayerimpl.cpp
	${CCL_DIR}/gui/graphics/graphicslayerimpl.h
	${CCL_DIR}/gui/graphics/graphicspath.cpp
	${CCL_DIR}/gui/graphics/graphicspath.h
	${CCL_DIR}/gui/graphics/igraphicscleanup.h
	${CCL_DIR}/gui/graphics/imaging/codecs/webpcodec.cpp
	${CCL_DIR}/gui/graphics/imaging/codecs/webpcodec.h
	${CCL_DIR}/gui/graphics/imaging/bitmap.cpp
	${CCL_DIR}/gui/graphics/imaging/bitmap.h
	${CCL_DIR}/gui/graphics/imaging/bitmapcodec.cpp
	${CCL_DIR}/gui/graphics/imaging/bitmapcodec.h
	${CCL_DIR}/gui/graphics/imaging/bitmapfilter.cpp
	${CCL_DIR}/gui/graphics/imaging/bitmapfilter.h
	${CCL_DIR}/gui/graphics/imaging/bitmappainter.cpp
	${CCL_DIR}/gui/graphics/imaging/bitmappainter.h
	${CCL_DIR}/gui/graphics/imaging/coloredbitmap.cpp
	${CCL_DIR}/gui/graphics/imaging/coloredbitmap.h
	${CCL_DIR}/gui/graphics/imaging/filmstrip.cpp
	${CCL_DIR}/gui/graphics/imaging/filmstrip.h
	${CCL_DIR}/gui/graphics/imaging/image.cpp
	${CCL_DIR}/gui/graphics/imaging/image.h
	${CCL_DIR}/gui/graphics/imaging/imagecache.cpp
	${CCL_DIR}/gui/graphics/imaging/imagecache.h
	${CCL_DIR}/gui/graphics/imaging/imagepart.cpp
	${CCL_DIR}/gui/graphics/imaging/imagepart.h
	${CCL_DIR}/gui/graphics/imaging/multiimage.cpp
	${CCL_DIR}/gui/graphics/imaging/multiimage.h
	${CCL_DIR}/gui/graphics/imaging/offscreen.cpp
	${CCL_DIR}/gui/graphics/imaging/offscreen.h
	${CCL_DIR}/gui/graphics/imaging/tiledimage.cpp
	${CCL_DIR}/gui/graphics/imaging/tiledimage.h
	${CCL_DIR}/gui/graphics/imaging/tiler.cpp
	${CCL_DIR}/gui/graphics/imaging/tiler.h
	${CCL_DIR}/gui/graphics/mutableregion.cpp
	${CCL_DIR}/gui/graphics/mutableregion.h
	${CCL_DIR}/gui/graphics/nativegraphics.cpp
	${CCL_DIR}/gui/graphics/nativegraphics.h
	${CCL_DIR}/gui/graphics/printservice.cpp
	${CCL_DIR}/gui/graphics/printservice.h
	${CCL_DIR}/gui/graphics/shapes/shapebuilder.cpp
	${CCL_DIR}/gui/graphics/shapes/shapebuilder.h
	${CCL_DIR}/gui/graphics/shapes/shapeimage.cpp
	${CCL_DIR}/gui/graphics/shapes/shapeimage.h
	${CCL_DIR}/gui/graphics/shapes/shapes.cpp
	${CCL_DIR}/gui/graphics/shapes/shapes.h
	${CCL_DIR}/gui/graphics/shapes/svg/svgparser.cpp
	${CCL_DIR}/gui/graphics/shapes/svg/svgparser.h
	${CCL_DIR}/gui/graphics/shapes/svg/svgpath.cpp
	${CCL_DIR}/gui/graphics/shapes/svg/svgpath.h
	${CCL_DIR}/gui/graphics/textlayoutbuilder.cpp
	${CCL_DIR}/gui/graphics/textlayoutbuilder.h

	${CCL_DIR}/gui/gui.cpp
	${CCL_DIR}/gui/gui.h
	${CCL_DIR}/gui/guihelper.cpp
	${CCL_DIR}/gui/guihelper.h
	${CCL_DIR}/gui/guiservices.cpp
	${CCL_DIR}/gui/guistubs.cpp

	${CCL_DIR}/gui/help/documentviewer.cpp
	${CCL_DIR}/gui/help/documentviewer.h
	${CCL_DIR}/gui/help/helpinfobuilder.cpp
	${CCL_DIR}/gui/help/helpinfobuilder.h
	${CCL_DIR}/gui/help/helpmanager.cpp
	${CCL_DIR}/gui/help/helpmanager.h
	${CCL_DIR}/gui/help/helpreferences.cpp
	${CCL_DIR}/gui/help/helpreferences.h
	${CCL_DIR}/gui/help/helptutorial.cpp
	${CCL_DIR}/gui/help/helptutorial.h
	${CCL_DIR}/gui/help/htmlviewer.cpp
	${CCL_DIR}/gui/help/htmlviewer.h
	${CCL_DIR}/gui/help/keyglyphpainter.cpp
	${CCL_DIR}/gui/help/keyglyphpainter.h
	${CCL_DIR}/gui/help/quickhelp.cpp
	${CCL_DIR}/gui/help/quickhelp.h
	${CCL_DIR}/gui/help/tutorialviewer.cpp
	${CCL_DIR}/gui/help/tutorialviewer.h
	${CCL_DIR}/gui/help/viewhighlights.cpp
	${CCL_DIR}/gui/help/viewhighlights.h

	${CCL_DIR}/gui/itemviews/dropbox.cpp
	${CCL_DIR}/gui/itemviews/dropbox.h
	${CCL_DIR}/gui/itemviews/headerview.cpp
	${CCL_DIR}/gui/itemviews/headerview.h
	${CCL_DIR}/gui/itemviews/itemview.cpp
	${CCL_DIR}/gui/itemviews/itemview.h
	${CCL_DIR}/gui/itemviews/itemviewaccessibility.cpp
	${CCL_DIR}/gui/itemviews/itemviewaccessibility.h
	${CCL_DIR}/gui/itemviews/itemviewbase.cpp
	${CCL_DIR}/gui/itemviews/itemviewbase.h
	${CCL_DIR}/gui/itemviews/listview.cpp
	${CCL_DIR}/gui/itemviews/listview.h
	${CCL_DIR}/gui/itemviews/namenavigator.cpp
	${CCL_DIR}/gui/itemviews/namenavigator.h
	${CCL_DIR}/gui/itemviews/treeitem.cpp
	${CCL_DIR}/gui/itemviews/treeitem.h
	${CCL_DIR}/gui/itemviews/treeview.cpp
	${CCL_DIR}/gui/itemviews/treeview.h

	${CCL_DIR}/gui/keyevent.cpp
	${CCL_DIR}/gui/keyevent.h

	${CCL_DIR}/gui/layout/alignview.cpp
	${CCL_DIR}/gui/layout/alignview.h
	${CCL_DIR}/gui/layout/anchorlayout.cpp
	${CCL_DIR}/gui/layout/anchorlayout.h
	${CCL_DIR}/gui/layout/boxlayout.cpp
	${CCL_DIR}/gui/layout/boxlayout.h
	${CCL_DIR}/gui/layout/clipperlayout.cpp
	${CCL_DIR}/gui/layout/clipperlayout.h
	${CCL_DIR}/gui/layout/divider.cpp
	${CCL_DIR}/gui/layout/divider.h
	${CCL_DIR}/gui/layout/dividergroup.cpp
	${CCL_DIR}/gui/layout/dividergroup.h
	${CCL_DIR}/gui/layout/dockpanel.cpp
	${CCL_DIR}/gui/layout/dockpanel.h
	${CCL_DIR}/gui/layout/flexboxlayout.cpp
	${CCL_DIR}/gui/layout/flexboxlayout.h
	${CCL_DIR}/gui/layout/idockpanel.h
	${CCL_DIR}/gui/layout/layoutprimitives.cpp
	${CCL_DIR}/gui/layout/layoutprimitives.h
	${CCL_DIR}/gui/layout/layoutview.cpp
	${CCL_DIR}/gui/layout/layoutview.h
	${CCL_DIR}/gui/layout/perspectiveswitcher.cpp
	${CCL_DIR}/gui/layout/perspectiveswitcher.h
	${CCL_DIR}/gui/layout/sizevariantlayout.cpp
	${CCL_DIR}/gui/layout/sizevariantlayout.h
	${CCL_DIR}/gui/layout/tablelayout.cpp
	${CCL_DIR}/gui/layout/tablelayout.h
	${CCL_DIR}/gui/layout/workspace.cpp
	${CCL_DIR}/gui/layout/workspace.h
	${CCL_DIR}/gui/layout/workspaceframes.cpp
	${CCL_DIR}/gui/layout/workspaceframes.h
	${CCL_DIR}/gui/layout/yogalayout.cpp

	${CCL_DIR}/gui/popup/contextmenu.cpp
	${CCL_DIR}/gui/popup/contextmenu.h
	${CCL_DIR}/gui/popup/extendedmenu.cpp
	${CCL_DIR}/gui/popup/extendedmenu.h
	${CCL_DIR}/gui/popup/itemviewpopup.cpp
	${CCL_DIR}/gui/popup/itemviewpopup.h
	${CCL_DIR}/gui/popup/menu.cpp
	${CCL_DIR}/gui/popup/menu.h
	${CCL_DIR}/gui/popup/menubarcontrol.cpp
	${CCL_DIR}/gui/popup/menubarcontrol.h
	${CCL_DIR}/gui/popup/menucontrol.cpp
	${CCL_DIR}/gui/popup/menucontrol.h
	${CCL_DIR}/gui/popup/menupopupselector.cpp
	${CCL_DIR}/gui/popup/menupopupselector.h
	${CCL_DIR}/gui/popup/palettepopup.cpp
	${CCL_DIR}/gui/popup/palettepopup.h
	${CCL_DIR}/gui/popup/parametermenubuilder.cpp
	${CCL_DIR}/gui/popup/parametermenubuilder.h
	${CCL_DIR}/gui/popup/popupselector.cpp
	${CCL_DIR}/gui/popup/popupselector.h
	${CCL_DIR}/gui/popup/popupslider.cpp
	${CCL_DIR}/gui/popup/popupslider.h

	${CCL_DIR}/gui/scriptgui.cpp
	${CCL_DIR}/gui/scriptgui.h

	${CCL_DIR}/gui/skin/coreskinmodel.cpp
	${CCL_DIR}/gui/skin/coreskinmodel.h
	${CCL_DIR}/gui/skin/form.cpp
	${CCL_DIR}/gui/skin/form.h
	${CCL_DIR}/gui/skin/skinattributes.cpp
	${CCL_DIR}/gui/skin/skinattributes.h
	${CCL_DIR}/gui/skin/skincontrols.cpp
	${CCL_DIR}/gui/skin/skincontrols.h
	${CCL_DIR}/gui/skin/skinelement.cpp
	${CCL_DIR}/gui/skin/skinelement.h
	${CCL_DIR}/gui/skin/skinelements3d.cpp
	${CCL_DIR}/gui/skin/skinelements3d.h
	${CCL_DIR}/gui/skin/skinexpression.cpp
	${CCL_DIR}/gui/skin/skinexpression.h
	${CCL_DIR}/gui/skin/skininteractive.cpp
	${CCL_DIR}/gui/skin/skininteractive.h
	${CCL_DIR}/gui/skin/skinlayouts.cpp
	${CCL_DIR}/gui/skin/skinlayouts.h
	${CCL_DIR}/gui/skin/skinmodel.cpp
	${CCL_DIR}/gui/skin/skinmodel.h
	${CCL_DIR}/gui/skin/skinparser.cpp
	${CCL_DIR}/gui/skin/skinparser.h
	${CCL_DIR}/gui/skin/skinregistry.cpp
	${CCL_DIR}/gui/skin/skinregistry.h
	${CCL_DIR}/gui/skin/skinshapes.cpp
	${CCL_DIR}/gui/skin/skinshapes.h
	${CCL_DIR}/gui/skin/skinwizard.cpp
	${CCL_DIR}/gui/skin/skinwizard.h
	${CCL_DIR}/gui/skin/skinviews.cpp
	${CCL_DIR}/gui/skin/skinviews.h
	${CCL_DIR}/gui/skin/zoomableview.cpp
	${CCL_DIR}/gui/skin/zoomableview.h

	${CCL_DIR}/gui/system/accessibility.cpp
	${CCL_DIR}/gui/system/accessibility.h
	${CCL_DIR}/gui/system/animation.cpp
	${CCL_DIR}/gui/system/animation.h
	${CCL_DIR}/gui/system/autofill.h
	${CCL_DIR}/gui/system/autofill.cpp
	${CCL_DIR}/gui/system/clipboard.cpp
	${CCL_DIR}/gui/system/clipboard.h
	${CCL_DIR}/gui/system/dragndrop.cpp
	${CCL_DIR}/gui/system/dragndrop.h
	${CCL_DIR}/gui/system/fontresource.cpp
	${CCL_DIR}/gui/system/fontresource.h
	${CCL_DIR}/gui/system/mousecursor.cpp
	${CCL_DIR}/gui/system/mousecursor.h
	${CCL_DIR}/gui/system/notifyicon.cpp
	${CCL_DIR}/gui/system/notifyicon.h
	${CCL_DIR}/gui/system/systemshell.cpp
	${CCL_DIR}/gui/system/systemshell.h
	${CCL_DIR}/gui/system/systemtimer.cpp
	${CCL_DIR}/gui/system/systemtimer.h
	${CCL_DIR}/gui/system/webbrowserview.cpp
	${CCL_DIR}/gui/system/webbrowserview.h
	${CCL_DIR}/gui/system/notificationcenter.cpp
	${CCL_DIR}/gui/system/notificationcenter.h

	${CCL_DIR}/gui/test/elementsizeparsertest.cpp
	${CCL_DIR}/gui/test/flexboxtest.cpp
	${CCL_DIR}/gui/test/layouttest.cpp

	${CCL_DIR}/gui/theme/colorreference.h
	${CCL_DIR}/gui/theme/colorscheme.cpp
	${CCL_DIR}/gui/theme/colorscheme.h
	${CCL_DIR}/gui/theme/palette.cpp
	${CCL_DIR}/gui/theme/palette.h
	${CCL_DIR}/gui/theme/renderer/backgroundrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/backgroundrenderer.h
	${CCL_DIR}/gui/theme/renderer/buttonrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/buttonrenderer.h
	${CCL_DIR}/gui/theme/renderer/comboboxrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/comboboxrenderer.h
	${CCL_DIR}/gui/theme/renderer/compositedrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/compositedrenderer.h
	${CCL_DIR}/gui/theme/renderer/dialoggrouprenderer.cpp
	${CCL_DIR}/gui/theme/renderer/dialoggrouprenderer.h
	${CCL_DIR}/gui/theme/renderer/dividerrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/dividerrenderer.h
	${CCL_DIR}/gui/theme/renderer/editboxrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/editboxrenderer.h
	${CCL_DIR}/gui/theme/renderer/headerviewrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/headerviewrenderer.h
	${CCL_DIR}/gui/theme/renderer/knobrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/knobrenderer.h
	${CCL_DIR}/gui/theme/renderer/labelrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/labelrenderer.h
	${CCL_DIR}/gui/theme/renderer/menubarrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/menubarrenderer.h
	${CCL_DIR}/gui/theme/renderer/scrollbarrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/scrollbarrenderer.h
	${CCL_DIR}/gui/theme/renderer/scrollpickerrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/scrollpickerrenderer.h
	${CCL_DIR}/gui/theme/renderer/selectboxrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/selectboxrenderer.h
	${CCL_DIR}/gui/theme/renderer/sliderrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/sliderrenderer.h
	${CCL_DIR}/gui/theme/renderer/tabviewrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/tabviewrenderer.h
	${CCL_DIR}/gui/theme/renderer/textboxrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/textboxrenderer.h
	${CCL_DIR}/gui/theme/renderer/updownboxrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/updownboxrenderer.h
	${CCL_DIR}/gui/theme/renderer/valuebarrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/valuebarrenderer.h
	${CCL_DIR}/gui/theme/renderer/valueboxrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/valueboxrenderer.h
	${CCL_DIR}/gui/theme/renderer/vectorpadrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/vectorpadrenderer.h
	${CCL_DIR}/gui/theme/renderer/trivectorpadrenderer.cpp
	${CCL_DIR}/gui/theme/renderer/trivectorpadrenderer.h
	${CCL_DIR}/gui/theme/theme.cpp
	${CCL_DIR}/gui/theme/theme.h
	${CCL_DIR}/gui/theme/thememanager.cpp
	${CCL_DIR}/gui/theme/thememanager.h
	${CCL_DIR}/gui/theme/usertheme.cpp
	${CCL_DIR}/gui/theme/usertheme.h
	${CCL_DIR}/gui/theme/visualstyle.cpp
	${CCL_DIR}/gui/theme/visualstyle.h
	${CCL_DIR}/gui/theme/visualstyleselector.cpp
	${CCL_DIR}/gui/theme/visualstyleselector.h
	${CCL_DIR}/gui/theme/visualstyleclass.cpp
	${CCL_DIR}/gui/theme/visualstyleclass.h

	${CCL_DIR}/gui/touch/gesturemanager.cpp
	${CCL_DIR}/gui/touch/gesturemanager.h
	${CCL_DIR}/gui/touch/touchcollection.cpp
	${CCL_DIR}/gui/touch/touchcollection.h
	${CCL_DIR}/gui/touch/touchhandler.cpp
	${CCL_DIR}/gui/touch/touchhandler.h
	${CCL_DIR}/gui/touch/touchinput.cpp
	${CCL_DIR}/gui/touch/touchinput.h

	${CCL_DIR}/gui/views/dialoggroup.cpp
	${CCL_DIR}/gui/views/dialoggroup.h
	${CCL_DIR}/gui/views/focusnavigator.cpp
	${CCL_DIR}/gui/views/focusnavigator.h
	${CCL_DIR}/gui/views/graphicsport.cpp
	${CCL_DIR}/gui/views/graphicsport.h
	${CCL_DIR}/gui/views/imageview.cpp
	${CCL_DIR}/gui/views/imageview.h
	${CCL_DIR}/gui/views/mousehandler.cpp
	${CCL_DIR}/gui/views/mousehandler.h
	${CCL_DIR}/gui/views/overscrollanimator.cpp
	${CCL_DIR}/gui/views/overscrollanimator.h
	${CCL_DIR}/gui/views/scrollview.cpp
	${CCL_DIR}/gui/views/scrollview.h
	${CCL_DIR}/gui/views/sprite.cpp
	${CCL_DIR}/gui/views/sprite.h
	${CCL_DIR}/gui/views/triggerview.cpp
	${CCL_DIR}/gui/views/triggerview.h
	${CCL_DIR}/gui/views/view.cpp
	${CCL_DIR}/gui/views/view.h
	${CCL_DIR}/gui/views/view3d.cpp
	${CCL_DIR}/gui/views/view3d.h
	${CCL_DIR}/gui/views/viewaccessibility.cpp
	${CCL_DIR}/gui/views/viewaccessibility.h
	${CCL_DIR}/gui/views/viewanimation.cpp
	${CCL_DIR}/gui/views/viewanimation.h
	${CCL_DIR}/gui/views/viewdecorator.cpp
	${CCL_DIR}/gui/views/viewdecorator.h

	${CCL_DIR}/gui/windows/appwindow.cpp
	${CCL_DIR}/gui/windows/appwindow.h
	${CCL_DIR}/gui/windows/childwindow.cpp
	${CCL_DIR}/gui/windows/childwindow.h
	${CCL_DIR}/gui/windows/desktop.cpp
	${CCL_DIR}/gui/windows/desktop.h
	${CCL_DIR}/gui/windows/dialog.cpp
	${CCL_DIR}/gui/windows/dialog.h
	${CCL_DIR}/gui/windows/nativewindow.h
	${CCL_DIR}/gui/windows/popupwindow.cpp
	${CCL_DIR}/gui/windows/popupwindow.h
	${CCL_DIR}/gui/windows/systemwindow.cpp
	${CCL_DIR}/gui/windows/systemwindow.h
	${CCL_DIR}/gui/windows/tooltip.cpp
	${CCL_DIR}/gui/windows/tooltip.h
	${CCL_DIR}/gui/windows/transparentwindow.cpp
	${CCL_DIR}/gui/windows/transparentwindow.h
	${CCL_DIR}/gui/windows/window.cpp
	${CCL_DIR}/gui/windows/window.h
	${CCL_DIR}/gui/windows/windowbase.cpp
	${CCL_DIR}/gui/windows/windowbase.h
	${CCL_DIR}/gui/windows/windowmanager.cpp
	${CCL_DIR}/gui/windows/windowmanager.h
)

# set up project folder structure
if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL cclgui_SOURCE_DIR)
	source_group ("main" FILES ${cclgui_main_sources})
	source_group (TREE ${CCL_DIR}/public PREFIX "public" FILES ${cclgui_public_source_files} ${cclgui_public_headers} ${cclgui_public_sources})
	source_group (TREE ${CCL_DIR}/meta/generated/cpp PREFIX "generated" FILES ${cclgui_generated_headers})
	source_group (TREE ${CCL_DIR}/gui PREFIX "source" FILES ${cclgui_source_files})
	source_group (TREE ${CCL_DIR}/app PREFIX "source\\app" FILES ${cclgui_app_sources})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
	set (cclgui_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
include (cclbase)
include (cclsystem)
include (ccltext)

set (cclgui "cclgui")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclgui ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
ccl_read_version (${cclgui} "${CCL_VERSION_FILE}" "CCLGUI")
if (NOT TARGET ${cclgui})
	if (${CCL_STATIC_ONLY})
		ccl_add_library (cclgui INTERFACE POSTFIX "${CCL_ISOLATION_POSTFIX}")
	else ()
		ccl_add_library (cclgui SHARED VENDOR ccl POSTFIX "${CCL_ISOLATION_POSTFIX}")
		set_target_properties (${cclgui} PROPERTIES FOLDER "ccl/${CCL_ISOLATION_POSTFIX}")
	endif ()

	# collect source files for target
	ccl_list_append_once (cclgui_sources
		${cclgui_main_sources}
		${cclgui_public_source_files}
		${cclgui_public_sources}
		${cclgui_app_sources}
		${cclgui_source_files}
	)

	if (NOT ${CCL_STATIC_ONLY})
		target_sources (${cclgui} PRIVATE ${cclgui_sources} ${CMAKE_CURRENT_LIST_FILE})
		ccl_target_headers (${cclgui} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclgui_public_headers} ${cclgui_generated_headers})

		target_link_libraries (${cclgui} PUBLIC ${cclbase} ${cclsystem} ${ccltext} PRIVATE ${webp_LIBRARIES} ${yoga_LIBRARIES})
		target_include_directories (${cclgui} INTERFACE "$<INSTALL_INTERFACE:${VENDOR_PUBLIC_HEADERS_DESTINATION}>")
		target_compile_definitions (${cclgui} PUBLIC "CCLGUI_AVAILABLE=1")
		
		ccl_export_symbols (${cclgui} ${cclgui_exports})
		
		ccl_add_resources (${cclgui} ${cclgui_resources})

		if (CCL_SYSTEM_INSTALL)
			set_target_properties (${cclgui} PROPERTIES SOVERSION ${CCL_VERSION})
			install (TARGETS ${cclgui} EXPORT ccl-targets DESTINATION "${CCL_LIBRARY_DESTINATION}"
									   LIBRARY DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
									   RUNTIME DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
									   ARCHIVE DESTINATION "${CCL_IMPORT_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
									   FRAMEWORK DESTINATION "${CCL_LIBRARY_DESTINATION}" COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
									   FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
			)
			install (FILES ${cclgui_public_sources} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION}/public COMPONENT public_headers)
			install (FILES $<TARGET_FILE_DIR:${cclgui}>/${cclgui}.pdb DESTINATION "${CCL_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
		elseif (VENDOR_APPLICATION_RUNTIME_DIRECTORY)
			install (TARGETS ${cclgui} LIBRARY DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
									   RUNTIME DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
									   FRAMEWORK DESTINATION "${VENDOR_APPLICATION_RUNTIME_DIRECTORY}"
			)
		endif ()
	endif ()
	list (REMOVE_DUPLICATES cclgui_public_sources)
	target_sources (${cclgui} INTERFACE $<BUILD_INTERFACE:${cclgui_public_sources}> $<INSTALL_INTERFACE:${CCL_PUBLIC_HEADERS_DESTINATION}/public/cclguiiids.cpp>)
elseif (NOT XCODE)
	ccl_include_platform_specifics (cclgui)
endif ()

ccl_list_append_once (CCL_STATIC_COMPILE_DEFINITIONS CCL_STATIC_ENABLE_GUI=1)
