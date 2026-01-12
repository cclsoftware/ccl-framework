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
// Filename    : core/platform/android/java/SSLChannel.java
// Description : SSL Channel
//
//************************************************************************************************

package dev.ccl.core;

import androidx.annotation.Keep;

import javax.net.ssl.*;
import java.nio.ByteBuffer;

//************************************************************************************************
// SSLChannel
//************************************************************************************************

@Keep
public class SSLChannel
{
	private final long nativeContext;

	private SSLEngine engine;

	private ByteBuffer plainInput;
	private ByteBuffer plainOutput;
	private ByteBuffer encryptedInput;
	private ByteBuffer encryptedOutput;

	private static final boolean DEBUG_LOG = false;

	public SSLChannel (long nativeContext, String peerName)
	{
		this.nativeContext = nativeContext;

		try
		{
			SSLContext sslContext = SSLContext.getInstance ("TLSv1.2");
			sslContext.init (null, null, null);

			engine = peerName.isEmpty () ? sslContext.createSSLEngine () : sslContext.createSSLEngine (peerName, 443);
			engine.setUseClientMode (true);

			// create buffers
			SSLSession session = engine.getSession ();
			plainInput = ByteBuffer.allocate (session.getApplicationBufferSize ());
			plainOutput = ByteBuffer.allocate (session.getApplicationBufferSize ());
			encryptedInput = ByteBuffer.allocate (session.getPacketBufferSize ());
			encryptedOutput = ByteBuffer.allocate (session.getPacketBufferSize ());

			if(DEBUG_LOG)
			{
				//for(String s: engine.getEnabledProtocols ())
				//	Logger.log ("SSLChannel engine.getEnabledProtocols: " + s);
				//for(String s: engine.getEnabledCipherSuites ())
				//	Logger.log ("SSLChannel engine.getEnabledCipherSuites: " + s);

				Logger.log ("SSLChannel: engine: " + engine.getPeerHost () + " : " + engine.getPeerPort ());
				Logger.log ("SSLChannel plainBuffer " + session.getApplicationBufferSize () + ", encBuffer " + session.getPacketBufferSize ());
			}
		}
		catch(Exception e)
		{
			Logger.logError ("SSLChannel Exception: " + e.getMessage (), e);
		}
	}

	public void close ()
	{
		if(DEBUG_LOG)
			Logger.log ("SSLChannel.close ");

		try
		{
			engine.closeOutbound ();
			
			checkHandshakeStatus ();

			engine.closeInbound ();
	
		}
		catch(Exception e)
		{
			Logger.logError ("SSLChannel.close Exception: " + e.getMessage (), e);
		}
	}

	public void handshake ()
	{
		try
		{
			engine.beginHandshake ();
			checkHandshakeStatus ();
		}
		catch(Exception e)
		{
			Logger.logError ("SSLChannel.handshake: Exception: " + e.getMessage (), e);
		}
	}

	private void checkHandshakeStatus ()
	{
		checkHandshakeStatus (engine.getHandshakeStatus ());
	}

	private void checkHandshakeStatus (SSLEngineResult.HandshakeStatus handshakeStatus)
	{
		if(DEBUG_LOG)
			Logger.log ("SSLChannel.checkHandshakeStatus: " + handshakeStatus);

		switch(handshakeStatus)
		{
		case NEED_WRAP:
			performWrap ();
			break;

		case NEED_UNWRAP:
			performUnwrap ();
			break;

		case NEED_TASK:
			Runnable task;
			while((task = engine.getDelegatedTask ()) != null)
				task.run ();

			checkHandshakeStatus ();
			break;

		case NOT_HANDSHAKING:
		case FINISHED:
		default:
			break;
		}
	}

	public int read (byte[] data, int size)
	{
		boolean done = false;
		while(!done)
		{
			if(plainInput.position () > 0)
			{
				// return already decoded data (might be less than requested)
				plainInput.flip ();

				int toCopy = Math.min (size, plainInput.limit ());
				if(toCopy > 0)
				{
					plainInput.get (data, 0, toCopy);
					plainInput.compact ();

					if(DEBUG_LOG)
						Logger.log ("SSLChannel.read: " + size + " copy " + toCopy + " plainInput pos: " + plainInput.position () + " limit: " + plainInput.limit ());
					return toCopy;
				}
			}

			// read data, unwrap and try again
			if(!performUnwrap ())
				done = true;
		}
		return 0;
	}

	public int write (byte[] data, int size)
	{
		if(DEBUG_LOG)
			Logger.log ("SSLChannel.write: " + size);

		plainOutput.clear ();
		plainOutput.put (data, 0, size);

		if(!performWrap ())
			return -1;

		return size; // we have taken all data
	}

