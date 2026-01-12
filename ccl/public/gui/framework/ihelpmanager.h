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
// Filename    : ccl/public/gui/framework/ihelpmanager.h
// Description : Help Manager Interface
//
//************************************************************************************************

#ifndef _ccl_ihelpmanager_h
#define _ccl_ihelpmanager_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IImage;
interface IWindow;
interface IPresentable;
interface IHelpInfoViewer;
interface IUnknownIterator;
interface IHelpCatalog;
interface IHelpTutorial;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** Help Info Builder [IHelpInfoBuilder, IPresentable] */
	DEFINE_CID (HelpInfoBuilder, 0x5196abae, 0xbcf5, 0x403f, 0xa1, 0x9a, 0xdb, 0xbc, 0xc8, 0xf9, 0xe6, 0xb2);

	/** Help Info Collection [IHelpInfoCollection] */
	DEFINE_CID (HelpInfoCollection, 0xee576883, 0x638d, 0x4a0b, 0x8a, 0x2e, 0x2, 0x7e, 0x77, 0x4c, 0x9a, 0xc5);
}

namespace Signals
{
	/** Signals related to help system. */
	DEFINE_STRINGID (kHelpManager, "CCL.HelpManager")

		/** (OUT) Help file not found. args[0]: IVariant, true to suppress error message. */
		DEFINE_STRINGID (kHelpFileNotFound, "HelpFileNotFound")
}

/** Class category for IHelpTutorialHandler. */
#define PLUG_CATEGORY_HELPTUTORIALHANDLER CCLSTR ("HelpTutorialHandler")

//************************************************************************************************
// IHelpManager
/** Help Manager - Access singleton instance via System::GetHelpManager().
	\ingroup gui_help */
//************************************************************************************************

interface IHelpManager: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// User Manual (Context-sensitive Help)
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Override default help location (local folder or web URL). */
	virtual tresult CCL_API setHelpLocation (UrlRef path) = 0;

	/** Add folder with additional help catalog. */
	virtual tresult CCL_API addHelpCatalog (UrlRef path, StringID category) = 0;

	/** Create iterator of IHelpCatalog objects. */
	virtual IUnknownIterator* CCL_API newCatalogIterator () const = 0;

	/** Show default location in given help catalog. */
	virtual tresult CCL_API showHelpCatalog (IHelpCatalog* catalog) = 0;

	/** Show given help location (can be a list of alternatives separated by semicolons). */
	virtual tresult CCL_API showLocation (StringRef location) = 0;

	/** Show context-sensitive help (invoker can be a window, view, menu item, etc.). */
	virtual tresult CCL_API showContextHelp (IUnknown* invoker = nullptr) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Info View
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Check if any info viewers are attached. */
	virtual tbool CCL_API hasInfoViewers () const = 0;

	/** Add help info viewer. */
	virtual tresult CCL_API addInfoViewer (IHelpInfoViewer* viewer) = 0;

	/** Remove help info viewer. */
	virtual tresult CCL_API removeInfoViewer (IHelpInfoViewer* viewer) = 0;

	/** Show info in all attached viewers. */
	virtual tresult CCL_API showInfo (IPresentable* info) = 0;
	
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Tutorials
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Add folder with tutorials. */
	virtual tresult CCL_API addTutorials (UrlRef path) = 0;

	/** Create iterator of IHelpTutorial objects. */
	virtual IUnknownIterator* CCL_API newTutorialIterator () const = 0;

	/** Show tutorial with optional delay. */
	virtual tresult CCL_API showTutorial (StringRef tutorialId, int delay = 0) = 0;

	/** Align a currently shown tutorial viewer with the specified control (e.g. below it). */
	virtual tresult CCL_API alignActiveTutorial (StringRef helpId) = 0;

	/** Center currently shown tutorial viewer. */
	virtual tresult CCL_API centerActiveTutorial () = 0;

	/** Focus currently shown tutorial viewer. */
	virtual tresult CCL_API focusActiveTutorial () = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Highlights
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Highlight a view specified by helpId. The id can be a path separated by '.'. */
	virtual tresult CCL_API highlightControl (StringRef helpId, IWindow* window = nullptr, tbool exclusive = true) = 0;

	/** Discard all hightlights */
	virtual tresult CCL_API discardHighlights () = 0;

	/** Indicate the begin / end of (multiple) modifications to control highlights (can reduce flickering). */
	virtual tresult CCL_API modifyHighlights (tbool begin) = 0;

	/** Dim all windows, use discardHighlights () to cancel. */
	virtual tresult CCL_API dimAllWindows () = 0;

	DECLARE_IID (IHelpManager)
};

DEFINE_IID (IHelpManager, 0x483542c, 0x56c, 0x448e, 0xbb, 0x39, 0xfa, 0x16, 0xd0, 0x24, 0x73, 0x34)

//************************************************************************************************
// IHelpCatalog
/** 
	\ingroup gui_help */
