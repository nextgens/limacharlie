/*
Copyright 2015 refractionPOINT

Licensed under the Apache License, Version 2.0 ( the "License" );
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <rpal_btree.h>
#include <rpal_synchronization.h>

#define RPAL_FILE_ID     4

#ifdef RPAL_PLATFORM_WINDOWS
  #pragma warning( disable: 4706 ) // Disabling error on constant expression in condition
#endif
// Based on: http://stuff.mit.edu/afs/sipb/user/gamadrid/nscript/btree.c

/* btree.c - A binary tree implementation */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* btree.h - A binary tree implementation */

/*
  Public functions
  ----------------

  BTREE btree_Create(size_t size, int (*cmp)(const void *, const void *))

  Create a btree which holds elements of size <size>.  The (non-strict)
  ordering or the elements is determined by the function <cmp>.  <cmp> should
  be a function which takes a two pointers to elements and returns a value
  less than, equal to, or greater than zero if the first value is less than,
  equal to, or greater than the second, respectively.


  int btree_Search(BTREE tree, void *key, void *ret)

  Search the <tree> for an element "equal" to <key> and return it in the space
  pointed to by <ret>.  If <ret> is NULL, then don't bother returning any
  values.  This routine returns a zero value on success and a non-zero value
  if it fails to find the <key> value.

  
  int btree_Minimum(BTREE tree, void *ret)
  int btree_Maximum(BTREE tree, void *ret)
  
  Return the minimum/maximum value in <tree> in the space pointed to by <ret>.
  If <ret> is NULL, then don't bother returning any values.  This routine
  returns a zero value on success and a non-zero value if it fails, (i.e., the
  tree is empty).

  
  int btree_Empty(BTREE tree)
  
  Returns a non-zero value if the <tree> is empty, and a zero value if it
  contains elements.


  int btree_Successor(BTREE tree, void *key, void *ret)
  int btree_Predecessor(BTREE tree, void *key, void *ret)

  Returns the value immediately after/before <key> in the <tree>.  The value
  is returned in the space pointed to by <ret>.  If <ret> is NULL, then don't
  bother to return anything.  This routine returns a zero value on success and
  a non-zero value if it fails.

  
  int btree_Insert(BTREE tree, void *elem)
  
  Insert <elem> into the <tree>.  Returns zero on success and non-zero if it
  fails.

  
  int btree_Delete(BTREE tree, void *elem)

  Remove <elem> from the <tree>.  Returns zero on success and non-zero if it
  fails, i.e., <elem> is not in the <tree> to begin with.

  
  void btree_Destroy(BTREE tree)

  Close <tree> and free all of the memory that it used.
*/

 
#ifndef BTREE_H_INC
#define BTREE_H_INC

/* ---------- Incomplete data types ---------- */
typedef struct _btree_struct *BTREE;


/* ---------- Public function declarations ---------- */ 
static  BTREE btree_Create(size_t size, int (*cmp)(const void *, const void *));
static int btree_Search(BTREE tree, void *key, void *ret);
static int btree_Minimum(BTREE tree, void *ret);
static int btree_Maximum(BTREE tree, void *ret);
static int btree_Empty(BTREE tree);
static int btree_Successor(BTREE tree, void *key, void *ret);
static int btree_Predecessor(BTREE tree, void *key, void *ret);
static int btree_Insert(BTREE tree, void *data);
static int btree_Delete(BTREE tree, void *data);
static void btree_Destroy(BTREE);
static void btree_print(BTREE);	/* Do NOT use. */
#endif /* BTREE_H_INC */


/* btreepriv.h - Private declarations for btrees */

#ifndef BTREEPRIV_H_INC
#define BTREEPRIV_H_INC

/* ---------- Private data type declarations ---------- */
struct _btree_struct {
  struct node *root;
  size_t node_size;		/* Size of the node is unknown, e.g., R-B trees. */
  size_t elem_size;
  unsigned int num_elems;
  int (*cmp)(const void *, const void *);
};

typedef struct node {
  struct node *parent;
  struct node *left;
  struct node *right;
} *btnode;

