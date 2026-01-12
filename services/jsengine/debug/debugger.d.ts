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
// Filename    : debugger.d.ts
// Description : Typings for Mozilla Debugger
//
//************************************************************************************************

declare namespace Debugger
{
//************************************************************************************************
// Debugger::Environment
//************************************************************************************************

class Environment
{
	readonly inspectable: boolean;
	readonly type: "declarative" | "object" | "with";
	readonly scopeKind: "function" | "function lexical" | null;
	readonly parent: Environment | null;
	readonly object: Object;
	readonly calleeScript: Script | null;
	readonly optimizedOut: boolean;

	names: () => string[];
	getVariable: (name: string) => Object | string | number | boolean | { missingArgument: true } | { uninitialized: true } | undefined;
	setVariable: (name: string, value: Object) => void;
	find: (name: string) => Environment | null;
}

//************************************************************************************************
// Debugger::SavedFrame
//************************************************************************************************

class SavedFrame
{
	readonly source: string;
	readonly sourceId: number;
	readonly line: number;
	readonly column: number;
	readonly functionDisplayName: string | null;
	readonly asyncCause: "Promise" | "Async" | null;
	readonly asyncParent: string | null;
	readonly parent: Object;

	toString: () => string;
}

//************************************************************************************************
// Debugger::Object
//************************************************************************************************

class Object
{
	readonly class: string;
	readonly callable: boolean;
	readonly name: string | undefined;
	readonly displayName: string | undefined;
	readonly parameterNames: string[] | undefined;
	readonly script: Script | undefined;
	readonly environment: Environment;
	readonly isError: boolean;
	readonly isMutedError: boolean;
	readonly errorMessageName: string | undefined;
	readonly errorLineNumber: number | undefined;
	readonly errorColumnNumber: number | undefined;
	readonly isBoundFunction: boolean;
	readonly isArrowFunction: boolean;
	readonly isGeneratorFunction: boolean;
	readonly isAsyncFunction: boolean;
	readonly isClassConstructor: boolean;
	readonly isPromise: boolean;
	readonly boundTargetFunction: Function | undefined;
	readonly boundThis: globalThis.Object | undefined;
	readonly boundArguments: globalThis.Object[] | undefined;
	readonly isProxy: boolean;
	readonly proxyTarget: Object | null | undefined;
	readonly proxyHandler: Object | null | undefined;
	readonly promiseState: "pending" | "fulfilled" | "rejected";
	readonly promiseValue: any;
	readonly promiseReason: any;
	readonly promiseAllocationSite: SavedFrame;
	readonly promiseResolutionSite: SavedFrame;
	readonly promiseID: number;
	readonly promiseDependentPromises: Object[];
	readonly promiseLifetime: number;
	readonly promiseTimeToResolution: number;

	getProperty: (key: PropertyKey, receiver?: Object) => CompletionValue;
	setProperty: (key: PropertyKey, value: any, receiver?: Object) => CompletionValue;
	getOwnPropertyDescriptor: (key: string) => globalThis.PropertyDescriptor | undefined;
	getOwnPropertyNames: () => string[];
	getOwnPropertyNamesLength: () => number;
	getOwnPropertySymbols: () => string[];
	defineProperty: (key: PropertyKey, attributes: PropertyDescriptor) => void;
	defineProperties: (properties: { [key: PropertyKey]: PropertyDescriptor }) => void;
	deleteProperty: (key: PropertyKey) => void;
	seal: () => void;
	freeze: () => void;
	preventExtensions: () => void;
	isSealed: () => boolean;
	isFrozen: () => boolean;
	isExtensible: () => boolean;
	makeDebuggeeValue: <T>(value: T) => T extends globalThis.Object ? Object : T;
	isSameNative: (obj: Object) => boolean;
	isSameNativeWithJitInfo: (obj: Object) => boolean;
	isNativeGetterWithJitInfo: (obj: Object) => boolean;
	decompile: (pretty: boolean) => string | undefined;
	call: (this: globalThis.Object | { asConstructor: true }, ...argument: any) => CompletionValue;
	apply: (this: globalThis.Object | { asConstructor: true }, arguments: any[] | null | undefined) => CompletionValue;
	executeInGlobal: (code: string, options?: EvalOptions) => CompletionValue;
	executeInGlobalWithBindings: (code: string, bindings: { [id: string]: globalThis.Object }, options?: EvalOptions & { useInnerBindings: boolean }) => CompletionValue;
	createSource: (options: { text: string, url: string, startLine: number, startColumn: number, sourceMapURL?: string, isScriptElement?: boolean, forceEnableAsmJS?: boolean }) => Source;
	asEnvironment: () => Environment;
	unwrap: () => Object;
	unsafeDereference: () => Object;
	forceLexicalInitializationByName: (binding: globalThis.Object) => boolean;
	getpromiseReactions: () => ({ resolve?: Object, reject?: Object, result: Promise<any> | null } | Object | Frame)[];
}

//************************************************************************************************
// Debugger::Source
//************************************************************************************************

class Source
{
	readonly text: string;
	readonly url: string | undefined;
	readonly startLine: number;
	readonly startColumn: number;
	readonly id: number;
	readonly sourceMapURL: string;
	readonly displayURL: string;
	readonly element: Object | undefined;
	readonly elementAttributeName: string | undefined;
	readonly introductionType: "eval" | "debugger eval" | "Function" | "GeneratorFunction" | "AsyncFunction" | "AsyncGenerator" | "Worklet" | "importScripts" | "eventHandler" | "srcScript" | "inlineScript" | "injectedScript" | "importedModule" | "javascriptURL" | "domTimer" | "self-hosted" | undefined;
	readonly introductionScript: Script | undefined;
	readonly introductionOffset: number | undefined;

