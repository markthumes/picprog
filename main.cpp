#include <stdio.h>
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

#define PAYLOADSZ 24 //PIC16F152xx Family Programming Spec Section 3.2

class Programmer{
public:
	Programmer( uint8_t mclr = GPIO_MCLR, uint8_t clock = GPIO_CLOCK, uint8_t data = GPIO_DATA ){
		m_mclr   = Pin(mclr);
		m_clock  = Pin(clock);
		m_data   = Pin(data);
	}
	~Programmer(){
		stop();
	}
	void start(){
		enterLVP();
	}
	void enterLVP(){
		m_mclr.setDirection(Pin::OUTPUT);
		m_clock.setDirection(Pin::OUTPUT);
		m_data.setDirection(Pin::OUTPUT);

		m_mclr.setVoltage(Pin::VLOW);
		m_clock.setVoltage(Pin::VLOW);
		m_data.setVoltage(Pin::VLOW);

		usleep(TENTS);

		m_mclr.setVoltage(Pin::VHIGH);
		usleep(TENTH);
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
			fprintf(stdout, "%x\n", data[i] );
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

	void commit(){
		beginIntProgramming();
	}

	void stop(){
		m_mclr.setVoltage(Pin::VHIGH);
	}

	void setConfig( uint32_t *words ){
		writeNVM( 0x8000, words, 4 );
		beginIntProgramming();
	}
	
private:
	Pin m_clock;
	Pin m_data;
	Pin m_mclr;
};

int main( int argc, char** argv ){

	Programmer pic;
	pic.start();

	pic.setPC( 0x0000 );
	pic.writeNVM( 0xac );
	pic.commit();

	uint32_t ids[10];
	pic.setPC( 0x0000 );
	for( int i = 0; i < 10; i++ ){
		ids[i] = pic.readNVM();
		pic.incPC();
		fprintf(stdout, "Memory[%d]: %x\n", i, ids[i]);
	}



	return 1;
}
