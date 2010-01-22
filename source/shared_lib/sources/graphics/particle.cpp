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

#include "particle.h"

#include <cassert>
#include <algorithm>

#include "util.h"
#include "particle_renderer.h"
#include "math_util.h"
#include "leak_dumper.h"

using namespace Shared::Util;

namespace Shared{ namespace Graphics{

// =====================================================
//	class ParticleSystem
// =====================================================

ParticleSystem::ParticleSystem(int particleCount){

	//init particle vector
	blendMode = bmOne;
	particles= new Particle[particleCount];
	state= sPlay;
	aliveParticleCount=0;
	active= true;
	visible= true;

	//vars
	texture= NULL;
	particleObserver= NULL;
	
	//params
	this->particleCount= particleCount;
	maxParticleEnergy= 250;
	varParticleEnergy= 50;
	pos= Vec3f(0.0f);
	color= Vec4f(1.0f);
	colorNoEnergy= Vec4f(0.0f);
	emissionRate= 15;
	speed= 1.0f;
}

ParticleSystem::~ParticleSystem(){
	delete [] particles;
}


// =============== VIRTUAL ======================

//updates all living particles and creates new ones
void ParticleSystem::update(){

	if(state!=sPause){
		for(int i=0; i<aliveParticleCount; ++i){
			updateParticle(&particles[i]);

			if(deathTest(&particles[i])){
				
				//kill the particle
				killParticle(&particles[i]);

				//mantain alive particles at front of the array
				if(aliveParticleCount>0){
					particles[i]= particles[aliveParticleCount];
				}

			}
		}

		if(state!=sFade){
			for(int i=0; i<emissionRate; ++i){
				Particle *p= createParticle();
				initParticle(p, i);
			}
		}
	}
}

void ParticleSystem::render(ParticleRenderer *pr, ModelRenderer *mr){
	if(active){
		pr->renderSystem(this);
	}
}


// =============== SET ==========================

void ParticleSystem::setState(State state){
	this->state= state;
}

void ParticleSystem::setTexture(Texture *texture){
	this->texture= texture;
}

void ParticleSystem::setPos(Vec3f pos){
	this->pos= pos;
}

void ParticleSystem::setColor(Vec4f color){
	this->color= color;
}

void ParticleSystem::setColorNoEnergy(Vec4f colorNoEnergy){
	this->colorNoEnergy= colorNoEnergy;
}

void ParticleSystem::setEmissionRate(int emissionRate){
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
}

void ParticleSystem::setObserver(ParticleObserver *particleObserver){
	this->particleObserver= particleObserver;
}

void ParticleSystem::setVisible(bool visible){
	this->visible= visible;
}

// =============== MISC =========================
void ParticleSystem::fade(){
	assert(state==sPlay);
	state= sFade;
	if(particleObserver!=NULL){
		particleObserver->update(this);
	}
}

int ParticleSystem::isEmpty() const{
	assert(aliveParticleCount>=0);
	return aliveParticleCount==0 && state!=sPause;
}

// =============== PROTECTED =========================

// if there is one dead particle it returns it else, return the particle with 
// less energy
Particle * ParticleSystem::createParticle(){

	//if any dead particles
	if(aliveParticleCount<particleCount){
		++aliveParticleCount;
		return &particles[aliveParticleCount-1];
	}

	//if not
	int minEnergy= particles[0].energy;
	int minEnergyParticle= 0;

	for(int i=0; i<particleCount; ++i){
		if(particles[i].energy<minEnergy){
			minEnergy= particles[i].energy;
			minEnergyParticle= i;
		}
	}

	return &particles[minEnergyParticle];
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

// ===========================================================================
//  FireParticleSystem
// ===========================================================================


FireParticleSystem::FireParticleSystem(int particleCount): ParticleSystem(particleCount){
	
	radius= 0.5f;
	speed= 0.01f;
	windSpeed= Vec3f(0.0f);
	
	setParticleSize(0.6f);
	setColorNoEnergy(Vec4f(1.0f, 0.5f, 0.0f, 1.0f));
}

void FireParticleSystem::initParticle(Particle *p, int particleIndex){
	ParticleSystem::initParticle(p, particleIndex);
	
	float ang= random.randRange(-2.0f*pi, 2.0f*pi);
	float mod= fabsf(random.randRange(-radius, radius));
	
	float x= sinf(ang)*mod;
	float y= cosf(ang)*mod;
	
	float radRatio= sqrtf(sqrtf(mod/radius));

	p->color= colorNoEnergy*0.5f + colorNoEnergy*0.5f*radRatio;
	p->energy= static_cast<int>(maxParticleEnergy*radRatio) + random.randRange(-varParticleEnergy, varParticleEnergy);
	p->pos= Vec3f(pos.x+x, pos.y+random.randRange(-radius/2, radius/2), pos.z+y); 
	p->lastPos= pos;
	p->size= particleSize;
	p->speed= Vec3f(0, speed+speed*random.randRange(-0.5f, 0.5f), 0) +  windSpeed;
}

void FireParticleSystem::updateParticle(Particle *p){
	p->lastPos= p->pos;
	p->pos= p->pos+p->speed;
	p->energy--;

	if(p->color.x>0.0f)
		p->color.x*= 0.98f;
	if(p->color.y>0.0f)
		p->color.y*= 0.98f;
	if(p->color.w>0.0f)
		p->color.w*= 0.98f;

	p->speed.x*=1.001f;

}

// ================= SET PARAMS ====================

void FireParticleSystem::setRadius(float radius){
	this->radius= radius;
}

void FireParticleSystem::setWind(float windAngle, float windSpeed){
	this->windSpeed.x= sinf(degToRad(windAngle))*windSpeed;
	this->windSpeed.y= 0.0f;
	this->windSpeed.z= cosf(degToRad(windAngle))*windSpeed;
}

// ===========================================================================
//  RainParticleSystem
// ===========================================================================


RainParticleSystem::RainParticleSystem(int particleCount):ParticleSystem(particleCount){
	setWind(0.0f, 0.0f);
	setRadius(20.0f);

	setEmissionRate(25);
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
	p->pos= Vec3f(pos.x+x, pos.y, pos.z+y); 
	p->lastPos= p->pos;
	p->speed= Vec3f(random.randRange(-speed/10, speed/10), -speed, random.randRange(-speed/10, speed/10)) + windSpeed;
}

bool RainParticleSystem::deathTest(Particle *p){
	return p->pos.y<0;
}

void RainParticleSystem::setRadius(float radius){
	this->radius= radius;

}	

void RainParticleSystem::setWind(float windAngle, float windSpeed){
	this->windSpeed.x= sinf(degToRad(windAngle))*windSpeed;
	this->windSpeed.y= 0.0f;
	this->windSpeed.z= cosf(degToRad(windAngle))*windSpeed;
}

// ===========================================================================
//  SnowParticleSystem
// ===========================================================================

SnowParticleSystem::SnowParticleSystem(int particleCount):ParticleSystem(particleCount){
	setWind(0.0f, 0.0f);
	setRadius(30.0f);

	setEmissionRate(2);
	setParticleSize(0.2f);
	setColor(Vec4f(0.8f, 0.8f, 0.8f, 0.8f));
	setSpeed(0.025f);
}

void SnowParticleSystem::initParticle(Particle *p, int particleIndex){
	
	ParticleSystem::initParticle(p, particleIndex);

	float x= random.randRange(-radius, radius);
	float y= random.randRange(-radius, radius);
	
	p->color= color;
	p->energy= 10000;
	p->pos= Vec3f(pos.x+x, pos.y, pos.z+y); 
	p->lastPos= p->pos;
	p->speed= Vec3f(0.0f, -speed, 0.0f) +  windSpeed;
	p->speed.x+= random.randRange(-0.005f, 0.005f);
	p->speed.y+= random.randRange(-0.005f, 0.005f);
}

bool SnowParticleSystem::deathTest(Particle *p){	
	return p->pos.y<0;
}

void SnowParticleSystem::setRadius(float radius){
	this->radius= radius;
}	

void SnowParticleSystem::setWind(float windAngle, float windSpeed){
	this->windSpeed.x= sinf(degToRad(windAngle))*windSpeed;
	this->windSpeed.y= 0.0f;
	this->windSpeed.z= cosf(degToRad(windAngle))*windSpeed;
}

// ===========================================================================
//  AttackParticleSystem
// ===========================================================================

AttackParticleSystem::AttackParticleSystem(int particleCount): ParticleSystem(particleCount){
	model= NULL;
	primitive= pQuad;
	offset= Vec3f(0.0f);
	gravity= 0.0f;
	direction= Vec3f(1.0f, 0.0f, 0.0f);
}

void AttackParticleSystem::render(ParticleRenderer *pr, ModelRenderer *mr){
	if(active){
		if(model!=NULL){
			pr->renderSingleModel(this, mr);
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

AttackParticleSystem::Primitive AttackParticleSystem::strToPrimitive(const string &str){
	if(str=="quad"){
		return pQuad;
	}
	else if(str=="line"){
		return pLine;
	}
	else{
		throw "Unknown particle primitive: " + str;
	}
}

// ===========================================================================
//  ProjectileParticleSystem
// ===========================================================================

ProjectileParticleSystem::ProjectileParticleSystem(int particleCount): AttackParticleSystem(particleCount){
	setEmissionRate(20);
	setColor(Vec4f(1.0f, 0.3f, 0.0f, 0.5f));
	setMaxParticleEnergy(100);
	setVarParticleEnergy(50);
	setParticleSize(0.4f);
	setSpeed(0.14f);

	trajectory= tLinear;
	trajectorySpeed= 1.0f;
	trajectoryScale= 1.0f;
	trajectoryFrequency = 1.0f;

	nextParticleSystem= NULL;
}

ProjectileParticleSystem::~ProjectileParticleSystem(){
	if(nextParticleSystem!=NULL){
		nextParticleSystem->prevParticleSystem= NULL;
	}
}

void ProjectileParticleSystem::link(SplashParticleSystem *particleSystem){
	nextParticleSystem= particleSystem;
	nextParticleSystem->setState(sPause);
	nextParticleSystem->prevParticleSystem= this;
}

void ProjectileParticleSystem::update(){

	if(state==sPlay){

		lastPos= pos;
		flatPos+= zVector * trajectorySpeed;
		Vec3f targetVector= endPos - startPos;
		Vec3f currentVector= flatPos - startPos;

		// ratio
		float t= clamp(currentVector.length() / targetVector.length(), 0.0f, 1.0f);
				
		// trajectory
		switch(trajectory){
		case tLinear:
			{	
				pos= flatPos;
			}
			break;

		case tParabolic:
			{
				float scaledT= 2.0f * (t-0.5f);
				float paraboleY= (1.0f-scaledT*scaledT) * trajectoryScale;

				pos = flatPos;
				pos.y+= paraboleY;
			}
			break;

		case tSpiral:
			{
				pos= flatPos;
				pos+= xVector * cos(t*trajectoryFrequency*targetVector.length())*trajectoryScale;
				pos+= yVector * sin(t*trajectoryFrequency*targetVector.length())*trajectoryScale;
			}
			break;

		default:
			assert(false);
		}
		
		direction= pos - lastPos;
		direction.normalize();

		//arrive destination
		if( flatPos.dist(endPos)<0.5f ){
			state= sFade;
			model= NULL;

			if(particleObserver!=NULL){
				particleObserver->update(this);
			}

			if(nextParticleSystem!=NULL){
				nextParticleSystem->setState(sPlay);
				nextParticleSystem->setPos(endPos);
			}
		}
	}

	ParticleSystem::update();
}

void ProjectileParticleSystem::initParticle(Particle *p, int particleIndex){

	ParticleSystem::initParticle(p, particleIndex);
		
	float t= static_cast<float>(particleIndex)/emissionRate;
		
	p->pos=	pos + (lastPos - pos) * t;
	p->lastPos= lastPos;
	p->speed= Vec3f(random.randRange(-0.1f, 0.1f), random.randRange(-0.1f, 0.1f), random.randRange(-0.1f, 0.1f)) * speed;
	p->accel= Vec3f(0.0f, -gravity, 0.0f);
	
	updateParticle(p);
}

void ProjectileParticleSystem::updateParticle(Particle *p){
	float energyRatio= clamp(static_cast<float>(p->energy)/maxParticleEnergy, 0.f, 1.f);
		
	p->lastPos+= p->speed;
	p->pos+= p->speed;
	p->speed+= p->accel;
	p->color = color * energyRatio + colorNoEnergy * (1.0f-energyRatio);
	p->size = particleSize * energyRatio + sizeNoEnergy * (1.0f-energyRatio);
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
}

ProjectileParticleSystem::Trajectory ProjectileParticleSystem::strToTrajectory(const string &str){
	if(str=="linear"){
		return tLinear;
	}
	else if(str=="parabolic"){
		return tParabolic;
	}
	else if(str=="spiral"){
		return tSpiral;
	}
	else{
		throw "Unknown particle system trajectory: " + str;
	}
}

// ===========================================================================
//  SplashParticleSystem
// ===========================================================================

SplashParticleSystem::SplashParticleSystem(int particleCount): AttackParticleSystem(particleCount){
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
	if(prevParticleSystem!=NULL){
		prevParticleSystem->nextParticleSystem= NULL;	
	}
}

void SplashParticleSystem::update(){
	ParticleSystem::update();
	if(state!=sPause){
		emissionRate-= emissionRateFade;
	}
}

void SplashParticleSystem::initParticle(Particle *p, int particleIndex){
	p->pos= pos;
	p->lastPos= p->pos;
	p->energy= maxParticleEnergy;
	p->size= particleSize;
	p->color= color;
	
	p->speed= Vec3f(
		horizontalSpreadA * random.randRange(-1.0f, 1.0f) + horizontalSpreadB, 
		verticalSpreadA * random.randRange(-1.0f, 1.0f) + verticalSpreadB, 
		horizontalSpreadA * random.randRange(-1.0f, 1.0f) + horizontalSpreadB);
	p->speed.normalize();
	p->speed= p->speed * speed;

	p->accel= Vec3f(0.0f, -gravity, 0.0f);
}

void SplashParticleSystem::updateParticle(Particle *p){
	float energyRatio= clamp(static_cast<float>(p->energy)/maxParticleEnergy, 0.f, 1.f);
		
	p->lastPos= p->pos;
	p->pos= p->pos + p->speed;
	p->speed= p->speed + p->accel;
	p->energy--;
	p->color = color * energyRatio + colorNoEnergy * (1.0f-energyRatio);
	p->size = particleSize * energyRatio + sizeNoEnergy * (1.0f-energyRatio);
}

// ===========================================================================
//  ParticleManager
// ===========================================================================

ParticleManager::~ParticleManager(){
	end();
}

void ParticleManager::render(ParticleRenderer *pr, ModelRenderer *mr) const{
	list<ParticleSystem*>::const_iterator it;

	for (it=particleSystems.begin(); it!=particleSystems.end(); it++){
		if((*it)->getVisible()){
			(*it)->render(pr, mr);
		}
	}
}

void ParticleManager::update(){
	list<ParticleSystem*>::iterator it;

	for (it=particleSystems.begin(); it!=particleSystems.end(); it++){
		(*it)->update();
		if((*it)->isEmpty()){
			delete *it;
			*it= NULL;
		}
	}
	particleSystems.remove(NULL);
}

void ParticleManager::manage(ParticleSystem *ps){
	particleSystems.push_back(ps);
}

void ParticleManager::end(){
	while(!particleSystems.empty()){
		delete particleSystems.front();
		particleSystems.pop_front();
	}
}

}}//end namespace
