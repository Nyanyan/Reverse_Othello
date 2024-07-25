/*
    Egaroucid Project

    @file bit.hpp
        Bit manipulation
    @date 2021-2024
    @author Takuto Yamana
    @license GPL-3.0 license
    @notice I referred to codes written by others
*/

#pragma once
#include "setting.hpp"
#include "common.hpp"
#if USE_SIMD
    #include "bit_simd.hpp"
#else
    #include "bit_generic.hpp"
#endif