	reparse: () => Script;
}

//************************************************************************************************
// Debugger::Frame
//************************************************************************************************

class Frame
{
	readonly type: "call" | "eval" | "global" | "module" | "wasmcall" | "debugger";
	readonly implementation: "interpreter" | "baseline" | "ion" | "wasm";
	readonly this: Object;
	readonly older: Frame | null;
	readonly olderSavedFrame: SavedFrame | undefined;
	readonly onStack: boolean;
	readonly terminated: boolean;
	readonly script: Script | null;
	readonly offset: number;
	readonly environment: Environment | null;
	readonly callee: Function | null;
	readonly constructing: boolean;
	readonly arguments: readonly any[]
	readonly asyncPromise: Object | undefined;

	onStep: (() => ResumptionValue) | undefined;
	onPop: ((reason: CompletionValue) => ResumptionValue) | undefined;
	eval: (code: string, options?: EvalOptions) => CompletionValue;
	evalWithBindings: (code: string, bindings: { [id: string]: globalThis.Object }, options?: EvalOptions) => CompletionValue;
}

//************************************************************************************************
// Debugger::Query
//************************************************************************************************

type Query = {
	minOffset?: number;
	maxOffset?: number;
	line?: number;
	minLine?: number;
	minColumn?: number;
	maxLine?: number;
	maxColumn?: number;
};

//************************************************************************************************
// Debugger::PossibleBreakpoint
//************************************************************************************************

type PossibleBreakpoint = {
	offset: number,
	lineNumber: number,
	columnNumber: number,
	isStepStart: boolean
};

//************************************************************************************************
// Debugger::BreakpointMetadata
//************************************************************************************************

type BreakpointMetadata = {
	lineNumber: number;
	columnNumber: number;
	isBreakpoint: boolean,
	isStepStart: boolean
};

//************************************************************************************************
// Debugger::ResumptionValue
//************************************************************************************************

type ResumptionValue = undefined | void | null | { return: any } | { throw: any };

//************************************************************************************************
// Debugger::CompletionValue
//************************************************************************************************

type CompletionValue = { return: any } | { throw: any, stack: SavedFrame } | null;

//************************************************************************************************
// Debugger::BreakpointHandler
//************************************************************************************************

type BreakpointHandler = {
	hit: (frame: Frame) => ResumptionValue
};

//************************************************************************************************
// Debugger::EvalOptions
//************************************************************************************************

type EvalOptions = {
	url: string,
	lineNumber: number
};

//************************************************************************************************
// Debugger::Script
//************************************************************************************************

class Script
{
	readonly isGeneratorFunction: boolean;
	readonly isAsyncFunction: boolean;
	readonly isFunction: boolean;
	readonly isModule: boolean;
	readonly displayName: string | undefined;
	readonly parameterNames: (string | undefined)[] | undefined;
	readonly url: string | null;
	readonly startLine: number;
	readonly startColumn: number;
	readonly lineCount: number;
	readonly source: Source | null;
	readonly sourceStart: number;
	readonly sourceLength: number;
	readonly mainOffset: number;
	readonly global: Object;
	readonly format: "js" | "wasm";

	getChildScripts: () => Script[];
	getPossibleBreakpoints: (query?: Query) => PossibleBreakpoint[];
	getPossibleBreakpointOffsets: (query?: Query) => number[];
	getOffsetMetadata: (offset: number) => BreakpointMetadata;
	setBreakpoint: (offset: number, handler: BreakpointHandler) => void;
	getBreakpoints: (offset?: number) => BreakpointHandler[];
	clearBreakpoint: (handler: BreakpointHandler, offset?: number) => void;
	clearAllBreakpoints: (offset?: number) => void;
	getEffectfulOffsets: () => number[];
	isInCatchScope: (offset: number) => boolean;
}
}

//************************************************************************************************
// Debugger
//************************************************************************************************

declare class Debugger
{
	onNewGlobalObject: (global: Debugger.Object) => void | undefined;
	addDebuggee: (global: Debugger.Object) => void;
	onNewScript: (script: Debugger.Script) => void | undefined;
}
