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
# Filename    : cclapp.cmake
# Description : CCL network library
#
#************************************************************************************************

# collect source files
ccl_list_append_once (cclapp_public_sources
	${CCL_DIR}/public/cclappiids.cpp
	${CCL_DIR}/public/cclguiiids.cpp
)

ccl_list_append_once (cclapp_public_headers
	${CCL_DIR}/public/app/iactionjournal.h
	${CCL_DIR}/public/app/ibrowser.h
	${CCL_DIR}/public/app/idocument.h
	${CCL_DIR}/public/app/idocumentfilter.h
	${CCL_DIR}/public/app/idocumentmetainfo.h
	${CCL_DIR}/public/app/ieditenvironment.h
	${CCL_DIR}/public/app/iedittask.h
	${CCL_DIR}/public/app/ifileicons.h
	${CCL_DIR}/public/app/ifileinforegistry.h
	${CCL_DIR}/public/app/inavigationserver.h
	${CCL_DIR}/public/app/ipluginpresentation.h
	${CCL_DIR}/public/app/ipreset.h
	${CCL_DIR}/public/app/ipresetmetainfo.h
	${CCL_DIR}/public/app/irootcomponent.h
	${CCL_DIR}/public/app/isafetyoption.h
	${CCL_DIR}/public/app/iselection.h
	${CCL_DIR}/public/app/presetmetainfo.h
	${CCL_DIR}/public/app/signals.h
	${CCL_DIR}/public/app/documentfilter.h
	${CCL_DIR}/public/app/documentlistener.h

	${CCL_DIR}/public/gui/commanddispatch.h
	${CCL_DIR}/public/gui/framework/abstractdraghandler.h
	${CCL_DIR}/public/gui/framework/controlscalepainter.h
	${CCL_DIR}/public/gui/framework/dialogbox.h
	${CCL_DIR}/public/gui/framework/guievent.h
	${CCL_DIR}/public/gui/framework/ialert.h
	${CCL_DIR}/public/gui/framework/icommandtable.h
	${CCL_DIR}/public/gui/framework/idialogbuilder.h
	${CCL_DIR}/public/gui/framework/idleclient.h
	${CCL_DIR}/public/gui/framework/idragndrop.h
	${CCL_DIR}/public/gui/framework/idrawable.h
	${CCL_DIR}/public/gui/framework/ifileselector.h
	${CCL_DIR}/public/gui/framework/imenu.h
	${CCL_DIR}/public/gui/framework/iparametermenu.h
	${CCL_DIR}/public/gui/framework/isystemsharing.h
	${CCL_DIR}/public/gui/framework/itemviewgeometry.h
	${CCL_DIR}/public/gui/framework/isystemshell.h
	${CCL_DIR}/public/gui/framework/iskinmodel.h
	${CCL_DIR}/public/gui/framework/iuserinterface.h
	${CCL_DIR}/public/gui/framework/iview.h
	${CCL_DIR}/public/gui/framework/popupselectorclient.h
	${CCL_DIR}/public/gui/framework/usercontrolbase.h
	${CCL_DIR}/public/gui/framework/usertooltip.h
	${CCL_DIR}/public/gui/framework/viewbox.h
	${CCL_DIR}/public/gui/graphics/brush.h
	${CCL_DIR}/public/gui/graphics/color.h
	${CCL_DIR}/public/gui/graphics/font.h
	${CCL_DIR}/public/gui/graphics/graphicsfactory.h
	${CCL_DIR}/public/gui/graphics/point.h
	${CCL_DIR}/public/gui/graphics/rect.h
	${CCL_DIR}/public/gui/graphics/transform.h
	${CCL_DIR}/public/gui/graphics/updatergn.h
	${CCL_DIR}/public/gui/graphics/3d/itransformconstraints3d.h
	${CCL_DIR}/public/gui/graphics/3d/modelfactory3d.h
	${CCL_DIR}/public/gui/graphics/3d/point3d.h
	${CCL_DIR}/public/gui/graphics/3d/ray3d.h
	${CCL_DIR}/public/gui/graphics/3d/shaderconstants3d.h
	${CCL_DIR}/public/gui/graphics/3d/transform3d.h
	${CCL_DIR}/public/gui/paramlist.h
	${CCL_DIR}/public/guiservices.h

	${CCL_DIR}/public/plugins/icomponent.h
	${CCL_DIR}/public/plugins/iobjecttable.h
	${CCL_DIR}/public/plugins/ipluginmanager.h
	${CCL_DIR}/public/plugins/iscriptengine.h
	${CCL_DIR}/public/plugins/iscriptingmanager.h
	${CCL_DIR}/public/plugins/iservicemanager.h
	${CCL_DIR}/public/plugins/stubobject.h	
)

