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
// Filename    : spidermonkeyutils.ts
// Description : Utilities for working with the SpiderMonkey Debug API
//
//************************************************************************************************

const DEBUG = true;

export type DebugLine = { lineNumber: number, script: Debugger.Script, debugPoints: { offset: number, columnNumber: number }[] };

export class ScriptManager
{
	private scripts: { script: Debugger.Script, path?: string }[] = [];

	//////////////////////////////////////////////////////////////////////////////////////////////////

	public addScript = (script: Debugger.Script) =>
	{
		if(script.source == null || script.source.url == null)
			return null; // ignore internal scripts

		let i = this.findScriptIndex (script.source.url);
		if(i > -1)
		{
			this.clearBreakpointsForScript (this.scripts[i].script);
			this.scripts[i] = { script: script };
		}
		else
			this.scripts.push ({ script: script });

		return this.getFileName (script.source.url);
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////

	public getFileName = (path: string) =>
	{
		let searchCharacter = "/";
		if(path.indexOf ("#") > -1)
			searchCharacter = "#";

		return path.substring (path.lastIndexOf (searchCharacter) + 1);
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////

	public findScriptIndex = (path: string) =>
	{
		return this.scripts.findIndex (s => s.script.url != null && path != null && this.getFileName (s.script.url) == this.getFileName (path));
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////

	public getScriptAt = (index: number) =>
	{
		if(this.scripts[index] == null)
			return null;

		return this.scripts[index].script;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	public findScript = (path: string) =>
	{
		let i = this.findScriptIndex (path);
		let result: { script: Debugger.Script, path?: string } | null = null;
		if(i > -1)
			result = this.scripts[i];

		if(result == null || result.script.source == null)
		{
			if(DEBUG)
			{
				println ('"' + path + '" is not loaded yet.');
				println ('Loaded scripts:');
				for(let s of this.scripts)
				{
					if(s.script.url != null)
						println (s.script.url);
				}
			}

			return null;
		}

		return result;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////

	public clearAllBreakpoints = () =>
	{
		for(let script of this.scripts)
			this.clearBreakpointsForScript (script.script);
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////

	public clearBreakpointsForScript = (script: Debugger.Script) =>
	{
		script.clearAllBreakpoints ();

		let functions = script.getChildScripts ();
		for(const func of functions)
			this.clearBreakpointsForScript (func);
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////

	public getFrameMetadata = (frame: Debugger.Frame) =>
	{
		if(frame.script == null || frame.script.url == null)
			return null;

		let path = frame.script.url;
		let result = this.findScript (path);
		if(result != null && result.path != null)
			path = result.path;

		let metadata = frame.script.getOffsetMetadata (frame.offset);
		return {
			name: this.getFileName (path) + ":" + (metadata.lineNumber + 1),
			source: {
				name: this.getFileName (path),
				path: path
			},
			line: metadata.lineNumber,
			col: metadata.columnNumber
		};
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////

	public collectDebugPointsForScript = (debugPoints: DebugLine[], script: Debugger.Script) =>
	{
		const possibleBreakpoints = script.getPossibleBreakpoints ();
		for(const possibleBreakpoint of possibleBreakpoints)
		{
			let line = debugPoints.find (e => e.lineNumber === possibleBreakpoint.lineNumber);
			if(line == null)
			{
				line = { lineNumber: possibleBreakpoint.lineNumber, script: script, debugPoints: [] };
				debugPoints.push (line);
			}

			line.debugPoints.push ({
				offset: possibleBreakpoint.offset,
				columnNumber: possibleBreakpoint.columnNumber
			});
		}

		for(const debugLine of debugPoints)
			debugLine.debugPoints.sort ((a, b) => a.columnNumber - b.columnNumber);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	public collectDebugLines = (debugLines: DebugLine[], script: Debugger.Script) =>
	{
		this.collectDebugPointsForScript (debugLines, script);

		let functions = script.getChildScripts ();
		for(const func of functions)
			this.collectDebugLines (debugLines, func);

		debugLines.sort ((a, b) => a.lineNumber - b.lineNumber);
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////

	public static isPrimitive = (value: any) =>
	{
		return (
			typeof value === 'number' ||
			typeof value === 'string' ||
			typeof value === 'boolean' ||
			typeof value === 'undefined' ||
			typeof value === 'bigint'
		);
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////

	public static isExcludedTopLevelSymbol = (symbol: string) =>
	{
		const ignoreNames = ["globalThis", "JSON", "Math", "WebAssembly", "NaN", "Infinity", "Intl", "Reflect", "NativeUserDataClass", "exports"/*, "Host"*/];
		return symbol.startsWith ("Native_0x") || ignoreNames.indexOf (symbol) > -1;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////

	public getObjectEnvironmentValues = (values: any, obj: Debugger.Object, name: string, environment: Debugger.Environment, resolvedObjects: { obj: Debugger.Object, value: any }[] = [], recursionDepth = 0) =>
	{
		if(obj == null)
		{
			values[name] = obj;
			return;
		}

		if(obj.callable)
			return;

		if(recursionDepth > 20 || (recursionDepth == 0 && ScriptManager.isExcludedTopLevelSymbol (name)))
			return;

		let findResult = resolvedObjects.find (item => item.obj === obj);
		if(findResult != null)
		{
			values[name] = findResult.value;
			return;
		}

		if(obj["getOwnPropertyNames"] != null)
		{
			let props = obj.getOwnPropertyNames ();
			if(props.length == 0 && obj.isProxy)
				values[name] = {}; // C++ objects
			else
			{
				values[name] = {};
				for(const prop of props)
				{
					let result = obj.getProperty (prop);
					if(result && "return" in result)
						this.getObjectEnvironmentValues (values[name], result.return, prop, environment, resolvedObjects, recursionDepth++);
				}
			}
		}
		else if(Array.isArray (obj))
		{
			values[name] = [];
			for(let item of obj)
			{
				let itemResult = { value: undefined };
				this.getObjectEnvironmentValues (itemResult, item, "value", environment, resolvedObjects, recursionDepth++);
				values[name].push (itemResult);
			}
		}
		else if(ScriptManager.isPrimitive (obj))
			values[name] = obj;
		else
			values[name] = {}; // fallback

		resolvedObjects.push ({ obj: obj, value: values[name] });
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////

	public getDebuggerFrameValues = (frame: Debugger.Frame) =>
	{
		let debuggerFrameValues: any = {};
		let parent = frame.environment;
		while(parent != null)
		{
			if(parent.inspectable)
			{
				let vars = parent.names ();
				for(let v of vars)
				{
					// Attempting to access internal values throws errors
					if(v.startsWith ("__"))
						continue;

					let variable = parent.getVariable (v);
					if(variable && typeof variable === 'object' && "class" in variable)
						this.getObjectEnvironmentValues (debuggerFrameValues, variable, v, parent);
					else
						debuggerFrameValues[v] = variable;
				}
			}

			parent = parent.parent;
		}

		return debuggerFrameValues;
	};
}
