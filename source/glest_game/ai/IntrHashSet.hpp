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

#ifndef __INTRUSIVE_HASH_SET_HPP__
#define __INTRUSIVE_HASH_SET_HPP__

#include <inttypes.h>
#include <memory.h>

/*
  This is base class for values that can be
	stored in intrusive hashset.
	
*/
struct IntrHashSetNodeBase{
  IntrHashSetNodeBase* ihsNextNode;
};

/*
	Values must be allocated in (some kind of) a heap.
	Not stack.
	Values are also nodes in a hash table,
	so deleting values that is in hash is bad thing to do.
*/


/*
  class TRAITS MUST implement following static members:

  bool TRAITS::isEqual(const T* a,const T* b);
  uint32_t TRAITS::getHashCode(const T* a);
*/

template <class T,class TRAITS>
class IntrHashSet{
protected:
  typedef T Node;
	// copy constructor is protected
	// values CANNOT be copied since
	// they are allocated outside of this container!
	IntrHashSet(const IntrHashSet&);
public:
  // default constructor
	// preAlloc values CANNOT be zero!
  IntrHashSet(int preAlloc=128)
  {
    hash=new Node*[preAlloc];
    memset(hash,0,sizeof(Node*)*preAlloc);
    hashSize=preAlloc;
    collisionsCount=0;
  }
	//destructor
  ~IntrHashSet()
  {
    if(hash)
    {
      delete [] hash;
    }
  }
  //just empties container without deleting values!
  void Clear()
  {
    collisionsCount=0;
    memset(hash,0,sizeof(Node*)*hashSize);
  }

  //insert allocated value
  void Insert(T* value)
  {
    if(collisionsCount>hashSize/2)
    {
      Rehash();
    }
    uint32_t hidx=getIndex(value);
    Node*& nodePtr=hash[hidx];
    collisionsCount+=nodePtr!=0?1:0;
    value->ihsNextNode=nodePtr;
    nodePtr=value;
  }

  //iterator
	//can be used to iterate over all values of hash
	//and also returned by find method and can be
	//use to delete value from hash faster
  class ConstIterator{
  public:
    ConstIterator(const IntrHashSet* argHash):node(0),parent(0),index(0),hash(argHash){}
    ConstIterator(const IntrHashSet& argHash):node(0),parent(0),index(0),hash(&argHash){}

    const T* Get()const
    {
      return node;
    }
		
    bool Found()const
    {
      return node!=0;
    }

    void Rewind()
    {
      index=0;
      node=0;
      parent=0;
    }

    bool Next()
    {
      if(index>=hash->hashSize)
      {
        return false;
      }
      if(index==0 && node==0 && parent==0)
      {
        parent=&hash->hash;
        node=*parent;
        if(node)
        {
          return true;
        }
      }
      while(node==0)
      {
        index++;
        if(index>=hash->hashSize)
        {
          return false;
        }
        parent=hash->hash+index;
        node=*parent;
        if(node)
        {
          return true;
        }
      }
      if(*parent!=node)
      {
        parent=(Node**)&(*parent)->ihsNextNode;
      }
      node=(Node*)node->ihsNextNode;
      if(node)
      {
        return true;
      }
      return Next();
    }

  protected:
    ConstIterator(const IntrHashSet* argHash,uint32_t argIndex,const Node* argNode,const Node*const* argParent):
      node(argNode),parent(argParent),index(argIndex),hash(argHash){}
    const Node* node;
    const Node*const* parent;
    uint32_t index;
    const IntrHashSet* hash;
    friend class IntrHashSet;
  };

  //mutable version of iterator
	//however it's not good idea to modify key part of stored value
  class Iterator{
  public:
    Iterator(IntrHashSet* argHash):node(0),parent(0),index(0),hash(argHash){}
    Iterator(IntrHashSet& argHash):node(0),parent(0),index(0),hash(&argHash){}

    T* Get()
    {
      return node;
    }
		
    bool Found()const
    {
      return node!=0;
    }

    void Rewind()
    {
      index=0;
      node=0;
      parent=0;
    }

    bool Next()
    {
      if(index>=hash->hashSize)
      {
        return false;
      }
      if(index==0 && node==0 && parent==0)
      {
        parent=hash->hash;
        node=*parent;
        if(node)
        {
          return true;
        }
      }
      while(node==0)
      {
        index++;
        if(index>=hash->hashSize)
        {
          return false;
        }
        parent=hash->hash+index;
        node=*parent;
        if(node)
        {
          return true;
        }
      }
      if(*parent!=node)
      {
        parent=(Node**)&(*parent)->ihsNextNode;
      }
      node=(Node*)node->ihsNextNode;
      if(node)
      {
        return true;
      }
      return Next();
    }

