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

#include "sound_file_loader.h"

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "sound.h"
#include "util.h"
#include "platform_util.h"
#include "byte_order.h"
#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace std;
using namespace Shared::Util;

namespace Shared{ namespace Sound{

// =====================================================
//	class WavSoundFileLoader
// =====================================================

void WavSoundFileLoader::open(const string &path, SoundInfo *soundInfo){
    char chunkId[]={'-', '-', '-', '-', '\0'};
    uint32 size32= 0;
    uint16 size16= 0; 
    int count;
    fileName = path;
	
	f.open(path.c_str(), ios_base::in | ios_base::binary);

	if(!f.is_open()){
		throw megaglest_runtime_error("Error opening wav file: "+ string(path));
	}

    //RIFF chunk - Id
    f.read(chunkId, 4);
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		for(unsigned int i = 0; i < 4; ++i) {
			chunkId[i] = Shared::PlatformByteOrder::fromCommonEndian(chunkId[i]);
		}
	}

	if(strcmp(chunkId, "RIFF")!=0){
		throw megaglest_runtime_error("Not a valid wav file (first four bytes are not RIFF):" + path);
	}

    //RIFF chunk - Size 
    f.read((char*) &size32, 4);
	if(bigEndianSystem == true) {
		size32 = Shared::PlatformByteOrder::fromCommonEndian(size32);
	}

    //RIFF chunk - Data (WAVE string)
    f.read(chunkId, 4);
	if(bigEndianSystem == true) {
		for(unsigned int i = 0; i < 4; ++i) {
			chunkId[i] = Shared::PlatformByteOrder::fromCommonEndian(chunkId[i]);
		}
	}
    
	if(strcmp(chunkId, "WAVE")!=0){
		throw megaglest_runtime_error("Not a valid wav file (wave data don't start by WAVE): " + path);
	}

    // === HEADER ===

    //first sub-chunk (header) - Id
    f.read(chunkId, 4);
	if(bigEndianSystem == true) {
		for(unsigned int i = 0; i < 4; ++i) {
			chunkId[i] = Shared::PlatformByteOrder::fromCommonEndian(chunkId[i]);
		}
	}
    
	if(strcmp(chunkId, "fmt ")!=0){
		throw megaglest_runtime_error("Not a valid wav file (first sub-chunk Id is not fmt): "+ path);
	}

    //first sub-chunk (header) - Size 
    f.read((char*) &size32, 4);
	if(bigEndianSystem == true) {
		size32 = Shared::PlatformByteOrder::fromCommonEndian(size32);
	}

    //first sub-chunk (header) - Data (encoding type) - Ignore
    f.read((char*) &size16, 2);
	if(bigEndianSystem == true) {
		size16 = Shared::PlatformByteOrder::fromCommonEndian(size16);
	}

    //first sub-chunk (header) - Data (nChannels)
    f.read((char*) &size16, 2);
	if(bigEndianSystem == true) {
		size16 = Shared::PlatformByteOrder::fromCommonEndian(size16);
	}

	soundInfo->setChannels(size16);

    //first sub-chunk (header) - Data (nsamplesPerSecond)
    f.read((char*) &size32, 4);
	if(bigEndianSystem == true) {
		size32 = Shared::PlatformByteOrder::fromCommonEndian(size32);
	}

	soundInfo->setsamplesPerSecond(size32);

    //first sub-chunk (header) - Data (nAvgBytesPerSec)  - Ignore
    f.read((char*) &size32, 4);
	if(bigEndianSystem == true) {
		size32 = Shared::PlatformByteOrder::fromCommonEndian(size32);
	}

    //first sub-chunk (header) - Data (blockAlign) - Ignore
    f.read((char*) &size16, 2);
	if(bigEndianSystem == true) {
		size16 = Shared::PlatformByteOrder::fromCommonEndian(size16);
	}

    //first sub-chunk (header) - Data (nsamplesPerSecond)
    f.read((char*) &size16, 2);
	if(bigEndianSystem == true) {
		size16 = Shared::PlatformByteOrder::fromCommonEndian(size16);
	}

	soundInfo->setBitsPerSample(size16);

	soundInfo->setBitRate(soundInfo->getSamplesPerSecond() * soundInfo->getChannels() * soundInfo->getBitsPerSample() / 8);

	if (soundInfo->getBitsPerSample() != 8 && soundInfo->getBitsPerSample()!=16){
		throw megaglest_runtime_error("Bits per sample must be 8 or 16: " + path);
	}
	bytesPerSecond= soundInfo->getBitsPerSample()*8*soundInfo->getSamplesPerSecond()*soundInfo->getChannels();

