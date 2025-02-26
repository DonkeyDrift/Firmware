#include "ex_io.h"
#include "timer.h"
#include "freertos/FreeRTOS.h"

EXIO::EXIO(I2C* channel, int addr) {
  this->Channel = channel->CreateDevice(addr);
  this->Output = 0x00;
  this->NewOutput = 0x00;
}

void EXIO::Init() {
  if(this->Channel) {
    // Clean Output
    this->Channel->WriteWord(2, 0);

    // Set Port1 Output
    this->Channel->WriteByte(7, 0x0e);
  }
}

void EXIO::Update() 
{
	if(this->NewOutput != this->Output)
	{
		this->Write();
	}
}

void EXIO::Beep(bool on) {
    this->NewOutput |= BIT_BUZZER;
	this->Write();
    vTaskDelay(pdMS_TO_TICKS(300));
    this->NewOutput &= ~BIT_BUZZER;
	this->Write();
}

void EXIO::PowerOn()
{
	this->NewOutput |= BIT_POWER_ON;
	this->NewOutput &= BIT_POWER_OFF;
	this->Write();
}

void EXIO::PowerOff()
{
	this->NewOutput |= BIT_POWER_OFF;
	this->NewOutput &= BIT_POWER_ON;
	this->Write();
}

void EXIO::Write() 
{
	this->Channel->WriteWord(2, this->NewOutput);
	this->Output = this->NewOutput;
}

void EXIO::Read()
{

}