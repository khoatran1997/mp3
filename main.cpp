#include "decoder.hpp"
#include "utility/time.hpp"
#include <stdint.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "event_groups.h"
#include "L0_LowLevel/LPC40xx.h"
#include "L0_LowLevel/interrupt.hpp"
#include "L2_HAL/sensors/temperature/temperature.hpp"
#include "utility/rtos.hpp"
#include "third_party/fatfs/source/ff.h"
#include "L3_Application/commandline.hpp"
#include "utility/log.hpp"
#include <cstring>
#include "semphr.h"
#include "L3_Application/commands/rtos_command.hpp"
#include "L3_Application/commandline.hpp"
#include "L3_Application/commands/common.hpp"
#define SIZE 20//size of song name buffer

//FATFS variables
FATFS fat_fs;
FIL fil;
DIR dir;
FILINFO fno;

//control state variables
bool next = false;
bool pause = false;

//song name buffer
char song_name[SIZE][13];
int song_count = 0;
int current_song = 0;

//freertos variables
QueueHandle_t data_queue;
SemaphoreHandle_t decoder_lock;

//decoder
LabGPIO Xdcs(2,0);  // VS1053 Data Select
LabGPIO Dreq(2,2);  // VS1053 Data Request Interrupt Pin
LabGPIO Mp3cs(2,5); // VS1053 Chip Select
LabGPIO Sdcs(2,7);  // SD Card Chip Select
LabGPIO Reset(2,9); //VS1053 hardware reset
decoder decode(&Xdcs,&Dreq,&Mp3cs,&Sdcs,&Reset);
uint16_t VOL = 0x4040;
uint16_t BASS = 0x00;

namespace
{
//command line
CommandList_t<32> command_list;
RtosCommand rtos_command;
CommandLine<command_list> ci;
//LpcSystemInfoCommand system_command;
void TerminalTask(void * ptr)
{
  LOG_INFO("Press Enter to Start Command Line!");
  ci.WaitForInput();
  LOG_WARNING("\nUser has quit from terminal!");
  vTaskDelete(nullptr);
}

}

bool isMP3(char* file_name){
	int size = strlen(file_name);
	char* point = strrchr(file_name,'.');//points to the last occurance of .
	if(point != NULL){
		return (strcmp(point,".MP3") == 0);
	}
	printf("error\n");
	return false;
}

void bass_up(){
	if((BASS & 0xff) <= 0xE0){
		BASS += 0x10;
		xSemaphoreTakeFromISR(decoder_lock,nullptr);
		decode.write_reg(VS1053_REG_BASS,BASS);
		xSemaphoreGiveFromISR(decoder_lock,nullptr);
	}
}

void bass_down(){
	if((BASS & 0xFF) >= 0x10){
		BASS -= 0x10;
		xSemaphoreTakeFromISR(decoder_lock,nullptr);
		decode.write_reg(VS1053_REG_BASS,BASS);
		xSemaphoreGiveFromISR(decoder_lock,nullptr);
	}
}

void trebble_up(){
	if(((BASS>>8) & 0xFF) <= 0xE0){
		BASS += 0x1000;
		xSemaphoreTakeFromISR(decoder_lock,nullptr);
		decode.write_reg(VS1053_REG_BASS,BASS);
		xSemaphoreGiveFromISR(decoder_lock,nullptr);
	}
}

void trebble_down(){
	if(((BASS>>8) & 0xFF) >= 0x10){
		BASS -= 0x1000;
		xSemaphoreTakeFromISR(decoder_lock,nullptr);
		decode.write_reg(VS1053_REG_BASS,BASS);
		xSemaphoreGiveFromISR(decoder_lock,nullptr);
	}
}

void vol_up(){
	if(VOL >= 0x1010){
		VOL -= 0x1010;
		xSemaphoreTakeFromISR(decoder_lock,nullptr);
		decode.write_reg(VS1053_REG_VOLUME,VOL);
		xSemaphoreGiveFromISR(decoder_lock,nullptr);
	}
}

void vol_down(){
	if(VOL <= 0xEEEE){
		VOL += 0x1010;
		xSemaphoreTakeFromISR(decoder_lock,nullptr);
		decode.write_reg(VS1053_REG_VOLUME,VOL);
		xSemaphoreGiveFromISR(decoder_lock,nullptr);
	}
}

