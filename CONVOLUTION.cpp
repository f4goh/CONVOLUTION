/********************************************************************************************
 * CONVOLUTION Arduino library
 * Created 15/05/2015
 * Anthony LE CREN f4goh@orange.fr 
 * Modified 24/07/2015
 * fix bug data ptr upper than 32
 * BSD license, all text above must be included in any redistribution
 *
 * Instance :
 *
 * Functions :
 *
 *******************************************************************************************/
 

#include <CONVOLUTION.h>
#include <avr/pgmspace.h>


CONVOLUTION Convolve;


CONVOLUTION::CONVOLUTION(){

}

/********************************************************
 * convolve
 ********************************************************/

 
void CONVOLUTION::convolution(byte *input, byte *output) {      //one byte input, two bytes output
   byte reg =0;
   int n,m;
   byte bits;
   byte mask_in;
   byte bit_out;
   unsigned int enc;
  
   int ptr=0;                            //ptr in encoded table
   for (n =0; n < size_buffer; n ++) {
        mask_in=0x80;                        
        bit_out=15;                      //use for 16 bits bitwrite 
        for (m=0; m < 8; m ++) {
          bits= (input[n] & mask_in) ? 1 : 0;    //1 if > 1, 0 else
          mask_in>>=1;                      //shift mask to get each bit
          reg = (reg >>1) | bits <<2;      //convol reg
          bitWrite(enc, bit_out--, bitRead(reg, 0) ^ bitRead(reg, 1) ^ bitRead(reg, 2));      //msb poly 7
          bitWrite(enc, bit_out--, bitRead(reg, 0) ^ bitRead(reg, 2));                       //lsb poly 5
        }
        output[ptr++]=(enc>>8);            //one 16bits enc word is completed then store in 2 bytes
        output[ptr++]=(enc);

    }
}



/********************************************************
 * pseudo random generator
 ********************************************************/
//127 cyclic values can be generated
//E,F2,C9,2,26,2E,B6,C,D4,E7,B4,2A,FA,51,B8,FE,1D,E5,92,4,4C,5D,6C,19,A9,CF,68,55,F4,A3,71,FC,
//3B,CB,24,8,98,BA,D8,33,53,9E,D0,AB,E9,46,E3,F8,77,96,48,11,31,75,B0,66,A7,3D,A1,57,D2,8D,C7,
//F0,EF,2C,90,22,62,EB,60,CD,4E,7B,42,AF,A5,1B,8F,E1,DE,59,20,44,C5,D6,C1,9A,9C,F6,85,5F,4A,37,
//1F,C3,BC,B2,40,89,8B,AD,83,35,39,ED,A,BE,94,6E,3F,87,79,64,81,13,17,5B,6,6A,73,DA,15,7D,28,DC,7F,

void CONVOLUTION::pseudo_random(byte *array)
{
  byte reg=0xff;
  byte pseudo;
  int n,m;
 
    for (int n =0; n < size_buffer*2; n ++)
    {
      for (m=7;m>=0;m--)
            {
            bitWrite(reg,0,bitRead(reg,4)^bitRead(reg,7));
            bitWrite(pseudo,m,bitRead(reg,0));
            reg<<=1;
            }
        array[n]^=pseudo;
    }
}

//buffer need to be a multiple of 8 bytes

/********************************************************
 * interleaving
 ********************************************************/

void CONVOLUTION::interleave(byte *array)
{
  byte inter[8];
  byte b,n,m;
  for (int n =0; n < (size_buffer/4); n ++)
    {
      for (m=0;m<8;m++)  {
                          inter[m]=array[m+n*8];    //copy 8 bytes to buffer
                         }
      for (m=0;m<8;m++) {                        //and interleave 8 bytes
                for (b=0;b<8;b++) {
                                bitWrite(array[m+n*8],b,bitRead(inter[b],m));
                                  }
                        }
    }
}

/********************************************************
 * CRC calculator en check
 ********************************************************/

