#include "DmfNRF.h"

//#if defined _SOFTSPI_H_
	//SoftSPI SPI = SoftSPI();
//#endif


void NRF::init(uint8_t ce_pin, uint8_t csn_pin){
	cePin = ce_pin;
	csnPin = csn_pin;

	pinMode(cePin,OUTPUT);
    pinMode(csnPin,OUTPUT);

    ce(0);
    csn(1);


	// Initialize spi module
	/*#ifdef __SOFTSPI_H__
		SPI.init(VirtualSpi_MOSI_Pin, VirtualSpi_MISO_Pin, VirtualSpi_SCK_Pin); // VirtualSpi : 11 = MOSI, 12 = MISO, 13 = SCK
	#endif*/
	SPI.begin();
	SPI.setDataMode(SPI_MODE0);
	SPI.setClockDivider(SPI_CLOCK_DIV4);//SPI_2XCLOCK_MASK
}
/*
DelayReTransmin = 0-15
CountReTransmin = 1-15
*/
void NRF::config(uint8_t channel, CrcLength MyCrc, AirDataRate DataRate, uint8_t DelayReTransmin, uint8_t CountReTransmin){

	SetupReter(DelayReTransmin,CountReTransmin); //delay=3, count=15

	setChannel(channel);

	setDataRate(DataRate);

	Crc = MyCrc;
	SetCRC(Crc);

	
	uint8_t status;
	readRegister(STATUS,&status,1);
	configRegister(STATUS,status|(1 << RX_DR)|(1 << TX_DS)|(1 << MAX_RT));
	SetpowerUpRx(); // Start receiver
    flushRx();
}


/*
اگه طول رو 0 وارد کنه یعنی این واحد غیر فعال بشه و اگه
1 وارد کنه یعنی حالت هشت بیتی رو انتخاب کرده
و اگه 2 رو وارد کنه یعنی حالت شونزده بیتی رو انتخاب کرده

تعیین CRC
برای تمام لوله ها مشترک هستش
*/
void NRF::SetCRC(CrcLength Length){
	uint8_t status;
	readRegister(CONFIG,&status,1);

	if(Length==CrcDisable)
		configRegister(CONFIG, status & (0xff - (1<<EN_CRC)) & (0xff - (1<<CRCO)) );
	else if(Length==Crc8Bit)
		configRegister(CONFIG, (status | (1<<EN_CRC)) & (0xff - (1<<CRCO)) );
	else if(Length==Crc16Bit)
		configRegister(CONFIG, status | (1<<EN_CRC) | (1<<CRCO) );
}
/*
ارسال 0 یعنی این واحد غیر فعال هستش 
و ارسال 8 یعنی این واحد 8 بیتی هستش
و 16 یعنی این واحد 16 بیتی هستش
*/
uint8_t NRF::getCRCLength(void){
	uint8_t status;
	readRegister(CONFIG,&status,1);
	
	if (status & (1<<EN_CRC)){
		if (status & (1<<CRCO))
			return 16;
		else
			return 8;
	}
	else
		return 0;
}


/*
تعیین AirDataRate
برای تمام لوله ها مشترک هستش
*/
void NRF::setDataRate(AirDataRate DataRate){
	uint8_t status;
	readRegister(RF_SETUP,&status,1);
	
	if(DataRate==2)
		configRegister(RF_SETUP, status | (1<<RF_DR) );
	else if(DataRate==1)
		configRegister(RF_SETUP, status & (0xff-(1<<RF_DR)) );
}
uint8_t NRF::getDataRate( void ){
	uint8_t status;
	readRegister(RF_SETUP,&status,1);

	status &= (1<<RF_DR);
	
	if(status) 		// 2Mbps
		return 2;
	else			// 1Mbps
		return 1;
}


/*
تعیین فرکانس کانال
برای تمام لوله ها مشترک هستش
*/
bool NRF::setChannel(uint8_t channel){
	if(channel>=0 && channel<=127){
		configRegister(RF_CH,channel);
		return true;
	}
	else
		return false;
}
uint8_t NRF::getChannel(){
	uint8_t status;
    readRegister(RF_CH,&status,1);
	return status;
}


void NRF::ce(bool level){
	digitalWrite(cePin,level);
}
void NRF::csn(bool level){
	digitalWrite(csnPin,level);
}


