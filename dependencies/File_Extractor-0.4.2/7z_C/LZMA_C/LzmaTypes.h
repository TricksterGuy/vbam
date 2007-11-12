/* 
LzmaTypes.h 

Types for LZMA Decoder

This file written and distributed to public domain by Igor Pavlov.
This file is part of LZMA SDK 4.40 (2006-05-01)
*/

#ifndef __LZMATYPES_H
#define __LZMATYPES_H

/* added */
#define _LZMA_IN_CB

typedef unsigned char Byte;
typedef unsigned short UInt16;

/* added */
#include <limits.h>
#if ULONG_MAX == 0xFFFFFFFF
	#define _LZMA_UINT32_IS_ULONG
#endif

#ifdef _LZMA_UINT32_IS_ULONG
typedef unsigned long UInt32;
#else
typedef unsigned int UInt32;
#endif

/* #define _LZMA_SYSTEM_SIZE_T */
/* Use system's size_t. You can use it to enable 64-bit sizes supporting */

#ifdef _LZMA_SYSTEM_SIZE_T
#include <stddef.h>
typedef size_t SizeT;
#else
typedef UInt32 SizeT;
#endif

#endif
