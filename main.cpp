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
#define SIZE 20//size of song name buffer

//FATFS variables
FATFS fat_fs;
FIL fil;
DIR dir;
FILINFO fno;
//control state variables
bool nextSong = false;


//song name buffer
char song_name[SIZE][13];
int song_count = 0;
int current_song = 0;
//freertos variables
// QueueHandle_t data_queue;
//decoder
LabGPIO Xdcs(2,0);  // VS1053 Data Select
LabGPIO Dreq(2,2);  // VS1053 Data Request Interrupt Pin
LabGPIO Mp3cs(2,5); // VS1053 Chip Select
LabGPIO Sdcs(2,7);  // SD Card Chip Select
LabGPIO Reset(2,9); //VS1053 hardware reset
decoder decode(&Xdcs,&Dreq,&Mp3cs,&Sdcs,&Reset);
// uint8_t song_buffer[512];


bool isMP3(char* file_name){
	int size = strlen(file_name);
	char* point = strrchr(file_name,'.');//points to the last occurance of .
	if(point != NULL){
		return (strcmp(point,".MP3") == 0);
	}
	printf("error\n");
	return false;
}

void decoderTask(void* p){
	UINT bytes_read;
	uint8_t data_buffer[2048];

	while(1){
	bytes_read = 2048;
	// f_open(&fil,"four4.mp3",FA_READ);
    f_open(&fil,song_name[current_song],FA_READ);
    
	while(bytes_read == 2048){
		f_read(&fil,data_buffer,2048,&bytes_read);
		//printf("send\n");
        decode.send_data(data_buffer,bytes_read);
	}
	f_close(&fil);
	vTaskSuspend(NULL);
	}
}

// void readFileTask(void *p){
// 	UINT bytes_read = 512;
// 	uint8_t data_buffer[512];
// 	bool first_song = false;
//     while(1){
//     	// if(!first_song){
//     	// 	//open first song
//     	// 	f_open(&fil,song_name[0],FA_READ);
//     	// } 
//     	//for testing
//   //   	printf("trying to open file\n");
// 		// FRESULT fr = f_open(&fil,"four4.mp3",FA_READ);
// 		// if(fr == FR_OK){
// 		// 	printf("open file success\n");
// 		// }
// 		// else{
// 		// 	printf("open file failed\n");
// 		// }
//     	// vTaskSuspendAll();
//     	while(1){
//     		vTaskSuspendAll();
//     		f_open(&fil,"four4.mp3",FA_READ);
//     		f_read(&fil,data_buffer,512,&bytes_read);
// 	    	printf("sending 512 bytes\n");
// 	        xQueueSend(data_queue,&data_buffer,portMAX_DELAY);
// 	         if(bytes_read != 512){
// 	        		break;
//       	  	}
//       	  	f_close(&fil);
//       	  	xTaskResumeAll();
//     	}
//     	vTaskSuspend(NULL);

//         // xTaskResumeAll();
//         // if(nextSong == true && song_count != 0){
//         // 	f_close(&fil);
//         // 	if(++current_song == song_count) current_song = 0;
//         // 	f_open(&fil,song_name[current_song],FA_READ);
//         // 	nextSong = false;
//         // }
//         // else if(song_count != 0){
//         // 	f_read(&fil,data_buffer,512,&bytes_read);
//         // 	if(bytes_read != 512){
//         // 		nextSong = true;
//         // 	}
//         // 	xQueueSend(data_queue,&data_buffer,portMAX_DELAY);

//         // 	// for(int i=0; i<bytes_read; i++){
//         // 	// 	xQueueSend(data_queue,&data_buffer[i],portMAX_DELAY);	
//         // 	// }
//         // }
//     }
// }

// void SendDataTask(void *p){
// 	uint8_t buffer[512];
// 	while(1){
// 		xQueueReceive(data_queue,&buffer,100);
// 		//printf("sending data to decoder\n");
// 		decode.send_data(buffer,512);
// 		vTaskDelay(10);
// 	}
// }


int main(){
	//mounting sd card
	printf("start mounting\n");
	f_mount(&fat_fs,"",0);


	// decoder setup
	decode.init();


	// int size = sizeof(HelloSampleMP3);
 //    while(1){
 //        decode.send_data(HelloSampleMP3,size);
 //        Delay(500);
 //    }
	

	
	//open root directory
	printf("start scanning\n");
	FRESULT res = f_opendir(&dir,"/");

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

	//remounting sd card
	printf("start mounting\n");
	f_mount(&fat_fs,"",0);


	// data_queue = xQueueCreate(10,sizeof(song_buffer));

	xTaskCreate(decoderTask,"decoder",1024,(void*)1,rtos::Priority::kMedium,nullptr);

	// xTaskCreate(readFileTask,"SD_Read",1024,(void*)1,rtos::Priority::kMedium,nullptr);
	// xTaskCreate(SendDataTask,"Decode_Send",1024,(void*)1,rtos::Priority::kMedium,nullptr);
	vTaskStartScheduler();

	while(1);

	//send a mp3 file to decoder for testing
	UINT bytes_read = 2048;
	uint8_t data_buffer[2048];
	// f_open(&fil,"four4.mp3",FA_READ);
    f_open(&fil,"one1.mp3",FA_READ);
	while(bytes_read == 2048){
		f_read(&fil,data_buffer,2048,&bytes_read);
		//printf("send\n");
        decode.send_data(data_buffer,bytes_read);
	}
	f_close(&fil);
	return 0;
}