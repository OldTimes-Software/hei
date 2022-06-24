/*
MIT License

Copyright (c) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <plcore/pl.h>

PL_EXTERN_C

typedef struct PLLinkedList PLLinkedList;
typedef struct PLLinkedListNode PLLinkedListNode;

PLLinkedList *PlCreateLinkedList( void );

PLLinkedListNode *PlInsertLinkedListNode( PLLinkedList *list, void *userPtr );

void PlDestroyLinkedList( PLLinkedList *list );
void PlDestroyLinkedListNode( PLLinkedList *list, PLLinkedListNode *node );
void PlDestroyLinkedListNodes( PLLinkedList *list );

PLLinkedListNode *PlGetNextLinkedListNode( PLLinkedListNode *node );
PLLinkedListNode *PlGetPrevLinkedListNode( PLLinkedListNode *node );
PLLinkedListNode *PlGetFirstNode( PLLinkedList *list );
void *PlGetLinkedListNodeUserData( PLLinkedListNode *node );
void PlSetLinkedListNodeUserData( PLLinkedListNode *node, void *userPtr );

unsigned int PlGetNumLinkedListNodes( PLLinkedList *list );

PLLinkedList *PlGetLinkedListNodeContainer( PLLinkedListNode *node );

typedef void ( *PLLinkedListIteratorCallback )( void *userData, bool *breakEarly );
void PlIterateLinkedList( PLLinkedList *linkedList, PLLinkedListIteratorCallback callbackHandler );

void **PlArrayFromLinkedList( PLLinkedList *list, unsigned int *numElements );

PL_EXTERN_C_END