void NRF::flushRx(){
    csn(0);
	SPI.transfer(FLUSH_RX);
    csn(1);
}
void NRF::flushTx(){
    csn(0);
	SPI.transfer(FLUSH_TX);
    csn(1);
}


void NRF::configRegister(uint8_t reg, uint8_t value){
    csn(0);
	SPI.transfer(W_REGISTER | (0x3F & reg));
	SPI.transfer(value);	
    csn(1);
}
void NRF::readRegister(uint8_t reg, uint8_t * value, uint8_t len){
    csn(0);
	SPI.transfer(R_REGISTER | (0x1F & reg));
	for(uint8_t i = 0;i < len;i++)
		value[i] = SPI.transfer(NULL);
    csn(1);
}
void NRF::writeRegister(uint8_t reg, char * value, uint8_t len){
	csn(0);
	SPI.transfer(W_REGISTER | (0x3F & reg));
	for(uint8_t i = 0;i < len;i++)
		SPI.transfer(value[i]);
    csn(1);
}


/*
تعیین آدرس گیرنده و فرستنده

توجه : فعلا ترجیحا از لوله 0 برا دریافت داده استفاده
نکنید و بزارید برا همین دریافت تصدیق نامه ها باشه 
تا بعد درستش کنم این تابع رو

AddressWidth = 3-5 byte
PipeNumber = 0-5

#define RX_ADDR_P0  0x0A
#define RX_ADDR_P1  0x0B
#define RX_ADDR_P2  0x0C
#define RX_ADDR_P3  0x0D
#define RX_ADDR_P4  0x0E
#define RX_ADDR_P5  0x0F
*/
void NRF::setReceiverAddress(uint8_t PipeNumber, char * AddressReceiver, uint8_t AddressWidth){
	setAddressWidth(AddressWidth); // تعیین تعداد بایت های آدرس
	ce(0);
		writeRegister(PipeNumber+10, AddressReceiver, AddressWidth);
	ce(1);	
}
void NRF::setSenderAddress(char * AddressSender, uint8_t AddressWidth){
	/*
	آدرس ارسال کننده اطلاعات و آدرس لوله 0 که قرار بسته تصدیق نامه رو دیافت کنه
	RX_ADDR_P0 must be set to the sending addr for auto ack to work.
	*/
	setAddressWidth(AddressWidth); // تعیین تعداد بایت های آدرس
	writeRegister(TX_ADDR, AddressSender, AddressWidth);
}


/*
width = 3-5 Byte

از ریجسترهاش این طور برداشت کردم که
این تعیین آدرس برای تمامی لوله ها مشترک هستش
*/
bool NRF::setAddressWidth(uint8_t width){
	if(width>=3 && width<=5)	width -= 2;
	else						return 0; // عملیات انجام نشد

	uint8_t status;
	readRegister(SETUP_AW,&status,1);
	configRegister(SETUP_AW, status | (width<<AW));
	
	return 1; // عملیات با موفقیت انجام شد
}
uint8_t NRF::getAddressWidth(){
	uint8_t status;
	readRegister(SETUP_AW,&status,1);
	return status;
}


void NRF::EnableDataPipe(uint8_t PipeNumber, bool EnablePipe){
	uint8_t status;
	readRegister(EN_RXADDR,&status,1);

	if(EnablePipe)
		status |= (1<<PipeNumber);
	else
		status &= (0xff - (1<<PipeNumber));

	configRegister(EN_RXADDR, status);
}


