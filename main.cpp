#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <vector>
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
#define TDIS 300

typedef struct reggi{
	uint8_t addr;
	uint8_t bank;
	reggi(uint8_t a, uint8_t b):addr(a),bank(b){
	}
}Register;

namespace Instructions{

	/* SPI Programming instructions */
	const uint8_t READNVM       	= 0xfc;
	const uint8_t READNVM_INCPC 	= 0xfe;
	const uint8_t SETPC		= 0x80;
	const uint8_t INCPC		= 0xf8;
	const uint8_t LOADNVM		= 0x00;
	const uint8_t LOADNVM_INCPC	= 0x02;
	const uint8_t BULKERASE		= 0x18;
	const uint8_t ROWERASE		= 0xf0;
	const uint8_t BEGININTPROGRAM   = 0xe0;
	const uint8_t BEGINEXTPROGRAM   = 0xc0;
	const uint8_t ENDEXTPROGRAM     = 0x82;

	const Register STATUS(0x03, 0 );
	const Register PORTA( 0x0c, 0 );
	const Register PORTB( 0x0d, 0 );
	const Register PORTC( 0x0e, 0 );
	const Register PORTD( 0x0f, 0 );
	const Register PORTE( 0x10, 0 );
	const Register TRISA( 0x12, 0 );
	const Register TRISB( 0x13, 0 );
	const Register TRISC( 0x14, 0 );
	const Register TRISD( 0x15, 0 );
	const Register TRISE( 0x16, 0 );
	const Register LATA ( 0x18, 0 );
	const Register LATB ( 0x19, 0 );
	const Register LATC ( 0x1a, 0 );
	const Register LATD ( 0x1b, 0 );
	const Register LATE ( 0x1c, 0 );
	const Register SFR  ( 0x0c, 0 );
	const Register GRAM ( 0x20, 0 );
	const Register CRAM ( 0x70, 0 ); //COMMON Register RAM

