//************************************************************************************************
//
// Audio Service
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
// Filename    : miniaudiograph.cpp
// Description : Audio Graph based on miniaudio
//
//************************************************************************************************

#include "miniaudiograph.h"

#include "ccl/extras/audio/cclaudiodataops.h"
#include "ccl/extras/audio/audioformatter.h"

namespace CCL {

static constexpr int kMaxNodeBusCount = MA_MAX_NODE_BUS_COUNT;
static ma_uint32 kOneChannelCounts[kMaxNodeBusCount] = {};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (MiniAudioNode, kFirstRun)
{
	for(int i = 0; i < kMaxNodeBusCount; i++)
		kOneChannelCounts[i] = 1;
	return true;
}

//************************************************************************************************
// MiniAudioNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioNode, AudioNodeBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioNode::~MiniAudioNode ()
{
	ASSERT (initDone == false) // must be detached from graph!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ma_node_graph* MiniAudioNode::getNodeGraph ()
{
	return graph ? static_cast<MiniAudioGraph*> (graph)->getNodeGraph () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioNode::uninitializeNode ()
{
	if(initDone)
	{
		ma_node_uninit (nodePointer, nullptr);
		
		initDone = false;
		nodePointer = nullptr;

		if(graph)
			graph->cleanupConnections (this); // all connections were just discarded
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioNode::onNodeAttached ()
{
	initializeNode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioNode::onNodeDetached ()
{
	uninitializeNode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioNode::onProcessingChanged (bool state)
{
	ASSERT (isValid ())
	ma_result result = ma_node_set_state (nodePointer, state ? ma_node_state_started : ma_node_state_stopped);
	ASSERT (result == MA_SUCCESS)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioNode::setStreamLayout (const IAudioStreamLayout& layout)
{
	CCL_DEBUGGER ("Can't change stream layout of this node!\n")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IAudioStreamLayout& CCL_API MiniAudioNode::getStreamLayout () const
{
	return streamLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MiniAudioNode::getBusOffset (AudioStreamDirection direction, int streamIndex) const
{
	int offset = 0;
	for(int i = 0; i < streamIndex; i++)
		offset += streamLayout.getChannelCount (direction, i);
	return offset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioNode::connect (IAudioNode* _destination, int outputIndex, int inputIndex)
{
	ASSERT (isValid ())
	if(!isValid ())
		return kResultUnexpected;

	auto* destination = unknown_cast<MiniAudioNode> (_destination);
	ASSERT (destination && destination->isValid ())
	if(!destination || !destination->isValid ())
		return kResultInvalidArgument;

	const IAudioStreamLayout& sourceLayout = this->streamLayout;
	const IAudioStreamLayout& destinationLayout = destination->getStreamLayout ();

	if(outputIndex < 0 || outputIndex >= sourceLayout.getStreamCount (kAudioOutputDirection) ||
	   inputIndex < 0 || inputIndex >= destinationLayout.getStreamCount (kAudioInputDirection))
		return kResultInvalidArgument;

	int channelCount = sourceLayout.getChannelCount (kAudioOutputDirection, outputIndex);
	if(channelCount <= 0 || channelCount != destinationLayout.getChannelCount (kAudioInputDirection, inputIndex))
		return kResultInvalidArgument;

	int sourceOffset = getBusOffset (kAudioOutputDirection, outputIndex);
	int destinationOffset = destination->getBusOffset (kAudioInputDirection, inputIndex);

	for(int i = 0; i < channelCount; i++)
	{
		ma_uint32 sourceBus = ma_uint32(sourceOffset + i);
		ma_uint32 destinationBus = ma_uint32(destinationOffset + i);
		ma_result result = ma_node_attach_output_bus (getNode (), sourceBus, destination->getNode (), destinationBus);
		ASSERT (result == MA_SUCCESS)
	}

	graph->storeConnection ({this, outputIndex, destination, inputIndex});
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioNode::disconnect (int outputIndex)
{
	ASSERT (isValid ())
	if(!isValid ())
		return kResultUnexpected;

	if(outputIndex < 0 || outputIndex >= streamLayout.getStreamCount (kAudioOutputDirection))
		return kResultInvalidArgument;

	int channelCount = streamLayout.getChannelCount (kAudioOutputDirection, outputIndex);
	int sourceOffset = getBusOffset (kAudioOutputDirection, outputIndex);

	for(int i = 0; i < channelCount; i++)
	{
		ma_uint32 busIndex = ma_uint32(sourceOffset + i);
		ma_result result = ma_node_detach_output_bus (getNode (), busIndex);
		ASSERT (result == MA_SUCCESS)
	}

	graph->unstoreSourceConnection (this, outputIndex);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioNode::disconnectAll ()
{
	ASSERT (isValid ())
	if(!isValid ())
		return kResultUnexpected;

	ma_result result = ma_node_detach_all_output_buses (getNode ());
	ASSERT (result == MA_SUCCESS)

	graph->unstoreSourceConnections (this);
	return kResultOk;
}

//************************************************************************************************
// MiniAudioOutputNode
//************************************************************************************************

void MiniAudioOutputNode::processFrames (ma_node* pNode,
										 const float** ppFramesIn, ma_uint32* pFrameCountIn,
										 float** ppFramesOut, ma_uint32* pFrameCountOut)
{
	auto* outputNode = static_cast<OutputNode*> (pNode);
	MiniAudioOutputNode* This = outputNode->owner;

	ActiveCallGuard activeCallGuard (*This);

	ma_uint32 numFrames = pFrameCountIn[0];
	int numChannels = int(This->vtable.inputBusCount);

	const AudioIOData* currentGraphData = This->getGraph ()->getCurrentIOData ();
	ASSERT (currentGraphData != nullptr)

	int streamIndex = This->streamIndex; // copy current stream index for this thread
	if(currentGraphData && streamIndex >= 0 && streamIndex < currentGraphData->numOutputs)
	{
		AudioIOData::StreamData& out = currentGraphData->outputs[streamIndex];
		int channelCount = get_min<int> (out.numChannels, numChannels);

		// mix, don't overwrite (output can be shared); numFrames is only the current chunk of the block
		int frameOffset = This->getGraph ()->getCurrentFrameOffset ();
		for(int ch = 0; ch < channelCount; ch++)
			AudioVectorOpsGeneric::vectorAdd (out.buffers32[ch] + frameOffset, ppFramesIn[ch], numFrames);
	}

	// ppFramesOut[0] (the dummy) is left untouched - MA_NODE_FLAG_SILENT_OUTPUT means miniaudio
	// excludes it from the mix regardless of its contents.

	pFrameCountOut[0] = numFrames;
	pFrameCountIn[0] = numFrames;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (MiniAudioOutputNode, MiniAudioNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioOutputNode::MiniAudioOutputNode ()
{
	node.owner = this;
	streamIndex = -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioOutputNode::setStreamLayout (const IAudioStreamLayout& layout)
{
	bool layoutSupported = AudioLayoutHelper (layout).matchesStreamCount (1, 0);
	ASSERT (layoutSupported)
	if(layoutSupported == false)
		return kResultInvalidArgument;

	AudioStreamDescription description;
	layout.getStreamDescription (description, kAudioInputDirection, 0);

	ProcessingSuspender suspender (*this);

	streamLayout.removeAll ();
	streamLayout.add (kAudioInputDirection,
		NEW AudioStream (description.name, AudioStream::kActive, description.channelCount, description.speakerType));

	reinitializeNode ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioOutputNode::setOutputStream (int index)
{
	streamIndex = index;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioOutputNode::getOutputStream () const
{
	return streamIndex;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioOutputNode::initializeNode ()
{
	ASSERT (initDone == false)

	int inputChannelCount = streamLayout.getChannelCount (kAudioInputDirection, 0);

	vtable.onProcess = processFrames;
	vtable.inputBusCount = ma_uint8(inputChannelCount);
	vtable.outputBusCount = 1;
	vtable.flags = MA_NODE_FLAG_CONTINUOUS_PROCESSING|MA_NODE_FLAG_SILENT_OUTPUT;

	ma_node_config config = ma_node_config_init ();
	config.initialState = ma_node_state_stopped;
	config.vtable = &vtable;
	config.pInputChannels = kOneChannelCounts;
	config.pOutputChannels = kOneChannelCounts;

	ma_result result = ma_node_init (getNodeGraph (), &config, nullptr, &node);
	ASSERT (result == MA_SUCCESS)

	nodePointer = &node;
	initDone = true;

	connectToRealEndpoint (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioOutputNode::uninitializeNode ()
{
	if(isValid ())
		connectToRealEndpoint (false);

	SuperClass::uninitializeNode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioOutputNode::connectToRealEndpoint (bool state)
{
	ASSERT (isValid () && getNodeGraph ())
	if(state)
	{
		ma_node* realEndpoint = ma_node_graph_get_endpoint (getNodeGraph ());
		ma_result result = ma_node_attach_output_bus (&node, 0, realEndpoint, 0);
		ASSERT (result == MA_SUCCESS)
	}
	else
	{
		ma_result result = ma_node_detach_all_output_buses (&node);
		ASSERT (result == MA_SUCCESS)
	}
}

//************************************************************************************************
// MiniAudioGraph
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioGraph, AudioGraphBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioGraph::~MiniAudioGraph ()
{
	cleanupAll ();

	if(initDone)
		ma_node_graph_uninit (&nodeGraph, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ma_node_graph* MiniAudioGraph::getNodeGraph ()
{
	if(!initDone)
	{
		// graph is created on first use and never re-created; endpoint is fixed to 1 channel -
		// every MiniAudioGraphOutputNode attaches its own dummy trigger bus directly to it
		ma_node_graph_config config = ma_node_graph_config_init (1);
		ma_result result = ma_node_graph_init (&config, nullptr, &nodeGraph);
		ASSERT (result == MA_SUCCESS)

		initDone = true;
	}
	return &nodeGraph;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioGraph::setProcessConfiguration (const AudioProcessConfiguration& configuration)
{
	ASSERT (configuration.floatType == kAudioFloat32)
	if(configuration.floatType != kAudioFloat32)
		return kResultInvalidArgument;

	tresult result = SuperClass::setProcessConfiguration (configuration);
	if(result == kResultOk)
	{
		discardBuffer.resize (configuration.maxBlockSize); // mono - see getNodeGraph ()
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioGraph::process (const AudioProcessData& data)
{
	ASSERT (initDone && active && processing)
	if(!initDone || !active || !processing)
		return kResultUnexpected;

	ASSERT (data.ioData.numSamples <= processConfiguration.maxBlockSize)
	if(data.ioData.numSamples > processConfiguration.maxBlockSize)
		return kResultInvalidArgument;

	for(int i = 0; i < data.ioData.numOutputs; i++)
	{
		AudioData outData (data.ioData.outputs[i].numChannels, data.ioData.numSamples, data.ioData.outputs[i].buffers32);
		AudioDataOpsGeneric::clear (outData);
	}

	void* discardBufferAddress = discardBuffer.getAddress ();
	ASSERT (discardBufferAddress != nullptr)
	AudioProcessContextPtr context = &data.context;
	ScopedVar<AudioProcessContextPtr> contextScope (currentProcessContext, context);

	AudioIODataPtr ioData = &data.ioData;
	ScopedVar<AudioIODataPtr> ioDataScope (currentIOData, ioData);

	// Drives the pull - every attached MiniAudioGraphOutputNode's dummy trigger bus fires
	// as a side-effect of this. The raw endpoint's own mixed result is never read.
	// 
	// miniaudio splits larger requests into chunks internally without telling the nodes where
	// they are in the block, so we pull chunk by chunk and provide the offset ourselves.
	// The block is split evenly so that no tiny remainder chunk occurs. Chunk boundaries are multiples of
	// kChunkAlignment frames (except the block end) to keep the buffer offsets friendly for vector operations.
	
	int numFrames = data.ioData.numSamples;
	int numChunks = (numFrames + kMaxChunkFrames - 1) / kMaxChunkFrames;
	int framesDone = 0;
	for(int chunkIndex = 0; chunkIndex < numChunks; chunkIndex++)
	{
		int chunkEnd = numFrames;
		if(chunkIndex < numChunks - 1)
		{
			chunkEnd = int(int64(numFrames) * (chunkIndex + 1) / numChunks);
			chunkEnd -= chunkEnd % kChunkAlignment;
		}

		int chunkFrames = chunkEnd - framesDone;
		if(chunkFrames <= 0)
			continue;

		// make chunking visible to the processors
		ScopedVar<int> ioDataOffsetScope (currentFrameOffset, framesDone);
		ScopedVar<int> contextOffsetScope (const_cast<AudioProcessContext&> (data.context).sampleOffset, framesDone);

		ma_uint64 framesRead = 0;
		ma_result result = ma_node_graph_read_pcm_frames (&nodeGraph, discardBufferAddress, ma_uint64(chunkFrames), &framesRead);
		ASSERT (result == MA_SUCCESS && framesRead == ma_uint64(chunkFrames))
		if(framesRead == 0)
			break;

		framesDone += int(framesRead);
	}

	return kResultOk;
}

//************************************************************************************************
// MiniAudioCustomNode
//************************************************************************************************

void MiniAudioCustomNode::processFrames (ma_node* pNode,
										 const float** ppFramesIn, ma_uint32* pFrameCountIn,
										 float** ppFramesOut, ma_uint32* pFrameCountOut)
{
	auto* customNode = static_cast<CustomNode*> (pNode);
	MiniAudioCustomNode* This = customNode->owner;
	ActiveCallGuard activeCallGuard (*This);

	AudioIOData& ioData = This->ioData;

	ma_uint32 numFrames = ioData.numInputs > 0 ? pFrameCountIn[0] :
						  ioData.numOutputs > 0 ? pFrameCountOut[0] : 0;

	ioData.numSamples = int(numFrames);

	// slice the real mono bus arrays into this node's logical streams

	int inputOffset = 0;
	for(int i = 0; i < ioData.numInputs; i++)
	{
		ioData.inputs[i].buffers32 = const_cast<float32**> (ppFramesIn) + inputOffset;
		inputOffset += ioData.inputs[i].numChannels;
	}

	int outputOffset = 0;
	for(int i = 0; i < ioData.numOutputs; i++)
	{
		ioData.outputs[i].buffers32 = ppFramesOut + outputOffset;
		outputOffset += ioData.outputs[i].numChannels;
	}

	const AudioProcessContext& context = This->getGraph ()->getCurrentProcessContext ();

	AudioParamData paramData;
	This->preProcess (paramData, context, ioData.numSamples);

	This->process ({sizeof(AudioProcessData), context, ioData, paramData});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioCustomNode, MiniAudioNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioCustomNode::MiniAudioCustomNode ()
{
	node.owner = this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioCustomNode::~MiniAudioCustomNode ()
{
	ioData.deallocateStreamData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioCustomNode::setStreamLayout (const IAudioStreamLayout& layout)
{
	// check hard limit of max. mono buses per node
	bool busCountExceeded = AudioLayoutHelper (layout).getTotalChannelCount (kAudioInputDirection) > kMaxNodeBusCount ||
							AudioLayoutHelper (layout).getTotalChannelCount (kAudioOutputDirection) > kMaxNodeBusCount;
	ASSERT (!busCountExceeded)
	if(busCountExceeded)
		return kResultOutOfMemory;

	ProcessingSuspender suspender (*this);
		
	int numInputStreams = layout.getStreamCount (kAudioInputDirection);
	int numOutputStreams = layout.getStreamCount (kAudioOutputDirection);

	ioData.deallocateStreamData ();
	ioData.allocateStreamData (numInputStreams, numOutputStreams);

	streamLayout.removeAll ();

	for(int i = 0; i < numInputStreams; i++)
	{
		AudioStreamDescription description;
		layout.getStreamDescription (description, kAudioInputDirection, i);

		ioData.inputs[i].numChannels = description.channelCount;
		
		streamLayout.add (kAudioInputDirection,
			NEW AudioStream (description.name, AudioStream::kActive, description.channelCount, description.speakerType));
	}

	for(int i = 0; i < numOutputStreams; i++)
	{
		AudioStreamDescription description;
		layout.getStreamDescription (description, kAudioOutputDirection, i);

		ioData.outputs[i].numChannels = description.channelCount;

		streamLayout.add (kAudioOutputDirection,
			NEW AudioStream (description.name, AudioStream::kActive, description.channelCount, description.speakerType));
	}

	reinitializeNode ();
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioCustomNode::initializeNode ()
{
	ASSERT (initDone == false)

	int totalInputChannels = streamLayout.getTotalChannelCount (kAudioInputDirection);
	int totalOutputChannels = streamLayout.getTotalChannelCount (kAudioOutputDirection);

	ASSERT (totalInputChannels <= kMaxNodeBusCount)
	ASSERT (totalOutputChannels <= kMaxNodeBusCount)

	vtable.onProcess = processFrames;
	vtable.inputBusCount = ma_uint8(totalInputChannels);
	vtable.outputBusCount = ma_uint8(totalOutputChannels);
	vtable.flags = MA_NODE_FLAG_CONTINUOUS_PROCESSING;

	ma_node_config config = ma_node_config_init ();
	config.initialState = ma_node_state_stopped;
	config.vtable = &vtable;
	config.pInputChannels = kOneChannelCounts;
	config.pOutputChannels = kOneChannelCounts;

	ma_result result = ma_node_init (getNodeGraph (), &config, nullptr, &node);
	ASSERT (result == MA_SUCCESS)

	nodePointer = &node;
	initDone = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MiniAudioCustomNode::isProcessing () const
{
	return isStarted ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioCustomNode::setProcessing (tbool state)
{
	if(!isValid ())
		return kResultUnexpected;

	onProcessingChanged (state != 0);
	return kResultOk;
}

//************************************************************************************************
// MiniAudioParamNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioParamNode, MiniAudioCustomNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioParamNode::initParamList (const ParameterDefinitionList* _paramList)
{
	paramList = _paramList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioParamNode::queryInterface (UIDRef iid, void** ptr)
{
	if(paramList != nullptr)
	{
		QUERY_INTERFACE (IParameterProvider)
		QUERY_INTERFACE (IParameterDefinitionProvider)
	}

	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioParamNode::countParameters () const
{
	ASSERT (paramList != nullptr)
	return paramList ? paramList->count () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioParamNode::getParameterAt (ParameterDefinition& definition, int index) const
{
	ASSERT (paramList != nullptr)
	if(paramList == nullptr || !paramList->isValidIndex (index))
		return kResultInvalidArgument;

	definition = paramList->at (index);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioParamNode::setParameterValue (int tag, VariantRef value)
{
	const ParameterDefinition* definition = paramList ? paramList->find (tag) : nullptr;
	return definition ? setValue (*definition, value) : kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioParamNode::setParameterValue (StringID name, VariantRef value)
{
	const ParameterDefinition* definition = paramList ? paramList->find (name) : nullptr;
	return definition ? setValue (*definition, value) : kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioParamNode::getParameterValue (Variant& value, int tag) const
{
	const ParameterDefinition* definition = paramList ? paramList->find (tag) : nullptr;
	return definition ? getValue (value, *definition) : kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioParamNode::getParameterValue (Variant& value, StringID name) const
{
	const ParameterDefinition* definition = paramList ? paramList->find (name) : nullptr;
	return definition ? getValue (value, *definition) : kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
 
tresult MiniAudioParamNode::setValue (const ParameterDefinition& definition, VariantRef _value)
{
	AudioParamValue value (_value.asDouble ());

	ASSERT (definition.hasRange ())
	if(definition.hasRange () == true)
	{
		value.setDouble (ccl_bound<double> (value.asDouble (), 
											definition.minValue.asDouble (), 
											definition.maxValue.asDouble ()));
	}

	// fails only if too many different parameters are pending, i.e. when the audio thread is not running
	if(!scheduleParamEvent (definition.tag, value))
		return kResultOutOfMemory;

	uiValues.setValue (definition.tag, value);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult MiniAudioParamNode::getValue (Variant& result, const ParameterDefinition& definition) const
{
	AudioParamValue value;
	if(uiValues.getValue (value, definition.tag))
		result = value.asDouble ();
	else
		result = definition.defaultValue;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MiniAudioParamNode::scheduleParamEvent (int tag, AudioParamValue value)
{
	AudioParamEvent event;
	event.tag = tag;
	event.value = value;
	return paramQueue.push (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioParamNode::clearParamEvents ()
{
	paramQueue.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioParamNode::preProcess (AudioParamData& data,
									 const AudioProcessContext& context,
									 int numSamples)
{
	// pending events are delivered once, with sampleOffset 0 (no automation yet)
	if(paramQueue.popAll (blockEvents) && !blockEvents.isEmpty ())
	{
		data.numEvents = blockEvents.count ();
		data.events = blockEvents.getItems ();
	}
}

//************************************************************************************************
// MiniAudioInternalParamNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioInternalParamNode, MiniAudioParamNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioInternalParamNode::processParams (const AudioProcessData& data)
{
	if(!data.paramData.isEmpty ())
		for(int i = 0; i < data.paramData.numEvents; i++)
		{
			const AudioParamEvent& e = data.paramData.events[i];
			applyValue (e.tag, e.value);
		}
}

//************************************************************************************************
// MiniAudioProcessorNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioProcessorNode, MiniAudioParamNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioProcessorNode::setStreamLayout (const IAudioStreamLayout& layout)
{
	setProcessor (nullptr); // reset processor, just in case

	return SuperClass::setStreamLayout (layout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioProcessorNode::setProcessor (IAudioProcessor* newProcessor)
{
	if(processor != newProcessor)
	{
		// stream layout must be configured matching with component
		UnknownPtr<IAudioComponent> newComponent (newProcessor);
		if(newComponent && newComponent->hasKnownStreamLayout ())
		{
			const IAudioStreamLayout& componentLayout = newComponent->getStreamLayout ();
			if(AudioLayoutHelper (streamLayout).matches (componentLayout) == false)
				return kResultInvalidArgument;
		}

		{
			ProcessingSuspender suspender (*this);

			if(component.isValid () && isAttached ())
			{
				if(graph->isActive ())
					component->setActive (false);
			}
		
			processor = newProcessor;
			component = newComponent;
		
			if(component.isValid () && isAttached ())
			{
				if(graph->isPrepared ())
				{
					AudioProcessConfiguration configuration;
					graph->getProcessConfiguration (configuration);
					component->setProcessConfiguration (configuration);
				}

				if(graph->isActive ())
					component->setActive (true);
			}

			clearParamEvents ();
		}

		// update cached parameters
		cachedParamList.removeAll ();
		uiValues.removeAll ();
		provider = UnknownPtr<IParameterDefinitionProvider> (newProcessor);
		if(provider)
		{
			int paramCount = provider->countParameters ();
			cachedParamList.setCount (paramCount);
			for(int i = 0; i < paramCount; i++)
				provider->getParameterAt (cachedParamList[i], i);
		}
		initParamList (provider.isValid () ? &cachedParamList : nullptr);
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioProcessor* CCL_API MiniAudioProcessorNode::getProcessor () const
{
	return processor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioProcessorNode::prepare (const AudioProcessConfiguration& configuration)
{
	SuperClass::prepare (configuration);
	
	if(component)
		component->setProcessConfiguration (configuration);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioProcessorNode::onDisplayLatencyChanged (int latencyInSamples)
{
	if(UnknownPtr<IAudioDisplayLatency> displayLatency = processor)
		displayLatency->setDisplayLatency (latencyInSamples);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioProcessorNode::onActiveChanged (bool state)
{
	SuperClass::onActiveChanged (state);

	if(component)
		component->setActive (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioProcessorNode::onProcessingChanged (bool state)
{
	if(state)
	{
		if(processor)
			processor->setProcessing (true);

		SuperClass::onProcessingChanged (true);
	}
	else
	{
		SuperClass::onProcessingChanged (false);
		
		if(processor)
			processor->setProcessing (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioProcessorNode::process (const AudioProcessData& data)
{
	return processor ? processor->process (data) : kResultOk;
}

//************************************************************************************************
// MiniAudioInputNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioInputNode, MiniAudioCustomNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioInputNode::MiniAudioInputNode ()
{
	streamIndex = -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioInputNode::setStreamLayout (const IAudioStreamLayout& layout)
{
	bool layoutSupported = AudioLayoutHelper (layout).matchesStreamCount (0, 1);
	ASSERT (layoutSupported)
	if(layoutSupported == false)
		return kResultInvalidArgument;

	return SuperClass::setStreamLayout (layout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioInputNode::setInputStream (int index)
{
	streamIndex = index;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioInputNode::getInputStream () const
{
	return streamIndex;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioInputNode::process (const AudioProcessData& data)
{
	ASSERT (data.ioData.numInputs == 0 && data.ioData.numOutputs == 1)
	AudioIOData::StreamData& out = data.ioData.outputs[0];

	const AudioIOData* currentGraphData = getGraph ()->getCurrentIOData ();
	ASSERT (currentGraphData != nullptr)

	int numChannelsProcessed = 0;
	int streamIndex = this->streamIndex; // copy current stream index for this thread
	if(currentGraphData && streamIndex >= 0 && streamIndex < currentGraphData->numInputs)
	{
		const AudioIOData::StreamData& in = currentGraphData->inputs[streamIndex];

		// numSamples is only the current chunk of the block
		int frameOffset = getGraph ()->getCurrentFrameOffset ();

		int channelCount = get_min (in.numChannels, out.numChannels);
		for(int ch = 0; ch < channelCount; ch++)
			AudioVectorOpsGeneric::vectorCopy (out.buffers32[ch], in.buffers32[ch] + frameOffset, data.ioData.numSamples);
		
		numChannelsProcessed = channelCount;
	}

	if(numChannelsProcessed < out.numChannels)
	{
		// clear remaining outputs
		for(int ch = numChannelsProcessed; ch < out.numChannels; ch++)
			AudioVectorOpsGeneric::vectorClear (out.buffers32[ch], data.ioData.numSamples);
	}

	return kResultOk;
}

//************************************************************************************************
// MiniAudioSourceNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioSourceNode, MiniAudioCustomNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioSourceNode::setStreamLayout (const IAudioStreamLayout& layout)
{
	bool layoutSupported = AudioLayoutHelper (layout).matchesStreamCount (0, 1);
	ASSERT (layoutSupported)
	if(layoutSupported == false)
		return kResultInvalidArgument;

	setSource (nullptr); // reset source, just in case

	return SuperClass::setStreamLayout (layout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioSourceNode::setSource (IAudioSource* newSource)
{
	if(newSource)
	{
		// stream layout must be configured first
		if(streamLayout.getStreamCount (kAudioOutputDirection) == 0)
			return kResultUnexpected;

		// source channel count must match
		if(newSource->getNumChannels () != streamLayout.getChannelCount (kAudioOutputDirection))
			return kResultInvalidArgument;
	}

	ProcessingSuspender suspender (*this);
	source = newSource;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioSource* CCL_API MiniAudioSourceNode::getSource () const
{
	return source;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioSourceNode::process (const AudioProcessData& data)
{
	ASSERT (data.ioData.numInputs == 0 && data.ioData.numOutputs == 1)

	if(source)
	{
		ASSERT (data.ioData.outputs[0].numChannels == source->getNumChannels ())

		AudioData audioData (data.ioData.outputs[0].numChannels,
							 data.ioData.numSamples,
							 data.ioData.outputs[0].buffers32);

		int numRead = source->read (audioData, audioData.numSamples);
		ASSERT (numRead >= 0)

		if(numRead >= 0 && numRead < audioData.numSamples)
		{
			AudioDataSpan remainder (audioData, numRead, audioData.numSamples - numRead);
			AudioDataOpsGeneric::clear (remainder);
		}
	}

	return kResultOk;
}

//************************************************************************************************
// MiniAudioGainer
//************************************************************************************************

MiniAudioGainer::MiniAudioGainer (TSeconds smoothTime)
: smoothTime (smoothTime)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioGainer::~MiniAudioGainer ()
{
	resize (0, 0); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioGainer::setGain (int index, float gain)
{
	ASSERT (index >= 0 && index < count)
	ma_gainer_set_gain (&gainers[index], gain); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioGainer::setGains (float gain)
{
	for(int i = 0; i < count; i++)
		ma_gainer_set_gain (&gainers[i], gain); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioGainer::resize (int newCount, SampleRate sampleRate)
{
	if(newCount == count && sampleRate == currentRate)
		return; // nothing changed

	if(gainers)
	{
		for(int i = 0; i < count; i++)
			ma_gainer_uninit (&gainers[i], nullptr);

		delete [] gainers;
		gainers = nullptr;
	}

	count = newCount;
	currentRate = sampleRate;

	if(count <= 0)
		return;

	ma_uint32 smoothTimeInFrames = ma_uint32(currentRate * smoothTime);

	gainers = NEW ma_gainer[count];
	for(int i = 0; i < count; i++)
	{
		ma_gainer_config config = ma_gainer_config_init (1, smoothTimeInFrames);
		ma_result result = ma_gainer_init (&config, nullptr, &gainers[i]);
		ASSERT (result == MA_SUCCESS)
	}
}

//************************************************************************************************
// MiniAudioGainNode
//************************************************************************************************

const ParameterDefinitionList& MiniAudioGainNode::getParamList ()
{
	static ParameterDefinitionList paramList;
	if(paramList.isEmpty ())
	{
		paramList.add (ParameterDefinition ()
					   .setType (kParamTypeFloat)
					   .setName (CSTR ("gain"))
					   .setTitle (CCLSTR ("Gain"))
					   .setTag (kGain)
					   .setRange (0., 100.)
					   .setDefault (1.)
					   .setUnit (Format::kGainDB)
					  );
	}
	return paramList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioGainNode, MiniAudioInternalParamNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioGainNode::MiniAudioGainNode ()
{
	initParamList (&getParamList ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioGainNode::~MiniAudioGainNode ()
{
	ASSERT (gainer.getCount () == 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioGainNode::setStreamLayout (const IAudioStreamLayout& layout)
{
	bool layoutSupported = AudioLayoutHelper (layout).matchesStreamCount (1, 1) &&
						   layout.getChannelCount (kAudioInputDirection) == layout.getChannelCount (kAudioOutputDirection);
	ASSERT (layoutSupported)
	if(layoutSupported == false)
		return kResultInvalidArgument;

	return SuperClass::setStreamLayout (layout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioGainNode::initializeNode ()
{
	SuperClass::initializeNode ();

	int numChannels = streamLayout.getChannelCount (kAudioInputDirection);
	gainer.resize (numChannels, graph->getSampleRate ());
	gainer.setGains (getGain ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioGainNode::uninitializeNode ()
{
	SuperClass::uninitializeNode ();

	gainer.resize (0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioGainNode::setGain (float gain)
{
	return setParameterValue (kGain, gain);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API MiniAudioGainNode::getGain () const
{
	Variant gainValue;
	getParameterValue (gainValue, kGain);
	return gainValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioGainNode::prepare (const AudioProcessConfiguration& configuration)
{
	gainer.resize (gainer.getCount (), configuration.sampleRate);
	gainer.setGains (getGain ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioGainNode::applyValue (int tag, AudioParamValue value)
{
	ASSERT (tag == kGain)
	if(tag == kGain)
	{
		gainer.setGains (value.asFloat ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioGainNode::process (const AudioProcessData& data)
{
	processParams (data);

	ASSERT (data.ioData.numInputs == 1 && data.ioData.numOutputs == 1)
	ASSERT (gainer.getCount () == data.ioData.inputs[0].numChannels)

	for(int ch = 0; ch < gainer.getCount (); ch++)
		gainer.process (ch, data.ioData.outputs[0].buffers32[ch], data.ioData.inputs[0].buffers32[ch], data.ioData.numSamples);

	return kResultOk;
}

//************************************************************************************************
// MiniAudioPanNode
//************************************************************************************************

const ParameterDefinitionList& MiniAudioPanNode::getParamList ()
{
	static ParameterDefinitionList paramList;
	if(paramList.isEmpty ())
	{
		paramList.add (ParameterDefinition ()
					   .setType (kParamTypeFloat)
					   .setFlags (kParamBehaviorBipolar)
					   .setName (CSTR ("pan"))
					   .setTitle (CCLSTR ("Pan"))
					   .setTag (kPan)
					   .setRange (-1., +1.)
					   .setDefault (0.)
					   .setUnit (Format::kStereoPan)
					  );
	}
	return paramList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioPanNode, MiniAudioInternalParamNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioPanNode::MiniAudioPanNode ()
{
	initParamList (& getParamList ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioPanNode::setStreamLayout (const IAudioStreamLayout& layout)
{
	bool layoutSupported = AudioLayoutHelper (layout).matchesStreamCount (1, 1);
	ASSERT (layoutSupported)
	if(layoutSupported == false)
		return kResultInvalidArgument;

	return SuperClass::setStreamLayout (layout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioPanNode::setPan (float pan)
{
	return setParameterValue (kPan, pan);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API MiniAudioPanNode::getPan () const
{
	Variant panValue;
	getParameterValue (panValue, kPan);
	return panValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioPanNode::applyGains ()
{
	for(int i = 0; i < matrix.getCount (); i++)
		gainer.setGain (i, matrix.getEntry (i).gain);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioPanNode::initializeNode ()
{
	SuperClass::initializeNode ();

	AudioStreamDescription inputDesc;
	streamLayout.getStreamDescription (inputDesc, kAudioInputDirection, 0);
	if(inputDesc.speakerType == SpeakerDef::kEmpty)
		inputDesc.speakerType = SpeakerDef::getDefault (inputDesc.channelCount);

	AudioStreamDescription outputDesc;
	streamLayout.getStreamDescription (outputDesc, kAudioOutputDirection, 0);
	if(outputDesc.speakerType == SpeakerDef::kEmpty)
		outputDesc.speakerType = SpeakerDef::getDefault (outputDesc.channelCount);

	matrix.build (inputDesc.channelCount, inputDesc.speakerType, outputDesc.channelCount, outputDesc.speakerType);

	gainer.resize (matrix.getCount (), graph->getSampleRate ());
	applyGains ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioPanNode::uninitializeNode ()
{
	SuperClass::uninitializeNode ();

	gainer.resize (0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioPanNode::prepare (const AudioProcessConfiguration& configuration)
{
	inputSnapshot.resize (configuration.maxBlockSize, streamLayout.getChannelCount (kAudioInputDirection), kAudioFloat32);
	scratchBuffer.resize (configuration.maxBlockSize, 1, kAudioFloat32);

	gainer.resize (matrix.getCount (), configuration.sampleRate);
	applyGains ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioPanNode::unprepare ()
{
	inputSnapshot.dispose ();
	scratchBuffer.dispose ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioPanNode::applyValue (int tag, AudioParamValue value)
{
	ASSERT (tag == kPan)
	if(tag == kPan)
	{
		matrix.setPan (value.asFloat ());
		applyGains ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioPanNode::process (const AudioProcessData& data)
{
	processParams (data);

	int numInputChannels = data.ioData.inputs[0].numChannels;
	int numOutputChannels = data.ioData.outputs[0].numChannels;
	int numSamples = data.ioData.numSamples;

	float32** inputBuffers = data.ioData.inputs[0].buffers32;

	// only worth checking for in-place processing if some input feeds more than one entry - otherwise
	// every entry reads its own input exactly once, before anything else could need it again, so
	// in-place processing is inherently safe
	if(matrix.hasFanOut ())
	{
		bool inplace = false;
		for(int n = 0; n < numInputChannels && !inplace; n++)
			for(int m = 0; m < numOutputChannels; m++)
				if(data.ioData.inputs[0].buffers32[n] == data.ioData.outputs[0].buffers32[m])
				{ 
					inplace = true; 
					break; 
				}

		if(inplace)
		{
			for(int ch = 0; ch < numInputChannels; ch++)
				AudioVectorOpsGeneric::vectorCopy (inputSnapshot.getBuffer32 (ch), inputBuffers[ch], numSamples);
			inputBuffers = inputSnapshot.getAudioData ().buffers32;
		}
	}

	// only channels nothing writes to need an explicit clear - everything else gets its content
	// entirely from the entry loop below
	for(int ch = 0; ch < numOutputChannels; ch++)
		if(matrix.isOutputUsed (ch) == false)
		{
			AudioVectorOpsGeneric::vectorClear (data.ioData.outputs[0].buffers32[ch], numSamples);
		}

	float32* scratch = scratchBuffer.getBuffer32 ();
	for(int i = 0; i < matrix.getCount (); i++)
	{
		const AudioPanMatrix::Entry& entry = matrix.getEntry (i);
		float32* output = data.ioData.outputs[0].buffers32[entry.outputChannel];

		if(entry.isMixNeeded () == false)
		{
			gainer.process (i, output, inputBuffers[entry.inputChannel], numSamples);
		}
		else
		{
			gainer.process (i, scratch, inputBuffers[entry.inputChannel], numSamples);
			AudioVectorOpsGeneric::vectorAdd (output, scratch, numSamples);
		}
	}

	return kResultOk;
}

//************************************************************************************************
// MiniAudioMeterNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioMeterNode, MiniAudioCustomNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioMeterNode::setStreamLayout (const IAudioStreamLayout& layout)
{
	bool layoutSupported = AudioLayoutHelper (layout).matchesStreamCount (1, 1) &&
						   layout.getChannelCount (kAudioInputDirection) == layout.getChannelCount (kAudioOutputDirection);
	ASSERT (layoutSupported)
	if(layoutSupported == false)
		return kResultInvalidArgument;

	return SuperClass::setStreamLayout (layout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioMeterSource& CCL_API MiniAudioMeterNode::getMeterSource (int streamIndex)
{
	ASSERT (streamIndex == 0)
	return levelDetector;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioMeterNode::initializeNode ()
{
	SuperClass::initializeNode ();

	updateLevelDetector ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioMeterNode::uninitializeNode ()
{
	SuperClass::uninitializeNode ();

	levelDetector.resize (0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioMeterNode::prepare (const AudioProcessConfiguration& configuration)
{
	SuperClass::prepare (configuration);

	updateLevelDetector ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioMeterNode::onDisplayLatencyChanged (int latencyInSamples)
{
	SuperClass::onDisplayLatencyChanged (latencyInSamples);

	updateLevelDetector ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioMeterNode::updateLevelDetector ()
{
	ASSERT (isAttached ())
	if(!isAttached ())
		return;

	int numChannels = streamLayout.getChannelCount (kAudioInputDirection);

	levelDetector.resize (numChannels, graph->getSampleRate (), graph->getDisplayLatency ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioMeterNode::onProcessingChanged (bool state)
{
	levelDetector.setProcessing (state);
	SuperClass::onProcessingChanged (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioMeterNode::process (const AudioProcessData& data)
{
	ASSERT (data.ioData.numInputs == 1 && data.ioData.numOutputs == 1)

	AudioData input (data.ioData.inputs[0].numChannels, data.ioData.numSamples, data.ioData.inputs[0].buffers32);
	levelDetector.process (input);

	int channelCount = get_min (data.ioData.inputs[0].numChannels, data.ioData.outputs[0].numChannels);
	for(int ch = 0; ch < channelCount; ch++)
	{
		float32* dst = data.ioData.outputs[0].buffers32[ch];
		const float32* src = data.ioData.inputs[0].buffers32[ch];
		if(dst != src)
			AudioVectorOpsGeneric::vectorCopy (dst, src, data.ioData.numSamples);
	}

	return kResultOk;
}