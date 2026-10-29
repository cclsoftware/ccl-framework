//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : cclgui.js
// Description : CCL GUI Javascript helpers
//
//************************************************************************************************

// Extend CCL namespace (or create if it doesn't exist yet)
if(typeof CCL === 'undefined')
	var CCL = {};

// GUI state
CCL.resizeObserver = null;
CCL.dpiMediaQuery = null;
CCL.currentDpi = null;
CCL.currentDataTransfer = null;
CCL.onJSEvent = null;
CCL.Constants = {};

CCL.initCallbacks = function ()
{
	this.onJSEvent = (eventId, targetId, arg1, arg2, arg3) =>
	{
		if(typeof targetId === 'undefined')
			targetId = "*";
		if(typeof arg1 === 'undefined')
			arg1 = null;
		if(typeof arg2 === 'undefined')
			arg2 = null;
		if(typeof arg3 === 'undefined')
			arg3 = null;
		return Module.CCLHandleJSEvent (eventId, targetId, arg1, arg2, arg3);
	};
};

CCL.initStyles = function ()
{
	try
	{
		const fileContent = Module.FS.readFile ('styles/ccl.css', { encoding: 'utf8' });
		const style = document.createElement ('style');
		style.textContent = fileContent;
		document.head.appendChild (style);
	}
	catch(e)
	{
		// CSS file not found - not an error
	}
};

CCL.initConstants = function ()
{
	try
	{
		const fileContent = Module.FS.readFile ('scripts/gui-constants.js', { encoding: 'utf8' });
		const blob = new Blob ([fileContent], { type: 'application/javascript' });
		const blobUrl = URL.createObjectURL (blob);
		this.importConstantsModule (blobUrl);
	}
	catch(e)
	{
		// file not found?
	}
};

/**
* Import an ES6 module and expose its exports to the CCL.Constants namespace
* @param {string} moduleUrl - URL to the module (e.g., blob URL)
*/
CCL.importConstantsModule = function (moduleUrl)
{
	const importScript = Object.assign (document.createElement ("script"),
	{
		type: "module",
		textContent: `
			import * as module from '${moduleUrl}';
			Object.assign (CCL.Constants, module);
		`
	});
	document.head.appendChild (importScript);
};

CCL.initResizeObserver = function ()
{
	this.resizeObserver = new ResizeObserver ((entries) =>
	{
		for(const entry of entries)
		{
			if(entry.target && entry.target.id)
				CCL.onJSEvent ('element-resize', entry.target.id);
		}
	});
};

CCL.initDpiObserver = function ()
{
	this.currentDpi = window.devicePixelRatio;

	function onDpiChange ()
	{
		const newDpi = window.devicePixelRatio;
		if(newDpi !== CCL.currentDpi)
		{
			CCL.currentDpi = newDpi;
			CCL.onJSEvent ('dpi-changed');
			setupDpiObserver ();
		}
	}

	function setupDpiObserver ()
	{
		if(CCL.dpiMediaQuery)
			CCL.dpiMediaQuery.removeEventListener ('change', onDpiChange);
		CCL.dpiMediaQuery = window.matchMedia (`(resolution: ${window.devicePixelRatio}dppx)`);
		CCL.dpiMediaQuery.addEventListener ('change', onDpiChange);
	}

	setupDpiObserver ();
};

CCL.initClipboard = function ()
{
	// Firefox shows permission popups - skip for now
	if(!navigator.userAgent.includes ("Firefox"))
	{
		window.addEventListener ("click", async function ()
		{
			try
			{
				const text = await navigator.clipboard.readText ();
				CCL.onJSEvent ('changed', 'clipboard', text);
			}
			catch(err)
			{
				// Permission denied or not available
			}
		});
	}
};

CCL.getFrameElement = function (id)
{
	return document.getElementById ('frame' + id);
};

CCL.getContentElement = function (id)
{
	return document.getElementById ('content' + id);
};

CCL.createImageCanvas = function (imageData, width, height, elementId, scaleFactor)
{
	if(typeof elementId === 'undefined')
		elementId = null;
	if(typeof scaleFactor === 'undefined')
		scaleFactor = 1.0;

	const canvas = document.createElement ('canvas');
	if(elementId)
		canvas.id = elementId;

	canvas.width = width;
	canvas.height = height;

	// Set CSS size for HiDPI displays
	if(scaleFactor > 1.0)
	{
		canvas.style.width = (width / scaleFactor) + 'px';
		canvas.style.height = (height / scaleFactor) + 'px';
	}

	const ctx = canvas.getContext ('2d');
	const imgData = ctx.createImageData (width, height);

	for(let i = 0; i < width * height * 4; i++)
		imgData.data[i] = HEAPU8[imageData + i];

	ctx.putImageData (imgData, 0, 0);
	return canvas;
};

CCL.downloadBlob = function (blob, filename)
{
	const url = URL.createObjectURL (blob);
	const a = document.createElement ('a');
	a.href = url;
	a.download = filename;
	a.style.display = 'none';
	document.body.appendChild (a);
	a.click ();

	setTimeout (() =>
	{
		document.body.removeChild (a);
		URL.revokeObjectURL (url);
	}, 100);
};

CCL.openBlobInNewTab = function (blob)
{
	const url = URL.createObjectURL (blob);
	const newWindow = window.open (url, '_blank');

	if(newWindow)
	{
		newWindow.addEventListener ('load', () =>
		{
			URL.revokeObjectURL (url);
		});
		return true;
	}
	else
	{
		URL.revokeObjectURL (url);
		return false;
	}
};

/**
* Load a font file from a URL
*/
CCL.loadFontToFS = function (url, filename)
{
	return new Promise ((resolve, reject) =>
	{
		const dir = '/fonts';
		CCL.makeDirectory (dir);

		FS.createPreloadedFile (dir, filename, url, true, false,
			() =>
			{
				console.log (`[FS] Loaded: ${filename}`);
				resolve ();
			},
			(err) =>
			{
				console.error (`[FS] Failed: ${filename}`, err);
				reject (err);
			},
			false  // Don't block startup
		);
	});
};

/**
* Load a fallback font file
*/
CCL.loadFallbackFont = function (name)
{
	const filename = name + '.ttf';
	const url = './fonts/' + filename;

	CCL.loadFontToFS (url, filename).then (() =>
	{
		CCL.onJSEvent ('fallback-loaded', 'fonts', name);
	})
	.catch ((err) =>
	{
		console.error ('[FS] Could not load fallback font', err);
	});
};

/**
* Read a browser File and write to virtual filesystem
*/
CCL.readFileToFS = function (file, path)
{
	return new Promise ((resolve, reject) =>
	{
		const reader = new FileReader ();
		reader.onload = (e) =>
		{
			try
			{
				const arrayBuffer = e.target.result;
				const uint8Array = new Uint8Array (arrayBuffer);
				FS.writeFile (path, uint8Array);
				resolve (path);
			}
			catch(err)
			{
				reject (err);
			}
		};
		reader.onerror = () => reject (reader.error);
		reader.readAsArrayBuffer (file);
	});
};

// Register GUI initializations
// Note: initCallbacks must be first (sets up onJSEvent used by others)
CCL.registerSyncInit (CCL.initCallbacks);
CCL.registerSyncInit (CCL.initStyles);
CCL.registerSyncInit (CCL.initConstants);
CCL.registerSyncInit (CCL.initResizeObserver);
CCL.registerSyncInit (CCL.initDpiObserver);
CCL.registerSyncInit (CCL.initClipboard);