/*
Set length of incoming payload
payloadSize : 1-32 byte
PipeNumber : 0-5


#define RX_PW_P0    0x11
#define RX_PW_P1    0x12
#define RX_PW_P2    0x13
#define RX_PW_P3    0x14
#define RX_PW_P4    0x15
#define RX_PW_P5    0x16
*/
void NRF::setStaticPayloadSize(uint8_t PipeNumber, uint8_t payloadSize){
	// روش ساده تر و کم حجم تر ولی سخت فهم تر
	configRegister(0x11 + PipeNumber, payloadSize);

	// روش سخت تر و پر حجم تر ولی خوانا تر
	/*switch(PipeNumber)
	{
		case 0 : configRegister(RX_PW_P0, payloadSize); break;
		case 1 : configRegister(RX_PW_P1, payloadSize); break;
		case 2 : configRegister(RX_PW_P2, payloadSize); break;
		case 3 : configRegister(RX_PW_P3, payloadSize); break;
		case 4 : configRegister(RX_PW_P4, payloadSize); break;
		case 5 : configRegister(RX_PW_P5, payloadSize); break;
	}*/
}
uint8_t NRF::getStaticPayloadSize(uint8_t PipeNumber){
	/*
	روش ساده تر و کم حجم تر ولی سخت فهم تر
	#define RX_PW_P0    0x11
	#define RX_PW_P1    0x12
	#define RX_PW_P2    0x13
	#define RX_PW_P3    0x14
	#define RX_PW_P4    0x15
	#define RX_PW_P5    0x16
	*/
	uint8_t status;
	readRegister(0x11 + PipeNumber,&status,1);
	return status;

	/*
	روش سخت تر و پر حجم تر ولی خوانا تر
	*/
	/*switch(PipeNumber)
	{
		case 0 : readRegister(RX_PW_P0,&status,1); break;
		case 1 : readRegister(RX_PW_P1,&status,1); break;
		case 2 : readRegister(RX_PW_P2,&status,1); break;
		case 3 : readRegister(RX_PW_P3,&status,1); break;
		case 4 : readRegister(RX_PW_P4,&status,1); break;
		case 5 : readRegister(RX_PW_P5,&status,1); break;
	}
	return status; */
}
uint8_t NRF::getDynamicPayloadSize(void){
	uint8_t result;

	if(payloadStatic==0){ // یعنی طول استایک غیر فعال هستش
		csn(0);           				// Pull down chip select
		SPI.transfer(R_RX_PL_WID);		// Send cmd
		result = SPI.transfer(NULL);	// Ricive Data(Dynamic Payload Size)
		csn(1);         				// Pull up chip select

		return result;
	}
	else{
		return 0;
	}	
}


/*
EN_DPL = Enables Dynamic Payload Length
Enable = 0-1
*/
void NRF::setDPL(bool Enable){
    uint8_t status;
	readRegister(FEATURE,&status,1);

	if(Enable)
		status |= (1<<EN_DPL);
	else
		status &= (0xff - (1<<EN_DPL));

	configRegister(FEATURE, status);
}
/*
PipeNumber:0-5

#define DPL_P5	    5
#define DPL_P4	    4
#define DPL_P3	    3
#define DPL_P2	    2
#define DPL_P1	    1
#define DPL_P0	    0
*/
void NRF::setDynamicPayload(uint8_t PipeNumber, bool EnablePipe){ // DYNPD
	uint8_t status;
	readRegister(DYNPD,&status,1);

	if(EnablePipe)
		status |= (1<<PipeNumber);
	else
		status &= (0xff - (1<<PipeNumber));

	configRegister(DYNPD, status);
}
/*
این تابع بهمون میگه که برای لوله مورد نظر طول داینامیک فعال هستش یا نه

PipeNumber = 0-5

#define DPL_P5	    5
#define DPL_P4	    4
#define DPL_P3	    3
#define DPL_P2	    2
#define DPL_P1	    1
#define DPL_P0	    0
*/
bool NRF::getDynamicPayload(uint8_t PipeNumber){ // DYNPD
	uint8_t status;
	readRegister(DYNPD,&status,1);

	if( status & (1<<PipeNumber) )
		return 1;
	else
		return 0;
}


/*
PipeNumber : 0-5
*/
void NRF::setAutoAck(uint8_t PipeNumber, bool Enable){
    uint8_t status;
	readRegister(EN_AA,&status,1);

    if(Enable)
      status |= (1<<PipeNumber);
    else
      status &= ( 0X7F - (1<<PipeNumber) );

    configRegister(EN_AA, status);
}
/*
PipeNumber:0-5
*/
bool NRF::isAutoAckEnabled(uint8_t PipeNumber){
	uint8_t status;
	readRegister(EN_AA,&status,1);

	if( status & (1<<PipeNumber) )
		return 1;
	else
		return 0;
}


