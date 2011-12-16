/* =========================================================================
 * Freetype GL - A C OpenGL Freetype engine
 * Platform:    Any
 * WWW:         http://code.google.com/p/freetype-gl/
 * -------------------------------------------------------------------------
 * Copyright 2011 Nicolas P. Rougier. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NICOLAS P. ROUGIER ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL NICOLAS P. ROUGIER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Nicolas P. Rougier.
 * ========================================================================= */
#ifdef USE_FREETYPEGL

#ifndef __VECTOR_H__
#define __VECTOR_H__

#ifdef	__cplusplus
extern "C" {
#endif


/**
 *  Generic vector structure.
 */
typedef struct
 {
     /** Pointer to dynamically allocated items. */
     void * items;

     /** Number of items that can be held in currently allocated storage. */
     size_t capacity;

     /** Number of items. */
     size_t size;

     /** Size (in bytes) of a single item. */
     size_t item_size;
} Vector;

/**
 *  Creates a vector.
 *
 *  @param  item_size    item size in bytes
 *  @return              a new empty vector
 */
  Vector *
  vector_new( size_t item_size );

/**
 *  Deletes a vector.
 *
 *  @param  self a vector structure
 */
  void
  vector_delete( Vector *self );

/**
 *  Returns a pointer to the item located at specified index.
 *
 *  @param  self  a vector structure
 *  @param  index the index of the item to be returned
 *  @return       pointer on the specified item
 */
  const void *
  vector_get( const Vector *self,
              size_t index );

/**
 *  Returns a pointer to the first item.
 *
 *  @param  self  a vector structure
 *  @return       pointer on the first item
 */
  const void *
  vector_front( const Vector *self );

/**
 *  Returns a pointer to the last item
 *
 *  @param  self  a vector structure
 *  @return pointer on the last item
 */
  const void *
  vector_back( const Vector *self );

/**
 *  Check if an item is contained within the vector.
 *
 *  @param  self  a vector structure
 *  @param  item  item to be searched in the vector
 *  @param  cmp   a pointer a comparison function
 *  @return       1 if item is contained within the vector, 0 otherwise
 */
  int
  vector_contains( const Vector *self,
                   const void *item,
                   int (*cmp)(const void *, const void *) );

/**
 *  Checks whether the vector is empty.
 *
 *  @param  self  a vector structure
 *  @return       1 if the vector is empty, 0 otherwise
 */
  int
  vector_empty( const Vector *self );

/**
 *  Returns the number of items
 *
 *  @param  self  a vector structure
 *  @return       number of items
 */
  size_t
  vector_size( const Vector *self );

/**
 *  Reserve storage such that it can hold at last size items.
 *
 *  @param  self  a vector structure
 *  @param  size  the new storage capacity
 */
  void
  vector_reserve( Vector *self,
                  const size_t size );

/**
 *  Returns current storage capacity
 *
 *  @param  self  a vector structure
 *  @return       storage capacity
 */
  size_t
  vector_capacity( const Vector *self );

/**
 *  Decrease capacity to fit actual size.
 *
 *  @param  self  a vector structure
 */
  void
  vector_shrink( Vector *self );

/**
 *  Removes all items.
 *
 *  @param  self  a vector structure
 */
  void
  vector_clear( Vector *self );

/**
 *  Replace an item.
 *
 *  @param  self  a vector structure
 *  @param  index the index of the item to be replaced
 *  @param  item  the new item
 */
  void
  vector_set( Vector *self,
              const size_t index,
              const void *item );

/**
 *  Erase an item.
 *
 *  @param  self  a vector structure
 *  @param  index the index of the item to be erased
 */
  void
  vector_erase( Vector *self,
                const size_t index );

/**
 *  Erase a range of items.
 *
 *  @param  self  a vector structure
 *  @param  first the index of the first item to be erased
 *  @param  last  the index of the last item to be erased
 */
  void
  vector_erase_range( Vector *self,
                      const size_t first,
                      const size_t last );

/**
 *  Appends given item to the end of the vector.
 *
 *  @param  self a vector structure
 *  @param  item the item to be inserted
 */
  void
  vector_push_back( Vector *self,
                    const void *item );

/**
 *  Removes the last item of the vector.
 *
 *  @param  self a vector structure
 */
  void
  vector_pop_back( Vector *self );

/**
 *  Resizes the vector to contain size items
 *
 *  If the current size is less than size, additional items are appended and
 *  initialized with value. If the current size is greater than size, the
 *  vector is reduced to its first size elements.
 *
 *  @param  self a vector structure
 *  @param  size the new size
 */
  void
  vector_resize( Vector *self,
                 const size_t size );

/**
 *  Insert a single item at specified index.
 *
 *  @param  self  a vector structure
 *  @param  index location before which to insert item
 *  @param  item  the item to be inserted
 */
  void
  vector_insert( Vector *self,
                 const size_t index,
                 const void *item );

/**
 *  Insert raw data at specified index.
 *
 *  @param  self  a vector structure
 *  @param  index location before which to insert item
 *  @param  data  a pointer to the items to be inserted
 *  @param  count the number of items to be inserted
 */
  void
  vector_insert_data( Vector *self,
                      const size_t index,
                      const void * data,
                      const size_t count );

/**
 *  Append raw data to the end of the vector.
 *
 *  @param  self  a vector structure
 *  @param  data  a pointer to the items to be inserted
 *  @param  count the number of items to be inserted
 */
  void
  vector_push_back_data( Vector *self,
                         const void * data, 
                         const size_t count );

/**
 *  Sort vector items according to cmp function.
 *
 *  @param  self  a vector structure
 *  @param  cmp   a pointer a comparison function
 */
  void
  vector_sort( Vector *self,
               int (*cmp)(const void *, const void *) );

#ifdef	__cplusplus
}
#endif

#endif /* __VECTOR_H__ */

#endif
