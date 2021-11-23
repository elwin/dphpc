#pragma once

#include <cinttypes>
#include <cstddef>
#include <utility>

// Rank of the root process
static constexpr int ROOT = 0;

#define TAG_BASE 123
#define TAG_VALIDATE TAG_BASE + 1
#define TAG_TIMING TAG_BASE + 2
#define TAG_ALLGATHER_ASYNC TAG_BASE + 3
#define TAG_ALLREDUCE_BUTTERFLY TAG_BASE + 4
#define TAG_ALLREDUCE_BUTTERFLY_REDUCE TAG_BASE + 5
#define TAG_ALLREDUCE_RABENSEIFNER TAG_BASE + 6
#define TAG_RABENSEIFNER_GATHER TAG_BASE + 7
#define TAG_RABENSEIFNER_GATHER_VEC_A TAG_BASE + 8
#define TAG_RABENSEIFNER_GATHER_VEC_B TAG_BASE + 9
#define TAG_G_RABENSEIFNER TAG_BASE + 10
#define TAG_ALLGATHER_BUTTERFLY TAG_BASE + 4
#define TAG_BRUCK_ASYNC TAG_BASE + 11

#ifdef NDEBUG
#define debug_log(...) ((void)0)
#else
#define debug_log(...) fprintf(stderr, __VA_ARGS__)
#endif

#define HERE(rank, fmt, ...) debug_log("[%d]%s:%d: " fmt "\n", rank, __FILE__, __LINE__, __VA_ARGS__)
#define HERE1(rank, msg) HERE(rank, "%s", msg)
#define HERE0(rank) HERE1(rank, __func__)
