#include "labgpio.hpp"

LPC_GPIO_TypeDef * GPIO_ports[6] = {LPC_GPIO0, LPC_GPIO1, LPC_GPIO2, LPC_GPIO3, LPC_GPIO4, LPC_GPIO5}; 
IsrPointer Lab_GPIO::pin_isr_map[kPorts][kPins] = {nullptr};  
  
  /* You should not modify any hardware registers at this point
   * You should store the port and pin using the constructor.
   */
       /// @param port - port number between 0 and 5
       /// @param pin - pin number between 0 and 32

  void Lab_GPIO::SetAsInput()                 /// Sets this GPIO as an input  
  {
    GPIO_ports[thePort]->DIR |= ~(1 << thePin);
  }
  
  void Lab_GPIO::SetAsOutput()          /// Sets this GPIO as an output
  {
    GPIO_ports[thePort]->DIR |= (1 << thePin);
  }
  
  void Lab_GPIO::SetDirection(Direction direction)            /// Sets this GPIO as an input
    /// @param output - true => output, false => set pin to input
  {
    if(direction == Direction::kOutput) SetAsOutput();
    else                                SetAsInput(); 
  }
  
  void Lab_GPIO::SetHigh()                  /// Set voltage of pin to HIGH
  {
    GPIO_ports[thePort]->SET |= (1 << thePin);
  }
  
  void Lab_GPIO::SetLow()           /// Set voltage of pin to LOW 
  {
    GPIO_ports[thePort]->CLR |= (1 << thePin);
  }
  
  void Lab_GPIO::set(State state)         /// Set pin state to high or low 
                    /// depending on the input state parameter.
                          /// Has no effect if the pin is set as "input".
    /// @param state - State::kHigh => set pin high, State::kLow => set pin low
  {
    if(state == State::kHigh) //if expression is 0, PIN is currently input
             SetHigh();
        else SetLow();
  }
  
  Lab_GPIO::State Lab_GPIO::Read()                /// Should return the state of the pin 
                    /// (input or output, doesn't matter)
    /// @return level of pin high => true, low => false
  {
      if(GPIO_ports[thePort]->PIN & (1 << thePin))
           return State::kHigh;
      else return State::kLow;
  }
 
  bool Lab_GPIO::ReadBool()           /// Should return the state of the pin 
                    /// (input or output, doesn't matter)
      /// @return level of pin high => true, low => false
  {
      return (bool) Read();
  }

  void Lab_GPIO::resetResistor()
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
  void Lab_GPIO::enablePullDownResistor()
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

  // This handler should place a function pointer within the lookup table for 
  // the GpioInterruptHandler() to find.
  //
  // @param isr  - function to run when the interrupt event occurs.
  // @param edge - condition for the interrupt to occur on.
  
  void Lab_GPIO::AttachInterruptHandler(IsrPointer isr, Edge edge)
  {
      //assign the correct edge per port (ports 0 & 2)
      if(thePort == 0 || thePort == 2)
      {
          switch(thePort)
          {
            case 0:
              switch(edge)
              {
                case Edge::kNone:    printf("No edge case was specified.\n"); break;
                case Edge::kRising:  LPC_GPIOINT->IO0IntEnR |= (1 << thePin); break;
                case Edge::kFalling: LPC_GPIOINT->IO0IntEnF |= (1 << thePin); break;
                case Edge::kBoth:    LPC_GPIOINT->IO0IntEnR |= (1 << thePin);
                                     LPC_GPIOINT->IO0IntEnF |= (1 << thePin); break;
                default: printf("Not a valid edge for interrupts.\n");
              }
              pin_isr_map[0][thePin] = isr; 
              break;
            case 2:
              switch(edge)
              {
                case Edge::kNone:    printf("No edge case was specified.\n"); break;
                case Edge::kRising:  LPC_GPIOINT->IO2IntEnR |= (1 << thePin); break;
                case Edge::kFalling: LPC_GPIOINT->IO2IntEnF |= (1 << thePin); break;
                case Edge::kBoth:    LPC_GPIOINT->IO2IntEnR |= (1 << thePin);
                                     LPC_GPIOINT->IO2IntEnF |= (1 << thePin); break;
                default: printf("Not a valid edge for interrupts.\n");
              }
              pin_isr_map[1][thePin] = isr; 
              break; 
            default: printf("Not a valid port for interrupts.\n");
          }           
      }
      //EnableInterrupts();
  }

  void Lab_GPIO::EnableInterrupts()
  {
      RegisterIsr(GPIO_IRQn, GpioInterruptHandler);
  }

  // This function is invoked by NVIC via the GPIO peripheral asynchronously.
  // This ISR should do the following:
  //  1) Find the Port and Pin that caused the interrupt via the IO0IntStatF,
  //     IO0IntStatR, IO2IntStatF, and IO2IntStatR registers.
  //  2) Lookup and invoke the user's registered callback.
  //
  // VERY IMPORTANT!
  //  - Be sure to clear the interrupt flag that caused this interrupt, or this 
  //    function will be called repetitively and lock your system.
  //  - NOTE that your code needs to be able to handle two GPIO interrupts 
  //    occurring at the same time.
  void Lab_GPIO::GpioInterruptHandler()
  {
      if(LPC_GPIOINT->IO0IntStatF | LPC_GPIOINT->IO0IntStatR)
      {
        for(int i = 0; i < 32; i++)
        {
            if((LPC_GPIOINT->IO0IntStatF & (1 << i)) || (LPC_GPIOINT->IO0IntStatR & (1 << i)))
            {
              pin_isr_map[0][i]();
              LPC_GPIOINT->IO0IntClr |= (1 << i);
              break;
            }    
        }
            //iterate through all pins to find the set pin
        //LPC_GPIOINT->IO0IntClr |= (1 << isr_index);
      }
      if(LPC_GPIOINT->IO2IntStatF | LPC_GPIOINT->IO2IntStatR)
      {
        for(int i = 0; i < 32; i++)
        {
            if((LPC_GPIOINT->IO2IntStatF & (1 << i)) || (LPC_GPIOINT->IO2IntStatR & (1 << i)))
            {
              pin_isr_map[1][i]();
              LPC_GPIOINT->IO2IntClr |= (1 << i);
              break;
            }
        }

            //iterate through all pins to find the set pin
        //LPC_GPIOINT->IO2IntClr |= (1 << isr_index);
      }
  }