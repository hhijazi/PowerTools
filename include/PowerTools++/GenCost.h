//
//  GenCost.h
//  PowerTools++
//
//  Created by Hassan Hijazi on 30/01/2015.
//  Copyright (c) 2015 NICTA. All rights reserved.
//

#ifndef __PowerTools____GenCost__
#define __PowerTools____GenCost__
#include <vector>
#include <stdio.h>

class GenCost{
public:
/** @brief Constant cost coefficient */
double c0;
/** @brief Linear cost coefficient */
double c1;
/** @brief Quadratic cost coefficient */
double c2;
};



class GenTimeCost{
public:
    /** @brief Constant cost coefficient */
    std::vector<double> c0;
    /** @brief Linear cost coefficient */
    std::vector<double> c1;
    /** @brief Quadratic cost coefficient */
    std::vector<double> c2;
};

#endif /* defined(__PowerTools____GenCost__) */
