#pragma once
#include <stdint.h>
#include "L0_LowLevel/interrupt.hpp"
#include "L0_LowLevel/LPC40xx.h"
#include <stdio.h>



class LabGPIO
{
 public:
  enum class Direction : uint8_t
  {
    kInput  = 0,
    kOutput = 1
  };
  enum class State : uint8_t
  {
    kLow  = 0,
    kHigh = 1
  };
  enum class Edge
  {
    kNone = 0,
    kRising,
    kFalling,
    kBoth
  };
  static constexpr size_t kPorts = 2;
  static constexpr size_t kPins = 32; 
  // This handler should place a function pointer within the lookup table for 
  // the GpioInterruptHandler() to find.
  //
  // @param isr  - function to run when the interrupt event occurs.
  // @param edge - condition for the interrupt to occur on.
  void AttachInterruptHandler(IsrPointer isr, Edge edge);
  // Register GPIO_IRQn here
  static void EnableInterrupts();

constexpr LabGPIO(uint8_t port, uint8_t pin):pinNum(pin),portNum(port){
};


void SetAsInput();
void SetAsOutput();
void SetDirection(Direction direction);
void SetHigh();
void SetLow();
void set(State state);
State Read();
bool ReadBool();
private:
	LPC_GPIO_TypeDef* portPtr[6]={LPC_GPIO0,LPC_GPIO1,LPC_GPIO2,LPC_GPIO3,LPC_GPIO4,LPC_GPIO5};
	//uint32_t risingEn[2] = {LPC_GPIOINT->IO0IntEnR,LPC_GPIOINT->IO2IntEnR};
	uint8_t pinNum;
	uint8_t portNum;
	static void GpioInterruptHandler();
	//static IsrPointer pin_isr_map[kPorts][kPins] = { nullptr };
	static IsrPointer pin_isr_map[kPorts][kPins];
};
