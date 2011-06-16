/// @file
/// @author  Boris Mikic
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#if HAVE_SDL
#include <SDL/SDL.h>

#include "Buffer.h"
#include "SDL_AudioManager.h"
#include "SDL_Player.h"
#include "Sound.h"
#include "xal.h"

namespace xal
{
	SDL_Player::SDL_Player(Sound* sound, Buffer* buffer) : Player(sound, buffer), playing(false),
		position(0), currentGain(1.0f)
	{
	}

	SDL_Player::~SDL_Player()
	{
	}

	int SDL_Player::_getData(unsigned char** data, int size)
	{
		int streamSize = this->buffer->load(this->looping, size);
		*data = this->buffer->getStream();
		if (!this->sound->isStreamed())
		{
			*data = &(*data)[this->position];
			streamSize = hmin(streamSize, streamSize - this->position);
		}
		return hmin(size, streamSize);
	}

	void SDL_Player::_update(float k)
	{
		Player::_update(k);
		int size = this->buffer->getSize();
		if (this->position >= size)
		{
			if (this->looping)
			{
				this->position -= this->position / size * size;
			}
			else if (this->playing)
			{
				this->_sysStop();
			}
		}
	}

	void SDL_Player::mixAudio(unsigned char* stream, int length)
	{
		if (this->playing)
		{
			unsigned char* data;
			int size = hmin(this->_getData(&data, length), length);
			printf("%d\n", size);
			if (size > 0)
			{
				/*
				int result;
				SDL_AudioSpec format = ((SDL_AudioManager*)xal::mgr)->getFormat();
				int srcFormat = (this->buffer->getBitsPerSample() == 16 ? AUDIO_S16 : AUDIO_S8);
				int srcChannels = this->buffer->getChannels();
				int srcSamplingRate = this->buffer->getSamplingRate();
				if (srcFormat != format.format || srcChannels != format.channels || srcSamplingRate != format.freq)
				{
					SDL_AudioCVT cvt;
					xal::log(hsprintf("%d %d %d %d", srcChannels, srcSamplingRate, format.channels, format.freq));
					result = SDL_BuildAudioCVT(&cvt, srcFormat, srcChannels, srcSamplingRate, format.format, format.channels, format.freq);
					if (result == -1)
					{
						xal::log("ERROR: Could not build converter");
						return;
					}
					cvt.buf = (Uint8*)malloc(size * cvt.len_mult);
					memcpy(cvt.buf, data, size * sizeof(unsigned char));
					cvt.len = size;
					result = SDL_ConvertAudio(&cvt);
					if (result == -1)
					{
						xal::log("ERROR: Could not convert audio");
						return;
					}
					data = (unsigned char*)cvt.buf;
					size = cvt.len_cvt;
					SDL_MixAudio(stream, data, size, SDL_MIX_MAXVOLUME);
					free(cvt.buf);
				}
				else
				{
					SDL_MixAudio(stream, data, size, SDL_MIX_MAXVOLUME);
				}
				//*/
				SDL_MixAudio(stream, data, size, (int)(SDL_MIX_MAXVOLUME * this->currentGain));
				this->position += size;
			}
		}
	}

	float SDL_Player::_sysGetOffset()
	{
		return this->offset;
	}

	void SDL_Player::_sysSetOffset(float value)
	{
		this->offset = value;
	}

	bool SDL_Player::_sysPreparePlay()
	{
		return true;
	}

	void SDL_Player::_sysPrepareBuffer()
	{
		if (!this->sound->isStreamed())
		{
			this->buffer->load(this->looping, this->buffer->getSize());
		}
	}

	void SDL_Player::_sysUpdateGain()
	{
		this->currentGain = this->_calcGain();
	}

	void SDL_Player::_sysUpdateFadeGain()
	{
		this->currentGain = this->_calcFadeGain();
	}

	void SDL_Player::_sysPlay()
	{
		this->playing = true;
	}

	void SDL_Player::_sysStop()
	{
		this->playing = false;
		if (!this->paused)
		{
			this->position = 0;
			this->buffer->rewind();
		}
	}

}
#endif