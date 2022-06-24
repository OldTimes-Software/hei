/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

PL_EXTERN_C

/* lower-level function pointers
 * these point to the standard C functions by default
 * DO NOT USE DIRECTLY, use *Alloc and *Free functions instead! */
extern PL_DLL void *( *pl_malloc )( size_t size );
extern PL_DLL void *( *pl_calloc )( size_t num, size_t size );
extern PL_DLL void *( *pl_realloc )( void *ptr, size_t newSize );
extern PL_DLL void ( *pl_free )( void *ptr );

/* callback for catching out of memory errors
 * only gets called if calling with abort on error */
typedef void ( *PLMemoryAbortCallbackT )( size_t failSize );
extern PL_DLL PLMemoryAbortCallbackT pl_memory_abort_cb;

void *PlCAlloc( size_t num, size_t size, bool abortOnFail );
void *PlMAlloc( size_t size, bool abortOnFail );
void *PlReAlloc( void *ptr, size_t newSize, bool abortOnFail );
void PlFree( void *ptr );

extern size_t PlGetTotalAllocatedMemory( void );

extern uint64_t PlGetTotalSystemMemory( void );
extern uint64_t PlGetTotalAvailableSystemMemory( void );
extern uint64_t PlGetCurrentMemoryUsage( void );

#define PlCAllocA( NUM, SIZE )     PlCAlloc( ( NUM ), ( SIZE ), true )
#define PlMAllocA( SIZE )          PlMAlloc( ( SIZE ), true )
#define PlReAllocA( PTR, NEWSIZE ) PlReAlloc( ( PTR ), ( NEWSIZE ), true )

#define PL_NEW( TYPE )       PlMAllocA( sizeof( TYPE ) )
#define PL_NEW_( TYPE, NUM ) PlCAllocA( NUM, sizeof( TYPE ) )
#define PL_DELETE( PTR )     PlFree( PTR )

PL_EXTERN_C_END