	public boolean performWrap ()
	{
		if(DEBUG_LOG)
			Logger.log ("SSLChannel.performWrap");
		try
		{
			boolean done = false;
			while(!done)
			{
				plainOutput.flip ();
				SSLEngineResult result = engine.wrap (plainOutput, encryptedOutput);

				plainOutput.compact ();

				if(DEBUG_LOG)
					Logger.log ("result: " + result);

				SSLEngineResult.Status status = result.getStatus ();
				switch (status)
				{
				case OK:
					flushOutput ();
					checkHandshakeStatus (result.getHandshakeStatus ());

					// repeat if there is still data to wrap
					if(DEBUG_LOG)
						Logger.log ("plainOutput.position " + plainOutput.position ());

					if(plainOutput.position () <= 0)
						done = true;
					break;

				case CLOSED:
					flushOutput ();
					checkHandshakeStatus (result.getHandshakeStatus ());
					close ();
					return false;

				case BUFFER_OVERFLOW:
					// enlarge buffer and try again
					encryptedOutput = adjustBuffer (encryptedOutput, engine.getSession ().getPacketBufferSize ());
					break;

				default:
				}
			}
		}
		catch(Exception e)
		{
			Logger.logError ("SSLChannel.performWrap Exception: " + e.getMessage (), e);
			return false;
		}
		return true;
	}

	public boolean performUnwrap ()
	{
		if(DEBUG_LOG)
			Logger.log ("SSLEngine.performUnwrap");

		// read input data first if buffer is empty
		if(encryptedInput.position () == 0)
			if(readInput () <= 0)
				return false;

		boolean done = false;
		while(!done)
		{
			encryptedInput.flip ();
			SSLEngineResult result;
			try
			{
				result = engine.unwrap (encryptedInput, plainInput);
				if(DEBUG_LOG)
				{
					Logger.log ("SSLEngine unwrap: encryptedInput pos: " + encryptedInput.position () + " limit: " + encryptedInput.limit () + "  plainInput pos: " + plainInput.position () + " limit: " + plainInput.limit ());
					Logger.log ("result: " + result);
				}
			}
			catch(Exception e) 
			{
				Logger.logError ("SSLChannel.performUnwrap: engine.unwrap Exception: " + e.getMessage (), e);
				close ();
				return false;
			}

			encryptedInput.compact ();
			//Logger.log ("  compact: encryptedInput pos: " + encryptedInput.position () + " limit: " + encryptedInput.limit ());

			SSLEngineResult.Status status = result.getStatus ();
			switch(status)
			{
			case OK:
				checkHandshakeStatus (result.getHandshakeStatus ());
				done = true;
				break;

			case CLOSED:
				checkHandshakeStatus (result.getHandshakeStatus ());
				close ();
				return false;

			case BUFFER_UNDERFLOW:
				// enlarge buffer if necessary
				encryptedInput = adjustBuffer (encryptedInput, engine.getSession ().getPacketBufferSize ());

				// read more input data and try to unwrap again
				if(readInput () < 0)
					return false;
	
				break;

			case BUFFER_OVERFLOW:
				// enlarge buffer and try again
				plainInput = adjustBuffer (plainInput, engine.getSession ().getApplicationBufferSize ());
				break;

			default:
			}
		}
		return true;
	}

	private int flushOutput ()
	{
		encryptedOutput.flip ();
		
		if(DEBUG_LOG)
			Logger.log ("SSLChannel.flushOutput pos: " + encryptedOutput.position () +" limit: " + encryptedOutput.limit ());

		if(encryptedOutput.limit () <= 0)
			return 0;

		int bytesWritten = writeEncrypted (nativeContext, encryptedOutput.array (), encryptedOutput.limit ());
		encryptedOutput.clear ();
		return bytesWritten;
	}

	private int readInput ()
	{
		int start = encryptedInput.position ();
		int size = encryptedInput.remaining ();
		int bytesRead = readEncrypted (nativeContext, encryptedInput.array (), start, size);

		if(DEBUG_LOG)
			Logger.log ("SSLChannel.readInput: pos: " + start + " remaining: " + size + " -> " + bytesRead);
		
		if(bytesRead >= 0)
		{
			if(bytesRead > 0)
				encryptedInput.position (encryptedInput.position () + bytesRead);
		}
		else
		{
			try
			{
				engine.closeInbound ();
				close ();
			}
			catch(Exception e)
			{
				Logger.logError ("SSLChannel.readInput Exception: " + e.getMessage (),e );
				close ();
			}
		}
		return bytesRead;
	}
	
	private ByteBuffer adjustBuffer (ByteBuffer oldBuffer, int newRemaining)
	{
		if(oldBuffer.remaining () < newRemaining)
		{
			oldBuffer.flip ();
			ByteBuffer newBuffer = ByteBuffer.allocate (oldBuffer.remaining () + newRemaining);
			newBuffer.put (oldBuffer);
			return newBuffer;
		}
		else
			return oldBuffer;
	}
	
	public static native int writeEncrypted (long nativeContext, byte[] data, int size);
	public static native int readEncrypted (long nativeContext, byte[] data, int start, int size);
}
