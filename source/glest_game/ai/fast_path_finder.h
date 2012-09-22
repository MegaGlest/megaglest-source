/*!
// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2012 Mark Vejvoda
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

** Copyright (c) 2007 by John W. Ratcliff mailto:jratcliff@infiniplex.net
**
** Portions of this source has been released with the PhysXViewer application, as well as
** Rocket, CreateDynamics, ODF, and as a number of sample code snippets.
**
** If you find this code useful or you are feeling particularily generous I would
** ask that you please go to http://www.amillionpixels.us and make a donation
** to Troy DeMolay.
**
** DeMolay is a youth group for young men between the ages of 12 and 21.
** It teaches strong moral principles, as well as leadership skills and
** public speaking.  The donations page uses the 'pay for pixels' paradigm
** where, in this case, a pixel is only a single penny.  Donations can be
** made for as small as $4 or as high as a $100 block.  Each person who donates
** will get a link to their own site as well as acknowledgement on the
** donations blog located here http://www.amillionpixels.blogspot.com/
**
** If you wish to contact me you can use the following methods:
**
** Skype Phone: 636-486-4040 (let it ring a long time while it goes through switches)
** Skype ID: jratcliff63367
** Yahoo: jratcliff63367
** AOL: jratcliff1961
** email: jratcliff@infiniplex.net

A* Algorithm Implementation using STL is
Copyright (C)2001-2005 Justin Heyes-Jones

  FixedSizeAllocator class
  Copyright 2001 Justin Heyes-Jones

  This class is a constant time O(1) memory manager for objects of
  a specified type. The type is specified using a template class.

  Memory is allocated from a fixed size buffer which you can specify in the
  class constructor or use the default.

  Using GetFirst and GetNext it is possible to iterate through the elements
  one by one, and this would be the most common use for the class.

  I would suggest using this class when you want O(1) add and delete
  and you don't do much searching, which would be O(n). Structures such as binary
  trees can be used instead to get O(logn) access time.

**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef FAST_ASTAR_H

#define FAST_ASTAR_H

//*** IMPORTANT : READ ME FIRST !!
//***
//***  This source code simply provides a C++ wrapper for the AStar Algorithm Implementation in STL written by Justin Heyes-Jones
//***  There is nothing wrong with Justin's code in any way, except that he uses templates.  My personal programming style is
//***  to use virtual interfaces and the PIMPLE paradigm to hide the details of the implementation.
//***
//***  To use my wrapper you simply have your own path node inherit the pure virtual interface 'AI_Node' and implement the
//***  following four methods.
//***
//***
//**  virtual float        getDistance(const AI_Node *node) = 0;  // Return the distance between two nodes
//**  virtual float        getCost(void) = 0;                     // return the relative 'cost' of a node.  Default should be 1.
//**  virtual unsigned int getEdgeCount(void) const = 0;          // Return the number of edges in a node.
//**  virtual AI_Node *    getEdge(int index) const = 0;          // Return a pointer to the node a particular edge is connected to.
//**
//** That's all there is to it.
//**
//** Here is an example usage:
//**
//** FastAstar *fa = createFastAstar();
//** astarStartSearch(fq,fromNode,toNode);
//** for (int i=0; i<10000; i++)
//** {
//**   bool finished = astarSearchStep(fa);
//**   if ( finished ) break;
//**  }
//**
//**  unsigned int count;
//**  AI_Node **solution = getSolution(fa,count);
//**
//**   ... do something you want with the answer
//**
//**  releaseFastAstar(fa);
//**
//*******************************
#include "data_types.h"

using namespace Shared::Platform;

class AI_Node
{
public:
  virtual float        getDistance(const AI_Node *node,void *userData) = 0;
  virtual float        getCost(void *userData) = 0;
  virtual unsigned int getEdgeCount(void *userData) const = 0;
  virtual AI_Node *    getEdge(int index,void *userData) const = 0;
  virtual int32 		getHashCode() const = 0;

  virtual ~AI_Node() {}
};

enum SearchState {
	SEARCH_STATE_NOT_INITIALISED,
	SEARCH_STATE_SEARCHING,
	SEARCH_STATE_SUCCEEDED,
	SEARCH_STATE_FAILED,
	SEARCH_STATE_OUT_OF_MEMORY,
	SEARCH_STATE_INVALID
};

class FastAstar;

FastAstar * createFastAstar(void);    // Create an instance of the FastAstar utility.
void       astarStartSearch(FastAstar *astar,AI_Node *from,AI_Node *to, void *userData);  // start a search.

bool       astarSearchStep(FastAstar *astar,unsigned int &searchCount); // step the A star algorithm one time.  Return true if the search is completed.
SearchState getLastSearchState(FastAstar *astar);

AI_Node ** getSolution(FastAstar *astar,unsigned int &count);  // retrieve the solution.  If this returns a null pointer and count of zero, it means no solution could be found.
void      releaseFastAstar(FastAstar *astar);  // Release the intance of the FastAstar utility.


#endif