ccl_list_append_once (cclapp_api_headers
	${CCL_DIR}/app/actions/action.h
	${CCL_DIR}/app/actions/actionexecuter.h
	${CCL_DIR}/app/actions/actionjournal.h
	${CCL_DIR}/app/actions/actionjournalcomponent.h
	${CCL_DIR}/app/actions/actionjournalpreview.h
	${CCL_DIR}/app/actions/iactioncontext.h
	${CCL_DIR}/app/actions/sideeffect.h
	${CCL_DIR}/app/actions/transaction.h

	${CCL_DIR}/app/application.h
	${CCL_DIR}/app/applicationspecifics.h

	${CCL_DIR}/app/browser/browser.h
	${CCL_DIR}/app/browser/browserextender.h
	${CCL_DIR}/app/browser/browsernode.h
	${CCL_DIR}/app/browser/filebrowser.h
	${CCL_DIR}/app/browser/filedraghandler.h
	${CCL_DIR}/app/browser/filesystemnodes.h
	${CCL_DIR}/app/browser/filexportdraghandler.h
	${CCL_DIR}/app/browser/nodenavigator.h
	${CCL_DIR}/app/browser/nodesorter.h
	${CCL_DIR}/app/browser/pluginbrowser.h
	${CCL_DIR}/app/browser/plugindraghander.h
	${CCL_DIR}/app/browser/pluginmanagement.h
	${CCL_DIR}/app/browser/pluginnodes.h
	${CCL_DIR}/app/browser/pluginselector.h
	${CCL_DIR}/app/browser/searchresultlist.h

	${CCL_DIR}/app/components/asynchandler.h
	${CCL_DIR}/app/components/colorpicker.h
	${CCL_DIR}/app/components/breadcrumbscomponent.h
	${CCL_DIR}/app/components/consolecomponent.h
	${CCL_DIR}/app/components/eulacomponent.h
	${CCL_DIR}/app/components/fontselector.h
	${CCL_DIR}/app/components/helpcomponent.h
	${CCL_DIR}/app/components/imageselector.h
	${CCL_DIR}/app/components/inplaceprogresscomponent.h
	${CCL_DIR}/app/components/listeditcomponent.h
	${CCL_DIR}/app/components/listvieweditcomponent.h
	${CCL_DIR}/app/components/notificationcomponent.h
	${CCL_DIR}/app/components/pathselector.h
	${CCL_DIR}/app/components/scenecomponent3d.h
	${CCL_DIR}/app/components/istartupprogress.h
	${CCL_DIR}/app/components/startupprogresscomponent.h
	${CCL_DIR}/app/components/stringlistcomponent.h
	${CCL_DIR}/app/components/fileexporter.h
	${CCL_DIR}/app/components/filerenamer.h
	${CCL_DIR}/app/components/isearchprovider.h
	${CCL_DIR}/app/components/searchcomponent.h
	${CCL_DIR}/app/components/searchprovider.h

	${CCL_DIR}/app/component.h
	${CCL_DIR}/app/componentalias.h
	${CCL_DIR}/app/componentaliasfactory.h
	${CCL_DIR}/app/componentfactory.h

	${CCL_DIR}/app/controls/colorizedview.h
	${CCL_DIR}/app/controls/dragcontrol.h
	${CCL_DIR}/app/controls/draghandler.h
	${CCL_DIR}/app/controls/dropboxmodel.h
	${CCL_DIR}/app/controls/itemselectorpopup.h
	${CCL_DIR}/app/controls/itemviewmodel.h
	${CCL_DIR}/app/controls/listviewitem.h
	${CCL_DIR}/app/controls/listviewmodel.h
	${CCL_DIR}/app/controls/spritebuilder.h
	${CCL_DIR}/app/controls/statsdata.h
	${CCL_DIR}/app/controls/statsview.h
	${CCL_DIR}/app/controls/treeviewmodel.h
	${CCL_DIR}/app/controls/treeviewnode.h
	${CCL_DIR}/app/controls/usercontrol.h
	${CCL_DIR}/app/controls/usersceneview3d.h

	${CCL_DIR}/app/debugmenu.h

	${CCL_DIR}/app/documents/autosaver.h
	${CCL_DIR}/app/documents/document.h
	${CCL_DIR}/app/documents/documentapp.h
	${CCL_DIR}/app/documents/documentassistant.h
	${CCL_DIR}/app/documents/documentblocks.h
	${CCL_DIR}/app/documents/documentdiagnostic.h
	${CCL_DIR}/app/documents/documentdialog.h
	${CCL_DIR}/app/documents/documentmanager.h
	${CCL_DIR}/app/documents/documentmetainfo.h
	${CCL_DIR}/app/documents/documentnavigation.h
	${CCL_DIR}/app/documents/documentperspective.h
	${CCL_DIR}/app/documents/documentrenamer.h
	${CCL_DIR}/app/documents/documenttemplates.h
	${CCL_DIR}/app/documents/documentversions.h
	${CCL_DIR}/app/documents/documentwindow.h
	${CCL_DIR}/app/documents/idocumentview.h
	${CCL_DIR}/app/documents/packagedocument.h
	${CCL_DIR}/app/documents/recentdocuments.h

	${CCL_DIR}/app/editing/addins/editaddin.h
	${CCL_DIR}/app/editing/addins/editaddincollection.h
	${CCL_DIR}/app/editing/addins/editaddindescription.h
	${CCL_DIR}/app/editing/addins/editenvironment.h
	${CCL_DIR}/app/editing/editcursor.h
	${CCL_DIR}/app/editing/editdraghandler.h
	${CCL_DIR}/app/editing/editextension.h
	${CCL_DIR}/app/editing/edithandler.h
	${CCL_DIR}/app/editing/editlayer.h
	${CCL_DIR}/app/editing/editmodel.h
	${CCL_DIR}/app/editing/editor.h
	${CCL_DIR}/app/editing/editview.h
	${CCL_DIR}/app/editing/inspector.h
	${CCL_DIR}/app/editing/iscale.h
	${CCL_DIR}/app/editing/scale.h
	${CCL_DIR}/app/editing/scaleview.h
	${CCL_DIR}/app/editing/selectaction.h
	${CCL_DIR}/app/editing/selection.h
	${CCL_DIR}/app/editing/snapper.h
	${CCL_DIR}/app/editing/tasks/edittask.h
	${CCL_DIR}/app/editing/tasks/edittaskcollection.h
	${CCL_DIR}/app/editing/tasks/edittaskdescription.h
	${CCL_DIR}/app/editing/tasks/edittaskhandler.h
	${CCL_DIR}/app/editing/tasks/edittaskpreview.h
	${CCL_DIR}/app/editing/tools/edittool.h
	${CCL_DIR}/app/editing/tools/itoolconfig.h
	${CCL_DIR}/app/editing/tools/selecttool.h
	${CCL_DIR}/app/editing/tools/toolaction.h
	${CCL_DIR}/app/editing/tools/toolbar.h
	${CCL_DIR}/app/editing/tools/toolcollection.h
	${CCL_DIR}/app/editing/tools/toolconfig.h
	
	${CCL_DIR}/app/utilities/appdiagnostic.h
	${CCL_DIR}/app/utilities/batchoperation.h
	${CCL_DIR}/app/utilities/boxedguitypes.h
	${CCL_DIR}/app/utilities/errorcontextdialog.h
	${CCL_DIR}/app/utilities/fileoperations.h
	${CCL_DIR}/app/utilities/imagebuilder.h
	${CCL_DIR}/app/utilities/imagefile.h
	${CCL_DIR}/app/utilities/menubuilder.h
	${CCL_DIR}/app/utilities/multiprogress.h
	${CCL_DIR}/app/utilities/multisprite.h
	${CCL_DIR}/app/utilities/pathclassifier.h
	${CCL_DIR}/app/utilities/pluginclass.h
	${CCL_DIR}/app/utilities/paramaccessor.h
	${CCL_DIR}/app/utilities/sortfolderlist.h
	${CCL_DIR}/app/utilities/fileicons.h
	${CCL_DIR}/app/utilities/shellcommand.h

	${CCL_DIR}/app/fileinfo/fileinfocomponent.h
	${CCL_DIR}/app/fileinfo/fileinforegistry.h
	${CCL_DIR}/app/fileinfo/filepreviewcomponent.h
	${CCL_DIR}/app/fileinfo/volumeinfocomponent.h
	${CCL_DIR}/app/fileinfo/pluginfileinfo.h
	${CCL_DIR}/app/fileinfo/presetfileinfo.h
	${CCL_DIR}/app/fileinfo/documentfileinfo.h

	${CCL_DIR}/app/modulecomponent.h

	${CCL_DIR}/app/navigation/navigationservice.h
	${CCL_DIR}/app/navigation/navigator.h
	${CCL_DIR}/app/navigation/navigatorbase.h
	${CCL_DIR}/app/navigation/webnavigator.h

	${CCL_DIR}/app/options/commandoption.h
	${CCL_DIR}/app/options/customization.h
	${CCL_DIR}/app/options/filetypeselector.h
	${CCL_DIR}/app/options/mainoption.h
	${CCL_DIR}/app/options/serviceoption.h
	${CCL_DIR}/app/options/useroption.h
	${CCL_DIR}/app/options/useroptionelement.h

	${CCL_DIR}/app/paramalias.h
	${CCL_DIR}/app/paramcontainer.h
	${CCL_DIR}/app/params.h

	${CCL_DIR}/app/presets/memorypreset.h
	${CCL_DIR}/app/presets/objectpreset.h
	${CCL_DIR}/app/presets/preset.h
	${CCL_DIR}/app/presets/presetbrowser.h
	${CCL_DIR}/app/presets/presetcollection.h
	${CCL_DIR}/app/presets/presetcomponent.h
	${CCL_DIR}/app/presets/presetdescriptor.h
	${CCL_DIR}/app/presets/presetdrag.h
	${CCL_DIR}/app/presets/presetfile.h
	${CCL_DIR}/app/presets/presetfileexporter.h
	${CCL_DIR}/app/presets/presetfileprimitives.h
	${CCL_DIR}/app/presets/presetfileregistry.h
	${CCL_DIR}/app/presets/presetmanager.h
	${CCL_DIR}/app/presets/presetnode.h
	${CCL_DIR}/app/presets/presetparam.h
	${CCL_DIR}/app/presets/presetstore.h
	${CCL_DIR}/app/presets/presetsystem.h
	${CCL_DIR}/app/presets/presettrader.h
	${CCL_DIR}/app/presets/selectpresetdialog.h
	${CCL_DIR}/app/presets/simplepreset.h

	${CCL_DIR}/app/safety/appsafety.h
	${CCL_DIR}/app/safety/appsafetymanager.h
)