// void readFileTask(void *p){
// 	UINT bytes_read = 1024;
// 	uint8_t data_buffer[1024]={};
//     while(1){
//     	f_open(&fil,song_name[current_song],FA_READ);
//     	while(!next){
  
//     		//while(pause);
//     		f_read(&fil,data_buffer,1024,&bytes_read);
// 	        xQueueSend(data_queue,&data_buffer,portMAX_DELAY);
// 	        if(bytes_read != 1024){
// 	        		break;
//       	  	} 
//       	  	//vTaskDelay(10);	
//     	}
//     	f_close(&fil);
//     	while(1);
//     	current_song++;
//     }
// }

// void SendDataTask(void *p){
// 	uint8_t buffer[1024];
// 	while(1){
// 		if(xQueueReceive(data_queue,&buffer,100) == pdTRUE){
// 			printf("sending data to decoder\n");
// 			//xSemaphoreTakeFromISR(decoder_lock,nullptr);
// 			decode.send_data(buffer,1024);
// 			//xSemaphoreGiveFromISR(decoder_lock,nullptr);
// 		}
// 		vTaskDelay(1);
// 	}
// }

void decodeTask(void *p){
	uint8_t data_buffer[2048];
	UINT bytes_read;
	while(1){
		f_open(&fil,song_name[current_song],FA_READ);
		bytes_read = 2048;
		while(bytes_read == 2048){
			f_read(&fil,data_buffer,2048,&bytes_read);
			decode.send_data(data_buffer,bytes_read);
			vTaskDelay(25);
		}
		f_close(&fil);
		vTaskDelay(100);
	}

}


int main(){
		// decoder setup
	// decode.init();

	// int size = sizeof(HelloSampleMP3);
 //    while(1){
 //        decode.send_data(HelloSampleMP3,size);
 //        Delay(500);
 //    }

	//mounting sd card
	printf("start mounting\n");
	f_mount(&fat_fs,"",0);

	//open root directory
	printf("start scanning\n");
	FRESULT res = f_opendir(&dir,"/");

	//scan for mp3 file
	if(res == FR_OK){
		for(;;){
			res = f_readdir(&dir,&fno);
			if(res != FR_OK || fno.fname[0] == 0){
				printf("Found %d songs\n", song_count);
				break;//error or end of loop
			}
			else if(fno.fattrib & AM_DIR){
				//a folder, ignore
			}
			else{
				//save song name if ends with "mp3"
				if(isMP3(fno.fname)){
					printf("%s\n",fno.fname);
					strcpy(song_name[song_count++],fno.fname);
				}
				else printf("not a song\n");
				//check for max of buffer
				if(song_count>=SIZE){
					printf("Maximum met!\n");
					break;
				}
			}
		}	
	}
	f_closedir(&dir);
	//exit if no song found
	if(song_count == 0) exit(1);

	//remounting sd card
	printf("start mounting\n");
	f_mount(&fat_fs,"",0);

	// decoder setup
	decode.init();
	// printf("init done\n");
		// UINT bytes_read;

	 //    	f_open(&fil,"one1.mp3",FA_READ);
	 //    	uint8_t data_buffer[1024];
  //   	while(!next){
  //   		// while(pause);
  //   		f_read(&fil,data_buffer,1024,&bytes_read);
  //   		decode.send_data(data_buffer,bytes_read);
	 //    	//printf("sending 1024 bytes\n");
	 //        //xQueueSend(data_queue,&data_buffer,portMAX_DELAY);
	 //        if(bytes_read != 1024){
	 //        		break;
  //     	  	} 
  //     	  	//vTaskDelay(10);	
  //   	}
  //   	f_close(&fil);



	//add rtos command
  	LOG_INFO("Adding rtos command to command line...");
	ci.AddCommand(&rtos_command);
	ci.Initialize();

	// decoder_lock = xSemaphoreCreateMutex();
	// data_queue = xQueueCreate(4,1024);

	xTaskCreate(TerminalTask, "Terminal", 501, nullptr, rtos::Priority::kLow, nullptr);
	// xTaskCreate(readFileTask,"SD_Read",1024,(void*)1,rtos::Priority::kMedium,nullptr);
	// xTaskCreate(SendDataTask,"Decode_Send",1024,(void*)1,rtos::Priority::kMedium,nullptr);
	xTaskCreate(decodeTask,"decoder",2048,nullptr,rtos::Priority::kMedium,nullptr);
	vTaskStartScheduler();
	
	while(1);	
	return 0;
}