/*
	count : 1-15 ---> اگه 1 قرار بدیم ارسال مجدد غیر فعال میشه
	delay : 0-15
*/
void NRF::SetupReter(uint8_t delay, uint8_t count){
	configRegister(SETUP_RETR,delay<<ARD | count<<ARC);
}


/*
AddressWidth = 3-5
PipeNumber = 0-5

if(payloadSize==0) DynamicPayload = Enable
payloadSize = 1-32 >>> Static = Enable
*/
void NRF::Pipe(uint8_t PipeNumber, bool EnablePipe, uint8_t payloadSize, bool enableACK){

	EnableDataPipe(PipeNumber, EnablePipe); // فعال کردن لوله مد نظر

	setAutoAck(PipeNumber, enableACK);

	if(payloadSize>=1 && payloadSize<=32){ // StaticPayload = Enable
		
		payloadStatic = payloadSize;

		// Set length of incoming payload(Static)
		setStaticPayloadSize(PipeNumber, payloadStatic);

		/*
		اینو غیر فعال نمیکنم شاید طرف یه لوله رو استاتیک کرد و یکی رو داینامیک
		و اون وقت مثلا اولی رو داینامیک میکنه و تابع زیر فعال میشه و لوله بعدی رو
		میاد استاتیک تعریف میکنه و وقتی برنامه داخل ین تابع بشه میبینه که باید
		طول داینامیک سراسری رو غیر فعال کنه یعنی تابع زیر رو باید غیر فعال کنه
		در نتیجه خرابکاری پیش میاد و نباید اینو غیر فعال کنیم.
		*/
		//setDPL(0);
		/*
		چون برای این لوله طول استاتیک رو انتخاب کردیم و لذا باید طول داینامیک برای این لوله رو غیر فعال کنیم
		*/
		setDynamicPayload(PipeNumber, 0);
	}
	else if(payloadSize==0){ // DynamicPayload = Enable
		payloadStatic = 0; // یعنی طول استایک غیر فعال هستش

		toggle_features();

		setDPL(1);

		setDynamicPayload(PipeNumber, 1);
	}
}


uint8_t NRF::CountLostPackets(){
	uint8_t status;
	readRegister(OBSERVE_TX,&status,1);
	status = (status & 0b11110000) >> PLOS_CNT;
	return status;
}
void NRF::ResetCountLostPackets(uint8_t ChannelChangeCount){
    uint8_t Channel = getChannel();

    Channel = Channel + ChannelChangeCount;
	if(Channel<0)Channel=127;
	if(Channel>127)Channel=0;
	
	setChannel(Channel);
}
uint8_t NRF::CountRetransmitPackets(){
	uint8_t status;
	readRegister(OBSERVE_TX,&status,1);
	status &= 0b00001111;
	return status;
}


