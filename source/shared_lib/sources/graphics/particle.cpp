// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "math_wrapper.h"
#include "particle.h"

#include <stdexcept>
#include <cassert>
#include <algorithm>

#include "util.h"
#include "particle_renderer.h"
#include "math_util.h"
#include "platform_common.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Shared {
namespace Graphics {

// =====================================================
//	class ParticleSystem
// =====================================================

const bool checkMemory = false;
static map<void *,int> memoryObjectList;

ParticleSystem::ParticleSystem(int particleCount) {
	if(checkMemory) {
		printf("++ Create ParticleSystem [%p]\n",this);
		memoryObjectList[this]++;
	}

	//assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);

	//init particle vector
	blendMode= bmOne;
	//particles= new Particle[particleCount];
	particles.clear();
	//particles.reserve(particleCount);
	particles.resize(particleCount);

	state= sPlay;
	aliveParticleCount= 0;
	active= true;
	visible= true;

	//vars
	texture= NULL;
	particleObserver= NULL;

	//params
	this->particleCount= particleCount;
	//this->particleCount= particles.size();
	maxParticleEnergy= 250;
	varParticleEnergy= 50;
	pos= Vec3f(0.0f);
	color= Vec4f(1.0f);
	colorNoEnergy= Vec4f(0.0f);
	emissionRate= 15.0f;
	emissionState= 1.0f; // initialized with 1 because we must have at least one particle in the beginning!
	speed= 1.0f;
	teamcolorNoEnergy= false;
	teamcolorEnergy= false;
	alternations= 0;
}

ParticleSystem::~ParticleSystem(){
	if(checkMemory) {
		printf("-- Delete ParticleSystem [%p]\n",this);
		memoryObjectList[this]--;
		assert(memoryObjectList[this] == 0);
	}

	//delete [] particles;
	particles.clear();

	delete particleObserver;
	particleObserver = NULL;
}

// =============== VIRTUAL ======================

//updates all living particles and creates new ones
void ParticleSystem::update(){
	if(aliveParticleCount > (int) particles.size()){
		throw runtime_error("aliveParticleCount >= particles.size()");
	}

	if(state != sPause){
		for(int i= 0; i < aliveParticleCount; ++i){
			updateParticle(&particles[i]);

			if(deathTest(&particles[i])){

				//kill the particle
				killParticle(&particles[i]);

				//maintain alive particles at front of the array
				if(aliveParticleCount > 0){
					particles[i]= particles[aliveParticleCount];
				}
			}
		}

		if(state != ParticleSystem::sFade){
			emissionState= emissionState + emissionRate;
			int emissionIntValue= (int) emissionState;
			for(int i= 0; i < emissionIntValue; i++){
				Particle *p= createParticle();
				initParticle(p, i);
			}
			emissionState= emissionState - (float) emissionIntValue;
		}
	}
}

void ParticleSystem::render(ParticleRenderer *pr, ModelRenderer *mr){
	if(active){
		pr->renderSystem(this);
	}
}

ParticleSystem::BlendMode ParticleSystem::strToBlendMode(const string &str){
	if(str == "normal"){
		return bmOne;
	}
	else if(str == "black"){
		return bmOneMinusAlpha;
	}
	else{
		throw runtime_error("Unknown particle mode: " + str);
	}
}

// =============== SET ==========================

void ParticleSystem::setState(State state){
	this->state= state;
	for(int i=getChildCount()-1; i>=0; i--)
		getChild(i)->setState(state);
}

void ParticleSystem::setTexture(Texture *texture){
	this->texture= texture;
}

void ParticleSystem::setPos(Vec3f pos){
	this->pos= pos;
	for(int i=getChildCount()-1; i>=0; i--)
		getChild(i)->setPos(pos);
}

void ParticleSystem::setColor(Vec4f color){
	this->color= color;
}

void ParticleSystem::setColorNoEnergy(Vec4f colorNoEnergy){
	this->colorNoEnergy= colorNoEnergy;
}

void ParticleSystem::setEmissionRate(float emissionRate){
	this->emissionRate= emissionRate;
}

void ParticleSystem::setMaxParticleEnergy(int maxParticleEnergy){
	this->maxParticleEnergy= maxParticleEnergy;
}

void ParticleSystem::setVarParticleEnergy(int varParticleEnergy){
	this->varParticleEnergy= varParticleEnergy;
}

void ParticleSystem::setParticleSize(float particleSize){
	this->particleSize= particleSize;
}

void ParticleSystem::setSpeed(float speed){
	this->speed= speed;
}

void ParticleSystem::setActive(bool active){
	this->active= active;
	for(int i=getChildCount()-1; i>=0; i--)
		getChild(i)->setActive(active);
}

void ParticleSystem::setObserver(ParticleObserver *particleObserver){
	this->particleObserver= particleObserver;
}

ParticleSystem* ParticleSystem::getChild(int i){
	throw std::out_of_range("ParticleSystem::getChild bad");
}

void ParticleSystem::setVisible(bool visible){
	this->visible= visible;
	for(int i=getChildCount()-1; i>=0; i--)
		getChild(i)->setVisible(visible);
}

// =============== MISC =========================
void ParticleSystem::fade(){
	if(particleObserver != NULL){
		if(state != sPlay) {
			char szBuf[4096]="";
			sprintf(szBuf,"state != sPlay, state = [%d]",state);
			//throw runtime_error(szBuf);
			//printf(szBuf);
			SystemFlags::OutputDebug(SystemFlags::debugError,"%s",szBuf);
		}
		assert(state == sPlay);
	}
	state= sFade;
	if(particleObserver != NULL){
		particleObserver->update(this);
	}
	for(int i=getChildCount()-1; i>=0; i--)
		getChild(i)->fade();
}

int ParticleSystem::isEmpty() const{
	assert(aliveParticleCount>=0);
	return aliveParticleCount == 0 && state != sPause;
}

// =============== PROTECTED =========================

// if there is one dead particle it returns it else, return the particle with 
// less energy
Particle * ParticleSystem::createParticle(){

	//if any dead particles
	if(aliveParticleCount < particleCount){
		++aliveParticleCount;
		return &particles[aliveParticleCount - 1];
	}

	//if not
	int minEnergy= particles[0].energy;
	int minEnergyParticle= 0;

	for(int i= 0; i < particleCount; ++i){
		if(particles[i].energy < minEnergy){
			minEnergy= particles[i].energy;
			minEnergyParticle= i;
		}
	}
	return &particles[minEnergyParticle];

	/*
	 //if any dead particles
	 if(aliveParticleCount < particleCount) {
	 ++aliveParticleCount;
	 return &particles[aliveParticleCount-1];
	 }

	 //if not
	 int minEnergy = particles[0].energy;
	 int minEnergyParticle = 0;

	 for(int i = 0; i < particleCount; ++i){
	 if(particles[i].energy < minEnergy){
	 minEnergy = particles[i].energy;
	 minEnergyParticle = i;
	 }
	 }

	 return &particles[minEnergyParticle];
	 */
}

void ParticleSystem::initParticle(Particle *p, int particleIndex){
	p->pos= pos;
	p->lastPos= p->pos;
	p->speed= Vec3f(0.0f);
	p->accel= Vec3f(0.0f);
	p->color= Vec4f(1.0f, 1.0f, 1.0f, 1.0);
	p->size= particleSize;
	p->energy= maxParticleEnergy + random.randRange(-varParticleEnergy, varParticleEnergy);
}

void ParticleSystem::updateParticle(Particle *p){
	p->lastPos= p->pos;
	p->pos= p->pos + p->speed;
	p->speed= p->speed + p->accel;
	p->energy--;
}

bool ParticleSystem::deathTest(Particle *p){
	return p->energy <= 0;
}

void ParticleSystem::killParticle(Particle *p){
	aliveParticleCount--;
}

void ParticleSystem::setFactionColor(Vec3f factionColor){
	this->factionColor= factionColor;
	Vec3f tmpCol;

	if(teamcolorEnergy){
		this->color= Vec4f(factionColor.x, factionColor.y, factionColor.z, this->color.w);
	}
	if(teamcolorNoEnergy){
		this->colorNoEnergy= Vec4f(factionColor.x, factionColor.y, factionColor.z, this->colorNoEnergy.w);
	}
	for(int i=getChildCount()-1; i>=0; i--)
		getChild(i)->setFactionColor(factionColor);
}

// ===========================================================================
//  FireParticleSystem
// ===========================================================================


FireParticleSystem::FireParticleSystem(int particleCount) :
	ParticleSystem(particleCount){

	radius= 0.5f;
	speed= 0.01f;
	windSpeed= Vec3f(0.0f);

	setParticleSize(0.6f);
	setColorNoEnergy(Vec4f(1.0f, 0.5f, 0.0f, 1.0f));
}

void FireParticleSystem::initParticle(Particle *p, int particleIndex){
	ParticleSystem::initParticle(p, particleIndex);

	float ang= random.randRange(-2.0f * pi, 2.0f * pi);
#ifdef USE_STREFLOP
	float mod= streflop::fabsf(random.randRange(-radius, radius));

	float x= streflop::sinf(ang)*mod;
	float y= streflop::cosf(ang)*mod;

	float radRatio= streflop::sqrtf(streflop::sqrtf(mod/radius));
#else
	float mod= fabsf(random.randRange(-radius, radius));

	float x= sinf(ang) * mod;
	float y= cosf(ang) * mod;

	float radRatio= sqrtf(sqrtf(mod / radius));
#endif

	p->color= colorNoEnergy * 0.5f + colorNoEnergy * 0.5f * radRatio;
	p->energy= static_cast<int> (maxParticleEnergy * radRatio)
	        + random.randRange(-varParticleEnergy, varParticleEnergy);
	p->pos= Vec3f(pos.x + x, pos.y + random.randRange(-radius / 2, radius / 2), pos.z + y);
	p->lastPos= pos;
	p->size= particleSize;
	p->speed= Vec3f(0, speed + speed * random.randRange(-0.5f, 0.5f), 0) + windSpeed;
}

void FireParticleSystem::updateParticle(Particle *p){
	p->lastPos= p->pos;
	p->pos= p->pos + p->speed;
	p->energy--;

	if(p->color.x > 0.0f)
		p->color.x*= 0.98f;
	if(p->color.y > 0.0f)
		p->color.y*= 0.98f;
	if(p->color.w > 0.0f)
		p->color.w*= 0.98f;

	p->speed.x*= 1.001f;

}

// ================= SET PARAMS ====================

void FireParticleSystem::setRadius(float radius){
	this->radius= radius;
}

void FireParticleSystem::setWind(float windAngle, float windSpeed){
#ifdef USE_STREFLOP
	this->windSpeed.x= streflop::sinf(degToRad(windAngle))*windSpeed;
	this->windSpeed.y= 0.0f;
	this->windSpeed.z= streflop::cosf(degToRad(windAngle))*windSpeed;
#else
	this->windSpeed.x= sinf(degToRad(windAngle)) * windSpeed;
	this->windSpeed.y= 0.0f;
	this->windSpeed.z= cosf(degToRad(windAngle)) * windSpeed;
#endif
}

// ===========================================================================
//  GameParticleSystem
// ===========================================================================

GameParticleSystem::GameParticleSystem(int particleCount):
	ParticleSystem(particleCount),
	primitive(pQuad),
	model(NULL),
	modelCycle(0.0f),
	tween(0.0f),
	offset(0.0f),
	direction(0.0f, 1.0f, 0.0f)
{}

GameParticleSystem::~GameParticleSystem(){
	for(Children::iterator it= children.begin(); it != children.end(); ++it){
		(*it)->setParent(NULL);
		(*it)->fade();
	}
}

GameParticleSystem::Primitive GameParticleSystem::strToPrimitive(const string &str){
	if(str == "quad"){
		return pQuad;
	}
	else if(str == "line"){
		return pLine;
	}
	else{
		throw runtime_error("Unknown particle primitive: " + str);
	}
}

int GameParticleSystem::getChildCount(){
	return children.size();
}

ParticleSystem* GameParticleSystem::getChild(int i){
	return children.at(i); // does bounds checking
}

void GameParticleSystem::addChild(UnitParticleSystem* child) {
	assert(!child->getParent());
	child->setParent(this);
	children.push_back(child);
}

void GameParticleSystem::removeChild(UnitParticleSystem* child){
	assert(this == child->getParent());
	Children::iterator it = std::find(children.begin(),children.end(),child);
	assert(it != children.end());
	children.erase(it);
}

void GameParticleSystem::setPos(Vec3f pos){
	this->pos= pos;
	positionChildren();
}

void GameParticleSystem::positionChildren() {
	Vec3f child_pos = pos - offset;
	for(int i=getChildCount()-1; i>=0; i--)
		getChild(i)->setPos(child_pos);
}

void GameParticleSystem::setOffset(Vec3f offset){
	this->offset= offset;
	positionChildren();
}

void GameParticleSystem::render(ParticleRenderer *pr, ModelRenderer *mr){
	if(active){
		if(model != NULL){
			pr->renderModel(this, mr);
		}
		switch(primitive){
			case pQuad:
				pr->renderSystem(this);
				break;
			case pLine:
				pr->renderSystemLine(this);
				break;
			default:
				assert(false);
		}
	}
}

void GameParticleSystem::setTween(float relative,float absolute) {
	if(model){
		// animation?
		if(modelCycle == 0.0f) {
			tween= relative;
		}
		else {
		#ifdef USE_STREFLOP
			if(streflop::fabs(absolute) <= 0.00001f){
		#else
			if(fabs(absolute) <= 0.00001f){
		#endif
				tween = 0.0f;
			}
			else {
		#ifdef USE_STREFLOP
				tween= streflop::fmod(absolute, modelCycle);
		#else
				tween= fmod(absolute, modelCycle);
		#endif
				tween /= modelCycle;
			}
		}

		truncateDecimal<float>(tween);
		if(tween < 0.0f || tween > 1.0f) {
			//printf("In [%s::%s Line: %d] WARNING setting tween to [%f] clamping tween, modelCycle [%f] absolute [%f] relative [%f]\n",__FILE__,__FUNCTION__,__LINE__,tween,modelCycle,absolute,relative);
			//assert(tween >= 0.0f && tween <= 1.0f);
		}

		tween= clamp(tween, 0.0f, 1.0f);
	}
	for(Children::iterator it= children.begin(); it != children.end(); ++it)
		(*it)->setTween(relative,absolute);
}

// ===========================================================================
//  UnitParticleSystem
// ===========================================================================
bool UnitParticleSystem::isNight= false;

UnitParticleSystem::UnitParticleSystem(int particleCount):
	GameParticleSystem(particleCount),
	parent(NULL){
	radius= 0.5f;
	speed= 0.01f;
	windSpeed= Vec3f(0.0f);

	setParticleSize(0.6f);
	setColorNoEnergy(Vec4f(1.0f, 0.5f, 0.0f, 1.0f));

	primitive= pQuad;
	gravity= 0.0f;

	fixed= false;
	rotation= 0.0f;
	relativeDirection= true;
	relative= false;
	staticParticleCount= 0;

	isVisibleAtNight= true;
	isVisibleAtDay= true;

	cRotation= Vec3f(1.0f, 1.0f, 1.0f);
	fixedAddition= Vec3f(0.0f, 0.0f, 0.0f);
	//prepare system for given staticParticleCount
	if(staticParticleCount > 0){
		emissionState= (float) staticParticleCount;
	}
	energyUp= false;
	
	delay = 0; // none
	lifetime = -1; // forever

	startTime = 0;
	endTime = 1;

	radiusBasedStartenergy = false;
}

UnitParticleSystem::~UnitParticleSystem(){
	if(parent){
		parent->removeChild(this);
	}
}

bool UnitParticleSystem::getVisible() const{
	if((isNight==true) && (isVisibleAtNight==true)){
		return visible;
	}
	else if((isNight==false) && (isVisibleAtDay==true)){
		return visible;
	}
	else return false;
}

void UnitParticleSystem::render(ParticleRenderer *pr, ModelRenderer *mr) {
	GameParticleSystem::render(pr,mr);
}

void UnitParticleSystem::setRotation(float rotation){
	this->rotation= rotation;
	for(Children::iterator it= children.begin(); it != children.end(); ++it)
		(*it)->setRotation(rotation);
}

void UnitParticleSystem::fade(){
	if(!parent || (lifetime<=0 && !(emissionRateFade && emissionRate > 0))){ // particle has its own lifetime?
		GameParticleSystem::fade();
	}
}

UnitParticleSystem::Shape UnitParticleSystem::strToShape(const string& str){
	if(str == "spherical"){
		return sSpherical;
	}
	else if(str == "conical"){
		return sConical;
	}
	else if(str == "linear"){
		return sLinear;
	}
	else{
		throw runtime_error("Unkown particle shape: " + str);
	}
}

void UnitParticleSystem::initParticle(Particle *p, int particleIndex){
	ParticleSystem::initParticle(p, particleIndex);

	const float ang= random.randRange(-2.0f * pi, 2.0f * pi);
#ifdef USE_STREFLOP	
	const float mod= streflop::fabsf(random.randRange(-radius, radius));
	const float radRatio= streflop::sqrtf(streflop::sqrtf(mod/radius));
#else
	const float mod= fabsf(random.randRange(-radius, radius));
	const float radRatio= sqrtf(sqrtf(mod / radius));
#endif
	p->color= color;
	if(radiusBasedStartenergy == true){
		p->energy= static_cast<int> (maxParticleEnergy * radRatio) + random.randRange(-varParticleEnergy,
		        varParticleEnergy);
	}
	else{
		p->energy= static_cast<int> (maxParticleEnergy) + random.randRange(-varParticleEnergy, varParticleEnergy);
	}

	p->lastPos= pos;
	oldPosition= pos;
	p->size= particleSize;
	p->accel= Vec3f(0.0f, -gravity, 0.0f);
	
	// work out where we start for our shape (set speed and pos)
	switch(shape){
	case sSpherical:
		angle = (float)random.randRange(0,360);
		// fall through
	case sConical:{
		Vec2f horiz = Vec2f(1,0).rotate(ang);
		Vec2f vert = Vec2f(1,0).rotate(degToRad(angle));
		Vec3f start = Vec3f(horiz.x*vert.y,vert.x,horiz.y).getNormalized(); // close enough
		p->speed = start * speed;
		start = start * random.randRange(minRadius,radius);
		p->pos = pos + offset + start;
	} break;
	case sLinear:{
	#ifdef USE_STREFLOP	
		float x= streflop::sinf(ang)*mod;
		float y= streflop::cosf(ang)*mod;	
	#else	
		float x= sinf(ang) * mod;
		float y= cosf(ang) * mod;	
	#endif
		const float rad= degToRad(rotation);
		if(!relative){
			p->pos= Vec3f(pos.x + x + offset.x, pos.y + random.randRange(-radius / 2, radius / 2) + offset.y, pos.z + y
				+ offset.z);
		}
		else{// rotate it according to rotation		
	#ifdef USE_STREFLOP
			p->pos= Vec3f(pos.x+x+offset.z*streflop::sinf(rad)+offset.x*streflop::cosf(rad), pos.y+random.randRange(-radius/2, radius/2)+offset.y, pos.z+y+(offset.z*streflop::cosf(rad)-offset.x*streflop::sinf(rad)));
	#else
			p->pos= Vec3f(pos.x + x + offset.z * sinf(rad) + offset.x * cosf(rad), pos.y + random.randRange(-radius / 2,
				radius / 2) + offset.y, pos.z + y + (offset.z * cosf(rad) - offset.x * sinf(rad)));
	#endif
		}
		p->speed= Vec3f(direction.x + direction.x * random.randRange(-0.5f, 0.5f), direction.y + direction.y
			* random.randRange(-0.5f, 0.5f), direction.z + direction.z * random.randRange(-0.5f, 0.5f));
		p->speed= p->speed * speed;
		if(relative && relativeDirection){
	#ifdef USE_STREFLOP
			p->speed=Vec3f(p->speed.z*streflop::sinf(rad)+p->speed.x*streflop::cosf(rad),p->speed.y,(p->speed.z*streflop::cosf(rad)-p->speed.x*streflop::sinf(rad)));
	#else
			p->speed= Vec3f(p->speed.z * sinf(rad) + p->speed.x * cosf(rad), p->speed.y, (p->speed.z * cosf(rad)
				- p->speed.x * sinf(rad)));
	#endif
		}
	} break;
	default: throw runtime_error("bad shape");
	}
}

void UnitParticleSystem::update(){
	// delay and timeline are only applicable for child particles
	if(parent && delay>0 && delay--){
		return;
	}
	if(parent && lifetime>0 && !--lifetime) {
		fade();
	}
	if(state != sPause) {
		emissionRate-= emissionRateFade;
		if(parent && emissionRate < 0.0f) {
			fade();
		}
	}
	if(fixed){
		fixedAddition= Vec3f(pos.x - oldPosition.x, pos.y - oldPosition.y, pos.z - oldPosition.z);
		oldPosition= pos;
	}
	ParticleSystem::update();
}

void UnitParticleSystem::updateParticle(Particle *p){
	float energyRatio;
	if(alternations > 0){
		int interval= (maxParticleEnergy / alternations);
		float moduloValue= static_cast<int> (static_cast<float> (p->energy)) % interval;

		if(moduloValue < interval / 2){
			energyRatio= (interval - moduloValue) / interval;
		}
		else{
			energyRatio= moduloValue / interval;
		}
		energyRatio= clamp(energyRatio, 0.f, 1.f);
	}
	else{
		energyRatio= clamp(static_cast<float> (p->energy) / maxParticleEnergy, 0.f, 1.f);
	}

	p->lastPos+= p->speed;
	p->pos+= p->speed;
	if(fixed){
		p->lastPos+= fixedAddition;
		p->pos+= fixedAddition;
	}
	p->speed+= p->accel;
	p->color= color * energyRatio + colorNoEnergy * (1.0f - energyRatio);
	p->size= particleSize * energyRatio + sizeNoEnergy * (1.0f - energyRatio);
	if(state == ParticleSystem::sFade || staticParticleCount < 1){
		p->energy--;
	}
	else{
		if(maxParticleEnergy > 2){
			if(energyUp){
				p->energy++;
			}
			else{
				p->energy--;
			}

			if(p->energy == 1){
				energyUp= true;
			}
			if(p->energy == maxParticleEnergy){
				energyUp= false;
			}
		}
	}
}

// ================= SET PARAMS ====================

void UnitParticleSystem::setWind(float windAngle, float windSpeed){
#ifdef USE_STREFLOP
	this->windSpeed.x= streflop::sinf(degToRad(windAngle))*windSpeed;
	this->windSpeed.y= 0.0f;
	this->windSpeed.z= streflop::cosf(degToRad(windAngle))*windSpeed;
#else
	this->windSpeed.x= sinf(degToRad(windAngle)) * windSpeed;
	this->windSpeed.y= 0.0f;
	this->windSpeed.z= cosf(degToRad(windAngle)) * windSpeed;
#endif
}

// ===========================================================================
//  RainParticleSystem
// ===========================================================================


RainParticleSystem::RainParticleSystem(int particleCount) :
	ParticleSystem(particleCount){
	setWind(0.0f, 0.0f);
	setRadius(20.0f);

	setEmissionRate(25.0f);
	setParticleSize(3.0f);
	setColor(Vec4f(0.5f, 0.5f, 0.5f, 0.3f));
	setSpeed(0.2f);
}

void RainParticleSystem::render(ParticleRenderer *pr, ModelRenderer *mr){
	pr->renderSystemLineAlpha(this);
}

void RainParticleSystem::initParticle(Particle *p, int particleIndex){
	ParticleSystem::initParticle(p, particleIndex);

	float x= random.randRange(-radius, radius);
	float y= random.randRange(-radius, radius);

	p->color= color;
	p->energy= 10000;
	p->pos= Vec3f(pos.x + x, pos.y, pos.z + y);
	p->lastPos= p->pos;
	p->speed= Vec3f(random.randRange(-speed / 10, speed / 10), -speed, random.randRange(-speed / 10, speed / 10))
	        + windSpeed;
}

bool RainParticleSystem::deathTest(Particle *p){
	return p->pos.y < 0;
}

void RainParticleSystem::setRadius(float radius){
	this->radius= radius;

}

void RainParticleSystem::setWind(float windAngle, float windSpeed){
#ifdef USE_STREFLOP
	this->windSpeed.x= streflop::sinf(degToRad(windAngle))*windSpeed;
	this->windSpeed.y= 0.0f;
	this->windSpeed.z= streflop::cosf(degToRad(windAngle))*windSpeed;
#else
	this->windSpeed.x= sinf(degToRad(windAngle)) * windSpeed;
	this->windSpeed.y= 0.0f;
	this->windSpeed.z= cosf(degToRad(windAngle)) * windSpeed;
#endif
}

// ===========================================================================
//  SnowParticleSystem
// ===========================================================================

SnowParticleSystem::SnowParticleSystem(int particleCount) :
	ParticleSystem(particleCount){
	setWind(0.0f, 0.0f);
	setRadius(30.0f);

	setEmissionRate(2.0f);
	setParticleSize(0.2f);
	setColor(Vec4f(0.8f, 0.8f, 0.8f, 0.8f));
	setSpeed(0.05f);
}

void SnowParticleSystem::initParticle(Particle *p, int particleIndex){

	ParticleSystem::initParticle(p, particleIndex);

	float x= random.randRange(-radius, radius);
	float y= random.randRange(-radius, radius);

	p->color= color;
	p->energy= 10000;
	p->pos= Vec3f(pos.x + x, pos.y, pos.z + y);
	p->lastPos= p->pos;
	p->speed= Vec3f(0.0f, -speed, 0.0f) + windSpeed;
	p->speed.x+= random.randRange(-0.005f, 0.005f);
	p->speed.y+= random.randRange(-0.005f, 0.005f);
}

bool SnowParticleSystem::deathTest(Particle *p){
	return p->pos.y < 0;
}

void SnowParticleSystem::setRadius(float radius){
	this->radius= radius;
}

void SnowParticleSystem::setWind(float windAngle, float windSpeed){
#ifdef USE_STREFLOP
	this->windSpeed.x= streflop::sinf(degToRad(windAngle))*windSpeed;
	this->windSpeed.y= 0.0f;
	this->windSpeed.z= streflop::cosf(degToRad(windAngle))*windSpeed;
#else
	this->windSpeed.x= sinf(degToRad(windAngle)) * windSpeed;
	this->windSpeed.y= 0.0f;
	this->windSpeed.z= cosf(degToRad(windAngle)) * windSpeed;
#endif
}

// ===========================================================================
//  AttackParticleSystem
// ===========================================================================

AttackParticleSystem::AttackParticleSystem(int particleCount) :
	GameParticleSystem(particleCount){
	primitive= pQuad;
	gravity= 0.0f;
}

// ===========================================================================
//  ProjectileParticleSystem
// ===========================================================================

ProjectileParticleSystem::ProjectileParticleSystem(int particleCount) :
	AttackParticleSystem(particleCount){
	setEmissionRate(20.0f);
	setColor(Vec4f(1.0f, 0.3f, 0.0f, 0.5f));
	setMaxParticleEnergy(100);
	setVarParticleEnergy(50);
	setParticleSize(0.4f);
	setSpeed(0.14f);

	trajectory= tLinear;
	trajectorySpeed= 1.0f;
	trajectoryScale= 1.0f;
	trajectoryFrequency= 1.0f;
	modelCycle=0.0f;

	nextParticleSystem= NULL;
}

ProjectileParticleSystem::~ProjectileParticleSystem(){
	if(nextParticleSystem != NULL){
		nextParticleSystem->prevParticleSystem= NULL;
	}
}

void ProjectileParticleSystem::link(SplashParticleSystem *particleSystem){
	nextParticleSystem= particleSystem;
	nextParticleSystem->setVisible(false);
	nextParticleSystem->setState(sPause);
	nextParticleSystem->prevParticleSystem= this;
}

void ProjectileParticleSystem::update(){
	if(state == sPlay){

		lastPos= pos;
		flatPos+= zVector * trajectorySpeed;
		Vec3f targetVector= endPos - startPos;
		Vec3f currentVector= flatPos - startPos;

		// ratio
		float relative= clamp(currentVector.length() / targetVector.length(), 0.0f, 1.0f);
#ifdef USE_STREFLOP
		float absolute= clamp(streflop::fabs(currentVector.length()), 0.0f, 1.0f);
#else
		float absolute= clamp(fabs(currentVector.length()), 0.0f, 1.0f);
#endif
		setTween(relative,absolute);

		// trajectory
		switch(trajectory) {
			case tLinear: {
				pos= flatPos;
			}
				break;

			case tParabolic: {
				float scaledT= 2.0f * (relative - 0.5f);
				float paraboleY= (1.0f - scaledT * scaledT) * trajectoryScale;

				pos= flatPos;
				pos.y+= paraboleY;
			}
				break;

			case tSpiral: {
				pos= flatPos;
#ifdef USE_STREFLOP
				pos+= xVector * streflop::cos(relative * trajectoryFrequency * targetVector.length()) * trajectoryScale;
				pos+= yVector * streflop::sin(relative * trajectoryFrequency * targetVector.length()) * trajectoryScale;
#else
				pos+= xVector * cos(relative * trajectoryFrequency * targetVector.length()) * trajectoryScale;
				pos+= yVector * sin(relative * trajectoryFrequency * targetVector.length()) * trajectoryScale;
#endif
			}
				break;

			default:
				assert(false);
		}

		direction= pos - lastPos;
		direction.normalize();
		// trigger update of child particles
		positionChildren(); 
		rotateChildren();

		//arrive destination
		if(flatPos.dist(endPos) < 0.5f){
			fade();
			model= NULL;

			if(particleObserver != NULL){
				particleObserver->update(this);
			}

			if(nextParticleSystem != NULL){
				nextParticleSystem->setVisible(true);
				nextParticleSystem->setState(sPlay);
				nextParticleSystem->setPos(endPos);
			}
		}
	}

	ParticleSystem::update();
}

void ProjectileParticleSystem::rotateChildren() {
	//### only on horizontal plane :(
#ifdef USE_STREFLOP
	float rotation = streflop::atan2(direction.x, direction.z);
#else
	float rotation = atan2(direction.x, direction.z);
#endif	
	rotation = radToDeg(rotation);
	for(Children::iterator it = children.begin(); it != children.end(); ++it)
		(*it)->setRotation(rotation);
}

void ProjectileParticleSystem::initParticle(Particle *p, int particleIndex){

	ParticleSystem::initParticle(p, particleIndex);

	float t= static_cast<float> (particleIndex) / emissionRate;

	p->pos= pos + (lastPos - pos) * t;
	p->lastPos= lastPos;
	p->speed= Vec3f(random.randRange(-0.1f, 0.1f), random.randRange(-0.1f, 0.1f), random.randRange(-0.1f, 0.1f))
	        * speed;
	p->accel= Vec3f(0.0f, -gravity, 0.0f);

	updateParticle(p);
}

void ProjectileParticleSystem::updateParticle(Particle *p){
	float energyRatio= clamp(static_cast<float> (p->energy) / maxParticleEnergy, 0.f, 1.f);

	p->lastPos+= p->speed;
	p->pos+= p->speed;
	p->speed+= p->accel;
	p->color= color * energyRatio + colorNoEnergy * (1.0f - energyRatio);
	p->size= particleSize * energyRatio + sizeNoEnergy * (1.0f - energyRatio);
	p->energy--;
}

void ProjectileParticleSystem::setPath(Vec3f startPos, Vec3f endPos){

	//compute axis
	zVector= endPos - startPos;
	zVector.normalize();
	yVector= Vec3f(0.0f, 1.0f, 0.0f);
	xVector= zVector.cross(yVector);

	//apply offset
	startPos+= xVector * offset.x;
	startPos+= yVector * offset.y;
	startPos+= zVector * offset.z;

	pos= startPos;
	lastPos= startPos;
	flatPos= startPos;

	//recompute axis
	zVector= endPos - startPos;
	zVector.normalize();
	yVector= Vec3f(0.0f, 1.0f, 0.0f);
	xVector= zVector.cross(yVector);

	// set members
	this->startPos= startPos;
	this->endPos= endPos;
	
	// direction
	direction = (endPos - lastPos);
	direction.normalize();
	rotateChildren();
}

ProjectileParticleSystem::Trajectory ProjectileParticleSystem::strToTrajectory(const string &str){
	if(str == "linear"){
		return tLinear;
	}
	else if(str == "parabolic"){
		return tParabolic;
	}
	else if(str == "spiral"){
		return tSpiral;
	}
	else{
		throw runtime_error("Unknown particle system trajectory: " + str);
	}
}

// ===========================================================================
//  SplashParticleSystem
// ===========================================================================

SplashParticleSystem::SplashParticleSystem(int particleCount) :
	AttackParticleSystem(particleCount){
	setColor(Vec4f(1.0f, 0.3f, 0.0f, 0.8f));
	setMaxParticleEnergy(100);
	setVarParticleEnergy(50);
	setParticleSize(1.0f);
	setSpeed(0.003f);

	prevParticleSystem= NULL;

	emissionRateFade= 1;
	verticalSpreadA= 1.0f;
	verticalSpreadB= 0.0f;
	horizontalSpreadA= 1.0f;
	horizontalSpreadB= 0.0f;
}

SplashParticleSystem::~SplashParticleSystem(){
	if(prevParticleSystem != NULL){
		prevParticleSystem->nextParticleSystem= NULL;
	}
}

void SplashParticleSystem::initParticleSystem() {
	startEmissionRate = emissionRate;
}

void SplashParticleSystem::update() {
	ParticleSystem::update();
	if(state != sPause) {
		emissionRate-= emissionRateFade;

		float t= 1.0f - ((emissionRate + startEmissionRate) / (startEmissionRate * 2.0f));
		t= clamp(t, 0.0f, 1.0f);
		setTween(t,t);

		if(emissionRate < 0.0f) {//otherwise this system lives forever!
			fade();
		}
	}
}

void SplashParticleSystem::initParticle(Particle *p, int particleIndex){
	p->pos= pos;
	p->lastPos= p->pos;
	p->energy= maxParticleEnergy;
	p->size= particleSize;
	p->color= color;

	p->speed= Vec3f(horizontalSpreadA * random.randRange(-1.0f, 1.0f) + horizontalSpreadB, verticalSpreadA
	        * random.randRange(-1.0f, 1.0f) + verticalSpreadB, horizontalSpreadA * random.randRange(-1.0f, 1.0f)
	        + horizontalSpreadB);
	p->speed.normalize();
	p->speed= p->speed * speed;

	p->accel= Vec3f(0.0f, -gravity, 0.0f);
}

void SplashParticleSystem::updateParticle(Particle *p){
	float energyRatio= clamp(static_cast<float> (p->energy) / maxParticleEnergy, 0.f, 1.f);

	p->lastPos= p->pos;
	p->pos= p->pos + p->speed;
	p->speed= p->speed + p->accel;
	p->energy--;
	p->color= color * energyRatio + colorNoEnergy * (1.0f - energyRatio);
	p->size= particleSize * energyRatio + sizeNoEnergy * (1.0f - energyRatio);
}

// ===========================================================================
//  ParticleManager
// ===========================================================================

ParticleManager::ParticleManager() {
	//assert(GlobalStaticFlags::getIsNonGraphicalModeEnabled() == false);
}

ParticleManager::~ParticleManager() {
	end();
}

void ParticleManager::render(ParticleRenderer *pr, ModelRenderer *mr) const{
	for(unsigned int i= 0; i < particleSystems.size(); i++){
		ParticleSystem *ps= particleSystems[i];
		if(ps != NULL && ps->getVisible()){
			ps->render(pr, mr);
		}
	}
}

bool ParticleManager::hasActiveParticleSystem(ParticleSystem::ParticleSystemType type) const{
	bool result= false;

	//size_t particleSystemCount= particleSystems.size();
	//int currentParticleCount= 0;

	vector<ParticleSystem *> cleanupParticleSystemsList;
	for(unsigned int i= 0; i < particleSystems.size(); i++){
		ParticleSystem *ps= particleSystems[i];
		if(ps != NULL){
			//currentParticleCount+= ps->getAliveParticleCount();

			bool showParticle= true;
			if(dynamic_cast<UnitParticleSystem *> (ps) != NULL || dynamic_cast<FireParticleSystem *> (ps) != NULL){
				showParticle= ps->getVisible() || (ps->getState() == ParticleSystem::sFade);
			}
			if(showParticle == true){
				//printf("Looking for [%d] current id [%d] i = %d\n",type,ps->getParticleSystemType(),i);

				if(type == ParticleSystem::pst_All || type == ps->getParticleSystemType()){
					//printf("FOUND particle system type match for [%d] current id [%d] i = %d\n",type,ps->getParticleSystemType(),i);
					result= true;
					break;
				}
			}
		}
	}

	return result;
}

void ParticleManager::update(int renderFps){
	Chrono chrono;
	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled) chrono.start();

	size_t particleSystemCount= particleSystems.size();
	int currentParticleCount= 0;

	vector<ParticleSystem *> cleanupParticleSystemsList;
	for(unsigned int i= 0; i < particleSystems.size(); i++){
		ParticleSystem *ps= particleSystems[i];
		if(ps != NULL && validateParticleSystemStillExists(ps) == true) {
			currentParticleCount+= ps->getAliveParticleCount();

			bool showParticle= true;
			if(dynamic_cast<UnitParticleSystem *> (ps) != NULL || dynamic_cast<FireParticleSystem *> (ps) != NULL){
				showParticle= ps->getVisible() || (ps->getState() == ParticleSystem::sFade);
			}
			if(showParticle == true){
				ps->update();
				if(ps->isEmpty() && ps->getState() == ParticleSystem::sFade){
					//delete ps;
					//*it= NULL;
					cleanupParticleSystemsList.push_back(ps);
				}
			}
		}
	}
	//particleSystems.remove(NULL);
	cleanupParticleSystems(cleanupParticleSystemsList);

	if(SystemFlags::getSystemSettingType(SystemFlags::debugPerformance).enabled && chrono.getMillis() > 0)
		SystemFlags::OutputDebug(SystemFlags::debugPerformance,"In [%s::%s] Line: %d took msecs: %lld, particleSystemCount = %d, currentParticleCount = %d\n",__FILE__,__FUNCTION__,__LINE__,chrono.getMillis(),particleSystemCount,currentParticleCount);
}

bool ParticleManager::validateParticleSystemStillExists(ParticleSystem * particleSystem) const{
	int index= findParticleSystems(particleSystem, this->particleSystems);
	return (index >= 0);
}

int ParticleManager::findParticleSystems(ParticleSystem *psFind, const vector<ParticleSystem *> &particleSystems) const{
	int result= -1;
	for(unsigned int i= 0; i < particleSystems.size(); i++){
		ParticleSystem *ps= particleSystems[i];
		if(ps != NULL && psFind != NULL && psFind == ps){
			result= i;
			break;
		}
	}
	return result;
}

void ParticleManager::cleanupParticleSystems(ParticleSystem *ps) {

	int index= findParticleSystems(ps, this->particleSystems);
	if(ps != NULL && index >= 0) {
//		printf("-- Delete cleanupParticleSystems [%p]\n",ps);
//		static map<void *,int> deleteList;
//		if(deleteList.find(ps) != deleteList.end()) {
//			assert(deleteList.find(ps) == deleteList.end());
//		}
//		deleteList[ps]++;

		delete ps;
		this->particleSystems.erase(this->particleSystems.begin() + index);
	}
}

void ParticleManager::cleanupParticleSystems(vector<ParticleSystem *> &particleSystems){

	for(int i= particleSystems.size()-1; i >= 0; i--){
		ParticleSystem *ps= particleSystems[i];
		cleanupParticleSystems(ps);
	}

	particleSystems.clear();
	//this->particleSystems.remove(NULL);
}

void ParticleManager::cleanupUnitParticleSystems(vector<UnitParticleSystem *> &particleSystems){

	for(int i= particleSystems.size()-1; i >= 0; i--){
		ParticleSystem *ps= particleSystems[i];
		cleanupParticleSystems(ps);
	}
	particleSystems.clear();
	//this->particleSystems.remove(NULL);
}

void ParticleManager::manage(ParticleSystem *ps){
	assert((std::find(particleSystems.begin(),particleSystems.end(),ps) == particleSystems.end()) && "particle cannot be added twice");
	particleSystems.push_back(ps);
	for(int i=ps->getChildCount()-1; i>=0; i--)
		manage(ps->getChild(i));
}

void ParticleManager::end(){
	while(particleSystems.empty() == false){
		ParticleSystem *ps = particleSystems.back();

//		printf("-- Delete end() [%p]\n",ps);
//		static map<void *,int> deleteList;
//		if(deleteList.find(ps) != deleteList.end()) {
//			assert(deleteList.find(ps) == deleteList.end());
//		}
//		deleteList[ps]++;

		delete ps;
		particleSystems.pop_back();
	}
}

}
}//end namespace
