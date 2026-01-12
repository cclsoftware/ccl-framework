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
// Filename    : ccl/app/editing/tools/toolbar.cpp
// Description : Toolbar Component
//
//************************************************************************************************

#include "ccl/app/editing/tools/toolbar.h"
#include "ccl/app/editing/tools/toolcollection.h"
#include "ccl/app/editing/tools/edittool.h"
#include "ccl/app/editing/editor.h"

#include "ccl/app/params.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/configuration.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/boxedtypes.h"

#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ipopupselector.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/viewfinder.h"
#include "ccl/public/gui/framework/controlproperties.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"

namespace CCL {

//************************************************************************************************
// ToolItem
//************************************************************************************************

class ToolItem: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (ToolItem, Object)

	ToolItem (const EditTool& tool);
	ToolItem (const EditToolMode& mode);

	PROPERTY_STRING (title, Title)						///< tool title displayed to user
	PROPERTY_MUTABLE_CSTRING (name, Name)				///< internal tool name
	PROPERTY_MUTABLE_CSTRING (iconName, IconName)	    ///< icon name
	PROPERTY_SHARED_AUTO (IImage, icon, Icon)           ///< icon (icon name is ignored when icon is set)

	PROPERTY_BOOL (ignoresModeIcons, IgnoresModeIcons)

	PROPERTY_AUTO_POINTER (ListParam, modeParam, ModeParam)
	void addModes (const EditTool& tool);
	int getModeCount () const;
	const ToolItem* getMode (int index) const;
	IImage* getToolIcon (ITheme* theme) const;

	// Object
	bool equals (const Object& obj) const override;	///< compares with an EditTool by name!
	bool toString (String& string, int flags = 0) const override;
};

//************************************************************************************************
// ToolPalette
//************************************************************************************************

class ToolPalette: public Object,
				   public AbstractPalette
{
public:
	ToolPalette (ToolBar& toolBar, bool withModes);

	class Item: public ToolItem
	{
	public:
		Item (const ToolItem& item, int toolIndex, int modeIndex)
		: ToolItem (item),
		  toolIndex (toolIndex),
		  modeIndex (modeIndex)
		{}

		PROPERTY_VARIABLE (int, toolIndex, ToolIndex)
		PROPERTY_VARIABLE (int, modeIndex, ModeIndex)
	};

	Iterator* newIterator () const;

	// IPalette
	int CCL_API getCount () const override;
	tbool CCL_API getDimensions (int& columns, int& cellWidth, int& cellHeight) const override;
	IImage* CCL_API createIcon (int index, int width, int height, const IVisualStyle& style) const override;
	tbool CCL_API getTitle (String& title, int index) const override;

	CLASS_INTERFACE (IPalette, Object)

protected:
	ToolBar& toolBar;
	ObjectArray items;
};

//************************************************************************************************
// ToolModeParameter
//************************************************************************************************

class ToolModeParameter: public MenuParam,
						 public IImageProvider
{
public:
	ToolModeParameter (StringID name, bool useToolItems);

	void addModeIconId (StringID iconId);
	IImage* getIconAt (int i, ITheme* theme) const;
		
	// MenuParam
	void CCL_API extendMenu (IMenu& menu, StringID name) override;

	// IImageProvider
	IImage* CCL_API getImage () const override;
	void CCL_API setImage (IImage* image, tbool update = false) override;

	CLASS_INTERFACE (IImageProvider, MenuParam)

private:
	bool useToolItems;
	Vector<MutableCString> modeIconIds;
};


class ToolModePalette;

//************************************************************************************************
// ToolModePaletteParameter
/** ToolModePaletteParameter alternative way of displaying tool modes
	ToolModePaletteParameters are used by the toolbar when kToolModePalettePresentation is set. */
//************************************************************************************************

class ToolModePaletteParameter: public ListParam,
								public IPaletteProvider,
								public IImageProvider
{
public:
	ToolModePaletteParameter (StringID name, bool useToolItems, int maxColumns = -1);
	~ToolModePaletteParameter ();

	IImage* getIconAt (int index) const;

	// IPaletteProvider
	IPalette* CCL_API getPalette () const override;
	void CCL_API setPalette (IPalette* palette) override;

	// IImageProvider
	IImage* CCL_API getImage () const override;
	void CCL_API setImage (IImage* image, tbool update = false) override;

	CLASS_INTERFACE2 (IPaletteProvider, IImageProvider, ListParam)

private:
	bool useToolItems;
	int maxColumns;
	mutable ToolModePalette* palette;
};

