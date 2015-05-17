/***************************************
Convolutional (Viterbi) Encoding and decoding
library testing
Started 03/05/2015 for GMSK TX 4800 bauds FM
f4goh@orange.fr

More Viterbi info :
http://users.ece.utexas.edu/~gerstl/ee382v-ics_f09/lectures/Viterbi.pdf

Rate 1/2
K=3
polynom 7,5


crc : 8A42
46,34,47,4F,48,8A,42,0,      //last byte always 0 , just before 16 bits crc
Data encoded :
3B,35,CD,4B,3B,36,4B,DA,4B,EC,EC,E2,FB,E,C0,0,
Data randomize :
35,C7,4,49,1D,18,FD,D6,9F,B,58,C8,1,5F,78,FE,
Data interleave :
5B,82,D7,78,F1,41,CA,C2,33,A3,A1,EF,E5,C0,EC,89,
Data with random noise:
5B,82,D7,78,0,41,CA,C2,33,A3,A1,EF,E5,C0,EC,89,    //5th byte cleared
Data deinterleave :
25,C7,4,49,D,8,ED,C6,9F,B,58,C8,1,5F,78,FE,
Data de-randomize :
2B,35,CD,4B,2B,26,5B,CA,4B,EC,EC,E2,FB,E,C0,0,
Nb errors :5
Data input :
46,34,47,4F,48,8A,42,0,
Data decoded :
46,34,47,4F,48,8A,42,0,
Check crc : 1
****************************************/

#include <CONVOLUTION.h>


#define size_input 8      //input size must be multiple of 8 bytes for interleaving

byte input[]={'F','4','G','O','H',0,0,0};    //original data

byte output[size_input*2];     //convolued,randmize and interleaved data  
byte history[size_input*8];    //buffer for viterbi decoding (large buffer need)
byte decoded[size_input];      //decoded data




void setup() {
   Serial.begin(57600);

   Convolve.size_buffer=size_input;    //need for coding and decoding
   
  //16 bits crc is added at the end of input array (size_input-3: msb , size_input-2: lsb)
  //so beware of that
   Convolve.add_crc(input);   

   Serial.print("crc : ");
   Serial.println(Convolve.crc,HEX);    
   
   Convolve.print_data(input);
   
   Convolve.convolution(input,output);    //convolve input to output
  
   Convolve.print_data_bis(output);
   
   Convolve.pseudo_random(output);        //randomize data
   Serial.println("Data randomize :");
   Convolve.print_data_bis(output);

   Convolve.interleave(output);          //interleave 8 bytes
   Serial.println("Data interleave :");
   Convolve.print_data_bis(output);

   Convolve.noise(output,1,1);            //nb bits to noize, mode 1 : by bit; mode 0 : by byte
   Serial.println("Data with random noise:");
   Convolve.print_data_bis(output);
   
   Convolve.interleave(output);            //use same for de-interleave
   Serial.println("Data deinterleave :");
   Convolve.print_data_bis(output);

   Convolve.pseudo_random(output);        //use same for de-randomize
   Serial.println("Data de-randomize :");
   Convolve.print_data_bis(output);

   Convolve.viterbi(output,history,decoded);  //decode data
   
   Serial.print("Nb errors :");
   Serial.println(Convolve.acc_error[1][0]);    //print nb errors

   Serial.println("Data input :");
   Convolve.print_data(input);          //to compare initial values
   Serial.println("Data decoded :");
   Convolve.print_data(decoded);
  
   Serial.print("Check crc : ");
   Serial.println(Convolve.check_crc(decoded));    //crc testing
   
}

void loop() {
}



