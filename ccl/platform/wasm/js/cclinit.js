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
// Filename    : cclinit.js
// Description : CCL WebAssembly Initialization
//
//************************************************************************************************

if(typeof CCL === 'undefined')
	var CCL = {};

Module.noInitialRun = true;
Module.onRuntimeInitialized = async () =>
{
	try
	{
		// Run synchronous initializations sequentially
		for(const callback of CCL.initSequence.sync)
			callback.call (CCL);

		// Run async initializations in parallel
		await Promise.all (CCL.initSequence.async.map (callback => callback.call (CCL)));

		// Start the application
		CCL.startup?.setPhase ('start');
		Module.callMain ();
	}
	catch(error)
	{
		CCL.startup?.fail ('The application could not be started.', error);
		throw error;
	}
};
