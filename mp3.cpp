#include <cstdint>
// #include <project_config.hpp>
// #include <iterator>
// #include "utility/log.hpp"
// #include "utility/time.hpp"
// #include "L0_LowLevel/LPC40xx.h"
// #include "L1_Drivers/pin.hpp"
// #include "utility/enum.hpp"
// #include "L0_LowLevel/interrupt.hpp"
#include <stdint.h>
#include <stdio.h>
// // #include "task.h"
// // #include "queue.h"
// #include "L2_HAL/switches/button.hpp"
// #include "L3_Application/oled_terminal.hpp"
// #include "vector"
#include "third_party/fatfs/source/ff.h"
#include "labspi.hpp"
#include "labgpio.hpp"

LabGPIO xdcs(2,0);  // VS1053 Data Select
LabGPIO dreq(2,2);  // VS1053 Data Request Interrupt Pin
LabGPIO mp3cs(2,5); // VS1053 Chip Select
LabGPIO sdcs(2,7);  // SD Card Chip Select

class decoder{
public:
	decoder(){

	}

	void init(){
		spi.Initialize(8, LabSpi::FrameModes::spi, 0);

		dreq.SetAsInput();
		xdcs.SetAsOutput();
		sdcs.SetAsOutput();
		mp3cs.SetAsOutput();

		mp3cs.SetLow();//chip select enable in init
		sdcs.SetHigh();
		xdcs.SetLow();
	}

	void data_enable(){
		xdcs.SetHigh();
	}

	void data_disable(){
		xdcs.SetLow();
	}

	void send(uint8_t data){
		spi.Transfer(data);
	}

	bool check(){
		return dreq.ReadBool();
	}



private:
	LabSpi spi;
	// LabGPIO xdcs(2,0);  // VS1053 Data Select
	// LabGPIO dreq(2,2);  // VS1053 Data Request Interrupt Pin
	// LabGPIO mp3cs(2,5); // VS1053 Chip Select
	// LabGPIO sdcs(2,7);  // SD Card Chip Select
};

bool isMP3(char* file_name){
	int size = strlen(file_name);
	char* point = strrchr(file_name,'.');//points to the last occurance of .
	if(point != NULL){
		return (strcmp(point,".mp3") == 0);
	}
	printf("error\n");//parameter not a file name
	return false;
}

FATFS fat_fs;
FIL fil;
DIR dir;
FILINFO fno;




int main(){
	char song_name[20][13];
	int song_count=0;

	//mounting and open root directory
	f_mount(&fat_fs,"",1);
	// FRESULT res = f_opendir(&dir,"/");
	
	//scan sd card and store song names in buffer
	// if(res == FR_OK){
	// 	for(;;){
	// 		res = f_readdir(&dir,&fno);
	// 		if(res != FR_OK || fno.fname[0] == 0) break;//error or end of loop
	// 		else if(fno.fattrib & AM_DIR){
	// 			//a folder, ignore
	// 		}
	// 		else{
	// 			printf("%s\n", fno.fname);//print file name for testing
	// 			if(isMP3(fno.fname)){
	// 				printf("%s\n",fno.fname);
	// 				// song_name[song_count] = static_cast<char>(fno.fname);
	// 				// if(++song_count >= 20){
	// 				// 	song_count = 19;
	// 				// 	printf("can only handle 20 songs\n");
	// 				// }
	// 			}
	// 		}
	// 	}
	// }

	//send a mp3 file to decoder for testing
	// UINT file_size;
	// uint8_t data_buffer[10000];//could be change if not enough to fit the file
	// f_open(&fil,"four4.mp3",FA_READ);
	// f_read(&fil,data_buffer,10000,&file_size);
	// printf("files ize: ");
	// printf("%i",file_size);

	//decoder decode;
	LabSpi decode;
	decode.Initialize((uint8_t)8,LabSpi::FrameModes::spi,(uint8_t)0);
	//decode.init();
	int i=0;

	decode.Transfer(0x88);
	decode.Transfer(0xA8);
	decode.Transfer(0xB8);
	while(1){}
	// decode.data_enable();

	// while(i<file_size){//check for eof
	// 	if(decode.check()){
	// 		decode.data_enable();
	// 		for(int j=0;j<32;j++){
	// 			if(i<file_size){//check again for eof
	// 				// printf("%x\n",data_buffer[i]);
	// 				decode.send(data_buffer[i++]);
	// 			}
	// 		}
	// 		decode.data_disable();
	// 	}
	// }


	return 0;
}