//************************************************************************************************
// ToolModePalette
//************************************************************************************************

class ToolModePalette : public Object,
						public AbstractPalette
{
public:
	ToolModePalette (const ToolModePaletteParameter* modeParam, int maxColumns = -1);

	PROPERTY_VARIABLE (Coord, cellWidth, CellWidth);
	PROPERTY_VARIABLE (Coord, cellHeight, CellHeight);

	// AbstractPalette
	int CCL_API getCount () const override;
	IImage* CCL_API createIcon (int index, int width, int height, const IVisualStyle& style) const override;
	tbool CCL_API getDimensions (int& columns, int& cellWidth, int& cellHeight) const override;

	CLASS_INTERFACE (IPalette, Object)

protected:
	const ToolModePaletteParameter* modeParam;
	int maxColumns;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum ToolBarTags
	{
		kTool = 100,
		kToolMode,
		kToolPalette,
		kToolImage
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_COMMAND ("Toolbar", "Next Tool")
REGISTER_COMMAND ("Toolbar", "Previous Tool")
REGISTER_COMMAND ("Toolbar", "Tool 1")
REGISTER_COMMAND ("Toolbar", "Tool 2")
REGISTER_COMMAND ("Toolbar", "Tool 3")
REGISTER_COMMAND ("Toolbar", "Tool 4")
REGISTER_COMMAND ("Toolbar", "Tool 5")
REGISTER_COMMAND ("Toolbar", "Tool 6")
REGISTER_COMMAND ("Toolbar", "Tool 7")
REGISTER_COMMAND ("Toolbar", "Tool 8")
REGISTER_COMMAND ("Toolbar", "Tool 9")
REGISTER_COMMAND ("Toolbar", "Tool 10")

//************************************************************************************************
// ToolItem
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ToolItem, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolItem::ToolItem (const EditTool& tool)
: title (tool.getTitle ()),
  name (tool.getName ()),
  icon (tool.getIcon ()),
  iconName (tool.getIconName ()),
  ignoresModeIcons (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolItem::ToolItem (const EditToolMode& mode)
: title (mode.getTitle ()),
  name (mode.getName ()),
  icon (mode.getIcon ()),
  iconName (mode.getIconName ()),
  ignoresModeIcons (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolItem::addModes (const EditTool& tool)
{
	ASSERT (modeParam)
	ForEach (tool.getModes (), EditToolMode, mode)
		if(!modeParam->contains (*mode))
		{
			ToolItem* item = NEW ToolItem (*mode);
			modeParam->appendObject (item);
			if(tool.getActiveMode () == item->getName ())
				modeParam->selectObject (item);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ToolItem::getModeCount () const
{
	return modeParam ? modeParam->getMax ().asInt () + 1 : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ToolItem* ToolItem::getMode (int index) const
{
	return modeParam ? modeParam->getObject<ToolItem> (index) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ToolItem::getToolIcon (ITheme* theme) const
{
	if(icon)
		return icon;

	if(iconName.isEmpty () == false)
		return theme->getImage (iconName);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolItem::equals (const Object& obj) const
{
	if(const EditTool* tool = ccl_cast<EditTool> (&obj))
		return name == tool->getName ();

	if(const EditToolMode* mode = ccl_cast<EditToolMode> (&obj))
		return name == mode->getName ();

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolItem::toString (String& string, int flags) const
{
	string = getTitle ();
	return true;
}

//************************************************************************************************
// ToolModeParameter
//************************************************************************************************

ToolModeParameter::ToolModeParameter (StringID name, bool useToolItems)
: MenuParam (name),
  useToolItems (useToolItems)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ToolModeParameter::extendMenu (IMenu& menu, StringID name)
{
	ToolBar* toolbar = unknown_cast<ToolBar> (controller);
	ITheme* theme = toolbar ? toolbar->getTheme () : RootComponent::instance ().getTheme ();

	// enable large icons
	menu.setMenuAttribute (IMenu::kMenuVariant, IMenu::strLargeVariant);

	// set mode icons
	for(int i = 0; i < menu.countItems (); i++)
	{
		if(IMenuItem* menuItem = menu.getItem (i))
			if(IImage* icon = getIconAt (i, theme))
				menuItem->setItemAttribute (IMenuItem::kItemIcon, Variant (icon));
	}

	if(useToolItems && toolbar)
		toolbar->extendModeMenu (menu, name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ToolModeParameter::getImage () const
{
	ToolBar* toolbar = unknown_cast<ToolBar> (controller);
	ITheme* theme = toolbar ? toolbar->getTheme () : RootComponent::instance ().getTheme ();

	if(useToolItems && theme)
		if(ToolItem* toolItem = toolbar->findToolItem (this))
			if(toolItem->isIgnoresModeIcons ())
				return toolItem->getToolIcon (theme);

	return getIconAt (value, theme);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ToolModeParameter::setImage (IImage* image, tbool update)
{
	ASSERT (false)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolModeParameter::addModeIconId (StringID iconId)
{
	modeIconIds.add (iconId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ToolModeParameter::getIconAt (int i, ITheme* theme) const
{
	if(theme)
	{
		if(useToolItems)
		{
			ToolItem* modeItem = (ToolItem*)list.at (i);
			return modeItem->getToolIcon (theme);	
		}
		else if(i < modeIconIds.count ())
		{
			return theme->getImage (modeIconIds.at (i));
		}
	}
	
	return nullptr;
};

//************************************************************************************************
// ToolModePalette
//************************************************************************************************

ToolModePalette::ToolModePalette (const ToolModePaletteParameter* modeParam, int maxColumns)
: modeParam (modeParam),
  cellWidth (34),
  cellHeight (34),
  maxColumns (maxColumns)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ToolModePalette::getCount () const
{
	return modeParam->getMax ().asInt () + 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ToolModePalette::createIcon (int index, int width, int height, const IVisualStyle &style) const
{
	if(IImage* icon = modeParam->getIconAt (index))
	{
		//TODO: set isTemplate attribute at shapeImage resources directly
		UnknownPtr<IObject> (icon)->setProperty (IImage::kIsTemplate, true);
		icon->retain ();
		return icon;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ToolModePalette::getDimensions (int& _columns, int& _cellWidth, int& _cellHeight) const
{
	_columns = getCount ();
	if(_columns > maxColumns && maxColumns > 0)
		_columns = maxColumns;

	_cellWidth = cellWidth;
	_cellHeight = cellHeight;

	return true;
}

//************************************************************************************************
// ToolModePaletteParameter
//************************************************************************************************

ToolModePaletteParameter::ToolModePaletteParameter (StringID name, bool useToolItems, int maxColumns)
: ListParam (name),
  palette (nullptr),
  maxColumns (maxColumns),
  useToolItems (useToolItems)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolModePaletteParameter::~ToolModePaletteParameter ()
{
	if(palette)
		palette->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPalette* CCL_API ToolModePaletteParameter::getPalette () const
{
	if(!palette)
		palette = NEW ToolModePalette (this, maxColumns);

	return palette;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ToolModePaletteParameter::setPalette (IPalette* palette)
{
	ASSERT (false)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ToolModePaletteParameter::getIconAt (int index) const
{
	if(useToolItems == true)
	{
		if(ToolBar* toolbar = unknown_cast<ToolBar> (controller))
		{
			ToolItem* iconItem = (ToolItem*)list.at (index);

			ToolItem* toolItem = toolbar->findToolItem (this);
			if(toolItem && toolItem->isIgnoresModeIcons ())
				iconItem = toolItem;

			if(iconItem)
			{
				ITheme* theme = RootComponent::instance ().getTheme ();
				return iconItem->getToolIcon (theme);
			}
		}
	}
	else
	{
		Variant variant (getValueAt (index));
		MutableCString name;
		variant.toCString (name);
		ITheme* theme = RootComponent::instance ().getTheme ();
		return theme ? theme->getImage (name) : nullptr;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ToolModePaletteParameter::getImage () const
{
	return getIconAt (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ToolModePaletteParameter::setImage (IImage* image, tbool update)
{
	ASSERT (false)
}

//************************************************************************************************
// ToolBar
//************************************************************************************************

IParameter* ToolBar::createModeParameterForIcons (CStringPtr iconNames[], int count, int maxColumns)
{
	auto p = NEW ToolModePaletteParameter (CString::kEmpty, false, maxColumns);
	for(int i = 0; i < count; i++)
		p->appendString (String (iconNames[i]));
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ToolBar::createModeParameter (const String modeStrings[], CStringPtr iconNames[], int count)
{
	auto p = NEW ToolModeParameter (CString::kEmpty, false);
	for(int i = 0; i < count; i++)
	{
		p->appendString (modeStrings[i]);
		p->addModeIconId (iconNames[i]);
	}	
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static CCL::Configuration::BoolValue toolMenuIconsEnabled ("Editing", "toolMenuIconsEnabled", true);

DEFINE_CLASS (ToolBar, Component)
DEFINE_STRINGID_MEMBER_ (ToolBar, kSetExtraTool, "SetExtraTool")

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolBar::ToolBar ()
: Component (CCLSTR ("ToolBar")),
  toolModePresentation (kToolModeMenuPresentation)
{
	toolItems.objectCleanup (true);

	toolParameter = paramList.addList (CSTR ("tool"), Tag::kTool);
	toolImageProvider = paramList.addImage (CSTR ("toolimage"), Tag::kToolImage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolBar::~ToolBar ()
{
	ASSERT (Settings::instance ().containsSaver (this) == false)

	ForEach (toolCollections, ToolCollection, collection)
		collection->setToolBar (nullptr);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ToolBar::initialize (IUnknown* context)
{
	if(!settingsPath.isEmpty ())
		Settings::instance ().addSaver (this);

	updateToolImageProvider ();
	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ToolBar::terminate ()
{
	if(!settingsPath.isEmpty ())
		Settings::instance ().removeSaver (this);

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolItem* ToolBar::getToolItem (const EditTool& tool)
{
	return (ToolItem*)toolItems.findEqual (tool);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolItem* ToolBar::findToolItem (const IParameter* modeParam)
{
	return (ToolItem*)toolItems.findIf ([&] (Object* o) { return ((ToolItem*)o)->getModeParam () == modeParam; });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolBar::addToolCollection (ToolCollection* collection)
{
	collection->retain ();
	toolCollections.add (collection);

	// add tool item if not already known
	ForEach (*collection, EditTool, tool)
		ToolItem* item = getToolItem (*tool);
		if(!item)
		{
			toolItems.add (item = NEW ToolItem (*tool));
			UnknownPtr<IListParameter> (toolParameter)->appendString (String (tool->getName ()));
		}

		// add modes to tool item
		if(!tool->getModes ().isEmpty ())
		{
			if(!item->getModeParam ())
			{
				ListParam* modeParam = nullptr;

				if(toolModePresentation == kToolModePalettePresentation)
					modeParam = NEW ToolModePaletteParameter (tool->getName (), true);
				else
					modeParam = NEW ToolModeParameter (tool->getName (), true);

				modeParam->connect (this, Tag::kToolMode);
				item->setModeParam (modeParam);
			}
			item->addModes (*tool);

			if(tool->ignoresModeIcons ())
				item->setIgnoresModeIcons (true);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolBar::removeToolCollection (ToolCollection* collection)
{
	if(toolCollections.remove (collection))
		collection->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolBar::setActiveTool (const EditTool* tool)
{
	int index = tool ? toolItems.index (*tool) : -1;
	toolParameter->setValue (index, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTool* ToolBar::getActiveTool (ToolCollection& collection)
{
	ToolItem* item = (ToolItem*)toolItems.at (toolParameter->getValue ().asInt ());
	return item ? collection.findTool (item->getName ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ToolBar::getToolIndex (StringID name) const
{
	int i = 0;
	ForEach (toolItems, ToolItem, toolItem)
		if(toolItem->getName () == name)
			return i;
		i++;
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API ToolBar::findParameter (StringID name) const
{
	if(name.startsWith ("@toolMode["))
	{
		int index = 0;
		::sscanf (name, "@toolMode[%d]", &index);
		ToolItem* toolItem = (ToolItem*)toolItems.at (index);
		ASSERT (toolItem)
		if(toolItem)
			return toolItem->getModeParam ();
	}
	return SuperClass::findParameter (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolBar::updateToolImageProvider ()
{
	const ToolItem* item = (ToolItem*)toolItems.at (toolParameter->getValue ().asInt ());
	if(item)
	{
		if(IParameter* modeParam = item->getModeParam ())
			item = item->getMode (modeParam->getValue ().asInt ());

		toolImageProvider->setImage (item->getToolIcon (getTheme ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ToolBar::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kTool:
		ForEach (toolCollections, ToolCollection, collection)
			collection->onToolChanged ();
		EndFor
		updateToolImageProvider ();
		break;

	case Tag::kToolMode:
		if(UnknownPtr<IListParameter> modeList = param)
		{
			// switch to tool first (better on beginEdit?)
			int toolIndex = getToolIndex (param->getName ());
			toolParameter->setValue (toolIndex, true);

			if(ToolItem* modeItem = unknown_cast<ToolItem> (modeList->getSelectedValue ()))
				ForEach (toolCollections, ToolCollection, collection)
					collection->onToolModeChanged (modeItem->getName ());
				EndFor

			updateToolImageProvider ();
			UnknownPtr<ISubject> (param)->signal (Message (IParameter::kUpdateMenu));
		}
		break;

	case Tag::kToolPalette :
		if(UnknownPtr<IListParameter> paletteParameter = param)
			if(ToolPalette::Item* selectedItem = (ToolPalette::Item*)unknown_cast<ToolItem> (paletteParameter->getSelectedValue ()))
			{
				// switch tool
				toolParameter->setValue (selectedItem->getToolIndex (), true);

				// switch mode
				if(selectedItem->getModeIndex () >= 0)
					if(ToolItem* toolItem = (ToolItem*)toolItems.at (selectedItem->getToolIndex ()))
						if(IParameter* modeParam = toolItem->getModeParam ())
							modeParam->setValue (selectedItem->getModeIndex (), true);
			}
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API ToolBar::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name.startsWith ("@tool"))
	{
		int index = -1;

		if(name.startsWith ("@tool["))
		{
			::sscanf (name, "@tool[%d]", &index);
		}
		else if(name.startsWith ("@tool:"))
		{
			auto toolID = name.subString (6);
			index = getToolIndex (toolID);
		}

		ASSERT (index >= 0)

		ToolItem* toolItem = (ToolItem*)toolItems.at (index);
		ASSERT (toolItem)
		if(toolItem)
		{
			StyleFlags style (getToolButtonStyle ());
			if(index == 0)
				style.setCommonStyle (Styles::kLeft);
			else if(index == toolItems.count () - 1)
				style.setCommonStyle (Styles::kRight);

			ControlBox toolButton (ClassID::ToolButton, toolParameter, bounds, style);
			toolButton.setAttribute (kButtonIcon, toolItem->getToolIcon (getTheme ()));
			toolButton.setAttribute (kRadioButtonValue, index);
			toolButton.setHelpIdentifier (String (toolItem->getName ()));
			toolButton.setTooltip (String () << toolItem->getTitle () << " @cmd[Toolbar|Tool " << index+1 << "]");

			if(toolItem->getModeParam ())
				toolButton.setAttribute (kToolButtonModeParam, toolItem->getModeParam ()->asUnknown ());

			return toolButton;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolBar::extendModeMenu (IMenu& menu, StringID name)
{
	ForEach (toolCollections, ToolCollection, collection)
		if(EditTool* tool = collection->findTool (name))
			if(tool->extendModeMenu (menu))
				break;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolBar::appendToolModes (IMenu& menu, int toolNumber, ToolItem& toolItem)
{
	int modeCount = toolItem.getModeCount ();
	for(int i = 0; i < modeCount; i++)
		if(const ToolItem* modeItem = toolItem.getMode (i))
		{
			MutableCString modeIndex;
			modeIndex.appendFormat ("Mode %d/%d", toolNumber, i + 1);

			IMenuItem* menuItem = menu.addCommandItem (modeItem->getTitle (), CSTR ("Toolmode"), modeIndex, this);
			if(toolMenuIconsEnabled)
			{
				IImage* icon = modeItem->getToolIcon (getTheme ());
				menuItem->setItemAttribute (IMenuItem::kItemIcon, icon);
			}
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ToolBar::appendContextMenu (IContextMenu& contextMenu)
{
	bool handledByTool = false;
	if(contextMenu.getContextID ().startsWith ("ToolButton:"))
	{
		int index = 0;
		if(sscanf (contextMenu.getContextID ().str (), "ToolButton:tool:%d", &index) == 1)
			if(ToolItem* item = (ToolItem*)toolItems.at (index))
				ForEach (toolCollections, ToolCollection, collection)
					if(EditTool* tool = collection->findTool (item->getName ()))
						if(tool->onContextMenu (contextMenu))
						{
							setActiveTool (tool);
							handledByTool = true;
							break;
						}
				EndFor
	}

	if(!handledByTool && toolItems.isMultiple ())
	{
		// switch tool via context menu
		UnknownPtr<IMenu> popupMenu (&contextMenu);
		ASSERT (popupMenu != nullptr)
		if(!popupMenu)
			return kResultFalse;

		int i = 1;
		ForEach (toolItems, ToolItem, toolItem)
			MutableCString toolIndex;
			toolIndex.appendFormat ("Tool %d", i);

			IMenuItem* menuItem = popupMenu->addCommandItem (toolItem->getTitle (), CSTR ("Toolbar"), toolIndex, this);
			if(toolMenuIconsEnabled)
			{
				IImage* icon = toolItem->getToolIcon (getTheme ());
				menuItem->setItemAttribute (IMenuItem::kItemIcon, icon);
			}

			// extended menu: add tool modes as split menu
			if(popupMenu->isExtendedMenu ())
				if(toolItem->getModeCount () > 1 && !toolItem->isIgnoresModeIcons ())
				{
					AutoPtr<IMenu> subMenu = popupMenu->createMenu ();
					menuItem->setItemAttribute (IMenuItem::kSplitMenu, static_cast<IMenu*> (subMenu));
					appendToolModes (*subMenu, i, *toolItem);
				}
			i++;
		EndFor

		// native menu: append tool modes separately
		if(!popupMenu->isExtendedMenu ())
		{
			contextMenu.addSeparatorItem ();

			i = 1;
			ForEach (toolItems, ToolItem, toolItem)
				if(toolItem->getModeCount () > 1 && !toolItem->isIgnoresModeIcons ())
				{
					IMenu* subMenu = popupMenu->createMenu ();
					subMenu->setMenuAttribute (IMenu::kMenuTitle, toolItem->getTitle ());
					popupMenu->addMenu (subMenu);
					appendToolModes (*subMenu, i, *toolItem);
				}
				i++;
			EndFor
		}

		contextMenu.addSeparatorItem ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolBar* ToolBar::findToolBarAtMouse ()
{
	if(IWindow* window = System::GetDesktop ().findWindowUnderCursor ())
	{
		UnknownPtr<IView> startView (window);
		if(startView)
		{
			Point p;
			System::GetGUI ().getMousePosition (p);
			startView->screenToClient (p);
			Rect vc;
			startView->getVisibleClient (vc);
			if(vc.pointInside (p))
			{
				auto findToolBarFromView = [] (IView* view) -> ToolBar*
				{
					// starting from mouse view, look in parent chain for an editor as controller
					while(view)
					{
						if(auto editor = unknown_cast<EditorComponent> (view->getController ()))
						{
							if(ToolBar* toolbar = editor->getToolBar ())
								return toolbar;

							break;
						}
						view = view->getParentView ();
					}
					return nullptr;
				};

				IView* view = startView->getChildren ().findChildView (p, true);
				ToolBar* toolBar = nullptr;

				// also try siblings underneath, the found view might be a purely decorative element overlapping the edit view we are loooking for
				while(view && !(toolBar = findToolBarFromView (view)))
					view = ViewFinder (view).findNextView (startView, p);

				if(toolBar)
					return toolBar;
			}
		}
	}
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool ToolBar::onToolCommand (CmdArgs args)
{
	if(args.category == "Toolbar")
	{
		int oldIndex = toolParameter->getValue ().asInt ();
		int index = oldIndex;
		int count = toolItems.count ();

		if(args.name == "Next Tool")
		{
			index++;
			if(index >= count)
				index = 0;
		}
		else if(args.name == "Previous Tool")
		{
			index--;
			if(index < 0)
				index = count - 1;
		}
		else if(args.name.startsWith ("Tool ")) // tool index
		{
			int64 value = 0;
			if(args.name.subString (5).getIntValue (value))
				index = (int)value - 1; // count humanly
		}
		//else // interpret command as tool name (unused yet)
		//	index = getToolIndex (args.name);
		else
			return false;

		if(index >= 0)
		{
			bool selected = (index == oldIndex);
			if(args.checkOnly ())
			{
				UnknownPtr<IMenuItem> menuItem (args.invoker);
				if(menuItem)
					menuItem->setItemAttribute (IMenuItem::kItemChecked, selected);
			}
			else
			{
				if(index >= count)
				{
					if(Component* parentComponent = getParentNode<Component> ())
					{
						Boxed::Variant messageResult;
						Message msg (kSetExtraTool, index + 1, index - count, messageResult.asUnknown ());
						parentComponent->notify (this, msg);
						if(messageResult.asVariant ().asBool ())
							return true;
					}
				}

				if(!selected)
					toolParameter->setValue (index, true);
				else
				{
					auto* toolItem = static_cast<ToolItem*> (toolItems.at (index));
					if(toolItem)
						if(ListParam* modeParam = toolItem->getModeParam ())
						{
							int mode = modeParam->getValue ().asInt () + 1;
							if(mode > modeParam->getMax ().asInt ())
								mode = 0;
							modeParam->setValue (mode, true);
						}
				}
			}
			return true;
		}
	}
	else if(args.category == "Toolmode")
	{
		int toolIndex = -1, modeIndex = -1;
		::sscanf (args.name, "Mode %d/%d", &toolIndex, &modeIndex);
		toolIndex--;
		modeIndex--;

		ToolItem* toolItem = toolIndex >= 0 ? (ToolItem*)toolItems.at (toolIndex) : nullptr;
		ListParam* modeParam = toolItem ? toolItem->getModeParam () : nullptr;
		if(modeParam)
		{
			if(args.checkOnly ())
			{
				bool selected = modeParam->getValue ().asInt () == modeIndex;
				UnknownPtr<IMenuItem> menuItem (args.invoker);
				if(menuItem)
					menuItem->setItemAttribute (IMenuItem::kItemChecked, selected);
			}
			else
				modeParam->setValue (modeIndex, true);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ToolBar::interpretCommand (const CommandMsg& args)
{
	if(args.category == "Toolbar" || args.category == "Toolmode")
	{
		bool isMenu = UnknownPtr<IMenuItem> (args.invoker).isValid ();
		if(ToolBar* toolBar = isMenu ? this : findToolBarAtMouse ())
			return toolBar->onToolCommand (args);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolBar::setToolModePresentation (ToolBar::ToolModePresentation presentation)
{
	toolModePresentation = presentation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolBar::popup (const Point& where, IView* parent)
{
	bool withModes = true; // via modifier?

	AutoPtr<IPopupSelector> popupSelector (ccl_new<IPopupSelector> (ClassID::PopupSelector));
	ASSERT (popupSelector != nullptr)
	popupSelector->setTheme (getTheme ());

	AutoPtr<ToolPalette> palette = NEW ToolPalette (*this, withModes);
	AutoPtr<PaletteParam> paletteParameter = NEW PaletteParam (CSTR ("palette"), palette);
	ForEach (*palette, ToolPalette::Item, item)
		paletteParameter->appendObject (return_shared (item));

		bool selected = false;
		if(item->getToolIndex () == toolParameter->getValue ().asInt ())
		{
			if(item->getModeIndex () == -1)
				selected = true;
			else if(ToolItem* activeItem = (ToolItem*)toolItems.at (toolParameter->getValue ()))
				if(IParameter* modeParam = activeItem->getModeParam ())
					if(item->getModeIndex () == modeParam->getValue ().asInt ())
						selected = true;
		}

		if(selected)
			paletteParameter->setValue (paletteParameter->getMax ());
	EndFor

	paletteParameter->connect (this, Tag::kToolPalette);
	Point p (where);
	p -= Point (16, 16); // move so that mouse is on first item
	popupSelector->popup (paletteParameter, PopupSizeInfo (p, parent));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ToolBar::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "numTools")
	{
		var = toolItems.count ();
		return true;
	}
	else if(propertyId == "activeTool")
	{
		var = toolParameter->getValue ();
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ToolBar::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "activeTool")
	{
		toolParameter->setValue (var, true);
		return true;
	}
	else
		return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolBar::loadState (const Attributes& a)
{
	if(Attributes* modeAttr = a.getAttributes ("toolModes"))
		ForEach (toolItems, ToolItem, toolItem)
			if(ListParam* modeList = toolItem->getModeParam ())
			{
				MutableCString modeName (modeAttr->getString (toolItem->getName ()));
				if(!modeName.isEmpty ())
					for(int i = 0, max = modeList->getMax ().asInt (); i <= max; i++)
					{
						ToolItem* modeItem = modeList->getObject<ToolItem> (i);
						if(modeItem && modeItem->getName () == modeName)
						{
							modeList->setValue (i, true);
							break;
						}
					}
			}
		EndFor

	MutableCString toolName;
	if(a.get (toolName, "tool"))
	{
		int toolIndex = getToolIndex (toolName);
		toolParameter->setValue (toolIndex, true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolBar::saveState (Attributes& a) const
{
	if(ToolItem* activeItem = (ToolItem*)toolItems.at (toolParameter->getValue ().asInt ()))
		a.set ("tool", activeItem->getName ());

	AutoPtr<PersistentAttributes> modeAttr = NEW PersistentAttributes;
	ForEach (toolItems, ToolItem, toolItem)
		if(ListParam* modeList = toolItem->getModeParam ())
			if(ToolItem* modeItem = unknown_cast<ToolItem> (modeList->getSelectedValue ()))
				modeAttr->set (toolItem->getName (), modeItem->getName ());
	EndFor
	if(!modeAttr->isEmpty ())
		a.set ("toolModes", modeAttr, Attributes::kShare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolBar::load (const Storage& storage)
{
	if(settingsPath.isEmpty ())
		loadState (storage.getAttributes ());

	return SuperClass::load (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolBar::save (const Storage& storage) const
{
	if(settingsPath.isEmpty ())
		saveState (storage.getAttributes ());

	return SuperClass::save (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolBar::restore (Settings& settings)
{
	ASSERT (!settingsPath.isEmpty ())
	loadState (settings.getAttributes (settingsPath));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolBar::flush (Settings& settings)
{
	ASSERT (!settingsPath.isEmpty ())
	saveState (settings.getAttributes (settingsPath));
}

//************************************************************************************************
// ToolPalette
//************************************************************************************************

ToolPalette::ToolPalette (ToolBar& toolBar, bool withModes)
: toolBar (toolBar)
{
	items.objectCleanup (true);

	if(withModes == false) // display tools only
	{
		int toolIndex = 0;
		ForEach (toolBar.toolItems, ToolItem, toolItem)
			items.add (NEW Item (*toolItem, toolIndex++, -1));
		EndFor
	}
	else // display tools with modes
	{
		int toolIndex = 0;
		ForEach (toolBar.toolItems, ToolItem, toolItem)
			int modeCount = toolItem->getModeCount ();
			if(modeCount > 1 && !toolItem->isIgnoresModeIcons ())
			{
				for(int modeIndex = 0; modeIndex < modeCount; modeIndex ++)
					if(const ToolItem* modeItem = toolItem->getMode (modeIndex))
					{
						Item* item = NEW Item (*modeItem, toolIndex, modeIndex);
						item->setTitle (String () << toolItem->getTitle () << " - " << modeItem->getTitle ());
						items.add (item);
					}
			}
			else
				items.add (NEW Item (*toolItem, toolIndex, -1));

			toolIndex++;
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* ToolPalette::newIterator () const
{
	return items.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ToolPalette::getCount () const
{
	return items.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ToolPalette::getDimensions (int& columns, int& cellWidth, int& cellHeight) const
{
	columns = ccl_min (items.count (), 8);

	ForEach (toolBar.toolItems, ToolItem, toolItem)
		if(IImage* icon = toolItem->getToolIcon (toolBar.getTheme ()))
			for(int i = 0, num = icon->getFrameCount (); i < num; i++)
			{
				icon->setCurrentFrame (i);
				ccl_lower_limit (cellWidth, icon->getWidth ());
				ccl_lower_limit (cellHeight, icon->getHeight ());
			}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ToolPalette::createIcon (int index, int width, int height, const IVisualStyle& style) const
{
	IImage* icon = nullptr;
	ToolItem* toolItem = (ToolItem*)items.at (index);
	if(toolItem)
		icon = toolItem->getToolIcon (toolBar.getTheme ());
	if(icon)
		icon->retain ();
	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ToolPalette::getTitle (String& title, int index) const
{
	ToolItem* toolItem = (ToolItem*)items.at (index);
	if(toolItem)
	{
		title = toolItem->getTitle ();
		return true;
	}
	return false;
}
