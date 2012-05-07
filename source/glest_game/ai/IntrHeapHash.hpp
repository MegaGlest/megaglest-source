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

#ifndef __INTRUSIVE_HEAP_HASH_SET_HPP__
#define __INTRUSIVE_HEAP_HASH_SET_HPP__

#ifndef WIN32
#include <inttypes.h>
#endif

#include <vector>
#include "IntrHashSet.hpp"

/*
Here is temlate class Intrusive Heap/HashSet.
Actuall values are stored in this container,
no copying is performed, so all values should
be allocated in a heap or in a storage with
lifetime as long as lifetime of container.
Values are also used as nodes of container,
so no value can be stored in two containers
of this kind simultaneously.
*/

/*
  This is what should be done with duplicate
	from hash point of view values.
*/
enum HHDuplicatePolicy{
  hhdpReplaceOld, //replace old value. old values is not deallocated!
  hhdpKeepOld, //container is not modified in this case.
  hhdpInsertDuplicate //duplicates are stored in container
};

/*
TRAITS class MUST implement following static members:
for heap - this is something like operator< or operator>
  bool TRAITS::Compare(const T* a,const T* b);
for hash
  bool TRAITS::isEqual(const T* a,const T* b);
  uint32_t TRAITS::getHashCode(const T& a);
*/

/*
  This is base class for values stored in a intrusive heap hash
*/
struct IntrHeapHashNodeBase:public IntrHashSetNodeBase{
  int ihhNodeIndex;
};

template <class T,class TRAITS,HHDuplicatePolicy policy=hhdpReplaceOld>
class IntrHeapHash{
public:
  //default constructor
	//preAlloc value cannot be 0!
  IntrHeapHash(int preAlloc=128)
  {
    hash=new Node*[preAlloc];
    memset(hash,0,preAlloc*sizeof(Node*));
    hashSize=preAlloc;
    heap=new Node*[preAlloc];
    memset(heap,0,preAlloc*sizeof(Node*));
    heapSize=0;
    heapAlloc=preAlloc;
    collisionsCount=0;
  }

  //guess what? destructor!
  ~IntrHeapHash()
  {
    delete [] hash;
    delete [] heap;
  }

  // clear content container
	// values are not deallocated!
  void Clear()
  {
    memset(hash,0,hashSize*sizeof(Node*));
    heapSize=0;
    collisionsCount=0;
  }

  //insert new value into container
	//return value is equal to inserted value for all policies if no duplicate detected
	//in case of duplicate for keep old policy 0 is returned
	//in case of duplicate for replace old policy old value is returned
  T* Insert(T* value)
  {
    if(collisionsCount>hashSize/2)
    {
      Rehash();
    }
    uint32_t hidx=getIndex(value);
    Node*& node=hash[hidx];
		T* rv=value;
    if(policy==hhdpReplaceOld || policy==hhdpKeepOld)
    {
      Node* ptr=FindHashNode(node,*value);
      if(ptr)
      {
        if(policy==hhdpKeepOld)
        {
          return 0;
        }
				DeleteHashNode(hash+hidx,ptr);
			  rv=ptr;
      }
    }
    value->ihhNodeIndex=heapSize;
    if(node)
    {
      collisionsCount++;
    }
    value->ihsNextNode=node;
    node=value;
    if(heapSize==heapAlloc)
    {
      heapAlloc*=2;
      Node** newHeap=new Node*[heapAlloc];
      memcpy(newHeap,heap,heapSize*sizeof(Node*));
      delete [] heap;
      heap=newHeap;
    }
    heap[heapSize++]=value;
    HeapPush();
		return rv;
  }

  // value at the head of heap
  const T* getHead()const
  {
    return *heap;
  }

  // same as previous but non const
  T* getHead()
  {
    return **heap;
  }

  // get and remove head value of heap
  T* Pop()
  {
    T* rv=*heap;
    uint32_t hidx=getIndex(rv);
    DeleteHashNode(&hash[hidx],rv);
    HeapSwap(heap,heap+heapSize-1);
    heapSize--;
    HeapPop();
    return rv;
  }

  //find value in hash
	//const version
  const T* Find(const T& value)const
  {
    uint32_t hidx=getIndex(&value);
    Node* ptr=hash[hidx];
    while(ptr)
    {
      if(TRAITS::isEqual(ptr,&value))
      {
        return ptr;
      }
      ptr=(Node*)ptr->ihsNextNode;
    }
    return 0;
  }