    count=0;
    do{
        count++;

        // === DATA ===
        //second sub-chunk (samples) - Id
        f.read(chunkId, 4);
    	if(bigEndianSystem == true) {
    		for(unsigned int i = 0; i < 4; ++i) {
    			chunkId[i] = Shared::PlatformByteOrder::fromCommonEndian(chunkId[i]);
    		}
    	}

		if(strncmp(chunkId, "data", 4)!=0){
			continue;
		}

        //second sub-chunk (samples) - Size
        f.read((char*) &size32, 4);
    	if(bigEndianSystem == true) {
   			size32 = Shared::PlatformByteOrder::fromCommonEndian(size32);
    	}

		dataSize= size32;
		soundInfo->setSize(dataSize);
    }
    while(strncmp(chunkId, "data", 4)!=0 && count<maxDataRetryCount);

	if(f.bad() || count==maxDataRetryCount){
		throw megaglest_runtime_error("Error reading samples: "+ path);
	}

	dataOffset= (uint32)f.tellg();

}

uint32 WavSoundFileLoader::read(int8 *samples, uint32 size){
	f.read(reinterpret_cast<char*> (samples), size);
	static bool bigEndianSystem = Shared::PlatformByteOrder::isBigEndian();
	if(bigEndianSystem == true) {
		Shared::PlatformByteOrder::toEndianTypeArray<int8>(samples,size);
	}

	return (uint32)f.gcount();
}

void WavSoundFileLoader::close(){
    f.close();
}

void WavSoundFileLoader::restart(){
	f.seekg(dataOffset, ios_base::beg);	
}

// =======================================
//        Ogg Sound File Loader
// =======================================

void OggSoundFileLoader::open(const string &path, SoundInfo *soundInfo){
	fileName = path;

#ifdef WIN32
	f = _wfopen(utf8_decode(path).c_str(), L"rb");
#else
	f= fopen(path.c_str(), "rb");
#endif
	if(f==NULL){
		throw megaglest_runtime_error("Can't open ogg file: "+path);
	}

	vf= new OggVorbis_File();
	if(vf==NULL) {
		throw megaglest_runtime_error("Can't create ogg object for file: "+path);
	}

	ov_open(f, vf, NULL, 0);

	vorbis_info *vi= ov_info(vf, -1);
	if(vi==NULL) {
		throw megaglest_runtime_error("Can't read ogg header info for file: "+path);
	}

	uint32 samples = static_cast<uint32>(ov_pcm_total(vf, -1));

	if(SystemFlags::getSystemSettingType(SystemFlags::debugSound).enabled) SystemFlags::OutputDebug(SystemFlags::debugSound,"In [%s::%s Line: %d] path = [%s] vi->version = %d, vi->channels = %d, vi->rate = %ld, vi->bitrate_upper = %ld, vi->bitrate_nominal = %ld, vi->bitrate_lower = %ld, vi->bitrate_window = %ld, samples = " MG_SIZE_T_SPECIFIER "\n",__FILE__,__FUNCTION__,__LINE__,path.c_str(),vi->version,vi->channels,vi->rate,vi->bitrate_upper,vi->bitrate_nominal,vi->bitrate_lower,vi->bitrate_window,samples);

	soundInfo->setChannels(vi->channels);
	soundInfo->setsamplesPerSecond(vi->rate);
	soundInfo->setBitsPerSample(16);
	soundInfo->setSize(samples * 2 * vi->channels);
	soundInfo->setBitRate(vi->bitrate_nominal);
}

uint32 OggSoundFileLoader::read(int8 *samples, uint32 size){
	int section;
	int totalBytesRead= 0;

	while(size>0) {
		int bytesRead= ov_read(vf, reinterpret_cast<char*> (samples), size,
							   0, 2, 1, &section);
		if(bytesRead==0) {
			break;
		}
		size-= bytesRead;
		samples+= bytesRead;
		totalBytesRead+= bytesRead; 
	}
	return totalBytesRead;
}

void OggSoundFileLoader::close() {
	if(vf != NULL) {
		ov_clear(vf);
		delete vf;
		vf= 0;
	}
	//fclose(f);
}

void OggSoundFileLoader::restart(){
	ov_raw_seek(vf, 0);
}

// =====================================================
//	class SoundFileLoaderFactory
// =====================================================

SoundFileLoaderFactory::SoundFileLoaderFactory(){
	registerClass<WavSoundFileLoader>("wav");
	registerClass<OggSoundFileLoader>("ogg");
}

SoundFileLoaderFactory *SoundFileLoaderFactory::getInstance(){
	static SoundFileLoaderFactory soundFileLoaderFactory;
	return &soundFileLoaderFactory;
}

}}//end namespace