	/* PIC Instruction Set */
	/* Byte-Oriented Operations */
	uint32_t ADDWF ( bool d, uint8_t f ){ return ( 0x0700 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t ADDWFC( bool d, uint8_t f ){ return ( 0x3d00 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t ANDWF ( bool d, uint8_t f ){ return ( 0x0500 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t ASRF  ( bool d, uint8_t f ){ return ( 0x3700 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t LSLF  ( bool d, uint8_t f ){ return ( 0x3500 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t LSRF  ( bool d, uint8_t f ){ return ( 0x3600 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t CLRF  (         uint8_t f ){ return ( 0x0180              | ( f & 0x7f ) ); }
	uint32_t CLRW  (                   ){ return ( 0x0100                             ); }
	uint32_t COMF  ( bool d, uint8_t f ){ return ( 0x0900 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t DECF  ( bool d, uint8_t f ){ return ( 0x0300 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t INCF  ( bool d, uint8_t f ){ return ( 0x0a00 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t IORWF ( bool d, uint8_t f ){ return ( 0x0400 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t MOVF  ( bool d, uint8_t f ){ return ( 0x0800 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t MOVWF (         uint8_t f ){ return ( 0x0080              | ( f & 0x7f ) ); }
	uint32_t RLF   ( bool d, uint8_t f ){ return ( 0x0d00 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t RRF   ( bool d, uint8_t f ){ return ( 0x0c00 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t SUBWF ( bool d, uint8_t f ){ return ( 0x0200 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t SUBWFB( bool d, uint8_t f ){ return ( 0x3b00 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t SWAPF ( bool d, uint8_t f ){ return ( 0x0e00 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t XORWF ( bool d, uint8_t f ){ return ( 0x0600 | ( d << 7 ) | ( f & 0x7f ) ); }

	/* Byte Oriented Skip Operations */
	uint32_t DECFSZ( bool d, uint8_t f ){ return ( 0x0b00 | ( d << 7 ) | ( f & 0x7f ) ); }
	uint32_t INCFSZ( bool d, uint8_t f ){ return ( 0x0f00 | ( d << 7 ) | ( f & 0x7f ) ); }

	/* Bit Oriented File Register Operations */
	uint32_t BCF   ( uint8_t d, uint8_t f ){ return ( 0x1000 | ( (d & 0x07) << 7 ) | ( f & 0x7f ) ); }
	uint32_t BSF   ( uint8_t d, uint8_t f ){ return ( 0x1400 | ( (d & 0x07) << 7 ) | ( f & 0x7f ) ); }

	/* Bit oriented skip operations */
	uint32_t BTFSC ( uint8_t d, uint8_t f ){ return ( 0x1800 | ( (d & 0x07) << 7 ) | ( f & 0x7f ) ); }
	//uint32_t BTFSS ( uint8_t d, uint8_t f ){ return ( 0x1800 | ( (d & 0x07) << 7 ) | ( f & 0x7f ) ); } //INCORRECT IN MANUAL
	
	/* Literal Operations */
	uint32_t ADDLW( uint8_t k ){ return ( 0x3e00 | ( k & 0xff ) ); }
	uint32_t ANDLW( uint8_t k ){ return ( 0x3900 | ( k & 0xff ) ); }
	uint32_t IORLW( uint8_t k ){ return ( 0x3800 | ( k & 0xff ) ); }
	uint32_t MOVLB( uint8_t k ){ return ( 0x0140 | ( k & 0x3f ) ); } //INCORRECT IN MANUAL, but found soln on microchip.com/forms/m1131489.aspx
	uint32_t MOVLP( uint8_t k ){ return ( 0x3180 | ( k & 0x7f ) ); }
	uint32_t MOVLW( uint8_t k ){ return ( 0x3000 | ( k & 0xff ) ); }
	uint32_t SUBLW( uint8_t k ){ return ( 0x3c00 | ( k & 0xff ) ); }
	uint32_t XORLW( uint8_t k ){ return ( 0x3a00 | ( k & 0xff ) ); }

	uint32_t BRA   ( uint16_t k ){ return ( 0x3200 | ( k & 0x1ff ) ); }
	uint32_t BRW   (            ){ return ( 0x000b ); }
	uint32_t CALL  ( uint16_t k ){ return ( 0x2000 | ( k & 0x7ff ) ); }
	uint32_t CALLW (            ){ return ( 0x000a ); } 
	uint32_t GOTO  ( uint16_t k ){ return ( 0x2800 | ( k & 0x7ff ) ); }
	//uint32_t RETFIE( uint16_t k ){ return ( 0x2800 | ( k & 0x7ff ) ); } //SEEMS INCORRECT IN MANUAL
	uint32_t RETLW ( uint8_t k ){ return ( 0x3400 | ( k & 0xff ) ); }
	uint32_t RETURN(            ){ return ( 0x0008 ); } 

	/* Inherent Ops */
	uint32_t CLRWDT(){ return 0x0064; } 
	uint32_t NOP   (){ return 0x0000; } 
	uint32_t RESET (){ return 0x0001; } 
	uint32_t SLEEP (){ return 0x0063; } 

	uint32_t SLEEP (uint8_t f){ return ( 0x0060 | ( f & 0x07 ) ); }

	/* C-compiler optimized */
	uint32_t ADDFSR( bool n, uint8_t k ){ return ( 0x3100 | ( n << 6 ) | ( k & 0x3f ) ); }

	uint32_t MOVIWmm( bool n, uint8_t m ){ return ( 0x0010 | ( n << 2 ) | ( m & 0x03 ) ); }
	uint32_t MOVIW  ( bool n, uint8_t k ){ return ( 0x3f00 | ( n << 6 ) | ( k & 0x3f ) ); }

	uint32_t MOVWImm( bool n, uint8_t m ){ return ( 0x0018 | ( n << 2 ) | ( m & 0x03 ) ); }
	uint32_t MOVWI  ( bool n, uint8_t k ){ return ( 0x3f80 | ( n << 6 ) | ( k & 0x3f ) ); }

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
	void addCMD( uint32_t cmd, const char* func = NULL, bool resolve = false ){
		if( func != NULL ){
			if( resolve ){
				Functions f(m_software.size(), func, true );
				m_func.push_back( f );
			}
			else{
				Functions f(m_software.size(), func, false );
				m_func.push_back( f ) ;
			}
		}
		m_software.push_back(cmd);

	}
	uint32_t resolve( uint32_t cmd, const char* name ){
		for( unsigned int i = 0; i < m_func.size(); i++){
			if( strcmp( name, m_func[i].name ) == 0 && m_func[i].call == false ){
				if( (cmd & 0x3800) == 0x2000 ){ //CALL
					return 0x3800 | ( m_func[i].addr & 0x7f );
				}
				else{
					fprintf(stderr, "UNIMPLEMENTED\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		fprintf(stderr, "UNEXPECTED END OF FUNCTION\n");
		return 0x0000;
	}
	void uploadMain(){
		uint32_t software_size = m_software.size();
		uint32_t minimum_size = software_size + 32 - (software_size % 32);
		uint32_t total_rows = minimum_size / 32;

		uint32_t *temp_mem = new uint32_t[minimum_size];
		for( unsigned int i = 0; i < minimum_size; i++ ){
			if( i < software_size ){
				for( unsigned int j = 0; j < m_func.size(); j++){
					if( i == m_func[j].addr ){
						if( m_func[j].call == true ){
							temp_mem[i] = resolve(
									m_software[i],
									m_func[j].name
								);
							continue;
						}
					}
				}
				temp_mem[i] = m_software[i];
			}
			else temp_mem[i] = 0x0000;
		}
		for(unsigned int i = 0; i < total_rows; i++ ){
			fprintf(stdout, "Row: %d [addr: %d]\n", i, 32*i);
			if( !writeRow( 32*i, &temp_mem[32*i], 32 ) ){
				fprintf(stderr, "Failed to write to memory\n");
			}
		}
		delete [] temp_mem;

	}
	~Programmer(){
		if( m_open ){
			vppFirstExit();
		}
	}
	void start(){
		fprintf(stdout, "Starting up piC Programmer\n");
		vppFirstEntry();
		enterLVP();
		fprintf(stdout, "Device %04x rev. %04x found.\n", getDeviceID(), getRevisionID() );
		getDCI();
		printDCI(stdout);
		m_open = true;
	}
	void enterLVP(){

		usleep(TENTS);
		m_mclr.setVoltage(Pin::VLOW);
		usleep(TENTH);

		uint32_t bits = 0x4d434850;
		write( bits, 32 );
		usleep(TENTH);
	}

	uint32_t readNVM(){
		write( Instructions::READNVM );
		usleep(TDLY);
		uint32_t ret = read( PAYLOADSZ );
		usleep(TDLY);
		return ret;
	}

	int readNVM( uint32_t address, uint32_t* data, size_t len ){
		setPC( address );
		for( unsigned int i = 0; i < len; i++ ){
			write( Instructions::READNVM_INCPC );
			usleep( TDLY );
			data[i] = read(PAYLOADSZ);
			usleep( TDLY );
		}
		return len;
	}

	int writeNVM( uint32_t data ){
		write( Instructions::LOADNVM );
		usleep( TDLY );
		write( data << STOPBIT, PAYLOADSZ );
		usleep( TDLY );
		return 1;
	}

	int writeNVM( uint32_t address, uint32_t* data, size_t len ){
		setPC( address );
		for( unsigned int i = 0; i < len; i++ ){
			write( Instructions::LOADNVM_INCPC );
			usleep( TDLY );
			write( data[i] << STOPBIT, PAYLOADSZ );
			usleep( TDLY );
		}
		return len;
	}

	void setPC(uint32_t address){
		// OPCODE
		write( Instructions::SETPC );
		usleep( TDLY );
		/* Shift left because LSB is at bit 0 */
		write( address << STOPBIT, PAYLOADSZ );
		usleep( TDLY );
	}
	void incPC(){
		write( Instructions::INCPC );
		usleep( TDLY );
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
		usleep( TPEXT );
	}

	void endExtProgramming(){
		write( Instructions::ENDEXTPROGRAM );
		usleep( TDIS );
	}

	void setConfig( uint32_t *words ){
		writeNVM( 0x8000, words, 4 );
		beginIntProgramming();
	}

	/********* ROUND 2 *********/
	bool writeRow( uint32_t address, uint32_t *data, size_t sz ){
		if( sz != 32 ){	
			fprintf(stderr, "Mismatch in sz\n");
			return false;
		}
		setPC( address );
		for( unsigned int i = 0; i < 32; i++ ){
			write( Instructions::LOADNVM_INCPC );
			usleep( TDLY );
			write( data[i] << STOPBIT, PAYLOADSZ );
			usleep( TDLY );
		}
		setPC( address );
		beginIntProgramming();
		for( unsigned int i = 0; i < 32; i++ ){
			write( Instructions::READNVM_INCPC );
			usleep( TDLY );
			uint32_t ret = read(PAYLOADSZ);
			fprintf(stdout, "Ret: %04x - Dat: %04x\n", ret, data[i] );
			if( ret != data[i] ) return false;
			usleep( TDLY );
		}
		return true;
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

	void powerOn(){
		m_mclr.setDirection(Pin::OUTPUT);
		m_pwr.setDirection(Pin::OUTPUT);
		m_mclr.setVoltage(Pin::VHIGH);
		m_pwr.setVoltage(Pin::VHIGH);
	}

	void stop(){
		vppFirstExit();
	}

protected:
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
		m_open = false;
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
	
private:
	uint16_t microid[10];
	uint16_t extuid[9];
	uint16_t ersiz;
	uint16_t wlsiz;
	uint16_t ursiz;
	uint16_t eesiz;
	uint16_t pcnt;

	std::vector<uint32_t> m_software;
	typedef struct funclocats{
		uint32_t addr;
		const char* name;
		bool call;
		funclocats( uint32_t a, const char* n, bool c = false ){
			addr = a;
			name = n;
			call = c;
		}
	}Functions;
	std::vector<Functions> m_func;

	bool m_open;

	Pin m_clock;
	Pin m_data;
	Pin m_mclr;
	Pin m_pwr;
};


using namespace Instructions;
int main( __attribute__((unused))int argc, __attribute__((unused))char** argv ){

	uint32_t config[] = {
		0x0111, //0x8007
		0x3218, //0x8008
		0x0000, //0x8009
		0x2b98, //0x800a
		0x0001  //0x800b
	};
	uint32_t userid[] = {
		'M', 'a', 'r', 'k'
	};

#if 1
	uint32_t program[] = {
		//MAIN
		BRA(51),
		0x0000,
		0x0000,
		0x0000, //CALL SAVE DATA...
		MOVLB(STATUS.bank),
		//MOVLW(0x01),
		//MOVWF(CRAM.addr+0),
		//MOVLW(1),
		//SUBWF(1,CRAM.addr+0),
		//BTFSC(3,STATUS.addr),
		//BRA(-5),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		NOP(),
		RETURN(),

		
		MOVLB(0), //INIT IO
		MOVLW(0x01),
		MOVWF(TRISA.addr),
		MOVLW(0xff),
		MOVWF(TRISB.addr),
		MOVLW(0x00),
		MOVWF(TRISC.addr),

		MOVLW(0x02),
		MOVWF(LATA.addr),
		CALL(3),
		MOVLW(0x00),
		MOVWF(LATA.addr),
		CALL(3),
		BRA(-7),

		//LOAD Temporary data in General RAM
		MOVLB(0),
		MOVLW(0xaa),
		MOVWF(GRAM.addr+0),
		MOVLW(0x02),
		MOVWF(GRAM.addr+1),
		MOVLW(0x03),
		MOVWF(GRAM.addr+2),
		MOVLW(0x04),
		MOVWF(GRAM.addr+3),
		MOVLW(0x05),
		MOVWF(GRAM.addr+4),
		MOVLW(0x06),
		MOVWF(GRAM.addr+5),
		MOVLW(0x07),
		MOVWF(GRAM.addr+6),
		MOVLW(0x08),
		MOVWF(GRAM.addr+7),
		//CALL(37), //CALL Parallel Shift
		MOVLB(0), //Parallel Shift
		MOVLW(0xdf), //Set Pin 5 as (inverted) mask
		XORWF(true,LATA.addr), //Set pin 5 low
		BTFSC(1,GRAM.addr+0), //Shift Register 1
		BRA(1), //if( set )
		BRA(3), //if( clr )
		MOVLW(0x80), //RA7 Pin
		IORWF(true,LATA.addr), //Set High
		BRA(2), //continue
		MOVLW(0x7f), //~RA7 Pin
		XORWF(true,LATA.addr), //Set low
		BTFSC(1,GRAM.addr+1), //Shift Register 2
		BRA(1), //if( set )
		BRA(3), //if( clr )
		MOVLW(0x40), //RA7 Pin
		IORWF(true,LATA.addr), //Set High
		BRA(2), //continue
		MOVLW(0xbf), //~RA7 Pin
		XORWF(true,LATA.addr), //Set low
		BTFSC(1,GRAM.addr+2), //Shift Register 3
		BRA(1), //if( set )
		BRA(3), //if( clr )
		MOVLW(0x01), //RC0 Pin
		IORWF(true,LATC.addr), //Set High
		BRA(2), //continue
		MOVLW(0xf7), //~RC0 Pin
		XORWF(true,LATC.addr), //Set low
		BTFSC(1,GRAM.addr+3), //Shift Register 4
		BRA(1), //if( set )
		BRA(3), //if( clr )
		MOVLW(0x02), //RC1 Pin
		IORWF(true,LATC.addr), //Set High
		BRA(2), //continue
		MOVLW(0xfd), //~RC1 Pin
		XORWF(true,LATC.addr), //Set low
		BTFSC(1,GRAM.addr+4), //Shift Register 5
		BRA(1), //if( set )
		BRA(3), //if( clr )
		MOVLW(0x04), //RC2 Pin
		IORWF(true,LATC.addr), //Set High
		BRA(2), //continue
		MOVLW(0xfb), //~RC2 Pin
		XORWF(true,LATC.addr), //Set low
		BTFSC(1,GRAM.addr+5), //Shift Register 6
		BRA(1), //if( set )
		BRA(3), //if( clr )
		MOVLW(0x08), //RC3 Pin
		IORWF(true,LATC.addr), //Set High
		BRA(2), //continue
		MOVLW(0xf7), //~RC3 Pin
		XORWF(true,LATC.addr), //Set low
		BTFSC(1,GRAM.addr+6), //Shift Register 7
		BRA(1), //if( set )
		BRA(3), //if( clr )
		MOVLW(0x10), //RC4 Pin
		IORWF(true,LATC.addr), //Set High
		BRA(2), //continue
		MOVLW(0xef), //~RC4 Pin
		XORWF(true,LATC.addr), //Set low
		BTFSC(1,GRAM.addr+7), //Shift Register 8
		BRA(1), //if( set )
		BRA(3), //if( clr )
		MOVLW(0x20), //RC5 Pin
		IORWF(true,LATC.addr), //Set High
		BRA(2), //continue
		MOVLW(0xdf), //~RC5 Pin
		XORWF(true,LATC.addr), //Set low
		BTFSC(1,GRAM.addr+8), //Shift register 9
		BRA(1), //if( set )
		BRA(3), //if( clr )
		MOVLW(0x40), //RC6 Pin
		IORWF(true,LATC.addr), //Set High
		BRA(2), //continue
		MOVLW(0xbf), //~RC6 Pin
		XORWF(true,LATC.addr), //Set low
		CALL(5), //DELAY LOWCLOCKTIME
		MOVLW(0x20), //Set Pin 5 as mask
		IORWF(true,LATA.addr), //Set pin 5 high
		CALL(5), //DELAY HIGHCLOCKTIME
		BRA(-79),
	};
#else
#endif


	Programmer pic;

	pic.addCMD(0x0000);
	pic.addCMD(BRA(13));
	pic.addCMD(0x0000);
	pic.addCMD(0x0000); //interrupt
	pic.addCMD(MOVLB(0), "init");
	pic.addCMD(MOVLW(0x01));
	pic.addCMD(MOVLW(0x01));
	pic.addCMD(MOVWF(TRISA.addr));
	pic.addCMD(MOVLW(0xff));
	pic.addCMD(MOVWF(TRISB.addr));
	pic.addCMD(MOVLW(0x00));
	pic.addCMD(MOVWF(TRISC.addr));
	pic.addCMD(MOVLW(0xaa));
	pic.addCMD(MOVWF(LATA.addr));
	pic.addCMD(RETURN());
	pic.addCMD(CALL(0), "init");

	pic.start(); //&Enter programming mode

	pic.setPC(0x8000); //according to table 3-2. This will erase all memory
	pic.bulkErase();
	//Write Program Memory
	//Verify Program Memory
	pic.uploadMain();
	//Write User IDs
	//Verify User Ids
	//Write Configuration words
	//verify configuration words
	
	pic.setPC(0x8000);
	for( int i = 0; i < 4; i++ ){
		pic.writeNVM( userid[i] );
		pic.beginIntProgramming();
		usleep( TPINT ); //probably redundant
		uint32_t ret = pic.readNVM();
		if( ret != userid[i] ){
			fprintf(stderr, "Failed to write userID\n");
		}
		fprintf(stdout, "Ret: %04x - uid: %04x\n", ret, userid[i]);
		pic.incPC();
	}


	fprintf(stdout, "\n");

	pic.setPC(0x8007);
	for( int i = 0; i < 5; i++ ){
		pic.writeNVM( config[i] );
		pic.beginIntProgramming();
		usleep( TPINT ); //probably redundant
		uint32_t ret = pic.readNVM();
		if( (ret & config[i]) != config[i] ){
			fprintf(stderr, "Failed to write configuration\n");
		}
		fprintf(stdout, "ret: %04x - conf: %04x\n", ret, config[i]);
		pic.incPC();
	}

	//STOP is automatically done in the constructor
	pic.stop();

	usleep(10000);
	pic.powerOn();

	return 1;
}