ccl_list_append_once (cclapp_public_source_files
	${CCL_DIR}/public/app/documentfilter.cpp
	${CCL_DIR}/public/app/documentlistener.cpp
	
	${CCL_DIR}/public/gui/commanddispatch.cpp
	${CCL_DIR}/public/gui/framework/controlscalepainter.cpp
	${CCL_DIR}/public/gui/framework/dialogbox.cpp
	${CCL_DIR}/public/gui/framework/guievent.cpp
	${CCL_DIR}/public/gui/framework/ialert.cpp
	${CCL_DIR}/public/gui/framework/idleclient.cpp
	${CCL_DIR}/public/gui/framework/idrawable.cpp
	${CCL_DIR}/public/gui/framework/iskinmodel.cpp
	${CCL_DIR}/public/gui/framework/popupselectorclient.cpp
	${CCL_DIR}/public/gui/framework/itemviewgeometry.cpp
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
	${CCL_DIR}/public/gui/graphics/3d/point3d.cpp
	${CCL_DIR}/public/gui/graphics/3d/ray3d.cpp
	${CCL_DIR}/public/gui/graphics/3d/transform3d.cpp
	${CCL_DIR}/public/gui/graphics/3d/itransformconstraints3d.cpp
	${CCL_DIR}/public/gui/graphics/3d/modelfactory3d.cpp
	${CCL_DIR}/public/gui/paramlist.cpp
	
	${CCL_DIR}/public/plugins/stubobject.cpp
)