/* ---------- Private macros ---------- */
#define root(t) (t->root)
#define node_size(t) (t->node_size)
#define elem_size(t) (t->elem_size)

#define parent(n) (n->parent)
#define left(n) (n->left)
#define right(n) (n->right)

#define data(t,n) (((char *)n) + node_size(t))
#define data_copy(t, d, s)      memcpy(d, s, elem_size(t))
#define data_compare(t, f, s)   ((t)->cmp)(f, s)

#endif /* BTREEPRIV_H_INC */


/* ---------- Private function declarations ---------- */
static btnode node_Minimum(btnode);
static btnode node_Maximum(btnode);
static btnode node_Search(BTREE, btnode, void *);
static void node_Close(btnode);
static btnode node_Successor(btnode);
static btnode node_Make(BTREE tree, void *data);
static void btree_reallyPrint(BTREE, btnode, unsigned int);
 
/* ---------------------------------------------------------------------- */
static BTREE btree_Create(size_t size, int (*cmp)(const void *, const void *))
{
  BTREE ret;

  ret = rpal_memory_alloc(sizeof(struct _btree_struct));
  if (ret) {
    ret->root = NULL;
    ret->node_size = sizeof(struct node);
    ret->elem_size = size;
    ret->cmp = cmp;
  }
  return ret;
}

 
/* ---------------------------------------------------------------------- */
static int btree_Search(BTREE tree, void *key, void *ret)
{
  btnode node;

  node = root(tree);

  node = node_Search(tree, node, key);

  if (node) {
       if (ret) data_copy(tree, ret, data(tree, node));
    return 0;
  }
  else {
    return 1;
  }
}

/* ---------------------------------------------------------------------- */
static btnode node_Search(BTREE tree, btnode node, void *key)
{
  int result;

  if (node) {
    while (node && (result = data_compare(tree, data(tree, node), key))) {
      if (result > 0) {
	node = left(node);
      }
      else {
	node = right(node);
      }
    }
  }
  return node;
}

 
/* ---------------------------------------------------------------------- */
static int btree_Minimum(BTREE tree, void *ret)
{
  btnode node;

  node = node_Minimum(root(tree));
  if (node) {
    data_copy(tree, ret, data(tree, node));
    return 0;
  }
  return 1;
}

/* ---------------------------------------------------------------------- */
static btnode node_Minimum(btnode node)
{
  if (node) {
    while (left(node)) {
      node = left(node);
    }
  }
  return node;
}

 
/* ---------------------------------------------------------------------- */
static int btree_Maximum(BTREE tree, void *ret)
{
  btnode node;

  node = node_Maximum(root(tree));
  if (node) {
    data_copy(tree, ret, data(tree, node));
    return 0;
  }
  return 1;
}

/* ---------------------------------------------------------------------- */
static btnode node_Maximum(btnode node)
{
  if (node) {
    while (right(node)) {
      node = right(node);
    }
  }
  return node;
}

 
/* ---------------------------------------------------------------------- */
static int btree_Empty(BTREE tree)
{
  return root(tree) ? 0 : 1;
}
 

/* ---------------------------------------------------------------------- */
static int btree_Successor(BTREE tree, void *key, void *ret)
{
  btnode node;

  if (!root(tree)) return 1;

  node = node_Search(tree, root(tree), key);
  
  if (!node) return 1;

  node = node_Successor(node);

  if (node) {
    data_copy(tree, ret, data(tree, node));
    return 0;
  }
  return 1;
}

/* ---------------------------------------------------------------------- */
static btnode node_Successor(btnode node)
{
  btnode node2;

  if (right(node)) {
    node = node_Minimum(right(node));
  }
  else {
    node2 = parent(node);
    while (node2 && node == right(node2)) {
      node = node2;
      node2 = parent(node);
    }
    node = node2;
  }
  return node;
}
 
