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
// Filename    : cclsystem.js
// Description : CCL System Javascript helpers
//
//************************************************************************************************

// Extend CCL namespace (or create if it doesn't exist yet)
if(typeof CCL === 'undefined')
	var CCL = {};

// Initialization registration system
if(typeof CCL.initSequence === 'undefined')
{
	CCL.initSequence = {
		sync: [],   // Synchronous functions, run sequentially
		async: []   // Async functions, run in parallel
	};
}

CCL.registerSyncInit = function (callback)
{
	this.initSequence.sync.push (callback);
};

CCL.registerAsyncInit = function (callback)
{
	this.initSequence.async.push (callback);
};

// Keyboard layout state
CCL.layoutMapCache = null;

CCL.initKeyboardLayout = async function ()
{
	if(navigator.keyboard)
		this.layoutMapCache = await navigator.keyboard.getLayoutMap ();
};

/**
* Find character on key in current keyboard layout
* @param {string} keyCode  - key code
* @param {bool} withCapsLock  - caps lock on or off
*/
CCL.findCharacterInLayoutMap = function (keyCode, withCapsLock)
{
	if(!this.layoutMapCache)
		return null;
	if(withCapsLock)
		return null; // Layout map does not contain these values

	for(const [code, value] of this.layoutMapCache.entries ())
	{
		if(code === keyCode)
			return value;
	}
	return null;
};

/**
* Initialize virtual filesystem with IndexedDB persistence
*/
CCL.initFS = async function ()
{
	const userDatadir = '/userdata';
	const documentsDir = userDatadir + '/Documents';
	const downloadsDir = userDatadir + '/Downloads';
	const settingsDir = userDatadir + '/Settings';

	Module.FS.mkdir (userDatadir);
	Module.FS.mount (IDBFS, { autoPersist: true }, userDatadir);

	return new Promise ((resolve, reject) =>
	{
		// Sync from IndexedDB to restore existing data
		FS.syncfs (true, (err) =>
		{
			if(err)
			{
				reject (err);
				return;
			}

			// Ensure directories exist after sync
			try
			{
				CCL.makeDirectory (documentsDir);
				CCL.makeDirectory (downloadsDir);
				CCL.makeDirectory (settingsDir);
				resolve (true);
			}
			catch(e)
			{
				reject (e);
			}
		});
	});
};

/**
* Make JS library functions exported for plugins resolvable by the dynamic linker.
* With MAIN_MODULE=2, wasm-metadce (run at -O3) removes every wasmImports entry 
* the main module doesn't use itself.
*/
CCL.restorePluginImports = function ()
{
	for(const key in Module)
	{
		if(!key.startsWith ('_') || typeof Module[key] !== 'function')
			continue;

		const name = key.slice (1);
		if(!(name in wasmImports) && !(name in wasmExports))
			wasmImports[name] = Module[key];
	}
};

/**
* Preload plugin modules ahead of the real dlopen() done later during plugin scanning, via a
* real async dlopen() (emscripten_dlopen_promise()) for each.
*
* This must be a genuine async dlopen(), not just an async compile+cache: Some browsers refuse
* synchronous WebAssembly instantiation for modules over 8MB on the main thread, so a plugin
* that isn't already fully (asynchronously) loaded by the time plugin scanning does its
* synchronous dlopen() would fail outright. A real dlopen() here also ensures the module ends
* up correctly tracked by Emscripten's own per-dso bookkeeping (dso->mem_allocated) - which
* only gets set via the real dlopen()/dlsync C code path - so that later pthread workers'
* dlsync catch-up correctly skips rerunning this module's global constructors instead of
* rerunning them on top of already-live shared state.
*
* Preloads MUST run one at a time, not concurrently: emscripten_dlopen()'s C implementation
* (dynlink.c) takes a process-wide, non-recursive write_lock synchronously and only releases it
* from that same dlopen's own async completion callback.
*/
CCL.preloadPlugins = async function ()
{
	const pluginsDir = '/Plugins';

	let files = [];
	try
	{
		files = Module.FS.readdir (pluginsDir).filter (filename => filename.endsWith ('.so'));
	}
	catch(dirErr)
	{
		// Plugins directory doesn't exist or can't be read - not an error
		return;
	}

	CCL.restorePluginImports ();

	for(let i = 0; i < files.length; i++)
	{
		const filename = files[i];

		CCL.startup?.setPhase ('plugins', i / files.length, `${i + 1} of ${files.length}`);

		// RTLD_NOW = 2 (see ccl/platform/shared/posix/system/*, matches the flags used by
		// WasmExecutableLoader::loadImage()'s later real ::dlopen() call for this same path).
		let promiseId = 0;
		try
		{
			promiseId = Module.ccall ('emscripten_dlopen_promise', 'number', ['string', 'number'], [pluginsDir + '/' + filename, 2]);
			await getPromise (promiseId);
		}
		catch(err)
		{
			// Preload failed - the real dlopen() during plugin scanning will retry and report it
			console.warn (`[CCL] Plug-in preload failed: ${filename}`, err);
		}
		finally
		{
			if(promiseId)
				Module.ccall ('emscripten_promise_destroy', null, ['number'], [promiseId]);
		}
	}

	CCL.startup?.setPhase ('plugins', 1);
};

/**
* Create a directory if it doesn't exist
* @param {string} dir  - directory
*/
CCL.makeDirectory = function (dir)
{
	try
	{
		FS.mkdir (dir);
	}
	catch(e)
	{
		// Already exists - not an error
	}
};

/**
* Get (or create) the non-extractable AES-GCM key used to encrypt locally-stored credentials.
* The raw key material never leaves the browser's crypto backend - only the opaque CryptoKey
* handle is exchanged with IndexedDB, so it round-trips across page loads without ever being
* readable from script.
*/
CCL.credentialsKeyPromise = null;

CCL.getCredentialsKey = function ()
{
	if(!this.credentialsKeyPromise)
	{
		this.credentialsKeyPromise = new Promise ((resolve, reject) =>
		{
			const openRequest = indexedDB.open ('CCLCredentials', 1);

			openRequest.onupgradeneeded = () =>
			{
				openRequest.result.createObjectStore ('keys');
			};

			openRequest.onerror = () => reject (openRequest.error);

			openRequest.onsuccess = () =>
			{
				const db = openRequest.result;
				const getRequest = db.transaction ('keys', 'readonly').objectStore ('keys').get ('credentialsKey');

				getRequest.onerror = () => reject (getRequest.error);
				getRequest.onsuccess = async () =>
				{
					if(getRequest.result)
					{
						resolve (getRequest.result);
						return;
					}

					try
					{
						const key = await crypto.subtle.generateKey ({ name: 'AES-GCM', length: 256 }, false, ['encrypt', 'decrypt']);

						const putTransaction = db.transaction ('keys', 'readwrite');
						putTransaction.objectStore ('keys').put (key, 'credentialsKey');
						putTransaction.oncomplete = () => resolve (key);
						putTransaction.onerror = () => reject (putTransaction.error);
					}
					catch(e)
					{
						reject (e);
					}
				};
			};
		});
	}
	return this.credentialsKeyPromise;
};

// Register system initializations
CCL.registerAsyncInit (CCL.initKeyboardLayout);
CCL.registerAsyncInit (CCL.initFS);
CCL.registerAsyncInit (CCL.preloadPlugins);
CCL.registerAsyncInit (CCL.getCredentialsKey);
