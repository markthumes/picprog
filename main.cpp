#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <inttypes.h>

#include "pic.h"

#define TENTS 1
#define TENTH 250
#define TCKL  1
#define TCKH  1
#define TDS   1
#define TDH   1
#define TDLY  1
#define TERAB 13000
#define TERAR 2800
#define TPEXT 2100
#define TPINT 2800
#define TEXIT 1

namespace Instructions{
	const uint8_t READNVM       	= 0xfc; //Working on read nvm
	const uint8_t READNVM_INCPC 	= 0xfe; //Working on read nvm
	const uint8_t SETPC		= 0x80;	//Working on read nvm
	const uint8_t INCPC		= 0xf8; //working on read nvm
	const uint8_t LOADNVM		= 0x00;
	const uint8_t LOADNVM_INCPC	= 0x02;
	const uint8_t BULKERASE		= 0x18;
	const uint8_t ROWERASE		= 0xf0;
	const uint8_t BEGININTPROGRAM   = 0xe0;
	const uint8_t BEGINEXTPROGRAM   = 0xc0;
	const uint8_t ENDEXTPROGRAM     = 0x82;

};

#define STOPBIT 1

#define GPIO_MCLR   9 
#define GPIO_CLOCK 11 
#define GPIO_DATA  10 
#define GPIO_POWER 22

#define PAYLOADSZ 24 //PIC16F152xx Family Programming Spec Section 3.2

class Programmer{
public:
	Programmer( uint8_t mclr = GPIO_MCLR, uint8_t clock = GPIO_CLOCK, uint8_t data = GPIO_DATA, uint8_t power = GPIO_POWER ){
		m_mclr   = Pin(mclr);
		m_clock  = Pin(clock);
		m_data   = Pin(data);
		m_pwr    = Pin(power);
	}
	~Programmer(){
		vppFirstExit();
	}
	void start(){
		fprintf(stdout, "Starting up piC Programmer\n");
		vppFirstEntry();
		enterLVP();
		fprintf(stdout, "Device %04x rev. %04x found.\n", getDeviceID(), getRevisionID() );
		getDCI();
		printDCI(stdout);
	}
	void enterLVP(){

		usleep(TENTS);
		m_mclr.setVoltage(Pin::VLOW);
		usleep(TENTH);

		uint32_t bits = 0x4d434850;
		write( bits, 32 );
		usleep(TENTH);
	}
	void write( uint32_t word, unsigned int numberofbits = 8 ){
		m_clock.setDirection(Pin::OUTPUT);
		m_data.setDirection(Pin::OUTPUT);
		for( int i = numberofbits - 1; i >= 0; i-- ){ //index 13 -> 0

			uint32_t bit = (word & (0x0001 << i)) >> i;
			m_data.setVoltage( bit == 0x0001 ? Pin::VHIGH : Pin::VLOW );

			m_clock.setVoltage(Pin::VHIGH);
			usleep( TCKH );
			m_clock.setVoltage(Pin::VLOW);
			usleep( TCKL );
		}
	}
	uint32_t read( unsigned int numberofbits = 8){
		uint32_t ret = 0;
		m_clock.setDirection(Pin::OUTPUT);
		m_data.setDirection(Pin::INPUT);
		for( int i = numberofbits - 1; i >= 0; i-- ){

			m_clock.setVoltage(Pin::VHIGH);
			usleep( TCKH );

			m_clock.setVoltage(Pin::VLOW);
			ret |= m_data.readVoltage() << i;
			usleep( TCKL );
		}
		//Ignore start and stop bit
		ret &= ~(0x01 << (numberofbits - 1)); //mask (first) start bit
		return ret >> 1; //shift over last (stop) bit
	}

	uint32_t readNVM(){
		write( Instructions::READNVM );
		return read( PAYLOADSZ );
	}

	int readNVM( uint32_t address, uint32_t* data, size_t len ){
		setPC( address );
		for( unsigned int i = 0; i < len; i++ ){
			write( Instructions::READNVM_INCPC );
			data[i] = read(PAYLOADSZ);
		}
		return len;
	}

	int writeNVM( uint32_t data ){
		write( Instructions::LOADNVM );
		write( data << STOPBIT, PAYLOADSZ );
		return 1;
	}

	int writeNVM( uint32_t address, uint32_t* data, size_t len ){
		setPC( address );
		for( unsigned int i = 0; i < len; i++ ){
			write( Instructions::LOADNVM_INCPC );
			write( data[i] << STOPBIT, PAYLOADSZ );
		}
		return len;
	}

