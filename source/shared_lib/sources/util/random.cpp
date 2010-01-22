#include "random.h"

#include <cassert>

#include "leak_dumper.h"

namespace Shared{ namespace Util{

// =====================================================
//	class Random
// =====================================================

const int Random::m= 714025;
const int Random::a= 1366;
const int Random::b= 150889;

Random::Random(){
	lastNumber= 0;
}

void Random::init(int seed){
	lastNumber= seed % m;
}

int Random::rand(){
	lastNumber= (a*lastNumber + b) % m;
	return lastNumber;
}

int Random::randRange(int min, int max){
	assert(min<=max);
	int diff= max-min;
	int res= min + static_cast<int>(static_cast<float>(diff+1)*Random::rand() / m);
	assert(res>=min && res<=max);
	return res;
}

float Random::randRange(float min, float max){
	assert(min<=max);
	float rand01= static_cast<float>(Random::rand())/(m-1);
	float res= min+(max-min)*rand01;
	assert(res>=min && res<=max);
	return res;
}

}}//end namespace