void NRF::getData(void * data){ // Reads payload bytes into data array
	char* getData = reinterpret_cast<char*>(data); //////////////////////////////////////////////////////////مطالعه در این زمینه
	uint8_t PayloadSize;

	// اگه طول داینامیک حامل فعال بود بیا و مقدار طول حامل رو پیدا کن و در متغییر سراسری مربوطه بریز
	if(payloadStatic==0)
		PayloadSize = getDynamicPayloadSize();
	else
		PayloadSize = payloadStatic;	


	//////////////////////////////////////////////////////////////// Read_RX_Payload(data);
	csn(0);           						// Pull down chip select
	SPI.transfer(R_RX_PAYLOAD);         	// Send cmd to read rx payload
	for(uint8_t i = 0; i < PayloadSize; i++)// Read payload
		getData[i] = SPI.transfer(NULL);
    csn(1);         						// Pull up chip select
	//////////////////////////////////////////////////////////////// Read_RX_Payload(data);


	uint8_t status;
	readRegister(STATUS,&status,1);
	configRegister(STATUS, status | (1<<RX_DR) | (1<<MAX_RT) | (1<<TX_DS) );   // Reset status register
}
/*
اگه متغییر
NoACK
یک بشه و اگه 
ACK
فعال باشه، با این کد میگیم که برای این حامل و فقط همین
لازم نیست که
PRX
تصدیق نامه ارسال کنه برای 
PTX
*/
void NRF::send(void * value, uint8_t PALevel, bool NoACK){
	char* DataSend = reinterpret_cast<char*>(value); /////////////////////////////////////////////////////////مطالعه در این زمینه
	uint8_t PayloadSize;

	
    while (PTX){
		uint8_t status;
		readRegister(STATUS,&status,1);
	    if( status & ((1 << TX_DS) | (1 << MAX_RT)) )
		{
		    PTX = 0;
		    break;
	    }
    }// Wait until last paket is send	

	
	ce(0);
    SetpowerUpTx(); // رفتن به مد ارسال
    flushTx(); // خالی کردن بافر ارسال
	setPALevel(PALevel); // تنظیم توان خروجی
	//Serial.print("PALevel = ");Serial.println(getPALevel());


	// اگه طول داینامیک حامل فعال بود بیا و مقدار طول حامل رو پیدا کن و در متغییر سراسری مربوطه بریز
	if(payloadStatic==0){ // طول داینامیک فعال هستش
		for(int i=0;;i++){
			if(DataSend[i]=='\0'){
				PayloadSize=i;
				Serial.print("Dynamic Payload Size = ");Serial.println(i);
				break;
			}
		}
	}
	else{ // طول استاتیک فعال هستش
		PayloadSize = payloadStatic;
	}


	if(NoACK){
		////////////////////////////////// setNoAck_forThisPayload ////////////////////////
		/*
		این تابع کارش اینه که تو بسته های 
		PTX
		یه بایتی وجود داره به نام
		NO_ACK
		که به 
		PRX
		میگه لازم نیست برام تصدیق نامه خودکار بفرستی
		یعنی ما میایم تقدیق نامه خودکار رو برای حامل جاری که قراره توسط
		PTX
		ارسال بشه رو غیر فعال میکنیم و میگیم برای این حامل تقدیق نامه نمیخوایم
		و فقط فقط برای این حامل و نه تمام حامل ها

		و باید شماره لوله ای که ازش قراره دیتا ارسال بشه رو به این تابع بدید 
		تا بیت مد نظر در حاملی که داخل این لوله قرار میگیره رو 1 کنه

		PipeNumber = 0-5
		
		برای استفاده از این ویژگی تصدیق نامه خودکار باید فعال باشه
		isAutoAckEnabled
		*/
		uint8_t status;
		readRegister(FEATURE,&status,1);
		status = status | (1<<EN_DYN_ACK);
		configRegister(FEATURE, status);

		csn(0);
		SPI.transfer(W_TX_PAYLOAD_NOACK | (0xB0 & FEATURE));
		for(uint8_t i = 0;i < PayloadSize;i++) 	// Write payload
			SPI.transfer(DataSend[i]);
		csn(1);

		Serial.println("NoACK For This Payload is Enabled(AutoAck is Not Checked).");
		////////////////////////////////// setNoAck_forThisPayload ////////////////////////
	}
	else{
		//////////////////////////////////////////////////////////////// Write_TX_Payload(value);
		csn(0);      							// Pull down chip select
		SPI.transfer(W_TX_PAYLOAD);				// Write cmd to write payload
		for(uint8_t i = 0;i < PayloadSize;i++) 	// Write payload
		SPI.transfer(DataSend[i]);
		csn(1);   								// Pull up chip select
		//////////////////////////////////////////////////////////////// Write_TX_Payload(value);
	}


    ce(1);	// Start transmission
}
bool NRF::isSending(){  // بررسی میکنه که آیا دیتا ارسال شده است یا نه
	if(PTX){
		uint8_t status;
		readRegister(STATUS,&status,1);

		if( status & ((1 << TX_DS)| (1 << MAX_RT)) ){
			configRegister(STATUS,status | (1 << TX_DS) | (1 << MAX_RT));

			SetpowerUpRx();

			/////////////////////تنها برای راحت شدن در امر نوشتن کتابخونه اینا رو قرار دادم و وجودشون ظرروری نیست//////////
			if(status & (1 << TX_DS)){
				if(isAutoAckEnabled(0) && isAutoAckEnabled(1))
					Serial.println("Data Send and ACK Received."); 
				else
					Serial.println("Data Send."); 
			}
			else if(status & (1 << MAX_RT)){
				Serial.println("Data Lose."); 
			}				
			/////////////////////تنها برای راحت شدن در امر نوشتن کتابخونه اینا رو قرار دادم و وجودشون ظرروری نیست//////////

			return 0; 
		}
		return 1;
	}
	return 0;
}