  //find value in hash
	//non const version
	//it's not good idea to modify key value of hash or key value of heap however
  T* Find(const T& value)
  {
    uint32_t hidx=getIndex(&value);
    Node* ptr=hash[hidx];
    while(ptr)
    {
      if(TRAITS::isEqual(ptr,&value))
      {
        return ptr;
      }
      ptr=(Node*)ptr->ihsNextNode;
    }
    return 0;
  }

  //get from hash all values equal to given
  void GetAll(const T& value,std::vector<T*>& values)const
  {
    uint32_t hidx=getIndex(&value);
    Node* ptr=hash[hidx];
    while(ptr)
    {
      if(TRAITS::isEqual(ptr,&value))
      {
        values.push_back(ptr);
      }
      ptr=(Node*)ptr->ihsNextNode;
    }
  }

  //delete value from hash
	//deleted object returned
  T* Delete(const T& value)
  {
    uint32_t hidx=getIndex(&value);
    int idx=DeleteHashNode(hash+hidx,value);
    if(idx==-1)
    {
      return 0;
    }
    if(idx==heapSize-1)
    {
      heapSize--;
      return heap[heapSize];
    }
    HeapSwap(heap+idx,heap+heapSize-1);
    heapSize--;
    HeapFix(idx);
		return heap[heapSize];
  }

  //delete all values equal to given
  void DeleteAll(const T& value)
  {
    uint32_t hidx=getIndex(&value);
    Node** nodePtr=hash+hidx;
    while(*nodePtr)
    {
      int idx=DeleteHashNode(nodePtr,value);
      if(idx==-1)
      {
        return;
      }
      if(idx==heapSize-1)
      {
        heapSize--;
        continue;
      }
      HeapSwap(heap+idx,heap+heapSize-1);
      heapSize--;
      HeapFix(idx);
    }
  }

  //get number of elements in container
  size_t getSize()const
  {
    return heapSize;
  }

  //check if container is empty
  bool isEmpty()const
  {
    return heapSize==0;
  }

  //iterator
	//can/should be used like this:
	//for(IHHTypeDef::Iterator it(ihh);it.Next();)
	//{
	//  dosomething(it.Get())
	//}
  class Iterator{
  public:
    Iterator(const IntrHeapHash& argHH)
    {
      begin=argHH.heap;
      current=0;
      end=argHH.heap+argHH.heapSize;
    }
    bool Next()
    {
		  if(current==0)
			{
			  current=begin;
			}else
			{
  			current++;
			}
      return current!=end;
    }
		T* Get()
		{
		  if(current)
			{
			  return *current;
			}
			return 0;
		}
    void Rewind()
    {
      current=begin;
    }
  protected:
    typename IntrHeapHash::Node **begin,**current,**end;
  };

protected:

  //protected copy constructor
	//values cannot be copied as well as container
  IntrHeapHash(const IntrHeapHash&);

  //typedef for convenience
  typedef T Node;
	//heap array
  Node** heap;
	//hash table
  Node** hash;
	//size of hash table
  size_t hashSize;
	//used size of heap
  size_t heapSize;
	//allocated size of heap
  size_t heapAlloc;

  //count of collisions in hash
  int collisionsCount;

  //get index of value in hash table
  uint32_t getIndex(const T* value)const
  {
    uint32_t hashCode=TRAITS::getHashCode(value);
    return hashCode%hashSize;
  }

  //resize hash table when
	//collisions count is too big
  void Rehash()
  {
    Node** oldHash=hash;
    Node** oldHashEnd=oldHash+hashSize;
    hashSize*=2;
    hash=new Node*[hashSize];
    memset(hash,0,hashSize*sizeof(Node*));
    collisionsCount=0;
    for(Node** it=oldHash;it!=oldHashEnd;it++)
    {
      Node* nodePtr=*it;
      while(nodePtr)
      {
        uint32_t hidx=getIndex(nodePtr);
        Node*& hnode=hash[hidx];
        if(hnode)
        {
          collisionsCount++;
        }
        Node* next=(Node*)nodePtr->ihsNextNode;
        nodePtr->ihsNextNode=hnode;
        hnode=nodePtr;
        nodePtr=next;

      }
    }
  }

