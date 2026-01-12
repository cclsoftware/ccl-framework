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
// Filename    : core/public/gui/coregameinterface.h
// Description : Game Interfaces
//
//************************************************************************************************

#ifndef _coregameinterface_h
#define _coregameinterface_h

#include "core/public/coreplugin.h"

namespace Core {

struct BitmapData;

//************************************************************************************************
// Class Definitions
//************************************************************************************************

#define CLASS_TYPE_GAMECORE	"GameCore"

#define DEFINE_GAMECORE_CLASS(VarName, displayName, classID, createInstance) \
	DEFINE_CORE_CLASSINFO (VarName, 0, CLASS_TYPE_GAMECORE, displayName, classID, "", createInstance)

//************************************************************************************************
// JoypadButton
//************************************************************************************************

DEFINE_ENUM (JoypadButton)
{
	kJoypadLeft,
	kJoypadRight,
	kJoypadUp,
	kJoypadDown,
	kJoypadA,
	kJoypadB,
	kJoypadStart,
	kJoypadSelect
};

//************************************************************************************************
// PointerValue
//************************************************************************************************

DEFINE_ENUM (PointerValue)
{
	kPointerDown,
	kPointerPositionX,
	kPointerPositionY
};

//************************************************************************************************
// IGameEnvironment
//************************************************************************************************

struct IGameEnvironment: IPropertyHandler
{
	virtual int getScreenWidth () const = 0;

	virtual int getScreenHeight () const = 0;

	virtual int getScreenFormat () const = 0;
	
	virtual bool isJoypadButtonPressed (JoypadButton button) const = 0;

	virtual int getPointerValue (PointerValue which) const = 0;

	static const InterfaceID kIID = FOUR_CHAR_ID ('G', 'm', 'E', 'v');
};

//************************************************************************************************
// IGameCore
//************************************************************************************************

struct IGameCore: IPropertyHandler
{
	virtual ErrorCode startup (IGameEnvironment* environment) = 0;
	
	virtual void shutdown () = 0;
	
	enum RunResult { kFrameDirty = 1<<0 };

	/** Run game for one frame, called periodically. */
	virtual int run () = 0;

	static const InterfaceID kIID = FOUR_CHAR_ID ('G', 'm', 'C', 'o');
};

//************************************************************************************************
// IGameBitmapRenderer
//************************************************************************************************

struct IGameBitmapRenderer: IPropertyHandler
{
	/**	Render current frame to bitmap.
		Can be called multiple times in case multiple physical displays are combined. */
	virtual ErrorCode renderFrame (BitmapData& data, int offsetX, int offsetY) = 0;

	static const InterfaceID kIID = FOUR_CHAR_ID ('G', 'm', 'B', 'R');
};

//************************************************************************************************
// IGameLibrary
//************************************************************************************************

struct IGameLibrary: IPropertyHandler
{
	virtual int getGameCount () const = 0;

	virtual CStringPtr getGameTitle (int index) const = 0;
	
	virtual IGameCore* getGameCore (int index) const = 0;

	static const InterfaceID kIID = FOUR_CHAR_ID ('G', 'm', 'L', 'b');
};

} // namespace Core

#endif // _coregameinterface_h
