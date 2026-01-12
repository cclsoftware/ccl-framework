//************************************************************************************************
//
// JavaScript Engine
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
// Filename    : daphandler.ts
// Description : Translation between DAP and SpiderMonkey
//
//************************************************************************************************

// This debugger implementation communicates via the Debug Adapter Protocol
import { DebugProtocol as DAP } from "@vscode/debugprotocol"
import { DebugLine, ScriptManager } from "./spidermonkeyutils";

namespace DAPConstants
{
	// Types
	export const kEvent = "event";
	export const kRequest = "request";
	export const kResponse = "response";

	// Events
	export const kStopped = "stopped";

	// Commands
	export const kSetBreakpoints = "setBreakpoints";
	export const kStackTrace = "stackTrace";
	export const kSource = "source";
	export const kScopes = "scopes";
	export const kVariables = "variables";
	export const kContinue = "continue";
	export const kNext = "next";
	export const kDisconnect = "disconnect";

	// Reasons
	export const kFailed = "failed";
	export const kBreakpoint = "breakpoint";
	export const kStep = "step";
}

const DEBUG = false;

let breakpoints: { [filename: string]: { path: string, lineNumbers: number[] } } = {};
let threadId = -1;
let currentStackFrame: Debugger.Frame | null = null;
let currentLine = -1;
let currentVariables: { [id: number]: DAP.Variable } = {};
let lastVariableId = 1;
let lastStackFrameId = 1;
const kTopLevelVariableId = 0xffffffff;

let paused = false;

let scriptManager = new ScriptManager ();

//////////////////////////////////////////////////////////////////////////////////////////////////

class BreakpointHandler
{
	constructor (public breakReason: string)
	{}

