
class decoder{
public:
	decoder(){

	}

	void init(){
		spi.Initialize();
		dreg.SetAsInput();
		xdcs.SetAsOutput();
		sdcs.SetAsOutput();
		mp3cs.SetAsOutput();
	}

	void send(uint8_t data){

	}



private:
	LabSpi spi;
	LabGPIO dreg(2,0);
	LabGPIO xdcs(2,2);
	LabGPIO mp3cs(2,5);
	LabGPIO sdcs(2,7);
};