/* ---------------------------------------------------------------------- */
static int btree_Predecessor(BTREE tree, void *key, void *ret)
{
  btnode node, node2;

  if (!root(tree)) return 1;

  node = node_Search(tree, root(tree), key);
  
  if (!node) return 1;

  if (left(node)) {
    node = node_Maximum(left(node));
  }
  else {
    node2 = parent(node);
    while (node2 && node == left(node2)) {
      node = node2;
      node2 = parent(node);
    }
    node = node2;
  }
  
  if (node) {
    data_copy(tree, ret, data(tree, node));
    return 0;
  }
  return 1;
}
 
/* ---------------------------------------------------------------------- */
static int btree_Insert(BTREE tree, void *data)
{
  btnode parent, node, newnode;

  newnode = node_Make(tree, data);
  if (!newnode) {
    return 1;
  }

  parent = NULL;
  node = root(tree);

  while (node) {
    parent = node;
    if (data_compare(tree, data, data(tree, node)) < 0) {
      node = left(node);
    }
    else {
      node = right(node);
    }
  }

  parent(newnode) = parent;

  if (!parent) {
    root(tree) = newnode;
  }
  else {
    if (data_compare(tree, data, data(tree, parent)) < 0) {
      left(parent) = newnode;
    }
    else {
      right(parent) = newnode;
    }
  }

  tree->num_elems++;

  return 0;
}

 
/* ---------------------------------------------------------------------- */
static int btree_Delete(BTREE tree, void *data)
{
  btnode node;
  btnode remove;
  btnode other;

  if (!root(tree)) return 1;

  node = node_Search(tree, root(tree), data);

  if (!node) return 1;
  
  if (!left(node) || !right(node)) {
    remove = node;
  }
  else {
    remove = node_Successor(node);
  }

  if (left(remove)) {
    other = left(remove);
  }
  else {
    other = right(remove);
  }

  if (other) {
    parent(other) = parent(remove);
  }
  
  if (!parent(remove)) {
    root(tree) = other;
  }
  else {
    if (remove == left(parent(remove))) {
      left(parent(remove)) = other;
    }
    else {
      right(parent(remove)) = other;
    }
  }

  if (node != remove) {
    data_copy(tree, data(tree, node), data(tree, remove));
  }
  if ( root(tree) == remove )
  {
      // should never get here...
     root(tree) = NULL;
  }
  //free(node);
  rpal_memory_free(remove);

  tree->num_elems--;

  return 0;
}

 
/* ---------------------------------------------------------------------- */
static void btree_Destroy(BTREE tree)
{
  if (root(tree)) {
    node_Close(root(tree));
  }
  rpal_memory_free(tree);
}

/* ---------------------------------------------------------------------- */
static void node_Close(btnode node)
{
  if (left(node)) {
    node_Close(left(node));
  }
  if (right(node)) {
    node_Close(right(node));
  }
  rpal_memory_free(node);
}

 
/* ---------------------------------------------------------------------- */
static btnode node_Make(BTREE tree, void *data)
{
  btnode ret;

  ret = rpal_memory_alloc(node_size(tree) + elem_size(tree));

  if (ret) {
    data_copy(tree, data(tree, ret), data);
    parent(ret) = left(ret) = right(ret) = NULL;
  }
  return ret;
}

/* ------------------------------------------------------------------------ */
/* 
  Note that this routine assumes that the data elements are char *'s
 */
#ifdef RPAL_PLATFORM_DEBUG
static void btree_print(BTREE tree)
{
  btree_reallyPrint(tree, root(tree), 2);
}

/* ------------------------------------------------------------------------ */
static void btree_reallyPrint(BTREE tree, btnode node, unsigned int spaces)
{
    unsigned int i = 0;
    if (node) {
    btree_reallyPrint(tree, left(node), spaces + 4);
    for( i = 0; i < spaces; i++ )
    {
        printf( " " );
    }
    printf( "*\n" );
    btree_reallyPrint(tree, right(node), spaces + 4);
  }
}
#endif

/* ---------------------------------------------------------------------- */
static unsigned int btree_NumElements(BTREE tree)
{
  unsigned int size = 0;
  if(tree)
  {
      size = tree->num_elems;
  }
  return size;
}


