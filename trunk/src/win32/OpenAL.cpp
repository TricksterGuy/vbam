// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 2007-2008 VBA-M development team

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or(at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ifndef NO_OAL


// === LOGALL writes very detailed informations to vba-trace.log ===
//#define LOGALL


#include "stdafx.h" // includes VBA.h for 'theApp.throttle'

// Interface
#include "Sound.h"

// OpenAL
#include <al.h>
#include <alc.h>
#include "LoadOAL.h"

// Windows
#include <windows.h> // for 'Sleep' function

// Internals
#include "../Sound.h"
#include "../Globals.h" // for 'speedup' and 'synchronize'

// Debug
#include <assert.h>
#define ASSERT_SUCCESS   assert( AL_NO_ERROR == ALFunction.alGetError() )

#ifndef LOGALL
// replace logging functions with comments
#define winlog //
#define debugState() //
#endif

class OpenAL : public ISound
{
public:
	OpenAL();
	virtual ~OpenAL();

	bool init();   // initialize the sound buffer queue
	void pause();  // pause the secondary sound buffer
	void reset();  // stop and reset the secondary sound buffer
	void resume(); // play/resume the secondary sound buffer
	void write();  // write the emulated sound to a sound buffer

private:
	OPENALFNTABLE  ALFunction;
	bool           initialized;
	bool           buffersLoaded;
	ALCdevice     *device;
	ALCcontext    *context;
	ALuint        *buffer;
	ALuint         tempBuffer;
	ALuint         source;
	int            freq;

#ifdef LOGALL
	void           debugState();
#endif
};


OpenAL::OpenAL()
{
	initialized = false;
	buffersLoaded = false;
	device = NULL;
	context = NULL;
	buffer = (ALuint*)malloc( theApp.oalBufferCount * sizeof( ALuint ) );
	memset( buffer, 0, theApp.oalBufferCount * sizeof( ALuint ) );
	tempBuffer = 0;
	source = 0;
}


OpenAL::~OpenAL()
{
	if( !initialized ) return;

	ALFunction.alSourceStop( source );
	ASSERT_SUCCESS;

	ALFunction.alSourcei( source, AL_BUFFER, 0 );
	ASSERT_SUCCESS;

	ALFunction.alDeleteSources( 1, &source );
	ASSERT_SUCCESS;

	ALFunction.alDeleteBuffers( theApp.oalBufferCount, buffer );
	ASSERT_SUCCESS;

	free( buffer );

	ALFunction.alcMakeContextCurrent( NULL );
	ASSERT_SUCCESS;

	ALFunction.alcDestroyContext( context );
	ASSERT_SUCCESS;

	ALFunction.alcCloseDevice( device );
	ASSERT_SUCCESS;
}

#ifdef LOGALL
void OpenAL::debugState()
{

	ALint value = 0;
	ALFunction.alGetSourcei( source, AL_SOURCE_STATE, &value );
	ASSERT_SUCCESS;

	winlog( " soundPaused = %i\n", soundPaused );
	winlog( " Source:\n" );
	winlog( "  State: " );
	switch( value )
	{
	case AL_INITIAL:
		winlog( "AL_INITIAL\n" );
		break;
	case AL_PLAYING:
		winlog( "AL_PLAYING\n" );
		break;
	case AL_PAUSED:
		winlog( "AL_PAUSED\n" );
		break;
	case AL_STOPPED:
		winlog( "AL_STOPPED\n" );
		break;
	default:
		winlog( "!unknown!\n" );
		break;
	}


	ALFunction.alGetSourcei( source, AL_BUFFERS_QUEUED, &value );
	ASSERT_SUCCESS;
	winlog( "  Buffers in queue: %i\n", value );

	ALFunction.alGetSourcei( source, AL_BUFFERS_PROCESSED, &value );
	ASSERT_SUCCESS;
	winlog( "  Buffers processed: %i\n", value );
}
#endif


bool OpenAL::init()
{
	winlog( "OpenAL::init\n" );
	assert( initialized == false );

	if( !LoadOAL10Library( NULL, &ALFunction ) ) {
		systemMessage( IDS_OAL_NODLL, "OpenAL32.dll could not be found on your system. Please install the runtime from http://openal.org" );
		return false;
	}

	if( theApp.oalDevice ) {
		device = ALFunction.alcOpenDevice( theApp.oalDevice );
	} else {
		device = ALFunction.alcOpenDevice( NULL );
	}
	assert( device != NULL );

	context = ALFunction.alcCreateContext( device, NULL );
	assert( context != NULL );

	ALCboolean retVal = ALFunction.alcMakeContextCurrent( context );
	assert( ALC_TRUE == retVal );

	ALFunction.alGenBuffers( theApp.oalBufferCount, buffer );
	ASSERT_SUCCESS;

	ALFunction.alGenSources( 1, &source );
	ASSERT_SUCCESS;

	freq = 44100 / soundQuality;

	// calculate the number of samples per frame first
	// then multiply it with the size of a sample frame (16 bit * stereo)
	soundBufferLen = ( freq / 60 ) * 4;


	setsystemSoundOn( true );
	initialized = true;
	return true;
}


void OpenAL::resume()
{
	if( !initialized ) return;
	winlog( "OpenAL::resume\n" );
	if( !buffersLoaded ) return;
	debugState();


	ALint sourceState = 0;
	ALFunction.alGetSourcei( source, AL_SOURCE_STATE, &sourceState );
	ASSERT_SUCCESS;
	if( sourceState != AL_PLAYING ) {
		ALFunction.alSourcePlay( source );
		ASSERT_SUCCESS;
	}
	debugState();
}


void OpenAL::pause()
{
	if( !initialized ) return;
	winlog( "OpenAL::pause\n" );
	if( !buffersLoaded ) return;
	debugState();


	ALint sourceState = 0;
	ALFunction.alGetSourcei( source, AL_SOURCE_STATE, &sourceState );
	ASSERT_SUCCESS;
	if( sourceState == AL_PLAYING ) {
		ALFunction.alSourcePause( source );
		ASSERT_SUCCESS;
	}
	debugState();
}


void OpenAL::reset()
{
	if( !initialized ) return;
	winlog( "OpenAL::reset\n" );
	if( !buffersLoaded ) return;
	debugState();

	ALint sourceState = 0;
	ALFunction.alGetSourcei( source, AL_SOURCE_STATE, &sourceState );
	ASSERT_SUCCESS;
	if( sourceState != AL_STOPPED ) {
		ALFunction.alSourceStop( source );
		ASSERT_SUCCESS;
	}
	debugState();
}


void OpenAL::write()
{
	if( !initialized ) return;
	winlog( "OpenAL::write\n" );

	debugState();

	ALint sourceState = 0;
	ALint nBuffersProcessed = 0;

	if( !buffersLoaded ) {
		// ==initial buffer filling==
		winlog( " initial buffer filling\n" );
		for( int i = 0 ; i < theApp.oalBufferCount ; i++ ) {
			// Filling the buffers explicitly with silence would be cleaner,
			// but the very first sample is usually silence anyway.
			ALFunction.alBufferData( buffer[i], AL_FORMAT_STEREO16, soundFinalWave, soundBufferLen, freq );
			ASSERT_SUCCESS;
		}

		ALFunction.alSourceQueueBuffers( source, theApp.oalBufferCount, buffer );
		ASSERT_SUCCESS;

		buffersLoaded = true;
	} else {
		// ==normal buffer refreshing==
		nBuffersProcessed = 0;
		ALFunction.alGetSourcei( source, AL_BUFFERS_PROCESSED, &nBuffersProcessed );
		ASSERT_SUCCESS;

		if( nBuffersProcessed == theApp.oalBufferCount ) {
			// we only want to know about it when we are emulating at full speed or faster:
			if( ( theApp.throttle >= 100 ) || ( theApp.throttle == 0 ) ) {
				if( systemVerbose & VERBOSE_SOUNDOUTPUT ) {
					static unsigned int i = 0;
					log( "OpenAL: Buffers were not refilled fast enough (i=%i)\n", i++ );
				}
			}
		}

		if( !speedup && synchronize && !theApp.throttle ) {
			// wait until at least one buffer has finished
			while( nBuffersProcessed == 0 ) {
				winlog( " waiting...\n" );
				// wait for about half the time one buffer needs to finish
				// unoptimized: ( sourceBufferLen * 1000 ) / ( freq * 2 * 2 ) * 1/2
				Sleep( soundBufferLen / ( freq >> 7 ) );
				ALFunction.alGetSourcei( source, AL_BUFFERS_PROCESSED, &nBuffersProcessed );
				ASSERT_SUCCESS;
			}
		} else {
			if( nBuffersProcessed == 0 ) return;
		}

		assert( nBuffersProcessed > 0 );

		// unqueue buffer
		tempBuffer = 0;
		ALFunction.alSourceUnqueueBuffers( source, 1, &tempBuffer );
		ASSERT_SUCCESS;

		// refill buffer
		ALFunction.alBufferData( tempBuffer, AL_FORMAT_STEREO16, soundFinalWave, soundBufferLen, freq );
		ASSERT_SUCCESS;

		// requeue buffer
		ALFunction.alSourceQueueBuffers( source, 1, &tempBuffer );
		ASSERT_SUCCESS;
	}

	// start playing the source if necessary
	ALFunction.alGetSourcei( source, AL_SOURCE_STATE, &sourceState );
	ASSERT_SUCCESS;
	if( !soundPaused && ( sourceState != AL_PLAYING ) ) {
		ALFunction.alSourcePlay( source );
		ASSERT_SUCCESS;
	}
}


ISound *newOpenAL()
{
	winlog( "newOpenAL\n" );
	return new OpenAL();
}

#endif