ccl_list_append_once (cclapp_source_files
	${CCL_DIR}/app/actions/action.cpp
	${CCL_DIR}/app/actions/actionexecuter.cpp
	${CCL_DIR}/app/actions/actionjournal.cpp
	${CCL_DIR}/app/actions/actionjournalcomponent.cpp
	${CCL_DIR}/app/actions/sideeffect.cpp
	${CCL_DIR}/app/actions/transaction.cpp

	${CCL_DIR}/app/application.cpp
	${CCL_DIR}/app/applicationspecifics.cpp

	${CCL_DIR}/app/browser/browser.cpp
	${CCL_DIR}/app/browser/browserextender.cpp
	${CCL_DIR}/app/browser/browsernode.cpp
	${CCL_DIR}/app/browser/filebrowser.cpp
	${CCL_DIR}/app/browser/filedraghandler.cpp
	${CCL_DIR}/app/browser/filesystemnodes.cpp
	${CCL_DIR}/app/browser/filexportdraghandler.cpp
	${CCL_DIR}/app/browser/nodenavigator.cpp
	${CCL_DIR}/app/browser/nodesorter.cpp
	${CCL_DIR}/app/browser/pluginbrowser.cpp
	${CCL_DIR}/app/browser/plugindraghander.cpp
	${CCL_DIR}/app/browser/pluginmanagement.cpp
	${CCL_DIR}/app/browser/pluginnodes.cpp
	${CCL_DIR}/app/browser/pluginselector.cpp
	${CCL_DIR}/app/browser/searchresultlist.cpp

	${CCL_DIR}/app/components/asynchandler.cpp
	${CCL_DIR}/app/components/colorpicker.cpp
	${CCL_DIR}/app/components/breadcrumbscomponent.cpp
	${CCL_DIR}/app/components/consolecomponent.cpp
	${CCL_DIR}/app/components/eulacomponent.cpp
	${CCL_DIR}/app/components/fontselector.cpp
	${CCL_DIR}/app/components/helpcomponent.cpp
	${CCL_DIR}/app/components/imageselector.cpp
	${CCL_DIR}/app/components/inplaceprogresscomponent.cpp
	${CCL_DIR}/app/components/listeditcomponent.cpp
	${CCL_DIR}/app/components/listvieweditcomponent.cpp
	${CCL_DIR}/app/components/notificationcomponent.cpp
	${CCL_DIR}/app/components/pathselector.cpp
	${CCL_DIR}/app/components/scenecomponent3d.cpp
	${CCL_DIR}/app/components/startupprogresscomponent.cpp
	${CCL_DIR}/app/components/stringlistcomponent.cpp
	${CCL_DIR}/app/components/fileexporter.cpp
	${CCL_DIR}/app/components/filerenamer.cpp
	${CCL_DIR}/app/components/searchcomponent.cpp
	${CCL_DIR}/app/components/searchprovider.cpp

	${CCL_DIR}/app/component.cpp
	${CCL_DIR}/app/componentalias.cpp
	${CCL_DIR}/app/componentfactory.cpp

	${CCL_DIR}/app/controls/colorizedview.cpp
	${CCL_DIR}/app/controls/dragcontrol.cpp
	${CCL_DIR}/app/controls/draghandler.cpp
	${CCL_DIR}/app/controls/dropboxmodel.cpp
	${CCL_DIR}/app/controls/itemselectorpopup.cpp
	${CCL_DIR}/app/controls/itemviewmodel.cpp
	${CCL_DIR}/app/controls/listviewitem.cpp
	${CCL_DIR}/app/controls/listviewmodel.cpp
	${CCL_DIR}/app/controls/spritebuilder.cpp
	${CCL_DIR}/app/controls/statsdata.cpp
	${CCL_DIR}/app/controls/statsview.cpp
	${CCL_DIR}/app/controls/treeviewmodel.cpp
	${CCL_DIR}/app/controls/treeviewnode.cpp
	${CCL_DIR}/app/controls/usercontrol.cpp
	${CCL_DIR}/app/controls/usersceneview3d.cpp

	${CCL_DIR}/app/debugmenu.cpp

	${CCL_DIR}/app/documents/autosaver.cpp
	${CCL_DIR}/app/documents/document.cpp
	${CCL_DIR}/app/documents/documentapp.cpp
	${CCL_DIR}/app/documents/documentassistant.cpp
	${CCL_DIR}/app/documents/documentblocks.cpp
	${CCL_DIR}/app/documents/documentdiagnostic.cpp
	${CCL_DIR}/app/documents/documentdialog.cpp
	${CCL_DIR}/app/documents/documentmanager.cpp
	${CCL_DIR}/app/documents/documentnavigation.cpp
	${CCL_DIR}/app/documents/documentperspective.cpp
	${CCL_DIR}/app/documents/documentrenamer.cpp
	${CCL_DIR}/app/documents/documenttemplates.cpp
	${CCL_DIR}/app/documents/documentversions.cpp
	${CCL_DIR}/app/documents/documentwindow.cpp
	${CCL_DIR}/app/documents/packagedocument.cpp
	${CCL_DIR}/app/documents/recentdocuments.cpp

	${CCL_DIR}/app/editing/addins/editaddin.cpp
	${CCL_DIR}/app/editing/addins/editaddincollection.cpp
	${CCL_DIR}/app/editing/addins/editaddindescription.cpp
	${CCL_DIR}/app/editing/addins/editenvironment.cpp
	${CCL_DIR}/app/editing/editcursor.cpp
	${CCL_DIR}/app/editing/editdraghandler.cpp
	${CCL_DIR}/app/editing/editextension.cpp
	${CCL_DIR}/app/editing/edithandler.cpp
	${CCL_DIR}/app/editing/editlayer.cpp
	${CCL_DIR}/app/editing/editmodel.cpp
	${CCL_DIR}/app/editing/editor.cpp
	${CCL_DIR}/app/editing/editview.cpp
	${CCL_DIR}/app/editing/inspector.cpp
	${CCL_DIR}/app/editing/scale.cpp
	${CCL_DIR}/app/editing/scaleview.cpp
	${CCL_DIR}/app/editing/selectaction.cpp
	${CCL_DIR}/app/editing/selection.cpp
	${CCL_DIR}/app/editing/snapper.cpp
	${CCL_DIR}/app/editing/tasks/edittask.cpp
	${CCL_DIR}/app/editing/tasks/edittaskcollection.cpp
	${CCL_DIR}/app/editing/tasks/edittaskdescription.cpp
	${CCL_DIR}/app/editing/tasks/edittaskhandler.cpp
	${CCL_DIR}/app/editing/tasks/edittaskpreview.cpp
	${CCL_DIR}/app/editing/tools/edittool.cpp
	${CCL_DIR}/app/editing/tools/selecttool.cpp
	${CCL_DIR}/app/editing/tools/toolaction.cpp
	${CCL_DIR}/app/editing/tools/toolbar.cpp
	${CCL_DIR}/app/editing/tools/toolcollection.cpp
	${CCL_DIR}/app/editing/tools/toolconfig.cpp
	
	${CCL_DIR}/app/utilities/appdiagnostic.cpp
	${CCL_DIR}/app/utilities/batchoperation.cpp
	${CCL_DIR}/app/utilities/boxedguitypes.cpp
	${CCL_DIR}/app/utilities/errorcontextdialog.cpp
	${CCL_DIR}/app/utilities/fileoperations.cpp
	${CCL_DIR}/app/utilities/imagebuilder.cpp
	${CCL_DIR}/app/utilities/imagefile.cpp
	${CCL_DIR}/app/utilities/menubuilder.cpp
	${CCL_DIR}/app/utilities/multiprogress.cpp
	${CCL_DIR}/app/utilities/multisprite.cpp
	${CCL_DIR}/app/utilities/pathclassifier.cpp
	${CCL_DIR}/app/utilities/pluginclass.cpp
	${CCL_DIR}/app/utilities/paramaccessor.cpp
	${CCL_DIR}/app/utilities/sortfolderlist.cpp
	${CCL_DIR}/app/utilities/fileicons.cpp
	${CCL_DIR}/app/utilities/shellcommand.cpp

	${CCL_DIR}/app/fileinfo/fileinfocomponent.cpp
	${CCL_DIR}/app/fileinfo/fileinforegistry.cpp
	${CCL_DIR}/app/fileinfo/filepreviewcomponent.cpp
	${CCL_DIR}/app/fileinfo/volumeinfocomponent.cpp
	${CCL_DIR}/app/fileinfo/pluginfileinfo.cpp
	${CCL_DIR}/app/fileinfo/presetfileinfo.cpp
	${CCL_DIR}/app/fileinfo/documentfileinfo.cpp

	${CCL_DIR}/app/modulecomponent.cpp

	${CCL_DIR}/app/navigation/navigationservice.cpp
	${CCL_DIR}/app/navigation/navigator.cpp
	${CCL_DIR}/app/navigation/navigatorbase.cpp
	${CCL_DIR}/app/navigation/webnavigator.cpp

	${CCL_DIR}/app/options/commandoption.cpp
	${CCL_DIR}/app/options/customization.cpp
	${CCL_DIR}/app/options/filetypeselector.cpp
	${CCL_DIR}/app/options/mainoption.cpp
	${CCL_DIR}/app/options/serviceoption.cpp
	${CCL_DIR}/app/options/useroption.cpp
	${CCL_DIR}/app/options/useroptionelement.cpp

	${CCL_DIR}/app/paramalias.cpp
	${CCL_DIR}/app/paramcontainer.cpp
	${CCL_DIR}/app/params.cpp

	${CCL_DIR}/app/presets/memorypreset.cpp
	${CCL_DIR}/app/presets/objectpreset.cpp
	${CCL_DIR}/app/presets/preset.cpp
	${CCL_DIR}/app/presets/presetbrowser.cpp
	${CCL_DIR}/app/presets/presetcollection.cpp
	${CCL_DIR}/app/presets/presetcomponent.cpp
	${CCL_DIR}/app/presets/presetdescriptor.cpp
	${CCL_DIR}/app/presets/presetdrag.cpp
	${CCL_DIR}/app/presets/presetfile.cpp
	${CCL_DIR}/app/presets/presetfileexporter.cpp
	${CCL_DIR}/app/presets/presetfileprimitives.cpp
	${CCL_DIR}/app/presets/presetfileregistry.cpp
	${CCL_DIR}/app/presets/presetmanager.cpp
	${CCL_DIR}/app/presets/presetnode.cpp
	${CCL_DIR}/app/presets/presetparam.cpp
	${CCL_DIR}/app/presets/presetstore.cpp
	${CCL_DIR}/app/presets/presetsystem.cpp
	${CCL_DIR}/app/presets/presettrader.cpp
	${CCL_DIR}/app/presets/selectpresetdialog.cpp
	${CCL_DIR}/app/presets/simplepreset.cpp

	${CCL_DIR}/app/safety/appsafety.cpp
	${CCL_DIR}/app/safety/appsafetymanager.cpp
)

