#include <htc.h>
#include <math.h>

//----------------------------------------------------------------------------//

#define uint32 unsigned long
#define uint16 unsigned int
#define uint8 unsigned char

//----------------------------------------------------------------------------//

/* Program device configuration word
 * Oscillator = HS
 * Watchdog Timer = Off
 * Power Up Timer = Off
 * Master Clear Enable = External
 * Code Protect = Off
 * Data EE Read Protect = Off
 * Brown Out Detect = BOD and SBOREN disabled
 * Internal External Switch Over Mode = disabled
 * Monitor Clock Fail-safe = disabled
 */
__CONFIG(HS & WDTDIS & PWRTDIS & MCLREN & UNPROTECT & UNPROTECT & BORDIS & IESODIS & FCMDIS);

//----------------------------------------------------------------------------//

volatile uint32 fRandomSeed;

//----------------------------------------------------------------------------//

#define PIN_LED1 0
#define PIN_LED2 1
#define FRAME_DELAY 2930

volatile uint8 fIoBuffer;

volatile uint16 fFrameCounter;

volatile uint8 fPwmLedCounter1;
volatile uint8 fPwmLedCounter2;
volatile uint8 fPwmLedValue1;
volatile uint8 fPwmLedValue2;

volatile uint8 fPwmHi;
volatile uint8 fPwmLo;

volatile uint8 fBrightness;
volatile bit fCanIncrement;

//----------------------------------------------------------------------------//

void srand(void)
{
	fRandomSeed = TMR0 ^ TMR1H ^ TMR1L;
}

uint8 randomByte(void)
{
	fRandomSeed = fRandomSeed * 1103515245L + 12345;
	return fRandomSeed >> 16;
}

//----------------------------------------------------------------------------//

void updateIoBuffer(void)
{
	GPIO = GPIO & 0b11111100 | fIoBuffer;
}

void setIoPinHigh(uint8 aPinNumber)
{
	fIoBuffer |= (1 << aPinNumber);
}

void setIoPinLow(uint8 aPinNumber)
{
	fIoBuffer &= ~ (1 << aPinNumber);
}

//----------------------------------------------------------------------------//

uint8 randPwmValue(void)
{
	return fPwmLo + randomByte() % (fPwmHi - fPwmLo);
}

void calcEdges(void)
{
	fPwmLo = (uint8)(0.1953125 * (float)fBrightness);
	fPwmHi = (uint8)(0.8828125 * (float)fBrightness + 30);
}

void incBrightness(void)
{
	if(fBrightness < 255)
	{
		fBrightness++;
		calcEdges();
	}
}

void decBrightness(void)
{
	if(fBrightness > 0)
	{
		fBrightness--;
		calcEdges();
	}
}

//----------------------------------------------------------------------------//

void interrupt isr(void)
{

	// Main timer interrupt
	if((T0IE)&&(T0IF))
	{
		// Software PWM Realisation
		fPwmLedCounter1++;
		fPwmLedCounter2++;
		
		if(fPwmLedValue1 > fPwmLedCounter1)
			setIoPinHigh(PIN_LED1);
		else
			setIoPinLow(PIN_LED1);
			
		if(fPwmLedValue2 > fPwmLedCounter2)
			setIoPinHigh(PIN_LED2);
		else
			setIoPinLow(PIN_LED2);
		
		updateIoBuffer();
		
		// Frames realisation
		fFrameCounter++;
		if(fFrameCounter >= FRAME_DELAY)
		{
			// New Led PWM value on each frame
			fPwmLedValue1 = randPwmValue();
			fPwmLedValue2 = randPwmValue();

			// Decremement brightness every frame
			if(!fCanIncrement)
				decBrightness();

			fFrameCounter = 0;
		}
	}
		
	// Incremental timer
	if((TMR1IE)&&(TMR1IF))
	{
		if(fCanIncrement)
			incBrightness();
			
		TMR1IF = 0;
	}
	
	// GPIO2 State change interrupt
	if((GPIE)&&(GPIF))
	{
		// If GPIO2 is high
		if(GPIO & 0b00000100)
		{
			// Increment brightness
			fCanIncrement = 1;
			incBrightness();
			
			// Restart incremental timer;
			TMR1H = 0;
			TMR1L = 0;
		}
		else
			fCanIncrement = 0;
		
		GPIF = 0;
	}
}

//----------------------------------------------------------------------------//

void initHardware(void)
{
	// Timer 1 interrupt enabled.
	PIE1	= 0b00000001;
	
	// Timer 0 interrupt enabled.
  // Peripheral interrupts enabled
	INTCON	= 0b01101000;
	
	// Internal oscillator set to 8MHz
	OSCCON	= 0b01110000;
	
	// Port directions: 1=input, 0=output
	TRISIO	= 0b00111100;
	
	// Timer 0 is prescaled by 1:2
	OPTION	= 0b00000000;
	
	// Timer 1 is on, prescaler is 1:8
	T1CON	= 0b00000001;
	
	// Disable comparators
	CMCON1	= 0x07;
	CMCON0 = 0x07;
	CCP1CON = 0x00;
	
	// Disable ADC & Analog inputs
	ADCON0 = 0x00;
	ANSEL = 0x00;
	
	//Interrupt on change for GPIO
	IOC = 0b00000100;
}

//----------------------------------------------------------------------------//

void initDevice(void)
{
	// Init randomizer
	srand();
	
	// Main timer Frames counter
	fFrameCounter = 0;

	// PWM Counters to null
	fPwmLedCounter1 = 0;
	fPwmLedCounter2 = 0;

	// Inital PWM Values
	fPwmLedValue1 = 1;
	fPwmLedValue2 = 1;

	// Default brightness values
	fBrightness = 0;
	fCanIncrement = 0;
	
	// Recalc PWM Edges
	calcEdges();
}

//----------------------------------------------------------------------------//

int main(void)
{
	// Init device
	initDevice();
	initHardware();
	
	// Start interrupts
	ei();

	for(;;)
		continue;
}