	void setPC(uint32_t address){
		// OPCODE
		write( Instructions::SETPC );
		/* Shift left because LSB is at bit 0 */
		write( address << STOPBIT, PAYLOADSZ );
	}
	void incPC(){
		write( Instructions::INCPC );
	}

	void bulkErase(){
		write( Instructions::BULKERASE );
		usleep(TERAB);
	}

	void rowErase(){
		write( Instructions::ROWERASE );
		usleep( TERAR );
	}

	void beginIntProgramming(){
		write( Instructions::BEGININTPROGRAM );
		usleep( TPINT );
	}

	void beginExtProgramming(){
		write( Instructions::BEGINEXTPROGRAM);
	}

	void endExtProgramming(){
		write( Instructions::ENDEXTPROGRAM );
	}

	void commit(){
		beginIntProgramming();
	}

	void write(){
		beginExtProgramming();
		usleep( TPEXT );
		endExtProgramming();
	}

	void vppFirstEntry(){
		m_mclr.setDirection(Pin::OUTPUT);
		m_clock.setDirection(Pin::OUTPUT);
		m_data.setDirection(Pin::OUTPUT);
		m_pwr.setDirection(Pin::OUTPUT);

		m_mclr.setVoltage(Pin::VLOW);
		m_clock.setVoltage(Pin::VLOW);
		m_data.setVoltage(Pin::VLOW);
		m_pwr.setVoltage(Pin::VLOW);

		m_mclr.setVoltage(Pin::VHIGH);
		usleep(TENTS);
		m_pwr.setVoltage(Pin::VHIGH);
		usleep(TENTH);
		m_data.setVoltage(Pin::VHIGH);
		m_clock.setVoltage(Pin::VHIGH);
		usleep(TCKH);
		m_clock.setVoltage(Pin::VLOW);
		usleep(TCKL);

		usleep(10000);
	}
	void vppFirstExit(){
		m_mclr.setDirection(Pin::OUTPUT);
		m_clock.setDirection(Pin::OUTPUT);
		m_data.setDirection(Pin::OUTPUT);
		m_pwr.setDirection(Pin::OUTPUT);

		m_clock.setVoltage(Pin::VLOW);
		usleep(TCKL);
		m_clock.setVoltage(Pin::VHIGH);
		usleep(TCKH);
		m_clock.setVoltage(Pin::VLOW);
		m_data.setVoltage(Pin::VLOW);
		m_pwr.setVoltage(Pin::VLOW);
		usleep(TEXIT);
		m_mclr.setVoltage(Pin::VLOW);
	}

	void setConfig( uint32_t *words ){
		writeNVM( 0x8000, words, 4 );
		beginIntProgramming();
	}

	/********* ROUND 2 *********/
	void multiWordProgramming( uint32_t address, uint32_t *data, size_t sz ){
		setPC( address );

	}

	uint16_t getDeviceID(){
		setPC( 0x8006 );
		return (uint16_t)readNVM();
	}
	uint16_t getRevisionID(){
		setPC( 0x8005 );
		return (uint16_t)(readNVM() & 0x0fff);
	}

	void getDIA(){
		setPC( 0x8100 );
		microid[9] = '\0';
		for( int i = 0; i < 9; i++ ){
			microid[i] = (uint16_t)readNVM();
		}
		setPC( 0x810a );
		extuid[8] = '\0';
		for( int i = 0; i < 8; i++ ){
			extuid[i] = (uint16_t)readNVM();
		}
	}

	void printDCI(FILE *fp){
		fprintf(fp, "Erase Row Size: %d words\n",ersiz);
		fprintf(fp, "Number of write latches per row: %d words\n",wlsiz);
		fprintf(fp, "Number of user erasable rows: %d rows\n",ursiz);
		fprintf(fp, "Data EEPROM memory size: %d bytes\n",eesiz);
		fprintf(fp, "Pin Count: %d pins\n", pcnt);
	}

	void getDCI(){
		setPC( 0x8200 ); ersiz = readNVM();
		setPC( 0x8201 ); wlsiz = readNVM();
		setPC( 0x8202 ); ursiz = readNVM();
		setPC( 0x8203 ); eesiz = readNVM();
		setPC( 0x8204 ); pcnt = readNVM();
	}
	
private:
	uint16_t microid[10];
	uint16_t extuid[9];
	uint16_t ersiz;
	uint16_t wlsiz;
	uint16_t ursiz;
	uint16_t eesiz;
	uint16_t pcnt;
	Pin m_clock;
	Pin m_data;
	Pin m_mclr;
	Pin m_pwr;
};

int main( int argc, char** argv ){

	uint32_t data[100];
	Programmer pic;
	pic.start();

	return 1;
}