# set up project folder structure
if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL cclapp_SOURCE_DIR)
	source_group (TREE "${CCL_DIR}/public" PREFIX "public" FILES ${cclapp_public_source_files} ${cclapp_public_sources} ${cclapp_public_headers})
	source_group (TREE "${CCL_DIR}/app" PREFIX "source" FILES ${cclapp_source_files} ${cclapp_api_headers})
	source_group ("cmake" FILES ${CMAKE_CURRENT_LIST_FILE})
	set (cclapp_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

ccl_list_append_once (CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# Find dependencies
find_package (ccl REQUIRED COMPONENTS cclbase)

set (cclapp "cclapp")
if (CCL_ISOLATION_POSTFIX)
	string (APPEND cclapp ".${CCL_ISOLATION_POSTFIX}")
endif ()

# Add targets
if (NOT TARGET ${cclapp})
	ccl_add_library (cclapp STATIC VENDOR ccl POSTFIX "${CCL_ISOLATION_POSTFIX}")

	# collect source files for target
	ccl_list_append_once (cclapp_sources
		${cclapp_public_sources}
		${cclapp_public_source_files}
		${cclapp_source_files}
	)

	target_sources (${cclapp} PRIVATE ${cclapp_sources} ${CMAKE_CURRENT_LIST_FILE})
	ccl_target_headers (${cclapp} INSTALL ${CCL_SYSTEM_INSTALL} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} BASE_DIRS ${CCL_DIR} FILES ${cclapp_public_headers} ${cclapp_api_headers})

	target_link_libraries (${cclapp} INTERFACE ${cclbase})
	target_include_directories (${cclapp} INTERFACE "$<INSTALL_INTERFACE:${VENDOR_PUBLIC_HEADERS_DESTINATION}>")

	if (CCL_SYSTEM_INSTALL)
		set_target_properties (${cclapp} PROPERTIES SOVERSION ${CCL_VERSION})
		install (TARGETS ${cclapp} EXPORT ccl-targets DESTINATION ${CCL_STATIC_LIBRARY_DESTINATION}
								ARCHIVE DESTINATION ${CCL_STATIC_LIBRARY_DESTINATION} COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
								FRAMEWORK DESTINATION ${CCL_STATIC_LIBRARY_DESTINATION} COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX}
								FILE_SET HEADERS DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION} COMPONENT public_headers
		)
		install (FILES ${cclapp_public_sources} DESTINATION ${CCL_PUBLIC_HEADERS_DESTINATION}/public COMPONENT public_headers)
		install (FILES $<TARGET_FILE_DIR:${cclapp}>/${cclapp}$<$<CONFIG:DEBUG>:d>.pdb DESTINATION "${CCL_STATIC_LIBRARY_DESTINATION}" OPTIONAL COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})
	endif ()
endif ()

ccl_list_append_once (CCL_IS_STATIC ${cclapp})
