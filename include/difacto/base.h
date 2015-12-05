/*!
 * Copyright (c) 2015 by Contributors
 */
#ifndef DIFACTO_BASE_H_
#define DIFACTO_BASE_H_
#include <string>
#include <utility>
#include <vector>

namespace difacto {

/*!
 * \brief use float as the weight and gradient type
 */
typedef float real_t;

/*!
 * \brief use uint64_t as the feature index type
 */
typedef uint64_t feaid_t;

/**
 * \brief a list of keyword arguments
 */
typedef std::vector<std::pair<std::string, std::string>> KWArgs;

/**
 * \brief reverse the bytes of x to make it more uniformly spanning the space
 */
inline feaid_t ReverseBytes(feaid_t x) {
  // return x;
  x = x << 32 | x >> 32;
  x = (x & 0x0000FFFF0000FFFFULL) << 16 |
      (x & 0xFFFF0000FFFF0000ULL) >> 16;
  x = (x & 0x00FF00FF00FF00FFULL) << 8 |
      (x & 0xFF00FF00FF00FF00ULL) >> 8;
  x = (x & 0x0F0F0F0F0F0F0F0FULL) << 4 |
      (x & 0xF0F0F0F0F0F0F0F0ULL) >> 4;
  return x;
}

}  // namespace difacto
#endif  // DIFACTO_BASE_H_