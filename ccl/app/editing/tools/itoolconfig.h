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
// Filename    : ccl/app/editing/tools/itoolconfig.h
// Description : Tool Configuration Interfaces
//
//************************************************************************************************

#ifndef _ccl_itoolconfig_h
#define _ccl_itoolconfig_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

struct MouseEvent;
class EditView;
class EditHandler;
class Iterator;
interface IToolMode;
interface IPresentable;
interface IContextMenu;
interface IMenu;

//////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUG_CATEGORY_TOOLSET "Toolset"

//************************************************************************************************
// IToolAction
//************************************************************************************************

interface IToolAction: IUnknown
{
	virtual String getCursor (EditView& editView, const MouseEvent& mouseEvent) = 0;

	virtual String getTooltip () = 0;

	virtual bool wantsCrossCursor (EditView& editView, const MouseEvent& mouseEvent) = 0;
	
	virtual EditHandler* onMouseDown (EditView& editView, const MouseEvent& mouseEvent) = 0;
	
	virtual int getIgnoreModifier () = 0;

	DECLARE_IID (IToolAction)
};

//************************************************************************************************
// IToolConfiguration
//************************************************************************************************

interface IToolConfiguration: IUnknown
{
	virtual String getTitle () = 0;

	virtual String getName () = 0;

	virtual String getIcon () = 0;	

	virtual IToolAction* findAction (EditView& editView, const MouseEvent& mouseEvent) = 0;
 
	virtual int countModes () = 0;

	virtual IToolMode* createMode (int index) = 0;

	virtual bool ignoresModeIcons () = 0; ///< icons of tool modes don't replace tool button icon

	virtual void onAttached (EditView& editView, bool state) = 0;

	virtual void onMouseLeave (EditView& editView, const MouseEvent& mouseEvent) = 0;

	virtual bool onContextMenu (IContextMenu& contextMenu) = 0;

	virtual bool extendModeMenu (IMenu& menu) = 0;

	DECLARE_IID (IToolConfiguration)
};

//************************************************************************************************
// IToolHelp (extends IToolConfiguration)
//************************************************************************************************

interface IToolHelp: IUnknown
{
	virtual IPresentable* findHelp (EditView& editView, const MouseEvent& mouseEvent) = 0;

	DECLARE_IID (IToolHelp)
};

//************************************************************************************************
// IToolMode
//************************************************************************************************

interface IToolMode: IUnknown
{
	virtual String getTitle () = 0;

	virtual String getName () = 0;

	virtual String getIcon () = 0;	

	virtual IToolConfiguration* getHandler () = 0; ///< optional, a tool implementation that defines the behavior in this mode

	DECLARE_IID (IToolMode)
};

//************************************************************************************************
// IToolSet
//************************************************************************************************

interface IToolSet: IUnknown
{
	virtual int countConfigurations () = 0;

	virtual IToolConfiguration* createConfiguration (int index) = 0;

	DECLARE_IID (IToolSet)
};

//************************************************************************************************
// INativeToolSet
//************************************************************************************************

interface INativeToolSet: IUnknown
{
	virtual Iterator* getTools () = 0;

	DECLARE_IID (INativeToolSet)
};

//************************************************************************************************
// IEditHandler
//************************************************************************************************

interface IEditHandler: IUnknown
{
	virtual void onBegin () = 0;

	virtual bool onMove (int moveFlags) = 0;

	virtual void onRelease (bool canceled) = 0;

	DECLARE_IID (IEditHandler)
};

} // namespace CCL

#endif // _ccl_itoolconfig_h
