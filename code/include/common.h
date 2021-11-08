#pragma once

#include <cinttypes>
#include <cstddef>
#include <utility>

#define TAG_BASE 123
#define TAG_VALIDATE TAG_BASE + 1
#define TAG_TIMING TAG_BASE + 2
#define TAG_ALLGATHER_ASYNC TAG_BASE + 3
#define TAG_ALLREDUCE_BUTTERFLY TAG_BASE + 4
#define TAG_ALLREDUCE_BUTTERFLY_REDUCE TAG_BASE + 5
