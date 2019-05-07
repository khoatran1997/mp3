#pragma once 

#include <cstdint>
#include <iterator>
#include "utility/time.hpp"
#include "utility/log.hpp"

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
  
  /* You should not modify any hardware registers at this point
   * You should store the port and pin using the constructor.
   */
  
  constexpr LabGPIO(uint8_t port, uint8_t pin) :
                    thePort(port), thePin(pin){};
      /// @param port - port number between 0 and 5
      /// @param pin - pin number between 0 and 32
  
  void SetAsInput();              /// Sets this GPIO as an input
  void SetAsOutput();             /// Sets this GPIO as an output
  void SetDirection(Direction direction);
      /// @param output - true => output, false => set pin to input  
  void SetHigh();                 /// Set voltage of pin to HIGH
  void SetLow();                  /// Set voltage of pin to LOW 
  void set(State state);          /// Set pin state to high or low 
                                  /// depending on the input state parameter.
                                  /// Has no effect if the pin is set as "input".
      /// @param state - State::kHigh => set pin high, State::kLow => set pin low
  State Read();                   /// Should return the state of the pin 
                                  /// (input or output, doesn't matter)
      /// @return level of pin high => true, low => false
  bool ReadBool();                /// Should return the state of the pin 
                                  /// (input or output, doesn't matter)
      /// @return level of pin high => true, low => false
  void resetResistor();
  void enablePullDownResistor();
 private:                       /// port, pin and any other variables should be placed here.
                                  /// NOTE: Pin state should NEVER be cached! Always check the hardware
                                  ///       registers for the actual value of the pin.
  uint8_t thePort;
  uint8_t thePin;
};