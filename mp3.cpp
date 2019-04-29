
class decoder{
public:
	decoder(){

	}

	void init(){
		spi.Initialize();

		dreq.SetAsInput();
		xdcs.SetAsOutput();
		sdcs.SetAsOutput();
		mp3cs.SetAsOutput();

		mp3cs.SetLow();//chip select enable in init
		sdcs.SetHigh();
		xdcs.SetHigh();
	}

	void data_enable(){
		xdcs.SetLow();
	}

	void data_disable(){
		xdcs.SetHigh();
	}

	void send(uint8_t data){
		spi.transfer(data);
	}

	bool check(){
		return dreq.ReadBool();
	}



private:
	LabSpi spi;
	LabGPIO xdcs(2,0);  // VS1053 Data Select
	LabGPIO dreq(2,2);  // VS1053 Data Request Interrupt Pin
	LabGPIO mp3cs(2,5); // VS1053 Chip Select
	LabGPIO sdcs(2,7);  // SD Card Chip Select
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
	char song_name[20][20];
	int song_count=0;

	//mounting and open root directory
	f_mount(&fat_fs,"",1);
	FRESULT res = f_opendir(&dir,"/");
	
	//scan sd card and store song names in buffer
	if(res == FR_OK){
		for(;;){
			res = f_readdir(&dir,&fno);
			if(res != FR_OK || fno.fname[0] == 0) break;//error or end of loop
			else if(fno.fattrib & AM_DIR){
				//a folder, ignore
			}
			else{
				printf("%s\n", fno.fname);//print file name for testing
				if(isMP3(fno.fname)){
					songname[song_count] = fno.fname;
					if(++song_count >= 20){
						song_count = 19;
						printf("can only handle 20 songs\n");
					}
				}
			}
		}
	}

	//send a mp3 file to decoder for testing
	UINT file_size;
	uint8_t data_buffer[10000000];//could be change if not enough to fit the file
	f_open(&fil,"file_name.mp3",FA_READ,10000000,&file_size);

	decoder decode;
	decode.init();
	int i=0;

	decode.data_enable();

	while(i<file_size){//check for eof
		if(decode.check()){
			for(int j=0;j<32;j++){
				if(i<file_size){//check again for eof
					decode.transfer(data_buffer[i++]);
				}
			}
		}
	}


	return 0;
}