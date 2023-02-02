// Logistic equation music //

#define MAXTEMPO  14  // 350 BPM 16th note
#define MINTEMPO  111 // 45 BPM 16th note
#define POLY      6   // note polyphony
  
  float r = 3.7f;
  float x = 0.1f;
  uint8_t notes[POLY];

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

/* Registers */
enum{
  REG_FREQ_A_LO = 0,
  REG_FREQ_A_HI,
  REG_FREQ_B_LO,
  REG_FREQ_B_HI,
  REG_FREQ_C_LO,
  REG_FREQ_C_HI,
  REG_FREQ_NOISE,
  REG_IO_MIXER,
  REG_LVL_A,
  REG_LVL_B,
  REG_LVL_C,
  REG_FREQ_ENV_LO,
  REG_FREQ_ENV_HI,
  REG_ENV_SHAPE,
  REG_IOA,
  REG_IOB
};

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

uint16_t note[] = {//MIDI note number
  15289, 14431, 13621, 12856, 12135, 11454, 10811, 10204,//0-o7
  9631, 9091, 8581, 8099, 7645, 7215, 6810, 6428,//8-15
  6067, 5727, 5405, 5102, 4816, 4545, 4290, 4050,//16-23
  3822, 3608, 3405, 3214, 3034, 2863, 2703, 2551,//24-31
  2408, 2273, 2145, 2025, 1911, 1804, 1703, 1607,//32-39
  1517, 1432, 1351, 1276, 1204, 1136, 1073, 1012,//40-47
  956, 902, 851, 804, 758, 716, 676, 638,//48-55
  602, 568, 536, 506, 478, 451, 426, 402,//56-63
  379, 358, 338, 319, 301, 284, 268, 253,//64-71
  239, 225, 213, 201, 190, 179, 169, 159,//72-79
  150, 142, 134, 127, 119, 113, 106, 100,//80-87
  95, 89, 84, 80, 75, 71, 67, 63,//88-95
  60, 56, 53, 50, 47, 45, 42, 40,//96-103
  38, 36, 34, 32, 30, 28, 27, 25,//104-111
  24, 22, 21, 20, 19, 18, 17, 16,//112-119
  15, 14, 13, 13, 12, 11, 11, 10,//120-127
  0//off
};

void note_chan_A(uint8_t i){
  send_data(REG_FREQ_A_LO, note[i]&0xff);
  send_data(REG_FREQ_A_HI, (note[i] >> 8)&0x0f);    
}

void note_chan_B(uint8_t i){
  send_data(REG_FREQ_B_LO, note[i]&0xff);
  send_data(REG_FREQ_B_HI, (note[i] >> 8)&0x0f);
}

void note_chan_C(uint8_t i){
  send_data(REG_FREQ_C_LO, note[i]&0xff);
  send_data(REG_FREQ_C_HI, (note[i] >> 8)&0x0f);
}

void envelope(uint16_t freq){       
  send_data(REG_FREQ_ENV_LO, freq & 0xff);
  send_data(REG_FREQ_ENV_HI, (freq >> 8)& 0xff);  
}

void set_env( bool hold, bool alternate, bool attack, bool cont){
  send_data(REG_ENV_SHAPE, (hold == true ? 0 : 1)|(alternate == true? 0 : 2)|(attack == true ? 0 : 4)|(cont == true ? 0 : 8));
}

void amp_chan_A(uint8_t ampl, bool envset){
  send_data(REG_LVL_A, (ampl & 0xf)|(envset != true ? 0 : B00010000));
}

void amp_chan_B(uint8_t ampl, bool envset){
  send_data(REG_LVL_B, (ampl & 0xf)|(envset != true ? 0 : B00010000));
}

void amp_chan_C(uint8_t ampl, bool envset){
  send_data(REG_LVL_C, (ampl & 0xf)|(envset != true ? 0 : B00010000));
}

void noise(uint8_t freq){
  send_data(REG_FREQ_NOISE, freq&0x1f);
}

void set_mix(bool tone_A, bool tone_B, bool tone_C, bool noise_A, bool noise_B, bool noise_C){
   send_data(REG_IO_MIXER, B11000000|(noise_C == true ? 0 : B00100000)|(noise_B == true? 0 : B00010000)|(noise_A == true ? 0 : B00001000)|(tone_C == true ? 0 : B00000100)|(tone_B == true ? 0 : B00000010)|(tone_A == true ? 0 : B00000001));
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

  // INTERRUPT ENABLE
  sei();

  // volume 0 - mute chan. A.B.C 
  send_data(REG_LVL_A, B00000000);
  send_data(REG_LVL_B, B00000000);
  send_data(REG_LVL_C, B00000000);

}

void loop(){

  for (int i = 0; i < POLY; i++){
  
    float nx = x;
    x = r * nx * (1.0f - nx);          
    notes[i] = 127.0f * x;
    
  }
  
  uint8_t notea = map(notes[0], 0, 127, 12, 72); // quantise note C1-c4
  uint8_t noteb = map(notes[1], 0, 127, 12, 72); // quantise note C1-c4
  uint8_t notec = map(notes[2], 0, 127, 12, 72); // quantise note C1-c4

  uint8_t vola = map(notes[3], 0, 127, 1, 13);
  uint8_t volb = map(notes[4], 0, 127, 1, 13);
  uint8_t volc = map(notes[5], 0, 127, 1, 13);

  set_mix(1, 1, 1, 0, 0, 1); // toneA, toneB, toneC, noiseA, noiseB, noiseC 0-1

  amp_chan_A(vola, 0); // 0-15
  amp_chan_B(volb, 0); // 0-15
  amp_chan_C(volc, 1); // 0-15

  set_env(0, 0, 1, 1); // hold, alter, attack, cont 0-1
  envelope(rand()%5000); // 0-65535

  noise(rand()%32); // 0-31
  
  note_chan_A(notea); // note 0-127
  note_chan_B(noteb); // note 0-127
  note_chan_C(notec); // note 0-127    

  uint8_t delay_ms = MINTEMPO;
  delay(2*delay_ms);

}