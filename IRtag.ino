// Timer.ino

#define IR_FREQUENCY 		38000
#define IR_PULSE_LENGHT  	400 	// a much larger number can burn de IR LED

#define PIN_PPM			2 	// must be 2, using external interrupt
#define PIN_IR_SENSOR 		3 	// must be 3, using external interrupt
#define PIN_BEEP		4
#define PIN_LED_HITED		7
#define PIN_LED_FIRE		8
#define PIN_SERVO 		9
#define PIN_IR_LED 		11	// must be pin 11.
#define PIN_DEBUG_LED 		13

void setup() 
	{
	pinMode(PIN_IR_LED, OUTPUT);     
	pinMode(PIN_DEBUG_LED, OUTPUT);     
	pinMode(PIN_BEEP, OUTPUT);
	pinMode(PIN_IR_SENSOR, INPUT);
	pinMode(PIN_PPM, INPUT);
	Serial.begin(9600);
	Serial.println("\r\nStarting...\r\n");

	// configure timer2 to generate IR carrier frequency on pin 11
	TCCR2B = 0;							// disable timer 2 clock
    TCCR2A = 0;							// reset timer 2 parameters
    ASSR &= ~_BV(AS2);					// use internal clock for source
	OCR2A = F_CPU / IR_FREQUENCY / 2; 	// set the frequency
	TCCR2A = _BV(WGM21);				// timer2 CTC mode
	TCCR2B = 1;							// enable timer 2 clock without prescaler
	
	attachInterrupt(0, IntPPM, CHANGE);
	attachInterrupt(1, IntIR, RISING);
	}

volatile int ctbeep = 0, ctdebug = 0;	
unsigned long tnow, tlast = 0;

unsigned long irnow, irlast = 0;
unsigned long irtime, irref;
char irpos = -1;

boolean state;

volatile byte irdata, irparity;
boolean irvalid;

void IntIR()
	{
	irnow = micros();
	irtime = irnow - irlast;
	irlast = irnow;
	
	ctbeep = 20;	// debug beep
	
	if (irvalid) return;

	if (irtime > (IR_PULSE_LENGHT * 6)) { irpos = 0; irdata = 0; irparity = 0; return; } // start pulse

	if (irpos) // invalid pulse filter
		{
		if (irtime > (irref * 1.5)) irpos = -1;
		if (irtime < (irref / 1.5)) irpos = -1;
		}

	if (irpos == 9) // read parity bit
		{
		irvalid = 0;
		if ((irtime > irref) && irparity) irvalid = 1;
		if ((irtime < irref) && !irparity) irvalid = 1;
		irpos = -1;
		}

	if (irpos > 0) // read 8 data bits
		{ 
		irdata = irdata << 1; 
		if (irtime > irref) irdata = irdata | 1;
		irparity = irparity ^ (irdata & 1);		
		irpos++;
		}
	
	if (irpos == 0) { irref = irtime; irpos++; }
	}

void IntPPM()
	{
	//digitalWrite(PIN_DEBUG_LED, HIGH);
	//delay(100);
	//digitalWrite(PIN_DEBUG_LED, HIGH);
	}

void SendIRPulse() 
	{
	// first was tried to use tone function, but there is a problem
	// with tone library and delayMicroseconds function
	// than was decided to go with timer2 to generate de IR carrier frequency
	
	// tone(PIN_IR_LED, IR_FREQUENCY);
	// delayMicroseconds(IR_PULSE_LENGHT);
	// noTone(PIN_IR_LED);
	
	TCCR2A |= _BV(COM2A0);				// enable oscilator on pin 11
	delayMicroseconds(IR_PULSE_LENGHT);
	TCCR2A &= ~_BV(COM2A0);				// disable oscilator on pin 11
	}

void SendIRData(byte data)
	{
	byte i, parity, aux = data;

	// send start pulse (larger pulse to acomodate sensor AGC)
	TCCR2A |= _BV(COM2A0);
	delayMicroseconds(IR_PULSE_LENGHT * 2);
	TCCR2A &= ~_BV(COM2A0);
	delayMicroseconds(IR_PULSE_LENGHT * 3);

	// send IR_PACKET_LENGHT bits of data
	parity = 0;
	for (i=0; i<8; i++)
		{
		SendIRPulse();	
		if (aux & 128) delayMicroseconds(IR_PULSE_LENGHT * 4); 
			else delayMicroseconds(IR_PULSE_LENGHT * 2);
		parity = parity ^ (aux & 128);
		aux = aux << 1;
		}

	// send the parity bit
	SendIRPulse();
	if (parity) delayMicroseconds(IR_PULSE_LENGHT * 4); 
		else delayMicroseconds(IR_PULSE_LENGHT * 2);

	// send stop pulse
	SendIRPulse();
	}

void IRDecode()
	{

	}

byte ct;

void loop() 
	{
	// every 1ms things
	tnow = millis();
	if (tnow > tlast) 
		{ 
		tlast = tnow; 
		if (ctbeep) ctbeep--;   digitalWrite(PIN_BEEP, ctbeep); 
		if (ctdebug) ctdebug--; digitalWrite(PIN_DEBUG_LED, ctdebug); 
		}	

	if (irvalid) 
		{	
		Serial.println(irdata);
		irvalid = 0;
		ctdebug = 250;
		}
	
	/*
	digitalWrite(PIN_DEBUG_LED,	HIGH);
	SendIRData(ct);
	ct++;
	digitalWrite(PIN_DEBUG_LED,	LOW);
	delay(480);
	*/
	}

