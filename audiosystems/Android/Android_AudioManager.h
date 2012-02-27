/// @file
/// @author  Boris Mikic
/// @version 2.4
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
/// 
/// @section DESCRIPTION
/// 
/// Represents an implementation of the AudioManager for Android.

#ifdef HAVE_ANDROID
#ifndef XAL_ANDROID_AUDIO_MANAGER_H
#define XAL_ANDROID_AUDIO_MANAGER_H

#ifndef __APPLE__
#include <AL/al.h>
#include <AL/alc.h>
#else
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <TargetConditionals.h>
#endif

#include <hltypes/hstring.h>

#include "AudioManager.h"
#include "xalExport.h"

namespace xal
{
	class Buffer;
	class Android_Player;
	class Player;
	class Sound;

	class xalExport Android_AudioManager : public AudioManager
	{
	public:
		friend class Android_Player;

		Android_AudioManager(chstr systemName, void* backendId, bool threaded = false, float updateTime = 0.01f, chstr deviceName = "");
		~Android_AudioManager();

	protected:
		ALCdevice* device;
		ALCcontext* context;

		Player* _createSystemPlayer(Sound* sound, Buffer* buffer);
		unsigned int _allocateSourceId();
		void _releaseSourceId(unsigned int sourceId);

	};
	
}

#endif
#endif