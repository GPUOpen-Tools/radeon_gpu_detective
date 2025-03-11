//=============================================================================
// Copyright (c) 2018-2025 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief A 128-bit uint used for hashes.
//=============================================================================

#ifndef RADEON_GPU_DETECTIVE_SOURCE_RGD_HASH_H_
#define RADEON_GPU_DETECTIVE_SOURCE_RGD_HASH_H_

#include <cstdint>

/// @brief Struct to hold a 128 bit hash.
typedef struct Rgd128bitHash
{
    /// @brief Default constructor
    Rgd128bitHash()
        : low(0)
        , high(0)
    {
    }

    /// @brief Constructor that takes the hash.
    ///
    /// @param [in] input_low  The low 64 bits of the hash.
    /// @param [in] input_high The high 64 bits of the hash.
    Rgd128bitHash(const uint64_t input_low, const uint64_t input_high)
    {
        low  = input_low;
        high = input_high;
    }

    // The low 64 bits of the hash.
    uint64_t low;

    // The high 64 bits of the hash.
    uint64_t high;
} Rgd128bitHash;

/// @brief Compare two 128-bit hashes for equality.
/// @param [in] a The first hash to compare.
/// @param [in] b The second hash to compare.
/// @return true if the hashes are equal.
static bool Rgd128bitHashCompare(const Rgd128bitHash& a, const Rgd128bitHash& b)
{
    return (a.low == b.low) && (a.high == b.high);
}

/// @brief Check if a 128-bit hash is zero.
/// @param [in] a The hash to check.
/// @return true if the hash is zero.
static bool Rgd128bitHashIsZero(const Rgd128bitHash& a)
{
    return (a.low == 0) && (a.high == 0);
}

/// @brief Copy one 128-bit hash to another.
/// @param [out] dest The destination hash.
/// @param [in] src The source hash.
static void Rgd128bitHashCopy(Rgd128bitHash& dest, const Rgd128bitHash& src)
{
    dest.low  = src.low;
    dest.high = src.high;
}

/// @brief Overloaded operator to check if one hash is less than another hash.
///
/// @param [in] a The first hash to compare.
/// @param [in] b The second to compare.
///
/// @return true if the first hash is less than the second hash.
inline bool operator<(const Rgd128bitHash& a, const Rgd128bitHash& b)
{
    if (a.high < b.high)
    {
        return true;
    }

    if (a.high > b.high)
    {
        return false;
    }

    // The hi bits of the hash are equal, use the low bits as a tiebreaker.
    return (a.low < b.low);
}

/// @brief Overloaded operator to check if two hashes are equal.
///
/// @param [in] a The first hash to compare.
/// @param [in] b The second to compare.
///
/// @return true if the hashes are equal.
inline bool operator==(const Rgd128bitHash& a, const Rgd128bitHash& b)
{
    return Rgd128bitHashCompare(a, b);
}

/// @brief Overloaded operator to check if two hashes are not equal.
///
/// @param [in] a The first hash to compare.
/// @param [in] b The second to compare.
///
/// @return true if the hashes are not equal.
inline bool operator!=(const Rgd128bitHash& a, const Rgd128bitHash& b)
{
    return !Rgd128bitHashCompare(a, b);
}

#endif  // RADEON_GPU_DETECTIVE_SOURCE_RGD_HASH_H_