/* ---------------------------------------------------------------------- */
static btnode node_ListToTree(btnode* orderedList, unsigned int start, unsigned int end, btnode parent)
{
    btnode root = NULL;
    unsigned int middle = 0;

    if( orderedList &&
        start <= end )
    {
        middle = start + ( ( end - start ) / 2 );
        root = orderedList[ middle ];
        root->parent = parent;
        if( start != end )
        {
            if( 0 != middle )
            {
                root->left = node_ListToTree( orderedList, start, middle - 1, root );
            }
            if( (unsigned int)(-1) != middle )
            {
                root->right = node_ListToTree( orderedList, middle + 1, end, root );
            }
        }
        else
        {
            root->left = NULL;
            root->right = NULL;
        }
    }

    return root;
}

static int btree_Balance(BTREE tree)
{
    int ret = -1;
    btnode* ordered = NULL;
    unsigned int i = 0;
    btnode tmp = NULL;

    if(tree)
    {
        if( NULL != ( ordered = rpal_memory_alloc( sizeof( btnode ) * tree->num_elems ) ) )
        {
            if( NULL != ( tmp = node_Minimum( tree->root ) ) )
            {
                do
                {
                    ordered[ i ] = tmp;
                    i++;
                }
                while( NULL != ( tmp = node_Successor( tmp ) ) );

                if( NULL != ( tree->root = node_ListToTree( ordered, 0, tree->num_elems - 1, NULL ) ) )
                {
                    ret = 0;
                }
            }

            rpal_memory_free( ordered );
        }
    }

    return ret;
}








//=============================================================================
//  RPAL API
//=============================================================================
typedef struct
{
    rRwLock lock;
    BTREE tree;
    RU32 elemSize;
    rpal_btree_free_f freeElem;

} _rBTree;

rBTree
    rpal_btree_create
    (
        RU32 elemSize,
        rpal_btree_comp_f cmp,
        rpal_btree_free_f freeElem
    )
{
    rBTree tree = NULL;
    _rBTree* pTree = NULL;

    if( 0 != elemSize &&
        NULL != cmp )
    {
        tree = rpal_memory_alloc( sizeof( _rBTree ) );

        if( NULL != tree )
        {
            pTree = (_rBTree*)tree;

            pTree->elemSize = elemSize;

            pTree->freeElem = freeElem;

            pTree->lock = rRwLock_create();

            if( NULL != pTree->lock )
            {
                pTree->tree = btree_Create( elemSize, (int(*)(const void*,const void*))cmp );

                if( NULL == pTree->tree )
                {
                    rRwLock_free( pTree->lock );
                    rpal_memory_free( pTree );
                    pTree = NULL;
                }
            }
            else
            {
                rpal_memory_free( pTree );
                pTree = NULL;
            }
        }
    }

    return tree;
}

RVOID
    rpal_btree_destroy
    (
        rBTree tree,
        RBOOL isBypassLocks
    )
{
    _rBTree* pTree = (_rBTree*)tree;

    RPVOID elem = NULL;

    if( rpal_memory_isValid( tree ) )
    {
        if( NULL != ( elem = rpal_memory_alloc( pTree->elemSize ) ) )
        {
            if( !isBypassLocks &&
                rpal_memory_isValid( pTree->lock ) )
            {
                rRwLock_write_lock( pTree->lock );
            }

            if( NULL != pTree->tree )
            {
                while( 0 == btree_Maximum( pTree->tree, elem ) )
                {
                    btree_Delete( pTree->tree, elem );
                    if( NULL != pTree->freeElem )
                    {
                        pTree->freeElem( elem );
                    }
                }

                RPAL_ASSERT( 1 == btree_Empty( pTree->tree ) );
                RPAL_ASSERT( 0 == btree_NumElements( pTree->tree ) );

                btree_Destroy( pTree->tree );
            }

            if( rpal_memory_isValid( pTree->lock ) )
            {
                rRwLock_free( pTree->lock );
            }

            rpal_memory_free( elem );
        }

        rpal_memory_free( tree );
    }
}

