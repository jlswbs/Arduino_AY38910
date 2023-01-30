const byte ad[8] = { 8, 9, 2, 3, 4, 5, 6, 7 }; // connect to DA0,1,...,7
const byte pinBC1 = 10;
const byte pinCLK = 11; // OC2A output pin for ATmega328 boards
const byte pinBDIR = 12;
const byte pinRST = 13;

//Fast pin switching macros
#define __BCPORT__ PORTB
#define __BC1__ 2  // PORT PIN
#define __BDIR__ 4 // PORT PIN

void initFrequencyGenerator(){
  // Set Timer 2 CTC mode OC2A toggles on compare match
  TCCR2A = 0x42;
  TCCR2B = 0x01; // prescaller
  TIMSK2 = 0;
  // This value determines the output frequency: 0 - 16MHz, 1 - 8MHz, 2 - 4MHz, 3 - 2MHz, 4 - 1MHz
  OCR2A = 3;
}


void setup(){

  // INTERRUPT DISABLE
  cli();

  //init pins
  for(byte i=0; i < 8; i++) pinMode(ad[i], OUTPUT);

  pinMode(0, INPUT);

  pinMode(pinBC1, OUTPUT);
  pinMode(pinBDIR, OUTPUT);
  pinMode(pinRST, OUTPUT);   

  //inactive mode
  digitalWrite(pinBC1, LOW);
  digitalWrite(pinBDIR, LOW);

  pinMode(pinCLK, OUTPUT);
  initFrequencyGenerator();

  // reset sequence
  digitalWrite(pinRST, LOW);
  delay(10);
  digitalWrite(pinRST, HIGH);  

  //serial init at 57600
  UBRR0H = 0;
  UBRR0L = 0x10;
  UCSR0C = 0x06;
  UCSR0A = 0;
  UCSR0B = 0x90;

  // INTERRUPT ENABLE
  sei();
}

void send_data(byte address, byte data) {
// WRITE REGISTER NUMBER
  //write address to DA0-DA7 pins
  PORTB |= address & 0x03;
  PORTD |= address & 0xFC;
  //validate addess
  //set BC1+BDIR bits, latch address mode
  __BCPORT__ |= (1 << __BDIR__) + (1 << __BC1__);
  asm("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop"); //set+hold address delay 500ns (400+100 min)
  //clear BC1+BDIR bits, inactive mode
  __BCPORT__ &= ~((1 << __BDIR__) + (1 << __BC1__));
  // reset pins to tristate mode
  PORTB &= ~(address & 0x03);
  PORTD &= ~(address & 0xFC);

// WRITE REGISTER DATA
  //write data to pins
  PORTB |= data & 0x03;
  PORTD |= data & 0xFC;
  //validate data
  //set BDIR bit, write to reg mode
  __BCPORT__ |= ( 1 << __BDIR__); 
  asm("nop\nnop\nnop\nnop"); //250ns delay (250min-500max) nop=62.5ns on 16MHz
  //clear BDIR bit, inactive mode
  __BCPORT__ &= ~( 1 << __BDIR__); 
  // reset pins to tristate mode
  PORTB &= ~(data & 0x03);
  PORTD &= ~(data & 0xFC);
}

// SERIAL INTERRUPT HANDLER
bool reg = false;
byte reg_num = 0;
ISR(USART_RX_vect)
{
    byte r = UDR0;
    if (bit_is_clear(UCSR0A, FE0))
    {
        if(reg == false)
        {
          if(r <= 15)
          {
            reg_num = r;
            reg = true;
          }
        }
        else
        {
          send_data(reg_num, r);
          reg = false;
        }
    }
}


void loop() {
  // Write your code here :)
}