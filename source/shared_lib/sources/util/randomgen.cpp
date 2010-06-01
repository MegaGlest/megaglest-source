#include "randomgen.h"
#include <cassert>
#include "util.h"
#include "leak_dumper.h"

namespace Shared { namespace Util {

// =====================================================
//	class RandomGen
// =====================================================

const int RandomGen::m= 714025;
const int RandomGen::a= 1366;
const int RandomGen::b= 150889;

RandomGen::RandomGen(){

#ifdef USE_STREFLOP
	lastNumber = streflop::RandomInit(0); // streflop
#else
	lastNumber= 0;
#endif
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] lastNumber = %d\n",__FILE__,__FUNCTION__,__LINE__,lastNumber);
}

void RandomGen::init(int seed){

#ifdef USE_STREFLOP
	lastNumber = streflop::RandomInit(seed); // streflop
#else
	lastNumber= seed % m;
#endif

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] seed = %d, lastNumber = %d\n",__FILE__,__FUNCTION__,__LINE__,seed,lastNumber);
}

int RandomGen::rand() {
	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] lastNumber = %d\n",__FILE__,__FUNCTION__,__LINE__,lastNumber);

	lastNumber= ((a*lastNumber) + b) % m;

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] lastNumber = %d\n",__FILE__,__FUNCTION__,__LINE__,lastNumber);

	return lastNumber;
}

int RandomGen::randRange(int min, int max){
	assert(min<=max);

#ifdef USE_STREFLOP
	int res = streflop::Random<true, false, float>(min, max); // streflop
#else
	int diff= max-min;
	int res= min + static_cast<int>((static_cast<float>(diff+1)*RandomGen::rand()) / m);
#endif
	assert(res>=min && res<=max);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] min = %d, max = %d, res = %d\n",__FILE__,__FUNCTION__,__LINE__,min,max,res);

	return res;
}

float RandomGen::randRange(float min, float max){
	assert(min<=max);

#ifdef USE_STREFLOP
	float res = streflop::Random<true, false, float>(min, max, randomState); // streflop
#else
	float rand01= static_cast<float>(RandomGen::rand())/(m-1);
	float res= min+((max-min)*rand01);
#endif

	assert(res>=min && res<=max);

	//SystemFlags::OutputDebug(SystemFlags::debugSystem,"In [%s::%s Line: %d] min = %f, max = %f, res = %f\n",__FILE__,__FUNCTION__,__LINE__,min,max,res);

	return res;
}

}}//end namespace
