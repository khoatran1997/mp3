#include "labgpio.hpp"

#include "L0_LowLevel/LPC40xx.h"

LPC_GPIO_TypeDef * GPIO_ports[6] = {LPC_GPIO0, LPC_GPIO1, LPC_GPIO2, LPC_GPIO3, LPC_GPIO4, LPC_GPIO5}; 
  /* You should not modify any hardware registers at this point
   * You should store the port and pin using the constructor.
   */
      /// @param port - port number between 0 and 5
      /// @param pin - pin number between 0 and 32

  void LabGPIO::SetAsInput()              /// Sets this GPIO as an input
  {
      GPIO_ports[thePort]->DIR |= ~(1 << thePin);
  }
  
  void LabGPIO::SetAsOutput()           /// Sets this GPIO as an output
  {
      GPIO_ports[thePort]->DIR |= (1 << thePin);
  }
  
  void LabGPIO::SetDirection(Direction direction) /// Sets this GPIO as an input
      /// @param output - true => output, false => set pin to input
  {
      if(direction == Direction::kOutput) SetAsOutput();
      else                                SetAsInput(); 
  }
  
  void LabGPIO::SetHigh()             /// Set voltage of pin to HIGH
  {
      GPIO_ports[thePort]->SET |= (1 << thePin);
  }
  
  void LabGPIO::SetLow()                /// Set voltage of pin to LOW 
  {
      GPIO_ports[thePort]->CLR |= (1 << thePin);
  }
  
  void LabGPIO::set(State state)            /// Set pin state to high or low 
                        /// depending on the input state parameter.
                        /// Has no effect if the pin is set as "input".
      /// @param state - State::kHigh => set pin high, State::kLow => set pin low
  {
      if(state == State::kHigh) //if expression is 0, PIN is currently input
           SetHigh();
      else SetLow();
  }
  
  LabGPIO::State LabGPIO::Read()                /// Should return the state of the pin 
                        /// (input or output, doesn't matter)
      /// @return level of pin high => true, low => false
  {
      if(GPIO_ports[thePort]->PIN & (1 << thePin))
           return State::kHigh;
      else return State::kLow;
  }
 
  bool LabGPIO::ReadBool()              /// Should return the state of the pin 
                        /// (input or output, doesn't matter)
     /// @return level of pin high => true, low => false
  {
      return (bool) Read();
  }

  void LabGPIO::resetResistor()
  {
      switch(thePort)
      {
          case 0: if(thePin == 30)      LPC_IOCON -> P0_30 &= ~(0b11 << 3);
                  else if(thePin == 29) LPC_IOCON -> P0_29 &= ~(0b11 << 3);
                  break;
          case 1: if(thePin == 19)      LPC_IOCON -> P1_19 &= ~(0b11 << 3);
                  else if(thePin == 15) LPC_IOCON -> P1_15 &= ~(0b11 << 3);
                  break;
          default:printf("Cannot reset pull down resistor.\n");
      }
  }
  void LabGPIO::enablePullDownResistor()
  {
      switch(thePort)
      {
          case 0: if(thePin == 30)     LPC_IOCON -> P0_30 |= (0b1 << 3);
                  else if(thePin == 29)LPC_IOCON -> P0_29 |= (0b1 << 3);
                  break;
          case 1: if(thePin == 19)     LPC_IOCON -> P1_19 |= (0b1 << 3);
                  else if(thePin == 15)LPC_IOCON -> P1_15 |= (0b1 << 3);
                  break;
          default:printf("Cannot set pull down resistor.\n");
      }
  }