void NRF::SetpowerUpRx(){
	PTX = 0;

	uint8_t status;
	readRegister(CONFIG,&status,1);

	/*
	اگه این تاخیر رو نزارم بیت
	MASK_MAX_RT
	از ریجستر کانفیگ
	1 میشه
	که حالا چرا یک میشه رو نمیدونم ولی اگه این دیلای 
	رو نزارم این بیت 1 شده و شمارنده تعداد بسته های از دست
	رفته هم غیر فعال میشه
	*/
	delay(2); // Tpd2stby + Tstby2a >>>> فک کنم

	configRegister(CONFIG, status | (1<<PWR_UP) | (1<<PRIM_RX) );
	ce(1);
	SetCRC(Crc);
}
void NRF::SetpowerUpTx(){
	PTX = 1;
	
	uint8_t status;
	readRegister(CONFIG,&status,1);
	configRegister(CONFIG, (status | (1<<PWR_UP)) & (0xff - (1<<PRIM_RX)) );
	SetCRC(Crc);
}
void NRF::SetpowerDown(){
	ce(0);
}
Mode NRF::getMode(){ // Mode : PowerDown, PowerUpRX, PowerUpTX, Standby1, Standby2
	uint8_t status;
	readRegister(CONFIG,&status,1);

	if(status & (1<<PWR_UP) ) // on
	{
		if( (status & (1<<PRIM_RX)) && digitalRead(cePin)) //PowerUpRX
			return PowerUpRX;
		else if( !(status & (1<<PRIM_RX)) && digitalRead(cePin) && TXFifo()==0)//Standby2, TXFifo=EMPTY
			return Standby2;
		else if( !digitalRead(cePin) && TXFifo()==0)//Standby1, TXFifo=EMPTY
			return	Standby1;
		else if( !(status & (1<<PRIM_RX)) && digitalRead(cePin) && TXFifo()==1)//PowerUpTX, TXFifo=ThereAreData
			return PowerUpTX;
	}
	else // off
		return PowerDown;
}


uint8_t NRF::TXFifo(void){ // EMPTY=0, ThereAreData=1, FULL=2
	uint8_t status;
	readRegister(FIFO_STATUS,&status,1);

	if(status & (1<<TX_EMPTY))
		return 0; //EMPTY
	else if(status & (1<<TX_FULL))
		return 2; //FULL
	else
		return 1; //ThereAreData
}
uint8_t NRF::RXFifo(uint8_t* pipe_num){ // EMPTY=0, ThereAreData=1, FULL=2
	uint8_t status;
	readRegister(FIFO_STATUS,&status,1);

	if(status & (1<<RX_EMPTY)){
		return 0; //EMPTY
	}
	else if(status & (1<<RX_FULL)){
		if (pipe_num){ // شماره لوله ای که دیتا داخلش قرار گرفته
			readRegister(STATUS,&status,1);
			*pipe_num = ( status >> RX_P_NO ) & 0b00000111;
		}
		return 2; //FULL
	}
	else{
		if (pipe_num){ // شماره لوله ای که دیتا داخلش قرار گرفته
			readRegister(STATUS,&status,1);
			*pipe_num = ( status >> RX_P_NO ) & 0b00000111;
		}
		return 1; //ThereAreData
	}
}


/*
1 : وقفه مد نظر رو فعال کن
0 : وقفه مد نظر رو غیر فعال کن

وقفه با 0 فعال و با 1 غیر فعال میشه
*/
void NRF::setIRQ(bool fail, bool tx, bool rx){
	uint8_t status;
	readRegister(CONFIG,&status,1);

	if(tx)	status &= (0xff - (1<<MASK_MAX_RT));
	else	status |= (1<<MASK_MAX_RT);

	if(fail)status &= (0xff - (1<<MASK_TX_DS));
	else	status |= (1<<MASK_TX_DS);

	if(rx)	status &= (0xff - (1<<MASK_RX_DR));
	else	status |= (1<<MASK_RX_DR);

	configRegister(CONFIG, status);
}
void NRF::getIRQ(bool *fail, bool *tx, bool *rx){
	uint8_t status;
	readRegister(CONFIG,&status,1);

	if(status & (1<<MASK_MAX_RT))	*fail=0;
	else 							*fail=1;

	if(status & (1<<MASK_TX_DS))	*tx=0;
	else							*tx=1;

	if(status & (1<<MASK_RX_DR)) 	*rx=0;
	else							*tx=1;
}


