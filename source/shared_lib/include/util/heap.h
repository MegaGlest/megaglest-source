// ==============================================================
//	This file is part of the Glest Advanced Engine
//
//	Copyright (C) 2010 James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#ifndef _MIN_HEAP_INCLUDED_
#define _MIN_HEAP_INCLUDED_

#include <cassert>

namespace Shared { namespace Util {

/** 'Nodes' nead to implement this implicit interface:

struct SomeNode {
	
	void setHeapIndex(int ndx); // typically, { heap_ndx = ndx;  }
	int  getHeapIndex() const;  // typically, { return heap_ndx; }
	
	bool operator<(const SomeNode &that) const;
};
*/

/** (Min) Heap, supporting node 'index awareness'.
  * stores pointers to Nodes, user needs to supply the actual nodes, preferably in single block
  * of memory, and preferably with as compact a node structure as is possible (to the point that the 
  * int 'heap_ndx' should be a bitfield using as few bits as you can get away with).
  */ 
template<typename Node> class MinHeap {
private:
	Node**	data;
	int		counter;
	int		capacity;

public:
	/** Construct MinHeap with a given capacity */
	MinHeap(int capacity = 1024) : counter(0), capacity(capacity) {
		data = new Node*[capacity];
	}

	~MinHeap() {
		delete [] data;
	}

	/** add a new node to the min heap */
	bool insert(Node *node) {
		if (counter == capacity) {
			return false;
		}
		data[counter] = node;
		data[counter]->setHeapIndex(counter);
		promoteNode(counter++);
		return true;
	}

	/** pop the best node off the min heap */
	Node* extract() {
		assert(counter);
		Node *res = data[0];
		if (--counter) {
			data[0] = data[counter];
			data[0]->setHeapIndex(0);
			demoteNode();
		}
		return res;
	}

	/** indicate a node has had its key decreased */
	void promote(Node *node) {
		assert(data[node->getHeapIndex()] == node);
		promoteNode(node->getHeapIndex());
	}

	int	 size() const	{ return counter;	}
	void clear()		{ counter = 0;		}
	bool empty() const	{ return !counter;	}

private:
	inline int parent(int ndx) const	{ return (ndx - 1) / 2; }
	inline int left(int ndx) const		{ return (ndx * 2) + 1; }

	void promoteNode(int ndx) {
		assert(ndx >= 0 && ndx < counter);
		while (ndx > 0 && *data[ndx] < *data[parent(ndx)]) {
			Node *tmp = data[parent(ndx)];
			data[parent(ndx)] = data[ndx];
			data[ndx] = tmp;
			data[ndx]->setHeapIndex(ndx);
			ndx = parent(ndx);
			data[ndx]->setHeapIndex(ndx);
		}
	}

	void demoteNode(int ndx = 0) {
		assert(counter);
		while (true) {
			int cndx = left(ndx);  // child index
			int sndx = ndx;  // smallest (priority) of data[ndx] and any children
			if (cndx < counter && *data[cndx] < *data[ndx]) sndx = cndx;
			if (++cndx < counter && *data[cndx] < *data[sndx]) sndx = cndx;
			if (sndx == ndx)  return;
			Node *tmp = data[sndx];
			data[sndx] = data[ndx];
			data[ndx] = tmp;
			data[ndx]->setHeapIndex(ndx);
			ndx = sndx;
			data[ndx]->setHeapIndex(ndx);
		}
	}
};

}} // end namespace Shared::Util

#endif // _MIN_HEAP_INCLUDED_