RBOOL
    rpal_btree_search
    (
        rBTree tree,
        RPVOID key,
        RPVOID ret,
        RBOOL isBypassLocks
    )
{
    RBOOL isSuccess = FALSE;
    _rBTree* pTree = (_rBTree*)tree;

    if( rpal_memory_isValid( tree ) &&
        NULL != key )
    {
        if( isBypassLocks ||
            rRwLock_read_lock( pTree->lock ) )
        {
            if( 0 == btree_Search( pTree->tree, key, ret ) )
            {
                isSuccess = TRUE;
            }

            if( !isBypassLocks )
            {
                rRwLock_read_unlock( pTree->lock );
            }
        }
    }

    return isSuccess;
}

RBOOL
    rpal_btree_remove
    (
        rBTree tree,
        RPVOID key,
        RPVOID ret,
        RBOOL isBypassLocks
    )
{
    RBOOL isSuccess = FALSE;
    _rBTree* pTree = (_rBTree*)tree;

    if( rpal_memory_isValid( tree ) &&
        NULL != key )
    {
        if( isBypassLocks ||
            rRwLock_write_lock( pTree->lock ) )
        {
            if( 0 == btree_Search( pTree->tree, key, ret ) )
            {
                if( 0 == btree_Delete( pTree->tree, key ) )
                {
                    isSuccess = TRUE;
                }
                else
                {
                    if( NULL != ret )
                    {
                        rpal_memory_zero( ret, pTree->elemSize );
                    }
                }
            }

            if( !isBypassLocks )
            {
                rRwLock_write_unlock( pTree->lock );
            }
        }
    }

    return isSuccess;
}

RBOOL
    rpal_btree_isEmpty
    (
        rBTree tree,
        RBOOL isBypassLocks
    )
{
    RBOOL isEmpty = TRUE;
    _rBTree* pTree = (_rBTree*)tree;

    if( rpal_memory_isValid( tree ) )
    {
        if( isBypassLocks ||
            rRwLock_read_lock( pTree->lock ) )
        {
            if( 0 != btree_Empty( pTree->tree ) )
            {
                RPAL_ASSERT( 0 == btree_NumElements( pTree->tree ) );
                isEmpty = TRUE;
            }
            else
            {
                RPAL_ASSERT( 0 != btree_NumElements( pTree->tree ) );
            }

            if( !isBypassLocks )
            {
                rRwLock_read_unlock( pTree->lock );
            }
        }
    }

    return isEmpty;
}


RBOOL
    rpal_btree_add
    (
        rBTree tree,
        RPVOID val,
        RBOOL isBypassLocks
    )
{
    RBOOL isSuccess = FALSE;
    _rBTree* pTree = (_rBTree*)tree;

    if( rpal_memory_isValid( tree ) )
    {
        if( isBypassLocks ||
            rRwLock_write_lock( pTree->lock ) )
        {
            if( 0 != btree_Search( pTree->tree, val, NULL ) )
            {
                if( 0 == btree_Insert( pTree->tree, val ) )
                {
                    isSuccess = TRUE;
                }
            }

            if( !isBypassLocks )
            {
                rRwLock_write_unlock( pTree->lock );
            }
        }
    }

    return isSuccess;
}

