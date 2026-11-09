#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef bool boolean;
typedef float float32;
typedef uint8 Std_ReturnType;

#define E_OK ((Std_ReturnType)0u)
#define E_NOT_OK ((Std_ReturnType)1u)

#endif
