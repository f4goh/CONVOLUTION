#Convolutional (Viterbi) Encoding and decoding library#
F4GOH Anthony f4goh@orange.fr <br>

May 2015

Use this library freely with Arduino 1.0.6

## Installation ##
To use the CONVOLUTION library:  
- Go to https://github.com/f4goh/CONVOLUTION, click the [Download ZIP](https://github.com/f4goh/CONVOLUTION/archive/master.zip) button and save the ZIP file to a convenient location on your PC.
- Uncompress the downloaded file.  This will result in a folder containing all the files for the library, that has a name that includes the branch name, usually CONVOLUTION-master.
- Rename the folder to  CONVOLUTION.
- Copy the renamed folder to the Arduino sketchbook\libraries folder.



## Usage notes ##


To use the CONVOLUTION library, see /exemples/Convolution_test.ino.

CMX589 Modem Library  soon...

73

```c++
#include <CONVOLUTION.h>
#define size_input 8      //input size must be multiple of 8 bytes for interleaving

byte input[]={'F','4','G','O','H',0,0,0};    //original data

byte output[size_input*2];     //convolued,randmize and interleaved data  
byte history[size_input*8];    //buffer for viterbi decoding (large buffer need)
byte decoded[size_input];      //decoded data

```

here is the results :

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