void CONVOLUTION::fcsbit(byte tbyte)
{
  crc ^= tbyte;
  if (crc & 1)
    crc = (crc >> 1) ^ 0x8408;  // X-modem CRC poly
  else
    crc = crc >> 1;
 
}

void CONVOLUTION::compute_crc(byte *array)
{
crc=0xffff;  
for (int n =0; n < (size_buffer-3); n ++)
    {
      for (int m=0;m<8;m++)  {                //each bit must be calculated separatly
                  CONVOLUTION::fcsbit(bitRead(array[n],m));
      }
    }
crc^=0xffff;
}

void CONVOLUTION::add_crc(byte *array)          //add crc to inupt array
{
CONVOLUTION::compute_crc(array);
array[size_buffer-3]=(crc>>8);
array[size_buffer-2]=(crc);
}

boolean CONVOLUTION::check_crc(byte *array)    //check decoded array if crc is good
{                                 //because viterbi decoding doesn't know if decoded data is good
CONVOLUTION::compute_crc(array);
unsigned int crc_decoded;
crc_decoded=(array[size_buffer-3]<<8)+array[size_buffer-2];
if(crc_decoded==crc) return true; else return false;
}

/********************************************************
 * Add noize for testing
 ********************************************************/

void CONVOLUTION::noise(byte *output,byte nb_noising,byte mode)    //add noise
{
    int n;
    int ptr;
    byte bits;
	randomSeed(analogRead(0));
if (mode==0)                                // noise(6,0) add 6 noise random bit 
  {
    for (n =0; n < nb_noising; n ++)
    {
    ptr=random(size_buffer*2);       
    bits=random(8); 
    bitWrite(output[ptr],bits,bitRead(output[ptr],bits)^1);
    }
  }
  else                                   // noise(x,1) add 1 noise random byte
     {
   for (n =0; n < nb_noising; n ++)
    {
      ptr=random(size_buffer*2);
      output[ptr]^=0xff;
    }
   }  
 }

 /********************************************************
 * THE Viterbi decoder
 ********************************************************/

void CONVOLUTION::viterbi(byte* output,byte* history,byte* decoded)                         //viterbi decode
{                                      //first compute error and history table, then find decoded data
int n,m;
byte dual;
byte mask;
static byte state_transitions_table[4][4]={{0,4,1,4},  //4 number as x (never used)
										   {0,4,1,4},
										   {4,0,4,1},
										   {4,0,4,1}};

   //compute error and history table
    for (n =0; n <2; n ++) for (m =0; m <4; m ++) acc_error[n][m]=0;   //clear acc_error_shift_table

   // building the accumulated error metric and state history table
   CONVOLUTION::compute_error_branch(0, 0, output[0]>>6,history);                           // ptr, current_state, encoded_input
   CONVOLUTION::compute_error_branch(1, 0, (output[0]>>4)&3,history);
   CONVOLUTION::compute_error_branch(1, 2, (output[0]>>4)&3,history);
   for (m =0; m <4; m ++) CONVOLUTION::compute_error_branch(2, m, (output[0]>>2)&3,history);
   for (m =0; m <4; m ++) CONVOLUTION::compute_error_branch(3, m, output[0]&3,history);
   
   for (n=4; n < (size_buffer*8) -1; n ++) for (m =0; m <4; m ++) {
                                                                   switch(n&3)      //select pair of bit in a byte
                                                                   {
                                                                   case 0 : dual=output[n/4]>>6;break;
                                                                   case 1 : dual=(output[n/4]>>4)&3;break;
                                                                   case 2 : dual=(output[n/4]>>2)&3;break;
                                                                   case 3 : dual=output[n/4]&3;break;
                                                                   }
                                                                   CONVOLUTION::compute_error_branch(n, m, dual,history);
                                                                   }
   //find decoded data
   // Traceback : Working backward through the state history and recreate the original message 
   byte current_state =0;
   byte predecessor_state;
   m=size_buffer-1;
     mask=1;
   for (n = (size_buffer*8) -1; n >0; n --) {   
      switch(current_state) {                    //found exact state in byte
         case 0 : predecessor_state = (history[n] &0x03);
         break;
         case 1 : predecessor_state = (history[n] &0x0c) >>2;
         break;
         case 2 : predecessor_state = (history[n] &0x30) >>4;
         break;
         case 3 : predecessor_state = (history[n] &0xc0) >>6;
         break;
      }
        bitWrite(decoded[m],mask,state_transitions_table[predecessor_state][current_state]);
        mask++;
       if (mask==8) {
                    mask=0;
                    m--;
                }
      current_state = predecessor_state;
   }
}

