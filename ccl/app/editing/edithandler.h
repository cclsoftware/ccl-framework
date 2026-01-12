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
// Filename    : ccl/app/editing/edithandler.h
// Description : Editing Handler
//
//************************************************************************************************

#ifndef _ccl_edithandler_h
#define _ccl_edithandler_h

#include "ccl/app/controls/usercontrol.h"

namespace CCL {

class EditView;
interface ISprite;

//************************************************************************************************
// IEditHandlerHook
/** Hook interface for EditHandler. */
//************************************************************************************************

interface IEditHandlerHook: IUnknown
{
	virtual String getActionCode (EditView& editView, const MouseEvent& mouseEvent) = 0;
	
	virtual String getCursor (EditView& editView, const MouseEvent& mouseEvent) = 0;
	
	virtual bool updateCrossCursor (bool& wantsCrossCursor, EditView& editView, const MouseEvent& mouseEvent) = 0;
	
	virtual bool getHelp (IHelpInfoBuilder& helpInfo) = 0;
	
	virtual void performActions (EditView& editView) = 0;

	virtual void onRelease (EditView& editView, bool canceled) = 0;

	DECLARE_IID (IEditHandlerHook)
};

//************************************************************************************************
// EditHandler
/** Mouse handler base class for graphical editing operations. */
//************************************************************************************************

class EditHandler: public UserControl::MouseHandler
{
public:
	DECLARE_CLASS (EditHandler, MouseHandler)
	DECLARE_METHOD_NAMES (EditHandler)

	EditHandler (EditView* view = nullptr);
	~EditHandler ();

	PROPERTY_SHARED_AUTO (IEditHandlerHook, hook, Hook)
	PROPERTY_VARIABLE (int, ignoreModifier, IgnoreModifier)
	PROPERTY_BOOL (wantsCrossCursor, WantsCrossCursor)

	virtual void updateCursor ();
	void setEditTooltip (StringRef tooltip);
	void hideEditTooltip ();

	EditView* getEditView () const;
	void setHookFromArgument (MessageRef msg, int argumentIndex);

	// MouseHandler
	void onRelease (bool canceled) override; ///< needs to be called from derived classes
	bool getHelp (IHelpInfoBuilder& helpInfo) override;

protected:
	bool tooltipUsed;

	String getCurrentActionCode (); ///< get action code from hook inside onMove

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// NullEditHandler
/** Empty editing handler to swallow mouse click. */
//************************************************************************************************

class NullEditHandler: public EditHandler
{
public:
	DECLARE_CLASS (NullEditHandler, EditHandler)

	NullEditHandler (EditView* view = nullptr);
	~NullEditHandler ();

private:
	SharedPtr<IView> viewHolder;
};

//************************************************************************************************
// DrawSelectionHandler
//************************************************************************************************

class DrawSelectionHandler: public EditHandler
{
public:
	DrawSelectionHandler (EditView* view = nullptr);

	// EditHandler
	void onBegin () override;
	bool onMove (int moveFlags) override;
	void onRelease (bool canceled) override;

protected:
	ISprite* sprite;
};

//************************************************************************************************
// DeleteEditHandler
//************************************************************************************************

class DeleteEditHandler: public EditHandler
{
public:
	DeleteEditHandler (EditView* view = nullptr);

	// EditHandler
	void onBegin () override;
	bool onMove (int moveFlags) override;
};

//************************************************************************************************
// AbstractEditHandlerHook
//************************************************************************************************

class AbstractEditHandlerHook: public Object, 
							   public IEditHandlerHook
{
public:
	// IEditHandlerHook
	String getActionCode (EditView& editView, const MouseEvent& mouseEvent) override { return String::kEmpty; }
	String getCursor (EditView& editView, const MouseEvent& mouseEvent) override { return String::kEmpty; }
	bool updateCrossCursor (bool& wantsCrossCursor, EditView& editView, const MouseEvent& mouseEvent) override { return false; }
	bool getHelp (IHelpInfoBuilder& helpInfo) override { return false; }
	void performActions (EditView& editView) override {}
	void onRelease (EditView& editView, bool canceled) override {}

	CLASS_INTERFACE (IEditHandlerHook, Object)
};

} // namespace CCL

#endif // _ccl_edithandler_h
