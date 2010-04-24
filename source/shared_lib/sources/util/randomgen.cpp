#include "randomgen.h"
#include <cassert>

#include "leak_dumper.h"

namespace Shared { namespace Util {

// =====================================================
//	class RandomGen
// =====================================================

const int RandomGen::m= 714025;
const int RandomGen::a= 1366;
const int RandomGen::b= 150889;

RandomGen::RandomGen(){
	lastNumber= 0;
}

void RandomGen::init(int seed){
	lastNumber= seed % m;
}

int RandomGen::rand(){
	lastNumber= (a*lastNumber + b) % m;
	return lastNumber;
}

int RandomGen::randRange(int min, int max){
	assert(min<=max);
	int diff= max-min;
	int res= min + static_cast<int>(static_cast<float>(diff+1)*RandomGen::rand() / m);
	assert(res>=min && res<=max);
	return res;
}

float RandomGen::randRange(float min, float max){
	assert(min<=max);
	float rand01= static_cast<float>(RandomGen::rand())/(m-1);
	float res= min+(max-min)*rand01;
	assert(res>=min && res<=max);
	return res;
}

}}//end namespace