  protected:
    Iterator(IntrHashSet* argHash,uint32_t argIndex,Node* argNode,Node** argParent):
      node(argNode),parent(argParent),index(argIndex),hash(argHash){}
    Node* node;
    Node** parent;
    uint32_t index;
    IntrHashSet* hash;
    friend class IntrHashSet;
  };

  //constant version of find
	//returned iterator can be used to obtain found value
	//or delete it from hashet

  ConstIterator Find(const T& value)const
  {
    uint32_t hidx=getIndex(&value);
    Node*const* nodePtr=&hash[hidx];
    if(!*nodePtr)return ConstIterator(0);
    if(TRAITS::isEqual((*nodePtr),&value))
    {
      return ConstIterator(this,hidx,*nodePtr,nodePtr);
    }
    Node*const* parentPtr=nodePtr;
    Node* node=(Node*)(*nodePtr)->ihsNextNode;
    while(node)
    {
      if(TRAITS::isEqual(node,&value))
      {
        return ConstIterator(this,hidx,node,parentPtr);
      }
      parentPtr=nodePtr;
      nodePtr=(Node**)&node->ihsNextNode;
      node=(Node*)node->ihsNextNode;
    }
    return ConstIterator(0);
  }
	
	//non constant version of find
  Iterator Find(const T& value)
  {
    uint32_t hidx=getIndex(&value);
    Node** nodePtr=&hash[hidx];
    if(!*nodePtr)return Iterator(0);
    if(TRAITS::isEqual((*nodePtr),&value))
    {
      return Iterator(this,hidx,*nodePtr,nodePtr);
    }
    Node** parentPtr=nodePtr;
    Node* node=(Node*)(*nodePtr)->ihsNextNode;
    while(node)
    {
      if(TRAITS::isEqual(node,&value))
      {
        return Iterator(this,hidx,node,parentPtr);
      }
      parentPtr=nodePtr;
      nodePtr=(Node**)&node->ihsNextNode;
      node=(Node*)node->ihsNextNode;
    }
    return Iterator(0);
  }

  //delete element from hashset 'by value'
	//actual value IS NOT DEALLOCATED
	//and should be deallocated separately
  void Delete(const T& value)
  {
    uint32_t hidx=getIndex(&value);
    Node** nodePtr=hash+hidx;
    if(TRAITS::isEqual((*nodePtr),&value))
    {
      Node* node=*nodePtr;
      *nodePtr=(Node*)node->ihsNextNode;
      if(*nodePtr)
      {
        collisionsCount--;
      }
      return;
    }
    Node* parentPtr=*nodePtr;
    Node* node=(Node*)(*nodePtr)->ihsNextNode;
    while(node)
    {
      if(TRAITS::isEqual(node,&value))
      {
        parentPtr->ihsNextNode=node->ihsNextNode;
        return;
      }
      parentPtr=node;
      node=(Node*)node->ihsNextNode;
    }
  }
	
	//delete element by iterator returned from Find
	//or used for iteration.
	//iterator used for Delete is no longer valid
	//for iteration, however element it refer to
	//is still here and Get is still valid after Delete.
	//actuall value also IS NOT DEALLOCATED!

  void Delete(const Iterator& hi)
  {
    Node** parentPtr=(Node**)hi.parent;
    Node* node=(Node*)hi.node;
    if(!node)
    {
      return;
    }
    if(*parentPtr==node)
    {
      *parentPtr=(Node*)node->ihsNextNode;
    }else
    {
      (*parentPtr)->ihsNextNode=node->ihsNextNode;
    }
    if(*parentPtr)
    {
      collisionsCount--;
    }
  }

protected:

  // get index of a bucket in a hash table
  uint32_t getIndex(const T* value)const
  {
    uint32_t hashCode=TRAITS::getHashCode(value);
    return hashCode%hashSize;
  }

  // increase hash table size if there are
	// too much collisions
  void Rehash()
  {
    Node** oldHash=hash;
    Node** oldHashEnd=hash+hashSize;
    hashSize*=2;
    hash=new Node*[hashSize];
    memset(hash,0,sizeof(Node*)*hashSize);
    collisionsCount=0;
    for(Node** it=oldHash;it!=oldHashEnd;it++)
    {
      Node* node=*it;
      Node* nextNode;
      while(node)
      {
        uint32_t hidx=getIndex(node);
        Node** nodePtr=hash+hidx;
        if(*nodePtr)
        {
          collisionsCount++;
        }
        nextNode=(Node*)node->ihsNextNode;
        node->ihsNextNode=*nodePtr;
        *nodePtr=node;
        node=nextNode;
      }
    }
  }
	// hash  table itself
  Node** hash;
	// size of hash table
  size_t hashSize;
	// number of collisions
  int collisionsCount;

};

#endif
