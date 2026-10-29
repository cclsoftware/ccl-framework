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
// Filename    : ccl/public/audio/iaudiograph.h
// Description : Audio Graph Interfaces
//
//************************************************************************************************

#ifndef _ccl_iaudiograph_h
#define _ccl_iaudiograph_h

#include "ccl/public/audio/iaudiocomponent.h"
#include "ccl/public/audio/iaudiometer.h"
#include "ccl/public/audio/iaudioprocessor.h"

namespace CCL {

interface IAudioSource;

//************************************************************************************************
// IAudioNode
/**	A node in an IAudioGraph with input and output streams.

	An input node (e.g. wrapping an IAudioSource) has zero input streams and one output stream.
	A processing node (e.g. gain or panning) typically has one stream of each direction.
	An output node has one input stream and zero output streams. 
	
	Additional interfaces:
	- IParameterProvider (optional, depending on node type)
*/
//************************************************************************************************

interface IAudioNode: IUnknown
{
	/** Set node identifier. */
	virtual void CCL_API setID (StringID id) = 0;

	/** Get node identifier. */
	virtual StringID CCL_API getID () const = 0;
	
	/**	Initialize node with given stream layout.
		
		The node layout can be changed for most node types. */
	virtual tresult CCL_API setStreamLayout (const IAudioStreamLayout& layout) = 0;
	
	/** Get stream layout for node. */
	virtual const IAudioStreamLayout& CCL_API getStreamLayout () const = 0;
	
	/**	Connect an output stream of this node to an input stream of destination.
		
		Fails if the two streams' channel counts differ. Multiple output streams may connect
		to the same input stream, in which case their signals are summed.
		If the given output stream is already connected elsewhere, it is moved - no need
		to disconnect first.
		
		Retains both source and destination for as long as this connection exists. */
	virtual tresult CCL_API connect (IAudioNode* destination, int outputIndex = 0, int inputIndex = 0) = 0;

	/** Disconnect a single output stream. 
	
		Releases the retained source and destination. */
	virtual tresult CCL_API disconnect (int outputIndex = 0) = 0;

	/**	Disconnect every output stream.
	 
		Releases each retained source and destination. */
	virtual tresult CCL_API disconnectAll () = 0;

	DECLARE_IID (IAudioNode)
};

DEFINE_IID (IAudioNode, 0x2af015fa, 0x4e6d, 0x4178, 0x9a, 0x16, 0x0b, 0x77, 0x69, 0x70, 0x57, 0x7b)

//************************************************************************************************
// IAudioGraph
/**	A realtime audio processing graph of IAudioNode instances.	
	The graph holds and configures nodes, and drives processing.
	
	Additional interfaces:
	- IAudioDisplayLatency (forwarded to nodes)
	- IAudioProcessor

	Float type is currently limited to 32 bit floating-point. */
//************************************************************************************************

interface IAudioGraph: IAudioComponent
{
	/** Add node to graph - graph takes ownership.
		
		Graph configures node with current process configuration,
		and node becomes ready to connect. */
	virtual tresult CCL_API addNode (IAudioNode* node) = 0;

	/** Remove node from graph - ownership is transferred to caller. */
	virtual tresult CCL_API removeNode (IAudioNode* node) = 0;

	/** Find node by identifier (returns first match). */
	virtual IAudioNode* CCL_API findNode (StringID nodeId) const = 0;

	DECLARE_IID (IAudioGraph)

	//////////////////////////////////////////////////////////////////////////////////////////////

	IAudioProcessor* asProcessor ();
	template <typename INodeType> INodeType* find (StringID nodeId) const;
};

DEFINE_IID (IAudioGraph, 0x2efbf991, 0x26eb, 0x4d22, 0x87, 0xbd, 0xcf, 0x5b, 0x0a, 0xa6, 0xd9, 0x7e)

//************************************************************************************************
// IAudioSourceNode
/**	A stock node wrapping an IAudioSource. */
//************************************************************************************************

interface IAudioSourceNode: IAudioNode
{
	/**	Assign audio source (retained).
	 
		Source must be safe to be read from a realtime-thread.
		Source channel count must match node configuration, otherwise it will be rejected. */
	virtual tresult CCL_API setSource (IAudioSource* source) = 0;

	/** Get currently assigned audio source. */
	virtual IAudioSource* CCL_API getSource () const = 0;

	DECLARE_IID (IAudioSourceNode)
};

DEFINE_IID (IAudioSourceNode, 0xaa216f04, 0x52dd, 0x4d10, 0xa9, 0x66, 0xab, 0x36, 0xe1, 0x53, 0xd8, 0xba)

//************************************************************************************************
// IAudioGainNode
/**	A stock processing node applying gain. */
//************************************************************************************************

interface IAudioGainNode: IAudioNode
{
	/**	Set gain, linear scale (1.0 = unity, 0.0 = silence).
	 
		Changes are ramped to avoid audible artifacts. */
	virtual tresult CCL_API setGain (float gain) = 0;