	public hit (frame: Debugger.Frame)
	{
		currentStackFrame = frame;
		lastVariableId = 1;
		currentVariables = {};
		let metaData = scriptManager.getFrameMetadata (frame);
		currentLine = metaData ? metaData.line : -1;

		sendMessage ({
			type: DAPConstants.kEvent,
			event: DAPConstants.kStopped,
			body: {
				reason: this.breakReason,
				threadId: threadId
			},
			seq: -1 // automatically filled in sendDebugMessage
		});

		paused = true;
		pause (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const setBreakpoints = (path: string, lineNumbers: number[]) =>
{
	breakpoints[scriptManager.getFileName (path)] = { lineNumbers: lineNumbers, path: path };

	let result: { [lineNumber: number]: boolean } = {};

	let findResult = scriptManager.findScript (path);
	if(findResult == null || findResult.script.source == null)
		return [];

	findResult.path = path;

	scriptManager.clearBreakpointsForScript (findResult.script);

	let debugLines: DebugLine[] = [];
	scriptManager.collectDebugLines (debugLines, findResult.script);

	for(const n of lineNumbers)
	{
		let line = debugLines.find (l => l.lineNumber + 1 == n); // n is 1-based
		result[n] = line != null;
		if(line != null)
		{
			const point = line.debugPoints[0]; // the first possible location in the line
			line.script.setBreakpoint (point.offset, new BreakpointHandler (DAPConstants.kBreakpoint));
		}
	}

	return result;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

const sendMessage = (message: DAP.Response | DAP.Event) =>
{
	sendDebugMessage (JSON.stringify (message));
};

//////////////////////////////////////////////////////////////////////////////////////////////////

const respondToMessage = (message: DAP.Request, success: boolean, body: any = {}) =>
{
	sendMessage ({
		type: DAPConstants.kResponse,
		command: message.command,
		success: success,
		request_seq: message.seq,
		body: body,
		seq: -1 // automatically filled in sendDebugMessage
	});
};

//////////////////////////////////////////////////////////////////////////////////////////////////

const stepFunction = (frame: Debugger.Frame): Debugger.ResumptionValue =>
{
	if(currentStackFrame)
		currentStackFrame.onStep = undefined;

	let mustStop = currentStackFrame == null;
	if(currentStackFrame)
	{
		let newMetaData = scriptManager.getFrameMetadata (frame);
		if(newMetaData == null)
			mustStop = true;
		else
			mustStop = newMetaData.line != currentLine;
	}

	if(mustStop)
		new BreakpointHandler (DAPConstants.kStep).hit (frame);
	else
		frame.onStep = function () { return stepFunction (this); };
};

//////////////////////////////////////////////////////////////////////////////////////////////////

declare global
{
	function onDebugMessage (data: string, threadId: number): void;
}

globalThis.onDebugMessage = (data: string, _threadId: number) =>
{
	threadId = _threadId;
	let message = JSON.parse (data) as DAP.Request;
	if(message.type == DAPConstants.kRequest)
	{
		if(message.command == DAPConstants.kSetBreakpoints)
		{
			let args = message.arguments as DAP.SetBreakpointsArguments;
			if(args.source.path == null || args.lines == null)
				return;

			let result = setBreakpoints (args.source.path, args.lines);
			let breakpoints: DAP.Breakpoint[] = [];
			for(const line in result)
			{
				breakpoints.push ({
					line: +line,
					verified: result[line],
					reason: result[line] ? undefined : DAPConstants.kFailed
				});
			}
			respondToMessage (message, true, {
				breakpoints: breakpoints
			});

			return;
		}
		else if(message.command == DAPConstants.kStackTrace)
		{
			if(currentStackFrame != null)
			{
				let args = message.arguments as DAP.StackTraceArguments;

				let stackFrames: DAP.StackFrame[] = [];
				let f: Debugger.Frame | null = currentStackFrame;
				while(f != null)
				{
					let metadata = scriptManager.getFrameMetadata (f);
					if(metadata != null)
					{
						stackFrames.push ({
							id: lastStackFrameId++,
							line: metadata.line + 1,
							column: metadata.col,
							name: metadata.name,
							source: metadata.source
						});
					}

					if(args.levels != null && stackFrames.length >= args.levels)
						break;

					f = f.older;
				}
				respondToMessage (message, true, {
					stackFrames: stackFrames,
					totalFrames: stackFrames.length
				});
			}
			else
				respondToMessage (message, false);

			return;
		}
		else if(message.command == DAPConstants.kSource)
		{
			let args = message.arguments as DAP.SourceArguments;
			let script = scriptManager.getScriptAt (args.sourceReference);
			let success = false;
			let content = "";
			if(script != null && script.source != null)
			{
				content = script.source.text;
				success = true;
			}
			respondToMessage (message, success, {
				content: content,
				mimeType: "text/javascript"
			});
			return;
		}
		else if(message.command == DAPConstants.kScopes)
		{
			respondToMessage (message, true, {
				scopes: [
					{
						expensive: false,
						name: "Locals",
						presentationHint: "locals",
						variablesReference: kTopLevelVariableId
					}
				]
			});
			return;
		}
		else if(message.command == DAPConstants.kVariables)
		{
			let variables: DAP.Variable[] = [];
			let values: any = null;
			let args = message.arguments as DAP.VariablesArguments;
			if(args.variablesReference > 0 && args.variablesReference != kTopLevelVariableId)
				values = currentVariables[args.variablesReference];
			else if(currentStackFrame != null)
				values = scriptManager.getDebuggerFrameValues (currentStackFrame);

			if(values != null)
			{
				for(const name in values)
				{
					if(values[name] === undefined)
						continue; // omit undefined values (e.g. future variable declarations)

					let primitiveValue = ScriptManager.isPrimitive (values[name]);
					let v: DAP.Variable = {
						name: name,
						value: primitiveValue ? values[name] + "" : typeof values[name],
						type: typeof values[name],
						variablesReference: primitiveValue ? 0 : lastVariableId
					};
					variables.push (v);
					if(!primitiveValue)
					{
						currentVariables[lastVariableId] = values[name];
						lastVariableId++;
					}
				}
			}
			respondToMessage (message, values != null, {
				variables: variables
			});

			return;
		}
		else if(message.command == DAPConstants.kContinue)
		{
			if(paused)
			{
				currentStackFrame = null;
				currentLine = -1;
				paused = false;
				pause (false); // resume
			}

			respondToMessage (message, true);

			return;
		}
		else if(message.command == DAPConstants.kNext)
		{
			if(currentStackFrame != null)
			{
				currentStackFrame.onStep = function () { return stepFunction (this); };
				currentStackFrame.onPop = function (reason)
				{
					if(this.older)
						this.older.onStep = function () { return stepFunction (this); };

					this.onStep = undefined;
					this.onPop = undefined;
				};
			}

			respondToMessage (message, true);

			if(paused)
			{
				paused = false;
				pause (false); // resume
			}

			return;
		}
		else if(message.command == DAPConstants.kDisconnect)
		{
			scriptManager.clearAllBreakpoints ();
			respondToMessage (message, true);

			return;
		}
	}

	if(DEBUG)
		println (JSON.stringify (message, null, "\t"));
};

//////////////////////////////////////////////////////////////////////////////////////////////////

let dbg = new Debugger;

//////////////////////////////////////////////////////////////////////////////////////////////////

dbg.onNewGlobalObject = (newGlobal) =>
{
	dbg.addDebuggee (newGlobal);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

dbg.onNewScript = (script) =>
{
	let fileName = scriptManager.addScript (script);
	if(fileName == null)
		return; // ignore internal scripts

	let breakpoint = breakpoints[fileName];
	if(breakpoint != null)
		setBreakpoints (breakpoint.path, breakpoint.lineNumbers);
};
