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
// Filename    : cclapp.js
// Description : Utilities for JavaScript Extensions
//
//************************************************************************************************

/** Function to serve as CCL namespace. */
function CCL () 
{}

/** Function to serve as CCL.JS namespace. */
CCL.JS = function ()
{}

/**
 * Shorcut to create instance via Plug-in Manager.
 * @param {string} className  name of class to instantiate
 */
function ccl_new (className)
{
	return Host.Classes.createInstance (className);
}

/** Get platform line ending. */
CCL.JS.EndLine = function ()
{
	switch(Host.getPlatform ())
	{
	case "win" : return "\r\n";
	case "mac" : return "\r";
	}
	return "\n";
}

/** Get host application. */
CCL.JS.getApplication = function ()
{
	return Host.hostapp.find ("Application");
}

/**
 * Lookup window manager object in host.
 * @returns {object} window manager object, may be null
 */
CCL.JS.getWindowManager = function ()
{
	return Host.Objects.getObjectByUrl ("://WindowManager");
}

/** Convert Variant value to bool, supporting {false, 0, "0"} and {true, 1, "1"}. */
CCL.JS.variantToBool = function (value)
{
	return value != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Package Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Package identifier. */
var kPackageID = "";

/** Script initialization function. */
function __init (args)
{
	kPackageID = args.packageInfo.getAttribute ("Package:ID");
}

/** Helper function to construct a resource URL. */
CCL.JS.ResourceUrl = function (fileName, isFolder)
{
	return Host.Url ("package://" + kPackageID + "/resources/" + fileName, isFolder);
}

/** Helper function to make a legal file name. */
CCL.JS.LegalFileName = function (fileName)
{
	let regExp = new RegExp ('[,/\:*?""<>|]', 'g');
	return fileName.replace (regExp, "_");
}

/**
 * Translate key string.
 * JSTRANSLATE is parsed by String Extractor.
 *
 * @param {string} key  translation table id
 */
function JSTRANSLATE (key)
{
	var theStrings = Host.Locales.getStrings (kPackageID);
	if(theStrings == null) // no translation table loaded
		return key;
	return theStrings.getString (key);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL.JS.Columns = 
{
	kIconID: "icon", 			// ListViewModel::kIconID
	kTitleID: "title", 			// ListViewModel::kTitleID
	kCheckBoxID: "check", 		// ListViewModel::kCheckBoxID
	
	// IColumnHeaderList::ColumnFlags
	kSizable: 1<<0,
	kMoveable: 1<<1,
	kSortable: 1<<5,
	kCanFit: 1<<6
};

CCL.JS.kChanged = "changed"; // generic message kChanged
CCL.JS.kExtendMenu = "extendMenu"; // IParameter::kExtendMenu
CCL.JS.kRequestFocus = "requestFocus"; // IParameter::kRequestFocus
CCL.JS.kSelectionChanged = "selectionChanged"; // IItemView::kSelectionChanged
CCL.JS.kItemOpened = "itemOpened"; // ListViewModel::kItemOpened
CCL.JS.kItemFocused = "itemFocused"; // ListViewModel::kItemFocused
CCL.JS.kEditItemCell = "editItemCell"; // ListViewModel::kEditItemCell
CCL.JS.kCommandSelected = "commandSelected"; // CommandSelector::kCommandSelected
CCL.JS.kOpenFile = "openFile"; // IFileHandler::kOpenFile
CCL.JS.kWindowOpened = "WindowOpened"; // IWindowManager::kWindowOpened
CCL.JS.kWindowClosed = "WindowClosed"; // IWindowManager::kWindowClosed

CCL.JS.DocumentEvent =
{
	kCreated: 0,
	kBeforeLoad: 1,		
	kLoadFinished: 2,	
	kLoadFailed: 3,	
	kBeforeSave: 4,	
	kSaveFinished: 5,	
	kClose: 6,	
	kActivate: 7,          
	kDeactivate: 8,       
	kViewActivated: 9,		
	kDestroyed: 10,
	kBeforeAutoSave: 11,
	kAutoSaveFinished: 12	
};

//************************************************************************************************
// Component
/** Basic GUI component definition. */
//************************************************************************************************

CCL.JS.Component = function ()
{
	this.interfaces = [Host.Interfaces.IObjectNode, 
					   Host.Interfaces.IComponent, 
					   Host.Interfaces.IController, 
					   Host.Interfaces.IParamObserver,
					   Host.Interfaces.IObserver,
					   Host.Interfaces.ICommandHandler];
}

CCL.JS.Component.prototype.getTheme = function ()
{
	return Host.GUI.Themes.getTheme (kPackageID);
}

// IComponent
CCL.JS.Component.prototype.initialize = function (context)
{
	// create parameter list
	this.paramList = ccl_new ("CCL:ParamList");
	this.paramList.controller = this;
	
	// remember context
	this.context = context;
	
    // create child array (IObjectNode)
	this.children = [];
	
	return Host.Results.kResultOk;
}

CCL.JS.Component.prototype.terminate = function ()
{
	// cleanup
	this.paramList.controller = 0; // list holds a reference to our stub object!
	this.paramList = 0;
	this.context = 0;
	
	this.children.length = 0; // remove all children (Note: native object are still kept by garbage collector!)
	
	return Host.Results.kResultOk;
}

// IParamObserver
CCL.JS.Component.prototype.paramChanged = function (param)
{
}

// IObserver
CCL.JS.Component.prototype.notify = function (subject, msg)
{
	if(msg.id == CCL.JS.kExtendMenu) // sent by menu parameters
	{
		var menu = msg.getArg (0);
		this.onExtendMenu (subject, menu);
	}
}

CCL.JS.Component.prototype.onExtendMenu = function (param, menu)
{
}

// ICommandHandler
CCL.JS.Component.prototype.checkCommandCategory = function (category)
{
	return false;
}

CCL.JS.Component.prototype.interpretCommand = function (msg)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Help system utility functions.
//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Highlight a control in the host UI.
 *
 * @param {string} helpId  UI element help identifier
 * @param {boolean} exclusive  highlight exclusively, default true
 */
CCL.JS.highlightControl = function (helpId, exclusive = true)
{
	Host.GUI.Help.highlightControl (helpId, exclusive);
}

/**
 * Discard any active control highlights.
 */
CCL.JS.discardHighlights = function ()
{
	Host.GUI.Help.discardHighlights ();
}

/**
 * Indicate the begin / end of (multiple) modifications to control highlights.
 */
CCL.JS.modifyHighlights = function (begin = true)
{
	Host.GUI.Help.modifyHighlights (begin);
}

/**
 * Dim all windows, use discardHighlights () to cancel.
 */
CCL.JS.dimAllWindows = function ()
{
	Host.GUI.Help.dimAllWindows ();
}

/**
 * Open tutorial.
 *
 * @param {string} tutorialId  tutorial identifier
 * @param {number} delay  open delay in ms
 */
CCL.JS.showTutorial = function (tutorialId, delay)
{
	Host.GUI.Help.showTutorial (tutorialId, delay);
}

/**
 * Align tutorial viewer with a specified control.
 *
 * @param {string} helpId  UI element help identifier
 */
CCL.JS.alignActiveTutorial = function (helpId)
{
	Host.GUI.Help.alignActiveTutorial (helpId);
}

/**
 * Center tutorial viewer (reset position).
 */
CCL.JS.centerActiveTutorial = function ()
{
	Host.GUI.Help.centerActiveTutorial ();
}

/**
 * Focus tutorial viewer (activate window).
 */
CCL.JS.focusActiveTutorial = function ()
{
	Host.GUI.Help.focusActiveTutorial ();
}
