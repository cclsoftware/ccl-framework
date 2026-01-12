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
// Filename    : ccl/app/editing/tools/toolbar.h
// Description : Toolbar Component
//
//************************************************************************************************

#ifndef _ccl_toolbar_h
#define _ccl_toolbar_h

#include "ccl/app/component.h"

#include "ccl/base/storage/isettings.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/graphics/point.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

class EditTool;
class ToolCollection;
class ToolItem;

interface IImage;
interface IMenu;

//************************************************************************************************
// ToolBar
/** Manages a list of tools that can be selected by the user.
	The tools can be provided by multiple tool collections.	They are identified by name,
	so different tools from different collections (editors) can have a common representation in the toolbar. */
//************************************************************************************************

class ToolBar: public Component,
			   public ISettingsSaver
{
public:
	DECLARE_CLASS (ToolBar, Component)

	ToolBar ();
	~ToolBar ();

	static IParameter* createModeParameterForIcons (CStringPtr iconNames[], int count, int maxColumns = -1);
	static IParameter* createModeParameter (const String modeStrings[], CStringPtr iconNames[], int count);

	PROPERTY_STRING (settingsPath, SettingsPath) ///< path for saving/loading global settings; if empty, state is saved via component load/save (e.g. in a document)

	enum ToolModePresentation
	{
		kToolModeMenuPresentation,
		kToolModePalettePresentation
	};
	
	void setToolModePresentation (ToolModePresentation presentation);
	PROPERTY_OBJECT (StyleFlags, toolButtonStyle, ToolButtonStyle)
	
	void popup (const Point& where, IView* parent);

	// Tool collections
	void addToolCollection (ToolCollection* collection);
	void removeToolCollection (ToolCollection* collection);

	void setActiveTool (const EditTool* tool);
	EditTool* getActiveTool (ToolCollection& collection);
	int getToolIndex (StringID name) const;

	// Internal methods
	void extendModeMenu (IMenu& menu, StringID name);
	ToolItem* findToolItem (const IParameter* modeParam);

	// Extra tools
	/** This message is sent via the notify method to the parent component when a tool command is handled which number exceeds the tool count.
	    (note: this is needed to make the findToolBarAtMouse mechnism work (which fails if the component handles the "Toolbar" commands itself)
		msg[0] = tool number (starting at 1), 
		msg[1] = offset to last official tool number
		msg[2] = (bool) Boxed::Variant result parameter (set to true when handled) */	
	DECLARE_STRINGID_MEMBER (kSetExtraTool) 

	// Component
	tresult CCL_API initialize (IUnknown* context) override;
	tresult CCL_API terminate () override;
	IParameter* CCL_API findParameter (StringID name) const override;
	tbool CCL_API paramChanged (IParameter* param) override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
	DECLARE_COMMAND_CATEGORY2 ("Toolbar", "Toolmode", Component)

	// ISettingsSaver
	void restore (Settings&) override;
	void flush (Settings&) override;

	CLASS_INTERFACE (ISettingsSaver, Component)

protected:
	friend class ToolPalette;
	
	ObjectList toolItems;
	ObjectList toolCollections;
	IParameter* toolParameter;
	IImageProvider* toolImageProvider;
	ToolModePresentation toolModePresentation;

	void updateToolImageProvider ();
	ToolItem* getToolItem (const EditTool& tool);
	ToolBar* findToolBarAtMouse ();
	tbool onToolCommand (CmdArgs args);
	void appendToolModes (IMenu& menu, int toolNumber, ToolItem& toolItem);
	void loadState (const Attributes& attributes);
	void saveState (Attributes& attributes) const;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

} // namespace CCL

#endif // _ccl_toolbar_h
