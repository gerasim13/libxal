/************************************************************************************\
This source file is part of the KS(X) audio library                                  *
For latest info, see http://code.google.com/p/libxal/                                *
**************************************************************************************
Copyright (c) 2010 Kresimir Spes (kreso@cateia.com), Boris Mikic                     *
*                                                                                    *
* This program is free software; you can redistribute it and/or modify it under      *
* the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php   *
\************************************************************************************/
#include <hltypes/hstring.h>

#include "AudioManager.h"
#include "StreamSound.h"
#include "Source.h"

#include <iostream>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#ifndef __APPLE__
#include <AL/al.h>
#include <AL/alc.h>
#else
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#include "Endianess.h"

namespace xal
{
/******* CONSTRUCT / DESTRUCT ******************************************/

	StreamSound::StreamSound(chstr filename, chstr category, chstr prefix) :
		bufferIndex(0), SoundBuffer(filename, category, prefix)
	{
		for (int i = 0; i < STREAM_BUFFER_COUNT; i++)
		{
			this->buffers[i] = 0;
		}
	}

	StreamSound::~StreamSound()
	{
		this->stopAll();
		for (int i = 0; i < STREAM_BUFFER_COUNT; i++)
		{
			if (this->buffers[i] != 0)
			{
				alDeleteBuffers(1, &this->buffers[i]);
			}
		}
		if (xal::mgr->isEnabled())
		{
			if (this->isOgg())
			{
				ov_clear(&this->oggStream);
			}
		}
	}
	
/******* METHODS *******************************************************/

	unsigned int StreamSound::getBuffer()
	{
		return this->buffers[this->bufferIndex];
	}
	
	void StreamSound::update(float k)
	{
		int queued;
		alGetSourcei(this->sourceId, AL_BUFFERS_QUEUED, &queued);
		if (queued == 0)
		{
			this->stop();
			this->_fillStartBuffers();
			return;
		}
		int count;
		alGetSourcei(this->sourceId, AL_BUFFERS_PROCESSED, &count);
		int bytes = 0;
		int result;
		if (count == 0)
		{
			return;
		}
		this->unqueueBuffers((this->bufferIndex + STREAM_BUFFER_COUNT - queued) % STREAM_BUFFER_COUNT, count);
		int i = 0;
		for (; i < count; i++)
		{
			result = this->_fillBuffer(this->buffers[(this->bufferIndex + i) % STREAM_BUFFER_COUNT]);
			if (result == 0)
			{
				break;
			}
			bytes += result;
		}
		if (bytes > 0)
		{
			this->queueBuffers(this->bufferIndex, i);
			if (count < STREAM_BUFFER_COUNT)
			{
				this->bufferIndex = (this->bufferIndex + i) % STREAM_BUFFER_COUNT;
			}
			else // underrun happened, sound was stopped
			{
				this->pause();
				this->play();
			}
		}
	}
	
	int StreamSound::_readStream(char* buffer, int size)
	{
		if (this->isOgg())
		{
			int section;
			return ov_read(&this->oggStream, buffer, size, 0, 2, 1, &section);
		}
		return 0;
	}
	
	void StreamSound::_resetStream()
	{
		if (this->isOgg())
		{
			ov_raw_seek(&this->oggStream, 0);
		}
	}
	
	int StreamSound::_fillBuffer(unsigned int buffer)
	{
		char data[STREAM_BUFFER_SIZE] = {0};
		int size = 0;
		int section;
		int result;
		while (size < STREAM_BUFFER_SIZE)
		{
			result = this->_readStream(&data[size], STREAM_BUFFER_SIZE - size);
			if (result > 0)
			{
				size += result;
			}
			else if (result == 0)
			{
				if (!this->isLooping())
				{
					break;
				}
				this->_resetStream();
			}
			else
			{
				xal::mgr->logMessage("Error while filling buffer for " + this->name);
			}
		}
		if (size > 0)
		{
#ifdef __BIG_ENDIAN__
			for (uint16_t* p = (uint16_t*)data; (char*)p < data + size; p++)
			{
				XAL_NORMALIZE_ENDIAN(*p);
			}
#endif	
			alBufferData(buffer, (this->vorbisInfo->channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
				data, size, this->vorbisInfo->rate);
		}
		return size;
	}
	
	void StreamSound::rewindStream()
	{
		this->unqueueBuffers();
		this->_fillStartBuffers();
	}

	void StreamSound::_fillStartBuffers()
	{
		this->_resetStream();
		for (int i = 0; i < STREAM_BUFFER_COUNT; i++)
		{
			this->_fillBuffer(this->buffers[i]);
		}
		this->bufferIndex = 0;
	}

	void StreamSound::queueBuffers(int index, int count)
	{
		if (index + count <= STREAM_BUFFER_COUNT)
		{
			alSourceQueueBuffers(this->sourceId, count, &this->buffers[index]);
		}
		else
		{
			alSourceQueueBuffers(this->sourceId, STREAM_BUFFER_COUNT - index, &this->buffers[index]);
			alSourceQueueBuffers(this->sourceId, count + index - STREAM_BUFFER_COUNT, this->buffers);
		}
	}
 
	void StreamSound::unqueueBuffers(int index, int count)
	{
		if (index + count <= STREAM_BUFFER_COUNT)
		{
			alSourceUnqueueBuffers(this->sourceId, count, &this->buffers[index]);
		}
		else
		{
			alSourceUnqueueBuffers(this->sourceId, STREAM_BUFFER_COUNT - index, &this->buffers[index]);
			alSourceUnqueueBuffers(this->sourceId, count + index - STREAM_BUFFER_COUNT, this->buffers);
		}
	}
 
	void StreamSound::queueBuffers()
	{
		int queued;
		alGetSourcei(this->sourceId, AL_BUFFERS_QUEUED, &queued);
		this->queueBuffers(this->bufferIndex, STREAM_BUFFER_COUNT - queued);
	}
 
	void StreamSound::unqueueBuffers()
	{
		int queued;
		alGetSourcei(this->sourceId, AL_BUFFERS_QUEUED, &queued);
		this->unqueueBuffers((this->bufferIndex + STREAM_BUFFER_COUNT - queued) % STREAM_BUFFER_COUNT, queued);
	}
 
	bool StreamSound::_loadOgg()
	{
		xal::mgr->logMessage("Loading ogg stream sound " + this->fileName);
		if (ov_fopen((char*)this->virtualFileName.c_str(), &this->oggStream) != 0)
		{
			xal::mgr->logMessage("Ogg: Error opening file!");
			return false;
		}
		alGenBuffers(STREAM_BUFFER_COUNT, this->buffers);
		this->vorbisInfo = ov_info(&this->oggStream, -1);
		unsigned long len = ov_pcm_total(&this->oggStream, -1) * this->vorbisInfo->channels * 2; // always 16 bit data
		this->duration = ((float)len) / (this->vorbisInfo->rate * this->vorbisInfo->channels * 2);
		int bytes;
		for (int i = 0; i < STREAM_BUFFER_COUNT; i++)
		{
			bytes = this->_fillBuffer(this->buffers[i]);
			if (bytes == 0)
			{
				alDeleteBuffers(STREAM_BUFFER_COUNT, this->buffers);
				this->buffers[0] = 0;
				xal::mgr->logMessage("Sound " + this->virtualFileName + " is too small to be streamed.");
				break;
			}
		}
		return true;
	}
	
}