	/** Get the currently configured gain. */
	virtual float CCL_API getGain () const = 0;

	DECLARE_IID (IAudioGainNode)
};

DEFINE_IID (IAudioGainNode, 0x37fd9100, 0xf0a8, 0x40ee, 0xa4, 0x05, 0x52, 0xe2, 0xe1, 0xd0, 0x7c, 0xc3)

//************************************************************************************************
// IAudioPanNode
/**	A stock processing node applying panning between the same or different channels configuration
	(e.g. mono to stereo, stereo to mono, stereo to stereo, etc.). */
//************************************************************************************************

interface IAudioPanNode: IAudioNode
{
	/**	Set pan position: -1.0 = full left, 0.0 = center, 1.0 = full right.
	
		Changes are ramped to avoid audible artifacts. */
	virtual tresult CCL_API setPan (float pan) = 0;

	/** Get the currently configured pan position. */
	virtual float CCL_API getPan () const = 0;

	DECLARE_IID (IAudioPanNode)
};

DEFINE_IID (IAudioPanNode, 0x1c592830, 0xe6d6, 0x445d, 0x98, 0xeb, 0xa1, 0x4a, 0x8a, 0xaf, 0x74, 0xad)

//************************************************************************************************
// IAudioMeterNode
/**	A stock node observing signal level without modifying it (passthrough).
	Data is exposed through IAudioMeterSource, time-aligned with what is currently audible. */
//************************************************************************************************

interface IAudioMeterNode: IAudioNode
{
	/** Get the meter source for one of this node's streams. */
	virtual IAudioMeterSource& CCL_API getMeterSource (int streamIndex = 0) = 0;

	DECLARE_IID (IAudioMeterNode)
};

DEFINE_IID (IAudioMeterNode, 0x4f2a8d17, 0x6b3c, 0x4e95, 0x8a, 0x1d, 0x9c, 0x5e, 0x3f, 0x7b, 0x2d, 0x64)

//************************************************************************************************
// IAudioInputNode
/** A stock node feeding one input stream from AudioIOData into the graph. */
//************************************************************************************************

interface IAudioInputNode: IAudioNode
{
	/**	Bind to an input stream index of the AudioIOData passed to the graph's process call.

		Stream channel count must match node configuration, otherwise it will be rejected
		while processing. */
	virtual tresult CCL_API setInputStream (int streamIndex) = 0;

	/** Get currently bound input stream index, or -1 if unbound. */
	virtual int CCL_API getInputStream () const = 0;

	DECLARE_IID (IAudioInputNode)
};

DEFINE_IID (IAudioInputNode, 0x6d8a2f31, 0x9c47, 0x4e82, 0xb1, 0x05, 0x3e, 0x7a, 0x92, 0xc4, 0x1f, 0x6b)

//************************************************************************************************
// IAudioOutputNode
/** A stock node writing one output stream from the graph to AudioIOData. */
//************************************************************************************************

interface IAudioOutputNode: IAudioNode
{
	/**	Bind to an output stream index of the AudioIOData passed to the graph's process call.

		Multiple output nodes may share the same stream index, in which case their signals
		are summed. */
	virtual tresult CCL_API setOutputStream (int streamIndex) = 0;

	/** Get currently bound output stream index, or -1 if unbound. */
	virtual int CCL_API getOutputStream () const = 0;

	DECLARE_IID (IAudioOutputNode)
};

DEFINE_IID (IAudioOutputNode, 0x84f1c6a0, 0x2b53, 0x4d97, 0xae, 0x38, 0x5c, 0x91, 0x7d, 0x4a, 0x2e, 0x60)

//************************************************************************************************
// IAudioProcessorNode
/**	A node whose processing is supplied by an externally assigned IAudioProcessor.
	
	The external processor can optionally implement IAudioComponent, which will then also keep
	activation and process configuration in sync with the graph. IAudioDisplayLatency on the
	processor is supported as well.
	
	This also enables nesting another IAudioGraph. */
//************************************************************************************************

interface IAudioProcessorNode: IAudioNode
{
	/**	Assign the processor implementing this node's behavior (retained).

		Its input/output streams must match the node's configured stream layout. */
	virtual tresult CCL_API setProcessor (IAudioProcessor* processor) = 0;

	/** Get currently assigned processor. */
	virtual IAudioProcessor* CCL_API getProcessor () const = 0;

	DECLARE_IID (IAudioProcessorNode)
};

DEFINE_IID (IAudioProcessorNode, 0x3016c365, 0xd091, 0x41cd, 0xa7, 0x94, 0x23, 0xec, 0x4e, 0xf1, 0x2b, 0xc0)

//////////////////////////////////////////////////////////////////////////////////////////////////
// IAudioGraph inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline IAudioProcessor* IAudioGraph::asProcessor ()
{
	return UnknownPtr<IAudioProcessor> (this);
}

template <typename INodeType>
INodeType* IAudioGraph::find (StringID nodeId) const
{
	return UnknownPtr<INodeType> (findNode (nodeId));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_iaudiograph_h
