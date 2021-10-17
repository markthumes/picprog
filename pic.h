#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <inttypes.h>

#ifndef __PIC_HEADER__
#define __PIC_HEADER__

#define BCM2708_PERI_BASE 0xfe000000
#define GPIO_BASE (BCM2708_PERI_BASE + 0x200000)

#define PAGE_SIZE  (4*1024)
#define BLOCK_SIZE (4*1024)

volatile unsigned *gpio;
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define GPIO_SET *(gpio+7)
#define GPIO_CLR *(gpio+10)
#define GET_GPIO(g) (*(gpio+13)&(1<<g))
#define GPIO_PULL *(gpio+37)


class GPIO{
public:
	GPIO(){
		m_init = false;
	}
	void init(){
		if( m_init ) return;
		m_init = true;
		if( (m_fdmem = open("/dev/mem", O_RDWR|O_SYNC)) < 0 ){
			fprintf(stdout, "file io\n");
			exit(EXIT_FAILURE);
		}
		gpio_map = mmap(
				NULL,
				BLOCK_SIZE,
				PROT_READ|PROT_WRITE,
				MAP_SHARED,
				m_fdmem,
				GPIO_BASE
			       );
		close( m_fdmem );
		if( gpio_map == MAP_FAILED ){
			fprintf(stderr, "MAJORFAILURE\n");
			exit( EXIT_FAILURE );
		}
		gpio = (volatile unsigned*)gpio_map;
	}
private:
	bool m_init;
	int m_fdmem;
	void* gpio_map;
};



class Pin{
public:
	typedef enum{
		VHIGH = 1, VLOW = 0
	}Voltage;
	typedef enum{
		INPUT = 1, OUTPUT = 0
	}Direction;
	Pin(int gpioPinNumber):m_pin(gpioPinNumber){
		pinout.init();
		setDirection(Direction::OUTPUT);
	}
	Pin(){
		pinout.init();
		m_pin = 0;
	}
	Pin( const Pin& p ){
		pinout = p.pinout;
		m_pin = p.m_pin;
	}
	void setDirection(Direction d){
		INP_GPIO(m_pin);
		if( d == OUTPUT ) OUT_GPIO(m_pin);
	}
	void setVoltage( Voltage v ){
		if( v == VHIGH ){
			GPIO_SET = 1 << m_pin;
		}
		else{
			GPIO_CLR = 1 << m_pin;
		}
	}
	Voltage readVoltage(){
		GPIO_PULL = 0;
		int i = GET_GPIO(m_pin);
		if( i > 0 ) return Voltage::VHIGH;
		return Voltage::VLOW;
	}
	~Pin(){
	}
	static GPIO pinout;
private:
	int m_pin;
};
GPIO Pin::pinout;


#endif