  //find and delete hash node from bucket by ptr
  void DeleteHashNode(Node** nodeBasePtr,Node* nodePtr)
  {
    if(*nodeBasePtr==nodePtr)
    {
      *nodeBasePtr=(Node*)nodePtr->ihsNextNode;
      if(*nodeBasePtr)
      {
        collisionsCount--;
      }
      return;
    }
    Node* parentPtr=*nodeBasePtr;
    Node* iterPtr=(Node*)parentPtr->ihsNextNode;
    while(iterPtr)
    {
      if(iterPtr==nodePtr)
      {
        parentPtr->ihsNextNode=nodePtr->ihsNextNode;
        collisionsCount--;
        return;
      }
      parentPtr=iterPtr;
      iterPtr=(Node*)iterPtr->ihsNextNode;
    }
  }

  //find and delete hash node from bucked by value
	//heap index returned
  int DeleteHashNode(Node** nodePtr,const T& data)
  {
    if(!*nodePtr)return -1;
    if(TRAITS::isEqual(*nodePtr,&data))
    {
      int rv=(*nodePtr)->ihhNodeIndex;
      *nodePtr=(Node*)(*nodePtr)->ihsNextNode;
      if(*nodePtr)
      {
        collisionsCount--;
      }
      return rv;
    }
    Node* parentPtr=*nodePtr;
    Node* node=(Node*)(*nodePtr)->ihsNextNode;
    while(node)
    {
      if(TRAITS::isEqual(node,&data))
      {
        parentPtr->ihsNextNode=node->ihsNextNode;
        collisionsCount--;
        return node->ihhNodeIndex;
      }
      parentPtr=node;
      node=(Node*)node->ihsNextNode;
    }
    return -1;
  }

  //find hash node in a bucked by value
  Node* FindHashNode(Node* nodePtr,const T& data)
  {
    while(nodePtr)
    {
      if(TRAITS::isEqual(nodePtr,&data))
      {
        return nodePtr;
      }
      nodePtr=(Node*)nodePtr->ihsNextNode;
    }
    return 0;
  }

  //float up tail element
  void HeapPush(int didx=-1)
  {
    int idx=didx==-1?heapSize-1:didx;
    int pidx;
    Node** b=heap;
    Node** i=b+idx;
    Node** pi;
    Node* ptr=*i;
    while(idx)
    {
      pidx=(idx-1)/2;
      pi=b+pidx;
      if(TRAITS::Compare(ptr,(*pi)))
      {
        *i=*pi;
        (*i)->ihhNodeIndex=idx;
        i=pi;
        idx=pidx;
      }else
      {
        break;
      }
    }
    *i=ptr;
    ptr->ihhNodeIndex=idx;
  }
	//drow down element from head
  void HeapPop(int idx=0)
  {
    if(heapSize<=1)
    {
      return;
    }
    Node **i,**b,**i1,**i2;
    int cidx1,cidx2;
    b=heap;
    i=b+idx;
    Node* ptr=*i;
    while(idx<heapSize)
    {
      cidx1=idx*2+1;
      cidx2=idx*2+2;
      if(cidx1>=heapSize)
      {
        break;
      }
      i1=b+cidx1;
      i2=b+cidx2;
      if(cidx2>=heapSize || TRAITS::Compare((*i1),(*i2)))
      {
        if(!TRAITS::Compare(ptr,(*i1)))
        {
          *i=*i1;
          (*i)->ihhNodeIndex=idx;
          idx=cidx1;
          i=i1;
        }else
        {
          break;
        }
      }else
      {
        if(!TRAITS::Compare(ptr,(*i2)))
        {
          *i=*i2;
          (*i)->ihhNodeIndex=idx;
          idx=cidx2;
          i=i2;
        }else
        {
          break;
        }
      }
    }
    *i=ptr;
    ptr->ihhNodeIndex=idx;
  }

  //either drow down or float up element at middle
  void HeapFix(int idx)
  {
    int pidx=(idx-1)/2;
    if(TRAITS::Compare(heap[idx],heap[pidx]))
    {
      HeapPush(idx);
    }else
    {
      HeapPop(idx);
    }
  }

  //swap two element fixing indeces
  void HeapSwap(Node** i1,Node** i2)
  {
    int idx1=(*i1)->ihhNodeIndex;
    int idx2=(*i2)->ihhNodeIndex;
    Node* ptr=*i1;
    *i1=*i2;
    (*i1)->ihhNodeIndex=idx1;
    (*i2)=ptr;
    (*i2)->ihhNodeIndex=idx2;
  }

};

#endif