RBOOL
    rpal_btree_manual_lock
    (
        rBTree tree
    )
{
    RBOOL isSuccess = FALSE;
    _rBTree* pTree = (_rBTree*)tree;

    if( rpal_memory_isValid( tree ) )
    {
        if( rRwLock_write_lock( pTree->lock ) )
        {
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}

RBOOL
    rpal_btree_manual_unlock
    (
        rBTree tree
    )
{
    RBOOL isSuccess = FALSE;
    _rBTree* pTree = (_rBTree*)tree;

    if( rpal_memory_isValid( tree ) )
    {
        if( rRwLock_write_unlock( pTree->lock ) )
        {
            isSuccess = TRUE;
        }
    }

    return isSuccess;
}


RBOOL
    rpal_btree_maximum
    (
        rBTree tree,
        RPVOID ret,
        RBOOL isBypassLocks
    )
{
    RBOOL isSuccess = FALSE;
    _rBTree* pTree = (_rBTree*)tree;

    if( rpal_memory_isValid( tree ) )
    {
        if( isBypassLocks ||
            rRwLock_read_lock( pTree->lock ) )
        {
            if( 0 == btree_Maximum( pTree->tree, ret ) )
            {
                isSuccess = TRUE;
            }

            if( !isBypassLocks )
            {
                rRwLock_read_unlock( pTree->lock );
            }
        }
    }

    return isSuccess;
}


RBOOL
    rpal_btree_minimum
    (
        rBTree tree,
        RPVOID ret,
        RBOOL isBypassLocks
    )
{
    RBOOL isSuccess = FALSE;
    _rBTree* pTree = (_rBTree*)tree;

    if( rpal_memory_isValid( tree ) )
    {
        if( isBypassLocks ||
            rRwLock_read_lock( pTree->lock ) )
        {
            if( 0 == btree_Minimum( pTree->tree, ret ) )
            {
                isSuccess = TRUE;
            }

            if( !isBypassLocks )
            {
                rRwLock_read_unlock( pTree->lock );
            }
        }
    }

    return isSuccess;
}


RBOOL
    rpal_btree_next
    (
        rBTree tree,
        RPVOID key,
        RPVOID ret,
        RBOOL isBypassLocks
    )
{
    RBOOL isSuccess = FALSE;
    _rBTree* pTree = (_rBTree*)tree;

    if( rpal_memory_isValid( tree ) )
    {
        if( isBypassLocks ||
            rRwLock_read_lock( pTree->lock ) )
        {
            if( 0 == btree_Successor( pTree->tree, key, ret ) )
            {
                isSuccess = TRUE;
            }

            if( !isBypassLocks )
            {
                rRwLock_read_unlock( pTree->lock );
            }
        }
    }

    return isSuccess;
}


RBOOL
    rpal_btree_previous
    (
        rBTree tree,
        RPVOID key,
        RPVOID ret,
        RBOOL isBypassLocks
    )
{
    RBOOL isSuccess = FALSE;
    _rBTree* pTree = (_rBTree*)tree;

    if( rpal_memory_isValid( tree ) )
    {
        if( isBypassLocks ||
            rRwLock_read_lock( pTree->lock ) )
        {
            if( 0 == btree_Predecessor( pTree->tree, key, ret ) )
            {
                isSuccess = TRUE;
            }

            if( !isBypassLocks )
            {
                rRwLock_read_unlock( pTree->lock );
            }
        }
    }

    return isSuccess;
}

RU32
    rpal_btree_getSize
    (
        rBTree tree,
        RBOOL isBypassLocks
    )
{
    RU32 size = 0;
    _rBTree* pTree = (_rBTree*)tree;

    if( rpal_memory_isValid( tree ) )
    {
        if( isBypassLocks ||
            rRwLock_read_lock( pTree->lock ) )
        {
            size = btree_NumElements( pTree->tree );
            
            if( !isBypassLocks )
            {
                rRwLock_read_unlock( pTree->lock );
            }
        }
    }

    return size;
}

RBOOL
    rpal_btree_optimize
    (
        rBTree tree,
        RBOOL isBypassLocks
    )
{
    RBOOL isSuccess = FALSE;
    _rBTree* pTree = (_rBTree*)tree;

    if( rpal_memory_isValid( tree ) )
    {
        if( isBypassLocks ||
            rRwLock_read_lock( pTree->lock ) )
        {
            // Inserts into the tree do not balance it as we
            // go since we may do bulk inserts before querying
            // which is usually the case. So now we will
            // rebalance the entire tree to make searches efficient.
            //btree_print( pTree->tree );
            if( 0 == btree_Balance( pTree->tree ) )
            {
                //btree_print( pTree->tree );
                isSuccess = TRUE;
            }

            if( !isBypassLocks )
            {
                rRwLock_read_unlock( pTree->lock );
            }
        }
    }

    return isSuccess;
}