Have fun with Viterbi
[Download PDF : Viterbi how to](http://users.ece.utexas.edu/~gerstl/ee382v-ics_f09/lectures/Viterbi.pdf)
To Understand pdf, take a look at the sample program downside without library

```c++
/***************************************
Test d'encodage par convolution
Rate 1/2
K=3
polynome 7,5

Test de decodage par viterbi

Prog Démarré le 03/05/2015 en prévision d'un implantation GMSK 4800 bauds FM
Terminé dans le 8/05/2015
d'après la doc
http://users.ece.utexas.edu/~gerstl/ee382v-ics_f09/lectures/Viterbi.pdf

Data input :
0,1,0,1,1,1,0,0,1,0,1,0,0,0,1,0,0,0,
Data encoded :
0,3,2,0,1,2,1,3,3,2,0,2,3,0,3,2,3,0,
Data encoded with noize:
0,3,3,0,1,2,1,3,3,2,0,0,3,0,3,2,3,0,
Accumulated error metric :
0,0,2,3,3,3,3,4,1,3,4,3,3,2,2,4,5,2,
0,0,3,1,2,2,3,1,4,4,1,4,2,3,4,4,2,5,
0,2,0,2,1,3,3,4,3,1,4,1,4,3,3,2,5,4,
0,0,3,1,2,1,1,3,4,4,3,4,2,3,4,4,4,5,
History state table :
0,0,0,1,0,1,1,0,1,0,0,1,0,1,0,0,0,1,
0,0,2,2,3,3,2,3,3,2,2,3,2,3,2,2,2,3,
0,0,0,0,1,1,1,0,1,0,0,1,1,0,1,0,0,1,
0,0,2,2,3,2,3,2,3,2,2,3,2,3,2,2,2,3,
Data input :
0,1,0,1,1,1,0,0,1,0,1,0,0,0,1,0,0,0,
Data decoded :
0,1,0,1,1,1,0,0,1,0,1,0,0,0,1,0,0,0,
****************************************/


#define size_input 18

byte input[]={0,1,0,1,1,1,0,0,1,0,1,0,0,0,1,0,0,0};
byte output[size_input];
byte decoded[size_input];
byte acc_error[size_input][4];
byte history[size_input][4];


void setup()
{
Serial.begin(57600);
int n,m;

Serial.println("Data input :");
print_data(input);

convolution();
Serial.println("Data encoded :");
print_data(output);
noise();
Serial.println("Data encoded with noize:");
print_data(output);

viterbi();

Serial.println("Accumulated error metric :");
print_matrix(acc_error);

Serial.println("History state table :");
print_matrix(history);

Serial.println("Data input :");
print_data(input);
Serial.println("Data decoded :");
print_data(decoded);
  
}

void loop() {
  
  
}

void convolution()
{
byte reg=0;
int n;
for (n=0;n<size_input;n++)
              {
               reg=(reg>>1)|input[n]<<2;
               bitWrite(output[n],1,bitRead(reg,0)^ bitRead(reg,1)^bitRead(reg,2));  //msb poly 7
               bitWrite(output[n],0,bitRead(reg,0)^ bitRead(reg,2));                 //lsb poly 5 (voir Galois hi)
              }
}


void noise()
{
output[2]=3;    
//output[3]=2;  
//output[7]=1; 
output[11]=0;
//output[15]=2;
}



void viterbi()
{
int n,m;
byte state_transitions_table[4][4]={{0,4,1,4},  //4 number as x (never used)
                                    {0,4,1,4},
                                    {4,0,4,1},
                                    {4,0,4,1}};

// building the accumulated error metric and state history table

compute_error_branch(0,0, output[0]); // ptr, current_state, encoded_input
compute_error_branch(1,0, output[1]);
compute_error_branch(1,2, output[1]);

for (n=2;n<size_input-1;n++)  for (m=0;m<4;m++) compute_error_branch(n,m, output[n]);

// Traceback : Working backward through the state history and recreate the original message 
byte current_state=0;
byte predecessor_state;
      
for (n=size_input-1;n>0;n--) {
      predecessor_state=history[n][current_state];
      decoded[n-1]=state_transitions_table[predecessor_state][current_state];
      current_state=predecessor_state;
}
 
}


void compute_error_branch(byte ptr,byte current_state, byte encoded_input)
{
byte transition[]={0,0,3,2, 3,0,0,2, 2,1,1,3, 1,1,2,3};
byte encoder_channel_A = transition[current_state*4];
byte next_state_A = transition[current_state*4+1];
byte encoder_channel_B = transition[current_state*4+2];
byte next_state_B = transition[current_state*4+3];

byte errorA=hamming_distance(encoded_input,encoder_channel_A);  
byte errorB=hamming_distance(encoded_input,encoder_channel_B); 
/*
Serial.println(encoder_channel_A);
Serial.println(next_state_A);
Serial.println(encoder_channel_B);
Serial.println(next_state_B);
Serial.println("traite");
Serial.println(errorA);
Serial.println(errorB);
Serial.println(acc_error[ptr][current_state]);
Serial.println("------------------");
*/

if ((current_state&1)==0) {
                     acc_error[ptr+1][next_state_A]=errorA+acc_error[ptr][current_state];
                     acc_error[ptr+1][next_state_B]=errorB+acc_error[ptr][current_state];
                     history[ptr+1][next_state_A]=current_state;
                     history[ptr+1][next_state_B]=current_state;
                     }
                     else
                     {
                     if (acc_error[ptr+1][next_state_A]>(errorA+acc_error[ptr][current_state])) {
                                                                                                   acc_error[ptr+1][next_state_A]=errorA+acc_error[ptr][current_state];
                                                                                                   history[ptr+1][next_state_A]=current_state;
                                                                                                }
                     if (acc_error[ptr+1][next_state_B]>(errorB+acc_error[ptr][current_state])) {
                                                                                                 acc_error[ptr+1][next_state_B]=errorB+acc_error[ptr][current_state];
                                                                                                  history[ptr+1][next_state_B]=current_state;
                                                                                                 }
                      }
                     
                    
}


byte hamming_distance(byte encoder_channel,byte encoded_input)
{
  
byte hamming_table[4][4]={{0,1,1,2},
                          {1,0,2,1},
                          {1,2,0,1},
                          {2,1,1,0}};

return hamming_table[encoder_channel][encoded_input];

//ou  
//return   (((a>>1)-(b>>1)) ? 1 : 0) + (((a&1)-(b&1)) ? 1 : 0);
}


void print_data(byte *array)
{
 for (int n=0;n<size_input;n++)
{
  Serial.print(array[n]);
  Serial.print(",");
}
Serial.println();
}

void print_matrix(byte array[][4])
{
int n,m;  
for (m=0;m<4;m++) {
      for (n=0;n<size_input;n++) {
                            Serial.print(array[n][m]);
                            Serial.print(',');
                  }
               Serial.println();
}
 
}
```




