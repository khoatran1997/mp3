#pragma once
#include "labgpio.hpp"


void LabGPIO::SetAsInput(){
	portPtr[portNum]->DIR &= ~(1<<pinNum);
}

void LabGPIO::SetAsOutput(){
	portPtr[portNum]->DIR |= (1<<pinNum);
}

void LabGPIO::SetDirection(Direction direction){
	if(direction==LabGPIO::Direction::kOutput) SetAsOutput();
	else SetAsInput();
}

void LabGPIO::SetHigh(){
	portPtr[portNum]->SET |= (1<<pinNum);
}

void LabGPIO::SetLow(){
	portPtr[portNum]->CLR |= (1<<pinNum);
}

void LabGPIO::set(State state){
	if(state==LabGPIO::State::kHigh) SetHigh();
	else SetLow();
}

LabGPIO::State LabGPIO::Read(){
	if(portPtr[portNum]->PIN & (1<<pinNum)) return LabGPIO::State::kHigh;
	else return LabGPIO::State::kLow;
}

bool LabGPIO::ReadBool(){
	return portPtr[portNum]->PIN & (1<<pinNum);
}

void LabGPIO::EnableInterrupts(){
	RegisterIsr(GPIO_IRQn, GpioInterruptHandler);
}

void LabGPIO::AttachInterruptHandler(IsrPointer isr, Edge edge){
uint8_t tempport = portNum;
if(tempport == 2) tempport--;
pin_isr_map[tempport][pinNum] = isr;
if(portNum == 2){
if(edge == LabGPIO::Edge::kRising ||edge == LabGPIO::Edge::kBoth) LPC_GPIOINT->IO2IntEnR |= (1<<pinNum);
else if(edge == LabGPIO::Edge::kFalling || edge == LabGPIO::Edge::kBoth) LPC_GPIOINT->IO2IntEnF |= (1<<pinNum);
}
else if(portNum == 0){
if(edge == LabGPIO::Edge::kRising ||edge == LabGPIO::Edge::kBoth) LPC_GPIOINT->IO0IntEnR |= (1<<pinNum);
else if(edge == LabGPIO::Edge::kFalling || edge == LabGPIO::Edge::kBoth) LPC_GPIOINT->IO0IntEnF |= (1<<pinNum);
}
}


void LabGPIO::GpioInterruptHandler(){
uint8_t port,pin;
for(uint8_t i=0;i<32;i++){
if((LPC_GPIOINT->IO0IntStatR & (1<<i)) || (LPC_GPIOINT->IO0IntStatF & (1<<i)) ){
port = 0;
pin = i;
LPC_GPIOINT->IO0IntClr |= (1<<pin);
pin_isr_map[port][pin]();
}
else if((LPC_GPIOINT->IO2IntStatR & (1<<i))  || (LPC_GPIOINT->IO2IntStatF & (1<<i))  ){
port = 2;
pin = i;
LPC_GPIOINT->IO2IntClr |= (1<<pin);
pin_isr_map[port][pin]();
}
}
}








