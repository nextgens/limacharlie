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

#ifndef _RPAL_BTREE_H
#define _RPAL_BTREE_H
#include <rpal/rpal.h>


rBTree
    rpal_btree_create
    (
        RU32 elemSize,
        rpal_btree_comp_f cmp,
        rpal_btree_free_f freeElem
    );

RVOID
    rpal_btree_destroy
    (
        rBTree tree,
        RBOOL isBypassLocks
    );

RBOOL
    rpal_btree_search
    (
        rBTree tree,
        RPVOID key,
        RPVOID pRet,
        RBOOL isBypassLocks
    );

RBOOL
    rpal_btree_remove
    (
        rBTree tree,
        RPVOID key,
        RPVOID ret,
        RBOOL isBypassLocks
    );

RBOOL
    rpal_btree_isEmpty
    (
        rBTree tree,
        RBOOL isBypassLocks
    );

RBOOL
    rpal_btree_add
    (
        rBTree tree,
        RPVOID val,
        RBOOL isBypassLocks
    );

RBOOL
    rpal_btree_manual_lock
    (
        rBTree tree
    );

RBOOL
    rpal_btree_manual_unlock
    (
        rBTree tree
    );

RBOOL
    rpal_btree_maximum
    (
        rBTree tree,
        RPVOID ret,
        RBOOL isBypassLocks
    );

RBOOL
    rpal_btree_minimum
    (
        rBTree tree,
        RPVOID ret,
        RBOOL isBypassLocks
    );

RBOOL
    rpal_btree_next
    (
        rBTree tree,
        RPVOID key,
        RPVOID ret,
        RBOOL isBypassLocks
    );

RBOOL
    rpal_btree_previous
    (
        rBTree tree,
        RPVOID key,
        RPVOID ret,
        RBOOL isBypassLocks
    );

RU32
    rpal_btree_getSize
    (
        rBTree tree,
        RBOOL isBypassLocks
    );

RBOOL
    rpal_btree_optimize
    (
        rBTree tree,
        RBOOL isBypassLocks
    );

#endif