void CONVOLUTION::compute_error_branch(int ptr,byte current_state, byte encoded_input,byte* history)
{
  static byte transition[]={0,0,3,2, 3,0,0,2, 2,1,1,3, 1,1,2,3};    //table to find transition and next state for channel A and B
   byte encoder_channel_A = transition[current_state *4];
   byte next_state_A = transition[current_state *4 +1];
   byte encoder_channel_B = transition[current_state *4 +2];
   byte next_state_B = transition[current_state *4 +3];
   byte errorA = CONVOLUTION::hamming_distance(encoded_input, encoder_channel_A);
   byte errorB = CONVOLUTION::hamming_distance(encoded_input, encoder_channel_B);

    if (current_state ==0) {                                          //shift accumulated error matrix
      for (int n =0; n <4; n ++) acc_error[0][n] = acc_error[1][n];
   }

   if ((current_state &1) ==0) {                                      //if state=0 or 2 then compute error and history
      acc_error[1][next_state_A] = errorA + acc_error[0][current_state];
      acc_error[1][next_state_B] = errorB + acc_error[0][current_state];
      history[ptr +1] = set_position(history[ptr +1], next_state_A, current_state);
      history[ptr +1] = set_position(history[ptr +1], next_state_B, current_state);
   } else {                                                          //else (state=1 or 3) then swapp error and history if necessary
            if (acc_error[1][next_state_A] > (errorA + acc_error[0][current_state])) {
                                                                                   acc_error[1][next_state_A] = errorA + acc_error[0][current_state];
                                                                                   history[ptr +1] = CONVOLUTION::set_position(history[ptr +1], next_state_A, current_state);
                                                                                      }
            if (acc_error[1][next_state_B] > (errorB + acc_error[0][current_state])) {
                                                                                   acc_error[1][next_state_B] = errorB + acc_error[0][current_state];
                                                                                   history[ptr +1] = CONVOLUTION::set_position(history[ptr +1], next_state_B, current_state);
                                                                                  }
   }
}

byte CONVOLUTION::hamming_distance(byte encoder_channel,byte encoded_input)
{
  
static byte hamming_table[4][4]={{0,1,1,2},        //compute hamming distance by table (more easy)
								 {1,0,2,1},
								 {1,2,0,1},
								 {2,1,1,0}};

return hamming_table[encoder_channel][encoded_input];

//ou  
//return   (((encoder_channel>>1)-(encoded_input>>1)) ? 1 : 0) + (((encoder_channel&1)-(encoded_input&1)) ? 1 : 0);
}

byte CONVOLUTION::set_position(byte value, byte next_state, byte current_state) {    //set state position in byte with & and | mask
   switch(next_state) {
      case 0 : value = value &0xfc | current_state;
      break;
      case 1 : value = value &0xf3 | (current_state <<2);
      break;
      case 2 : value = value &0xcf | (current_state <<4);
      break;
      case 3 : value = value &0x3f | (current_state <<6);
      break;
   }
   return  value;
}

 /********************************************************
 * Print array routines
 ********************************************************/

void CONVOLUTION::print_data(byte * array) {
   for (int n =0; n < size_buffer; n ++) {
      Serial.print(array[n],HEX);
      Serial.print(",");
   }
   Serial.println();
}

void CONVOLUTION::print_data_bis(byte * array) {
   for (int n =0; n < size_buffer*2; n ++) {
      Serial.print(array[n],HEX);
      Serial.print(",");
   }
   Serial.println();
}
