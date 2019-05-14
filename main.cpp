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


// Initialize button and led here
Lab_GPIO SW3(0, 29); // toggle
Lab_GPIO SW2(0, 30); // select
Lab_GPIO SW1(1, 15);  //Needs pull down resistor (-)
Lab_GPIO SW0(1, 19);  //Needs pull down resistor (+)


OledTerminal oledterm;


//FATFS variables
FATFS fat_fs;
FIL fil;
DIR dir;
FILINFO fno;

//control state variables
bool next = false;
bool pause = false;
bool prev = false;
bool sw3pressed=false;
bool sw2pressed=false;
int vol = 12; 
uint16_t bass = 8;
int treble = 16;
int toggle = 0;
uint16_t VOL = 0x4040;
uint16_t BASS = 0x7af6;
int seed = 0;
char setting[5][10]={"volume","bass","treble","next/prev","shuffle"};

//song name buffer
char song_name[SIZE][100];
int song_count = 0;
int current_song = 0;

//freertos variables
// QueueHandle_t data_queue;
SemaphoreHandle_t decoder_lock;
SemaphoreHandle_t display_lock;

bool shuffle=false;

//decoder
Lab_GPIO Xdcs(2,0);  // VS1053 Data Select
Lab_GPIO Dreq(2,2);  // VS1053 Data Request Interrupt Pin
Lab_GPIO Mp3cs(2,5); // VS1053 Chip Select
Lab_GPIO Sdcs(2,7);  // SD Card Chip Select
Lab_GPIO Reset(2,9); //VS1053 hardware reset
decoder decode(&Xdcs,&Dreq,&Mp3cs,&Sdcs,&Reset);

namespace {
  //command line
  CommandList_t<32> command_list;
  RtosCommand rtos_command;
  CommandLine<command_list> ci;
  //LpcSystemInfoCommand system_command;
  void TerminalTask(void * ptr){
    LOG_INFO("Press Enter to Start Command Line!");
    ci.WaitForInput();
    LOG_WARNING("\nUser has quit from terminal!");
    vTaskDelete(nullptr);
  }
}

bool isMP3(char* file_name){
  if(file_name[0] == '.'){ //check for hidden files (i.e ._FILENAME)
    return false;
  }
  int size = strlen(file_name);
  char* point = strrchr(file_name,'.');//points to the last occurance of .
  if(point != NULL){
    return ((strcmp(point,".mp3") == 0) || (strcmp(point,".MP3") == 0));
  }
  printf("error\n");
  return false;
}

void bass_up(){
  if(bass<16){
    BASS += 0x10;
    bass++;
    while(!xSemaphoreTakeFromISR(decoder_lock,nullptr));
    decode.write_reg(VS1053_REG_BASS,BASS);
    xSemaphoreGiveFromISR(decoder_lock,nullptr);
  }
}

void bass_down(){
  if(bass>0){
    BASS -= 0x10;
    bass--;
    while(!xSemaphoreTakeFromISR(decoder_lock,nullptr));
    decode.write_reg(VS1053_REG_BASS,BASS);
    xSemaphoreGiveFromISR(decoder_lock,nullptr);
  }
}

void trebble_up(){
  if(treble<16){
    BASS += 0x1000;
    treble++;
    while(!xSemaphoreTakeFromISR(decoder_lock,nullptr));
    decode.write_reg(VS1053_REG_BASS,BASS);
    xSemaphoreGiveFromISR(decoder_lock,nullptr);
  }
}

void trebble_down(){
  if(treble>0){
    BASS -= 0x1000;
    treble--;
    while(!xSemaphoreTakeFromISR(decoder_lock,nullptr));
    decode.write_reg(VS1053_REG_BASS,BASS);
    xSemaphoreGiveFromISR(decoder_lock,nullptr);
  }
}

void vol_up(){
  if(vol<16){
    printf("vol up isr called\n");
    VOL -= 0x1010;
    vol++;
    while(!xSemaphoreTakeFromISR(decoder_lock,nullptr));
    decode.write_reg(VS1053_REG_VOLUME,VOL);
    xSemaphoreGiveFromISR(decoder_lock,nullptr);
  }
}

void vol_down(){
  if(vol>0){
    VOL += 0x1010;
    vol--;
    while(!xSemaphoreTakeFromISR(decoder_lock,nullptr));
    decode.write_reg(VS1053_REG_VOLUME,VOL);
    xSemaphoreGiveFromISR(decoder_lock,nullptr);
  }
}

void toggleFunction() //use SW0
{
  if(SW3.ReadBool() && !sw3pressed){
    sw3pressed = true;
  }
  else if(!SW3.ReadBool() && sw3pressed){
    sw3pressed = false;
    toggle +=1;
    if(toggle > 4){
      toggle = 0;
    }
    xSemaphoreGiveFromISR(display_lock,nullptr);
  }
  //vTaskDelay(10);
    //vTaskDelay(500);
}

