#include <Arduino.h>
#include "nRF24L01.h"

//#if defined _SOFTSPI_H_
	//#include <SoftSPI.h>
//#else
	#include <SPI.h>
//#endif



enum CrcLength{CrcDisable=0, Crc8Bit=1, Crc16Bit=2};
enum Mode{PowerDown=0, PowerUpRX=1, PowerUpTX=2, Standby1=3, Standby2=4};
enum AirDataRate{_1Mbps=1, _2Mbps=2};
//enum TX_RX_FifoStatus{EMPTY=0, FULL=1, ThereAreData=2};



class NRF
{
	public:
		void init(uint8_t ce_pin, uint8_t csn_pin);
		void setReceiverAddress(uint8_t PipeNumber, char * AddressReceiver, uint8_t AddressWidth);
		void setSenderAddress(char * AddressSender, uint8_t AddressWidth);
		void Pipe(uint8_t PipeNumber, bool EnablePipe, uint8_t payloadSize, bool enableACK);
		void config(uint8_t channel, CrcLength CRCval, AirDataRate DataRate, uint8_t DelayReTransmin, uint8_t CountReTransmin);

		

		void setStaticPayloadSize(uint8_t PipeNumber, uint8_t payloadSize);
		uint8_t getStaticPayloadSize(uint8_t PipeNumber);
		
		void setDPL(bool Enable);
		void setDynamicPayload(uint8_t PipeNumber, bool EnablePipe);
		uint8_t getDynamicPayloadSize(void);
		bool getDynamicPayload(uint8_t PipeNumber);

		bool setChannel(uint8_t channel);
		uint8_t getChannel();

		void flushRx();
		void flushTx();

		void ce(bool level);
		void csn(bool level);

		void send(void * value, uint8_t PALevel, bool NoACK);
		bool isSending();
		void getData(void * data);

		void configRegister(uint8_t reg, uint8_t value);
		void readRegister(uint8_t reg, uint8_t * value, uint8_t len);
		void writeRegister(uint8_t reg, char * value, uint8_t len);

		void SetpowerUpRx();
		void SetpowerUpTx();
		void SetpowerDown();
		Mode getMode();

		bool setAddressWidth(uint8_t width);
		uint8_t getAddressWidth();
		void EnableDataPipe(uint8_t PipeNumber, bool EnablePipe);

		uint8_t CountLostPackets();
		void ResetCountLostPackets(uint8_t ChannelChangeCount);
		uint8_t CountRetransmitPackets();

		void SetCRC(CrcLength Length);
		uint8_t getCRCLength(void);

		void setDataRate(AirDataRate DataRate);
		uint8_t getDataRate(void);

		uint8_t TXFifo(void);
		uint8_t RXFifo(uint8_t* pipe_num);

		void setIRQ(bool tx, bool fail, bool rx);
		void getIRQ(bool *fail, bool *tx, bool *rx);

		void toggle_features(void);

		uint8_t getPayloadSize(void);

		void setPALevel(uint8_t level);
		uint8_t getPALevel(void);

		bool testCarrier(void);
		void findBestChannel(uint8_t * ChannelList, uint8_t StartChannel, uint8_t EndChannel, int Delay);

		void setAutoAck(uint8_t PipeNumber, bool Enable);
		bool isAutoAckEnabled(uint8_t PipeNumber);
		void SetupReter(uint8_t delay, uint8_t count); // AutoRetransmit Count & Delay




		//uint8_t FindChannelAnoderNrf_RiciveData(uint8_t Delay);




		uint8_t payloadStatic; 	//Payload width, if(payload==0)DynamicPayload=Enable
		uint8_t PTX; 		//In sending mode.
		uint8_t cePin; 		//CE Pin controls RX / TX, default 8.
		uint8_t csnPin; 	//CSN Pin Chip Select Not, default 7.
		CrcLength Crc;
};