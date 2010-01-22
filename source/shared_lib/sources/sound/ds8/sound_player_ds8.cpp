// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "sound_player_ds8.h"

#include <cassert>
#include <cmath>

#include "util.h"
#include "leak_dumper.h"

namespace Shared{ namespace Sound{ namespace Ds8{

using namespace Util;

// =====================================================
//	class SoundBuffer
// =====================================================

// ===================== PUBLIC ========================

SoundBuffer::SoundBuffer(){
	dsBuffer= NULL;
	sound= NULL;
}

bool SoundBuffer::isFree(){
	if(dsBuffer==NULL){
		return true;
	}

	DWORD status;
	dsBuffer->GetStatus(&status);
    
    if(status & DSBSTATUS_BUFFERLOST){
        end();
        return true;
    }
    return false;
}

bool SoundBuffer::isReady(){
	DWORD status;
	dsBuffer->GetStatus(&status);

	if ((status & DSBSTATUS_PLAYING) || (status & DSBSTATUS_BUFFERLOST)){
        return false;
	}
    return true;
}

// ==================== PROTECTED ======================

void SoundBuffer::createDsBuffer(IDirectSound8 *dsObject){
	IDirectSoundBuffer *buffer;
	const SoundInfo *soundInfo= sound->getInfo();

	//format
	WAVEFORMATEX format;
    format.wFormatTag= WAVE_FORMAT_PCM;
    format.nChannels= soundInfo->getChannels();
    format.nSamplesPerSec= soundInfo->getSamplesPerSecond();
    format.nAvgBytesPerSec= (soundInfo->getBitsPerSample() * soundInfo->getSamplesPerSecond() * soundInfo->getChannels())/8;
    format.nBlockAlign= (soundInfo->getChannels() * soundInfo->getBitsPerSample())/8;
    format.wBitsPerSample= soundInfo->getBitsPerSample();
    format.cbSize= 0;

	//buffer desc
    DSBUFFERDESC dsBufferDesc; 
    memset(&dsBufferDesc, 0, sizeof(DSBUFFERDESC)); 
    dsBufferDesc.dwSize= sizeof(DSBUFFERDESC); 
    dsBufferDesc.dwFlags= DSBCAPS_CTRLVOLUME;
    dsBufferDesc.dwBufferBytes= size; 
    dsBufferDesc.lpwfxFormat= &format; 

	//create buffer
	HRESULT hr= dsObject->CreateSoundBuffer(&dsBufferDesc, &buffer, NULL);
	if (hr!=DS_OK){
        throw runtime_error("Failed to create direct sound buffer");
	}

	//query dx8 interface
	hr = buffer->QueryInterface(IID_IDirectSoundBuffer8, (void**) &dsBuffer);
    buffer->Release();
	if (hr!=S_OK){
        throw runtime_error("Failed to create direct sound 8 static buffer");
	}
}

// =====================================================
//	class StaticBuffer
// =====================================================

// ===================== PUBLIC ========================

void StaticSoundBuffer::init(IDirectSound8 *dsObject, Sound *sound){
	if(this->sound==NULL){
		this->sound= sound;
		this->size= sound->getInfo()->getSize();
		createDsBuffer(dsObject);
		dsBuffer->SetCurrentPosition(0);
		fillDsBuffer();
	}
}

void StaticSoundBuffer::end(){
	dsBuffer->Stop();
	dsBuffer->Release();
	dsBuffer= NULL;
	sound= NULL;
}

void StaticSoundBuffer::play(){
	dsBuffer->SetVolume(dsVolume(sound->getVolume()));
	dsBuffer->Play(0, 0, 0); 
}

// ===================== PRIVATE =======================

void StaticSoundBuffer::fillDsBuffer(){
    void * writePointer;
    unsigned long size;
	
	//lock
	HRESULT hr= dsBuffer->Lock(0, 0, &writePointer, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER);
	if (hr!=DS_OK){
        throw runtime_error("Failed to Lock direct sound buffer");
	}

	//copy memory
	memcpy(writePointer, getStaticSound()->getSamples(), size);

	//unlock
    hr= dsBuffer->Unlock(writePointer, size, NULL, 0);
	if (hr!=DS_OK){
        throw runtime_error("Failed to Unlock direct sound buffer");
	}
}

// =====================================================
//	class StrBuffer
// =====================================================

// ===================== PUBLIC ========================

StrSoundBuffer::StrSoundBuffer(){
	state= sFree;
}

void StrSoundBuffer::init(IDirectSound8 *dsObject, Sound *sound, uint32 strBufferSize){
	state= sStopped;
	if(this->sound==NULL){
		this->sound= sound;
		this->size= strBufferSize;
		createDsBuffer(dsObject);
		dsBuffer->SetCurrentPosition(0);
		fillDsBuffer();
	}
	else if(this->sound!=sound){
		this->sound= sound;
		this->size= strBufferSize;
		dsBuffer->SetCurrentPosition(0);
		fillDsBuffer();
	}
}

void StrSoundBuffer::end(){
	state= sFree;
	dsBuffer->Stop();
	dsBuffer->Release();
	dsBuffer= NULL;
	sound= NULL;
}

void StrSoundBuffer::play(int64 fadeOn){
	assert(state==sStopped);
	lastPlayCursor= 0;
	if(fadeOn==0){
		state= sPlaying;
		dsBuffer->SetVolume(dsVolume(sound->getVolume()));
	}
	else{
		this->fade= fadeOn;
		state= sFadingOn;
		chrono.start();
		dsBuffer->SetVolume(dsVolume(0.f));
	}
	dsBuffer->Play(0, 0, DSBPLAY_LOOPING); 
}

void StrSoundBuffer::update(){
	
	switch(state){
	case sFadingOn:
		if(chrono.getMillis()>fade){
			dsBuffer->SetVolume(dsVolume(sound->getVolume()));
			state= sPlaying;
		}
		else{
			dsBuffer->SetVolume(dsVolume(sound->getVolume()*(static_cast<float>(chrono.getMillis())/fade)));
		}
		refreshDsBuffer();
		break;

	case sFadingOff:
		if(chrono.getMillis()>fade){
			state= sStopped;
			dsBuffer->Stop();
		}
		else{
			dsBuffer->SetVolume(dsVolume(sound->getVolume()*(1.0f-static_cast<float>(chrono.getMillis())/fade)));
			refreshDsBuffer();
		}
		break;

	case sPlaying:
		dsBuffer->SetVolume(dsVolume(sound->getVolume()));
		refreshDsBuffer();
		break;

	default:
		break;
	}
}

void StrSoundBuffer::stop(int64 fadeOff){

	if(fadeOff==0){
		dsBuffer->Stop();
		state= sStopped;
	}
	else{
		this->fade= fadeOff;
		state= sFadingOff;
		chrono.start();
	}

}

// ===================== PRIVATE =======================

void StrSoundBuffer::fillDsBuffer(){
    void * writePointer;
    unsigned long size;

	//lock
	HRESULT hr= dsBuffer->Lock(0, 0, &writePointer, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER);
	if (hr!=DS_OK){
        throw runtime_error("Failed to Lock direct sound buffer");
	}

	//copy memory
	readChunk(writePointer, size);

	//unlock
	hr= dsBuffer->Unlock(writePointer, size, NULL, 0 );
	if (hr!=DS_OK){
        throw runtime_error("Failed to Unlock direct sound buffer");
	}
}


void StrSoundBuffer::refreshDsBuffer(){
    void *writePointer1, *writePointer2;
    DWORD size1, size2;
	DWORD playCursor;
	DWORD bytesToLock;

	DWORD status;
	dsBuffer->GetStatus(&status);
	assert(!(status & DSBSTATUS_BUFFERLOST));

	HRESULT hr= dsBuffer->GetCurrentPosition(&playCursor, NULL);
	if(hr!=DS_OK){
		throw runtime_error("Failed to Lock query play position");
	}

	//compute bytes to lock
	if(playCursor>=lastPlayCursor){
		bytesToLock= playCursor - lastPlayCursor;
	}
	else{
		bytesToLock= size - (lastPlayCursor - playCursor);
	}

	//copy data
	if(bytesToLock>0){

		//lock
		HRESULT hr=dsBuffer->Lock(lastPlayCursor, bytesToLock, &writePointer1, &size1, &writePointer2, &size2, 0);
		if (hr!=DS_OK){
			throw runtime_error("Failed to Lock direct sound buffer");
		}

		//copy memory
		assert(size1+size2==bytesToLock);

		readChunk(writePointer1, size1);
		readChunk(writePointer2, size2);

		//unlock
		hr= dsBuffer->Unlock(writePointer1, size1, writePointer2, size2);
		if (hr!=DS_OK){
			throw runtime_error("Failed to Unlock direct sound buffer");
		}
	}		

	lastPlayCursor= playCursor;
}

void StrSoundBuffer::readChunk(void *writePointer, uint32 size){
	StrSound *s= getStrSound();
	uint32 readSize= s->read(static_cast<int8*>(writePointer), size);
	if(readSize<size){
		StrSound *next= s->getNext()==NULL? s: s->getNext();
		next->restart();
		next->read(&static_cast<int8*>(writePointer)[readSize], size-readSize);
		next->setVolume(s->getVolume());
		sound= next;
	}
}

// =====================================================
//	class SoundPlayerDs8
// =====================================================

SoundPlayerDs8::SoundPlayerDs8(){
    dsObject= NULL;
}

void SoundPlayerDs8::init(const SoundPlayerParams *params){
    HRESULT hr;
    
	this->params= *params;

	//reserve memory for buffers
	staticSoundBuffers.resize(params->staticBufferCount);
	strSoundBuffers.resize(params->strBufferCount);

	//create object
    hr=DirectSoundCreate8(NULL, &dsObject, NULL);
	if (hr!=DS_OK){
        throw runtime_error("Can't create direct sound object");
	}
    
	//Set cooperative level
    hr= dsObject->SetCooperativeLevel(GetActiveWindow(), DSSCL_PRIORITY); 
	if (hr!=DS_OK){
        throw runtime_error("Can't set cooperative level of dsound");
	}
    
}

void SoundPlayerDs8::end(){
}

void SoundPlayerDs8::play(StaticSound *staticSound){
	int bufferIndex= -1;

    assert(staticSound!=NULL);
	
	//if buffer found, play the sound
	if (findStaticBuffer(staticSound, &bufferIndex)){
		staticSoundBuffers[bufferIndex].init(dsObject, staticSound);
		staticSoundBuffers[bufferIndex].play(); 
    }

}

void SoundPlayerDs8::play(StrSound *strSound, int64 fadeOn){
	int bufferIndex= -1;
	
	//play sound if buffer found
	if(findStrBuffer(strSound, &bufferIndex)){
		strSoundBuffers[bufferIndex].init(dsObject, strSound, params.strBufferSize);
		strSoundBuffers[bufferIndex].play(fadeOn);
	}
}

void SoundPlayerDs8::stop(StrSound *strSound, int64 fadeOff){
	//find the buffer with this sound and stop it
	for(int i= 0; i<params.strBufferCount; ++i){
		if(strSoundBuffers[i].getSound()==strSound){
			strSoundBuffers[i].stop(fadeOff);
		}
	}
}

void SoundPlayerDs8::stopAllSounds(){
	for(int i=0; i<params.strBufferCount; ++i){
		if(!strSoundBuffers[i].isFree()){
			strSoundBuffers[i].stop(0);
			strSoundBuffers[i].end();
		}
	}
	for(int i=0; i<params.staticBufferCount; ++i){
		if(!staticSoundBuffers[i].isFree()){
			staticSoundBuffers[i].end();
		}
	}
}

void SoundPlayerDs8::updateStreams(){
	for(int i=0; i<params.strBufferCount; ++i){
		strSoundBuffers[i].update();
	}
}

// ===================== PRIVATE =======================
			
bool SoundPlayerDs8::findStaticBuffer(Sound *sound, int *bufferIndex){
    bool bufferFound= false;

    assert(sound!=NULL);
	
    //1st: we try fo find a stopped buffer with the same sound
	for(int i=0; i<staticSoundBuffers.size(); ++i){
		if(sound==staticSoundBuffers[i].getSound() && staticSoundBuffers[i].isReady()){
			*bufferIndex= i;
			bufferFound= true;
			break;
		}
	}

    //2nd: we try to find a free buffer
    if(!bufferFound){
        for(uint32 i=0; i<staticSoundBuffers.size(); ++i){
			if(staticSoundBuffers[i].isFree()){
				*bufferIndex= i;
				bufferFound= true;
                break;
            }
        }
    }

    //3rd: we try to find a stopped buffer
    if(!bufferFound){
        for(int i=0; i<staticSoundBuffers.size(); ++i){
			if(staticSoundBuffers[i].isReady()){
				staticSoundBuffers[i].end();
				*bufferIndex= i;
				bufferFound= true;
                break;
            }
        }
    }

	return bufferFound;
}

bool SoundPlayerDs8::findStrBuffer(Sound *sound, int *bufferIndex){
    bool bufferFound= false;

    assert(sound!=NULL);
	assert(sound->getVolume()<=1.0f && sound->getVolume()>=0.0f);

    //We try to find a free or ready buffer
    if(!bufferFound){
        for(uint32 i=0; i<strSoundBuffers.size(); ++i){
			if(strSoundBuffers[i].isFree() || strSoundBuffers[i].isReady()){
				*bufferIndex= i;
				bufferFound= true;
                break;
            }
        }
    }

	return bufferFound;
}

// =====================================================
//	Misc
// =====================================================

long dsVolume(float floatVolume){
	float correctedVol= log10f(floatVolume*9.f+1.f);
	long vol= static_cast<long>(DSBVOLUME_MIN+correctedVol*(DSBVOLUME_MAX-DSBVOLUME_MIN));
	return clamp(vol, DSBVOLUME_MIN, DSBVOLUME_MAX);
}

}}}//end namespace