//************************************************************************************************

interface IHelpCatalog: IUnknown
{
	/** Get catalog title. */
	virtual StringRef CCL_API getTitle () const = 0;

	/** Get catalog category. */
	virtual StringID CCL_API getCategory () const = 0;

	DECLARE_STRINGID_MEMBER (kGlobal)

	DECLARE_IID (IHelpCatalog)
};

DEFINE_IID (IHelpCatalog, 0xdc2bde7b, 0xfa19, 0x4be2, 0x93, 0xdc, 0x93, 0x5b, 0xb3, 0x74, 0x49, 0xee)
DEFINE_STRINGID_MEMBER (IHelpCatalog, kGlobal, "global")

//************************************************************************************************
// IHelpTutorial
/** 
	\ingroup gui_help */
//************************************************************************************************

interface IHelpTutorial: IUnknown
{
	/** Get tutorial identifier. */
	virtual StringRef CCL_API getID () const = 0;

	/** Get tutorial title. */
	virtual StringRef CCL_API getTitle () const = 0;

	/** Get tutorial category. */
	virtual StringRef CCL_API getCategory () const = 0;

	DECLARE_STRINGID_MEMBER (kGlobal)
	DECLARE_STRINGID_MEMBER (kDocument)

	DECLARE_IID (IHelpTutorial)
};

DEFINE_IID (IHelpTutorial, 0xef63543f, 0x4185, 0x4be8, 0xbd, 0xa3, 0x7a, 0x47, 0x8a, 0x41, 0xf4, 0x74)
DEFINE_STRINGID_MEMBER (IHelpTutorial, kGlobal, "global") // stand-alone, may appear in application level menu
DEFINE_STRINGID_MEMBER (IHelpTutorial, kDocument, "document") // may appear in document level menu

//************************************************************************************************
// IHelpTutorialHandler
/** 
	\ingroup gui_help */
//************************************************************************************************

interface IHelpTutorialHandler: IUnknown
{
	virtual void CCL_API onShowTutorialStep (IHelpTutorial& tutorial, StringRef stepId) = 0;

	virtual void CCL_API onTutorialClosed (IHelpTutorial& tutorial) = 0;

	DECLARE_IID (IHelpTutorialHandler)
};

DEFINE_IID (IHelpTutorialHandler, 0xccebafc6, 0xbccf, 0x4ddf, 0xbf, 0x6d, 0xcd, 0xea, 0x7e, 0x7, 0x75, 0xdb)

//************************************************************************************************
// IHelpInfoViewer
/** 
	\ingroup gui_help */
//************************************************************************************************

interface IHelpInfoViewer: IUnknown
{
	/** Update help info. */
	virtual void CCL_API updateHelpInfo (IPresentable* info) = 0;

	DECLARE_IID (IHelpInfoViewer)
};

DEFINE_IID (IHelpInfoViewer, 0x6082e39b, 0x16d6, 0x4689, 0x95, 0x30, 0x56, 0x5, 0xee, 0x8d, 0x18, 0x4b)

//************************************************************************************************
// IHelpInfoBuilder
/** 
	\ingroup gui_help */
//************************************************************************************************

interface IHelpInfoBuilder: IUnknown
{
	DEFINE_ENUM (AttrID)
	{
		kIcon,
		kTitle,
		kDescription,
		kIgnoreModifiers,
	};
	
	/** Set attribute. */
	virtual void CCL_API setAttribute (AttrID id, VariantRef value) = 0;
	
	/** Add modifier option. */
	virtual void CCL_API addOption (uint32 modifiers, IImage* icon, StringRef text) = 0;
	
	/** Add modifier option. */
	virtual void CCL_API addOption (uint32 modifiers, StringID iconName, StringRef text) = 0;

	/** Hilite option with given modifiers. */
	virtual void CCL_API setActiveOption (uint32 modifiers) = 0;
	
	DECLARE_IID (IHelpInfoBuilder)
};

DEFINE_IID (IHelpInfoBuilder, 0x4652fd17, 0x9482, 0x4a8e, 0xb7, 0x46, 0xd0, 0x9, 0x51, 0xc0, 0xff, 0x1)

//************************************************************************************************
// IHelpInfoCollection
/** 
	\ingroup gui_help */
//************************************************************************************************

interface IHelpInfoCollection: IUnknown
{
	/** Get info with identifier. */
	virtual IHelpInfoBuilder* CCL_API getInfo (StringID id) const = 0;

	/** Add info with identifier (object is shared). */
	virtual tresult CCL_API addInfo (StringID id, IHelpInfoBuilder* helpInfo) = 0;

	DECLARE_IID (IHelpInfoCollection)
};

DEFINE_IID (IHelpInfoCollection, 0x92d2c3da, 0x6f70, 0x4131, 0xa1, 0x2f, 0xb, 0x85, 0xdc, 0xe1, 0xb, 0x5)

} // namespace CCL

#endif // _ccl_ihelpmanager_h
