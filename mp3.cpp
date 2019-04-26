
class decoder{
public:
	decoder(){

	}

	void init(){
		spi.Initialize();
		xdcs.SetAsOutput();
		dreq.SetAsInput();
		sdcs.SetAsOutput();
		mp3cs.SetAsOutput();
	}

	void send(uint8_t data){

	}



private:
	LabSpi spi;
	LabGPIO xdcs(2,0);  // VS1053 Data Select
	LabGPIO dreq(2,2);  // VS1053 Data Request Interrupt Pin
	LabGPIO mp3cs(2,5); // VS1053 Chip Select
	LabGPIO sdcs(2,7);  // SD Card Chip Select
};

int main(){
	printf("Hello World");
	return 0;
}