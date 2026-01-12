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
// Filename    : ccl/app/editing/tools/edittool.h
// Description : Editing Tool
//
//************************************************************************************************

#ifndef _ccl_edittool_h
#define _ccl_edittool_h

#include "ccl/app/editing/tools/itoolconfig.h"
#include "ccl/app/controls/usercontrol.h"

#include "ccl/public/gui/graphics/iimage.h"

#include "ccl/base/collections/objectlist.h"

namespace CCL {

class EditToolMode;
class EditView;
class EditHandler;
interface IMouseCursor;
interface IPresentable;
interface IMenu;

//************************************************************************************************
// EditTool
/** Editing Tool base class. */
//************************************************************************************************

class EditTool: public Object
{
public:
	DECLARE_CLASS (EditTool, Object)

	EditTool (StringID name = nullptr, StringRef title = nullptr);
	~EditTool ();

	PROPERTY_STRING (title, Title)						///< tool title displayed to user
	PROPERTY_MUTABLE_CSTRING (name, Name)				///< internal tool name
	PROPERTY_MUTABLE_CSTRING (cursorName, CursorName)	///< mouse cursor name
	PROPERTY_MUTABLE_CSTRING (iconName, IconName)		///< icon name
	PROPERTY_SHARED_AUTO (IImage, icon, Icon)			///< icon (icon name is ignored when icon is set)
	
	enum Flags { kCrossCursor = 1<<0, kIgnoresModeIcons = 1<<1 };
	PROPERTY_VARIABLE (int, flags, Flags)
	PROPERTY_FLAG (flags, kCrossCursor, wantsCrossCursor)
	PROPERTY_FLAG (flags, kIgnoresModeIcons, ignoresModeIcons)

	PROPERTY_VARIABLE (int, ignoreModifier, IgnoreModifier)	///< modfier keys processed by tool (should be ignored elsewhere)
	
	// Modes
	void addMode (EditToolMode* mode);
	const ObjectList& getModes () const;
	bool setActiveMode (StringID name);
	StringID getActiveMode () const;
	EditTool* getActiveModeHandler (); ///< returns a handler of the active mode, or this

	IMouseCursor* getMouseCursor () const;
	void setMouseCursor (IMouseCursor* cursor);

	virtual void onAttached (EditView& editView, bool state);
	virtual bool onContextMenu (IContextMenu& contextMenu);
	virtual bool extendModeMenu (IMenu& menu);

	virtual void mouseEnter (EditView& editView, const MouseEvent& mouseEvent);
	virtual void mouseMove (EditView& editView, const MouseEvent& mouseEvent);
	virtual void mouseLeave (EditView& editView, const MouseEvent& mouseEvent);
	virtual EditHandler* mouseDown (EditView& editView, const MouseEvent& mouseEvent);
	
	virtual ITouchHandler* createTouchHandler (EditView& editView, const TouchEvent& event);
	
	virtual String getTooltip ();
	virtual IPresentable* createHelpInfo (EditView& editView, const MouseEvent& mouseEvent);

protected:
	IMouseCursor* mouseCursor;
	ObjectList modes;
	EditToolMode* activeMode;

	virtual void setActiveMode (EditToolMode* mode);
};

//************************************************************************************************
// EditToolMode
/** Editing Tool Mode. */
//************************************************************************************************

class EditToolMode: public Object
{
public:
	DECLARE_CLASS (EditToolMode, Object)

	PROPERTY_STRING (title, Title)						///< title displayed to user
	PROPERTY_MUTABLE_CSTRING (name, Name)				///< internal mode name
	PROPERTY_MUTABLE_CSTRING (iconName, IconName)	    ///< icon name
	PROPERTY_SHARED_AUTO (IImage, icon, Icon)           ///< icon (icon name is ignored when icon is set)
	PROPERTY_AUTO_POINTER (EditTool, handler, Handler)	///< optional, a tool implementation that defines the behavior in this mode
};

//************************************************************************************************
// NativeToolSet
//************************************************************************************************

class NativeToolSet: public CCL::Object,
					 public CCL::IToolSet,
					 public CCL::INativeToolSet
{
public:
	NativeToolSet ();

	PROPERTY_OBJECT (ObjectList, tools, Tools)

	// IToolSet
	int countConfigurations () override { return 0; }
	CCL::IToolConfiguration* createConfiguration (int index) override { return nullptr; }

	// INativeToolSet
	Iterator* getTools () override;

	CLASS_INTERFACE2 (IToolSet, INativeToolSet, Object)
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline const ObjectList& EditTool::getModes () const
{ return modes; }

inline StringID EditTool::getActiveMode () const
{ return activeMode ? activeMode->getName () : CString::kEmpty; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_edittool_h
