#include "labspi.hpp"


// bool LabSpi::lock=false;

void LabSpi::delay(){
    for(int i=0;i<500;i++){
    }
}



    // bool LabSpi::test_and_set(bool mutex){
    //     if(mutex) return true;
    //     else{
    //         mutex=true;
    //         return false;
    //     }
    // }

	bool LabSpi::Initialize(uint8_t data_size_select, LabSpi::FrameModes format, uint8_t divide){

        if (data_size_select<4 || data_size_select>16) return false;
        
        //power
        LPC_SC->PCONP |= (1<<20);

        //pin
        LPC_IOCON->P1_0 = (LPC_IOCON->P1_0 & ~0b111) | 0b100;
        LPC_IOCON->P1_1 = (LPC_IOCON->P1_1 & ~0b111) | 0b100;
        LPC_IOCON->P1_4 = (LPC_IOCON->P1_4 & ~0b111) | 0b100;

        //enable
        LPC_SSP2->CR1 |= (1<<1);

        //cs
        LPC_GPIO1->DIR |= (1<<10);
        LPC_GPIO1->PIN |= (1<<10);
        LPC_GPIO0->DIR |= (1<<6);
        LPC_GPIO0->PIN |= (1<<6);

        //cr0
        // LPC_SSP2->CR0 = (LPC_SSP2->CR0 & ~0b111111) | 0b000111;
        LPC_SSP2->CR0 &= ~0xffff;
        LPC_SSP2->CR0 |= data_size_select-1;
        LPC_SSP2->CR0 |= format<<4;
        LPC_SSP2->CPSR |= divide;
        return true;

    }

    uint8_t LabSpi::Transfer(uint8_t send){
        //while(test_and_set(lock));
        LPC_SSP2->DR = send;
        //delay();
        while(LPC_SSP2->SR & (1<<4));
        //uint8_t temp = (uint8_t)(LPC_SSP2->DR & 0xFF);
        //lock = false;
        // return temp;
        return (uint8_t)(LPC_SSP2->DR & 0xFF);
    }


 