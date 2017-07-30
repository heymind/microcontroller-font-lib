#pragma once

#include <stdint.h>
#include <stdbool.h>
#ifndef FF_ERROR
#define FF_ERROR int
#endif

#ifdef DEBUG
#define CHK_OUTPUT(f, rst, msg) DEBUG("\n{%s}=%d | %s |%s()|[%s:%d]\n", #f, rst, msg, __func__, __FILE__, __LINE__)

#else
#ifdef LOG
#define CHK_OUTPUT(ignore, rst, msg) LOG("code=%d|%s \n", rst, msg)
#else
#define CHK_OUTPUT(...) ((void)0)
#endif
#endif

#define CHK_ERR_(f_) CHK_ERR(f_, "")
#define CHK_ERR(f_, msg)              \
    {                                 \
        int rst;                      \
        if ((rst = (f_)))             \
        {                             \
            CHK_OUTPUT(f_, rst, msg); \
            abort();                  \
        }                             \
    }                                 \
    while (0)
#define CHK_ERR_GOTO_(f_, where) CHK_ERR_GOTO(f_, "", where)
#define CHK_ERR_GOTO(f_, msg, where)  \
    {                                 \
        int rst;                      \
        if ((rst = (f_)))             \
        {                             \
            CHK_OUTPUT(f_, rst, msg); \
            goto where;               \
        }                             \
    }                                 \
    while (0)

#define CHK_OK_(f_) CHK_OK(f_, "")
#define CHK_OK(f_, msg)             \
    {                               \
        if (!(f_))                  \
        {                           \
            CHK_OUTPUT(f_, 0, msg); \
            abort();                \
        }                           \
    }                               \
    while (0)
#define CHK_OK_GOTO_(f_, where) CHK_OK_GOTO(f_, "", where)
#define CHK_OK_GOTO(f_, msg, where) \
    {                               \
        if (!(f_))                  \
        {                           \
            CHK_OUTPUT(f_, 0, msg); \
            goto where;             \
        }                           \
    }                               \
    while (0)

typedef struct __attribute__((__packed__))
{
    uint8_t version;
    uint8_t flags;
} FFHeader;

static bool FFHeaderIs1Bit(const FFHeader *header)
{
    return (header->flags & 1) == 1;
}

static bool FFHeaderIsMono(const FFHeader *header)
{
    return ((header->flags >> 1) & 1) == 1;
}

typedef struct __attribute__((__packed__))
{
    uint8_t left;
    uint8_t top;
    uint8_t width;
    uint8_t advance;
    uint32_t offset;
} FFMeta;

typedef struct __attribute__((__packed__))
{
    uint16_t start; //utf-16 plane 1
    uint16_t end;
} FFMetaHeader;