void NRF::toggle_features(void){
	csn(0);  
	SPI.transfer(ACTIVATE);
	SPI.transfer(0x73);
    csn(1);
}


bool NRF::testCarrier(void){
	uint8_t status;
	readRegister(CD,&status,1);
	return (status & 0b00000001);	
}
/*
ChannelList(Size) = (EndChannel - StartChannel) + 1
StartChannel >=0
EndChannel <=127
Delay = 100-1000

برا این شروع و پایان کانال رو گزاشتم که
اونایی که با کمبود حافظه هستن به مشکل بر نخورن
محاسبات از کانال
StartChannel
شروع شده و در کانال
EndChannel
پایان می یابد
*/
void NRF::findBestChannel(uint8_t * ChannelList, uint8_t StartChannel, uint8_t EndChannel, int Delay){
	// صفر کردن تمام خونه ها
	//Serial.println("StartCleaning");
	for(uint8_t i = 0; i<=(EndChannel-StartChannel); i++)
		ChannelList[i] = 0;


	// دریافت میزان نویز در هر کانال و ذخیره کردن در آرایه
	//Serial.println("StartReading");
	uint8_t Channel = StartChannel-1;
	do{
		setChannel(++Channel);

		for (int i=0; i<=Delay*10; i++){
			if (testCarrier())
				ChannelList[Channel]++;
		}
		//Serial.println(Channel);
	}
	while (Channel <= EndChannel);


  /*Serial.println("StartArrangement");
  uint8_t a;
  for (int j = 0; j < 127; j++)
  {
    for (int i = 0; i < 127 - j; i++)
      if (ChannelList[i] > ChannelList[i + 1])
      {
        a = ChannelList[i];
        ChannelList[i] = ChannelList[i + 1];
        ChannelList[i + 1] = a;
      }
  }*/
}


/*
level : 0-3 ---> هر چی عدد بزرگ تر باشه جریان مصرفی بیشتر و قدر امواج بیشتر
*/
void NRF::setPALevel(uint8_t level){ // PA = Power Amplifier, Set RF Output Power in TX mode

	uint8_t status;
	readRegister(RF_SETUP,&status,1);

	if(level==0) 		status = status & 0b11111001;
	else if(level==1)	status = (status | 0b00000010) & 0b11111011;
	else if(level==2)	status = (status & 0b11111101) | 0b00000100;
	else if(level==3)	status = status | 0b00000110;

	configRegister(RF_SETUP,status);
}
uint8_t NRF::getPALevel(void){
	uint8_t status;
	readRegister(RF_SETUP,&status,1);
	status = (status>>1) & 0b00000011;
	return status;
}









/////////////// HACK ///////////// HACK //////////////// HACK ////////////// HACK /////////// HACK ///////////// HACK ///////
// این تابع تکمیل شود
/*uint8_t NRF::FindChannelAnoderNrf_RiciveData(uint8_t Delay){
	int i;
	unsigned long time;

	for(i=0;i<=127;i++)
	{
		time = millis();

		//flushRx(); // خطا؟
		init(8, 7); // cePin=8, csnPin=7
		setAddressWidth(5); // AddressWidth = 5(Byte)
		setRxADDR((char *)"clie1"); // Set the reciving address.
		setTxADDR((char *)"serv1"); // Set the sending address.
		config(i, sizeof(unsigned long), Crc8Bit, _1Mbps, ShockBurst); // channel=1, payloadSize = sizeof(unsigned long)

		while(RXFifo(NULL)==0 && ( millis() - time ) < Delay){} // 5 ثانیه بستگی به سرعت ارسال اطلاعات توسط فرستنده داره - اگه اطلاعات رو سریع میفرستاد ما هم این زمان رو کم میکنیم

		if(RXFifo(NULL)!=0)
			goto down;
	}
	down:
	return i;
}*/
/////////////// HACK ///////////// HACK //////////////// HACK ////////////// HACK /////////// HACK ///////////// HACK ///////