/*
===============================================================================

						ZONE MEMORY ALLOCATION

===============================================================================
*/

#pragma once

#include <cstddef> // size_t

#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif

// Stack allocation
#ifdef _WIN32
#define Z_StackAlloc(x) _alloca(x)
#else
#define Z_StackAlloc(x) alloca(x)
#endif

// Normal
void	*Z_Malloc( size_t size );
void	*Z_Calloc( size_t size );
void	*Z_Realloc( void *block, size_t size );
char	*Z_CopyString( const char *in );
void	Z_Free( void *block );

// Tagged
void	*Z_TagMalloc( size_t size, uint16 tag );
void	Z_TagFree( void *block );
void	Z_TagFreeGroup( uint16 tag );

// Status
void	Z_Init();
void	Z_Shutdown();	// Warns of unfreed allocations