void interruptSwitch(){ // go control
  if(SW2.ReadBool() && !sw2pressed){
    sw2pressed = true;
  }
  else if(!SW2.ReadBool() && sw2pressed){
    sw2pressed = false;
    taskENTER_CRITICAL();
    switch(toggle){
      case 0: //volume
        if(SW1.ReadBool())//volume down
        {
          vol_down();
          xSemaphoreGiveFromISR(display_lock,nullptr);
          // voldown = true;
          // display = true;
        }
        else if(SW0.ReadBool())//volume up
        {
          vol_up();
          xSemaphoreGiveFromISR(display_lock,nullptr);
          // volup = true;
          // display = true;
        }
        else{
            pause = !pause;
        }
        break;

      case 1: //bass
        if(SW1.ReadBool()){//bass down
          bass_down();
          xSemaphoreGiveFromISR(display_lock,nullptr);
          // bdown = true;
          // display = true;
        }
        else if(SW0.ReadBool()){//bass up
          bass_up();
          xSemaphoreGiveFromISR(display_lock,nullptr);
          // bup = true;
          // display = true;
        }
        else{
          pause = !pause;
        }

        break; 

      case 2: //treble
        if(SW1.ReadBool()){
          trebble_down();
          xSemaphoreGiveFromISR(display_lock,nullptr);
          // tredown = true;
          // display = true;
        }
        else if(SW0.ReadBool()){
          trebble_up();
          xSemaphoreGiveFromISR(display_lock,nullptr);
          // treup = true;
          // display = true;
          
        }
        else {
          pause = !pause;
        }
        break; 

      case 3: //next/prev
        if(SW1.ReadBool())
        {
          prev = true;
        }
        if(SW0.ReadBool())
        {
          next = true;
        }
        break; 

      case 4: //shuffle
        shuffle = true;

        break;
      default: printf("whatever\n");
        break;
    }
    taskEXIT_CRITICAL();
  }
  //vTaskDelay(10);
}

void displayTask(void *p){
    while(1){
    while(!xSemaphoreTake(display_lock,100));
    oledterm.Clear();
    oledterm.printf("%s\nvol:%i, bass:%i, treble:%i\n%s", song_name[current_song],vol,bass,treble,setting[toggle]);
    }
}

void decodeTask(void *p){
  uint8_t data_buffer[2048];
  UINT bytes_read;
  while(1){
    pause = false;
    next = false;
    prev = false;
    // current_song=10;
    f_open(&fil,song_name[current_song],FA_READ);
    // f_open(&fil,song_name[10],FA_READ);
    bytes_read = 2048;
    while(bytes_read == 2048 && !next && !prev && !shuffle){
    // while(bytes_read == 2048){
      f_read(&fil,data_buffer,2048,&bytes_read);
      seed++;
      // while(!xSemaphoreTake(decoder_lock,100));
      taskENTER_CRITICAL();
      decode.send_data(data_buffer,bytes_read);
      taskEXIT_CRITICAL();
      xSemaphoreGive(decoder_lock);
      while(pause){
        vTaskDelay(10);
      }
      vTaskDelay(30);
    }
    f_close(&fil);
    if(prev){
      if(current_song==0){
        current_song=song_count-1;
      }
      else{
        current_song--;
      }
    }
    else if(shuffle){
      shuffle = false;
      srand(seed);
      current_song = (rand() % song_count);
    }
    else{
      if(current_song+1<song_count){
        current_song++;
      }
      else{
        current_song = 0;
      }
    }
    xSemaphoreGiveFromISR(display_lock,nullptr);
    vTaskDelay(200);
  }
}

int main(){
  oledterm.Initialize();
  oledterm.printf("OLED Initialized...\n");
  SW1.resetResistor();
  SW1.enablePullDownResistor();

  SW0.resetResistor();
  SW0.enablePullDownResistor();

  SW2.AttachInterruptHandler(&interruptSwitch, Lab_GPIO::Edge::kBoth);
  SW2.EnableInterrupts();

  SW3.AttachInterruptHandler(&toggleFunction, Lab_GPIO::Edge::kBoth);
  SW3.EnableInterrupts();


    // decoder setup
  // decode.init();

  // int size = sizeof(HelloSampleMP3);
 //    while(1){
 //        decode.send_data(HelloSampleMP3,size);
 //        Delay(500);
 //    }

  //mounting sd card
  printf("start mounting\n");
  f_mount(&fat_fs,"",1);

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
  // printf("TEST: \n");
  int songnum = 1;
  for(int i=0;i<song_count;i++){
    printf("%i.%s \n",songnum,song_name[i]);
    songnum++;
  }
  f_closedir(&dir);
  //exit if no song found
  if(song_count == 0) exit(1);

  //remounting sd card
  printf("start mounting\n");
  f_mount(&fat_fs,"",1);

  // decoder setup
  decode.init();

  // UINT bytes_read;
  // uint8_t data_buffer[2048];
  // f_open(&fil,song_name[10],FA_READ);
  //   bytes_read = 2048;
  //   // while(bytes_read == 2048 && !next && !prev){
  //   while(1){
  //     f_read(&fil,data_buffer,2048,&bytes_read);
  //     decode.send_data(data_buffer,bytes_read);

  //     //vTaskDelay(10);
  //   }


  //add rtos command
  LOG_INFO("Adding rtos command to command line...");
  ci.AddCommand(&rtos_command);
  ci.Initialize();

  decoder_lock = xSemaphoreCreateMutex();
  display_lock = xSemaphoreCreateMutex();

  

  xTaskCreate(TerminalTask, "Terminal", 501, nullptr, rtos::Priority::kLow, nullptr);


  xTaskCreate(displayTask,"Display",501,nullptr,rtos::Priority::kMedium,nullptr);
  xTaskCreate(decodeTask,"decoder",4096,nullptr,rtos::Priority::kHigh,nullptr);
  vTaskStartScheduler();
  
  while(1); 
  return 0;
}