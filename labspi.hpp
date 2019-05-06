#pragma once
#include <stdint.h>
#include "L0_LowLevel/LPC40xx.h"
#include <stdio.h>

class LabSpi
{
 public:


    enum FrameModes : uint8_t
    {
        /* Fill this out based on the datasheet. */
        spi = 0,
        ti = 1,
        micro= 2,
    };


    bool Initialize(uint8_t data_size_select, FrameModes format, uint8_t divide);

    uint8_t Transfer(uint8_t send);

 private:
	// Fill in as needed 
    //static bool lock;
    void delay();
    // bool test_and_set(bool mutex);
    
};