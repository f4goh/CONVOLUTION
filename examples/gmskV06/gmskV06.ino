/*
GMSK for Arduino message text V0.5

Compatible avec le protocole V1.0

test de resolution du probmème d'intervale asynchrone sur la laison série

ajout du car S vers la tablette en fin d'émission de trame (mais pas de retour d'info sur une tablette tactile secondaire)

ajout du delay de décodage en réception 40ms (36ms mini)
et test du retour de la trame complete au lieu du char S 

*/

#include <CONVOLUTION.h>
//#include <EEPROM.h>



#define pinRXCLK 2    //input
#define pinTXCLK 3    //input
#define pinSN 4       //input *
#define pinRXD 5      //input *
#define pinTXD 6      //output *
#define pinCOS 7      //input *
#define pinPTT 8      //output *
#define pinPLLACQ A0   //output *
//10 cs sdcard
//11 mosi
//12 miso
//13 sck
#define pinRXDCACQ 9    //output
#define pinCOSLED A1    //output *
#define pinDebugT A2     //output led1 emission de la trame gmsk
#define pinDebugR A3     //output led3 réception de la trame gmsk

#define bitSync 0xcc            //bitsync format and 16 bits SyncFrame
#define SyncFrameMsb 0xec
#define SyncframeLsb 0xa3

byte invert;      //enable to change TX/RX  level, according to TRX model

//char callsign[]={'F','4','G','O','H',':'};


byte headerRx[6];
byte headerTx[]={bitSync,bitSync,bitSync,bitSync,bitSync,bitSync,bitSync,bitSync,SyncFrameMsb,SyncframeLsb};

#define BUFSIZE 48          //size of primary buffer
#define DATABUFSIZE 48-3          //size of data buffer


byte bufferTxRx[BUFSIZE];      //data to encode or decoded data

byte bufferConv[BUFSIZE*2];      //buffer RX and convolued,randomize and interleaved data 
byte history[BUFSIZE*8];    //buffer for viterbi decoding (large buffer need)


int ptrTXRaw=0;
byte car;
byte pttMode=0; //0 = normal  , 1 manuel
byte pttState=0; //0 = rx  , 1 tx
byte commandState=0;

volatile byte stateRxSync=0;
volatile byte stateTxSync=0;

volatile int bitCount=0;
volatile int byteCount=0;
volatile int nBuffer=0;
volatile int TXenable=0;
volatile int count=0;
volatile int done=0;



void setup(void)
{
 Serial.begin(115200);
//  Serial.flush();
// Serial.print("Free Ram: ");
// Serial.println(freeRam(),DEC);

// set all the pin modes and initial states
  pinMode(pinRXD, INPUT);    
  pinMode(pinTXD, OUTPUT);    
  pinMode(pinPTT, OUTPUT);    
  pinMode(pinPLLACQ, OUTPUT);
  pinMode(pinRXDCACQ, OUTPUT);
  pinMode(pinCOSLED, OUTPUT);
  pinMode(pinDebugT, OUTPUT);
  pinMode(pinDebugR, OUTPUT);  
  digitalWrite(pinDebugT, LOW);
  digitalWrite(pinDebugR, LOW);
  
  digitalWrite(pinPTT, LOW);
  digitalWrite(pinCOSLED, LOW);
  
  digitalWrite(pinPLLACQ, HIGH);
  digitalWrite(pinRXDCACQ, HIGH);

 Convolve.size_buffer=BUFSIZE;    //need for coding and decoding
 
 //gestion de la commande invert

invert=0x80;

/* 

invert=EEPROM.read(0);
if (invert&0x80==0) {            //defaut config no inversion (yaesu)
                    invert=0x80;
                    EEPROM.write(0,invert);                    
                    }

*/

// initialize the tx and rx interrupts
  attachInterrupt(0, rxINT, FALLING);
//  attachInterrupt(1, txINT, RISING);
stateRxSync=0;  //rx header priority

}

int freeRam () {                        // Count free ram, thanks to KI6ZUM
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}  

//T23456789012345678901234567890123456789012345

void loop(void)
{
 if (Serial.available()>0){
                           car=Serial.read();
                           //Serial.print((char)car);
                           if (ptrTXRaw==0)  {commandState=InternalCommand(car);
                                              //Serial.print(commandState);
                                             }
                           if(commandState==0) {
                                               //Serial.println(ptrTXRaw);
                                               bufferTxRx[ptrTXRaw]=car;
                                               ptrTXRaw++;
                                               digitalWrite(pinDebugT, HIGH);
                                              }

                    //fonction test commande (pour le web H) on verra ça plus tard
                           if (ptrTXRaw==DATABUFSIZE) {      //si le buffer est plein
                                                              //Serial.println("/");
                                                              //Serial.print("buffer full");
                                                              convol();                       //convolution here
                                                              if (pttMode==0) Txmode(1);                      //change irq to TX
                                                              txingData(0,sizeof(headerTx));   //send header
                                                              txingData(1,sizeof(bufferConv));   // send data
                                                              if (pttMode==0) Txmode(0);                      //change irq to RX
                                                              ptrTXRaw=0;                     //set ptr to 1 for next sentence
                                                              digitalWrite(pinDebugT, LOW);
                                                              delay(40);                        //tempo de décodage
                                                              print_data(bufferTxRx,DATABUFSIZE,2);              // retour d'ACQ
                                                              //Serial.write(83);    //car S ok pour une autre trame
                                                              //Serial.println("done");
                                                              //bitCount=0;
                                                              //byteCount=0;
                                                              }
                                                      
                          }
if (done==1){
     decode();     //decode viterbi here   
     //print_data(bufferConv,sizeof(bufferConv),0); 
     if (Convolve.check_crc(bufferTxRx)==1)  {
                                             //Serial.print("RX:");
                                             print_data(bufferTxRx,DATABUFSIZE,2);    //n'affiche pas le nb d'octets
                                             }
                                           else
                                           {
                                           Serial.print("V");
                                           Serial.write(Convolve.acc_error[1][0]);    //print nb errors    
                                           print_data(bufferTxRx,DATABUFSIZE-2,2);    //a revoir
                                           }
     done=0;
     digitalWrite(pinDebugR, LOW);
     }
}

byte InternalCommand(byte cmd)
{
if ((cmd&0x80)==0x80)
{
 if (cmd!=0xF0)
 { 
invert=cmd&0x03;
pttMode=(cmd>>3)&1;
pttState=(cmd>>2)&1;
if (pttMode==1) Txmode(pttState);
/*
Serial.println("-------cmd");
Serial.println(cmd,HEX);
Serial.println(invert,HEX);
Serial.println(pttMode,HEX);
Serial.println(pttState,HEX);
Serial.println("-----------");
*/
 }
}
return (cmd>>7);
}


void convol()
{
   Convolve.add_crc(bufferTxRx); 
/*   
   Serial.print("crc : ");
   Serial.println(Convolve.crc,HEX);    
   
   Convolve.print_data(bufferTxRx);
*/   
   Convolve.convolution(bufferTxRx,bufferConv);    //convolve input to output
  
//   Convolve.print_data_bis(bufferConv);
   
   Convolve.pseudo_random(bufferConv);        //randomize data
/*   Serial.println("Data randomize :");
   Convolve.print_data_bis(bufferConv);
*/
   Convolve.interleave(bufferConv);          //interleave 8 bytes
/*   Serial.println("Data interleave :");
   Convolve.print_data_bis(bufferConv);  */
}

void decode()
{
   Convolve.interleave(bufferConv);            //use same for de-interleave
/*   Serial.println("Data deinterleave :");
   Convolve.print_data_bis(bufferConv);*/

   Convolve.pseudo_random(bufferConv);        //use same for de-randomize
/*   Serial.println("Data de-randomize :");
   Convolve.print_data_bis(bufferConv);*/

   Convolve.viterbi(bufferConv,history,bufferTxRx);  //decode data
   
/*   Serial.print("Nb errors :");
   Serial.println(Convolve.acc_error[1][0]);    //print nb errors
     Convolve.print_data(bufferTxRx);
  
   Serial.print("Check crc : ");
   Serial.println(Convolve.check_crc(bufferTxRx));    //crc testing
   */
}

//irq for rxing data
void rxINT()
{

switch (stateRxSync)
{
case 0 : 
     if ((invert&0x01)==0x01)
     LshiftBit(headerRx,sizeof(headerRx),(~digitalRead(pinRXD))&1);     // save the received bit in bit Lshift buffer inverted
     else
      LshiftBit(headerRx,sizeof(headerRx),digitalRead(pinRXD));        // save the received bit in bit Lshift buffer non inverted
     
     break;
case 1 : 
     if ((invert&0x01)==0x01)
     LshiftByte(bufferConv,sizeof(bufferConv),(~digitalRead(pinRXD))&1);        // save the received bit in byte Lshift buffer inverted
     else
     LshiftByte(bufferConv,sizeof(bufferConv),digitalRead(pinRXD));        // save the received bit in byte Lshift buffer non inverted
     
     break;
}
/*
    // debug
    Serial.print(digitalRead(pinRXD));
    if (count == 64) {Serial.println("");count=0;}
    count++;
  */ 

   
} // end rxINT


//irq for txing data
void txINT()
{
if (TXenable==0) return;

    // only do something if the counter is still within the buffer space
    if (byteCount < BUFSIZE*2) {
      
      switch (nBuffer)
      {
        case 0 : 
                 if ((invert&0x02)==0x02)
                 digitalWrite(pinTXD, (headerTx[byteCount] >> (7-(bitCount)))&0x1 ? LOW:HIGH); //tx header inverted
                 else
                 digitalWrite(pinTXD, (headerTx[byteCount] >> (7-(bitCount)))&0x1 ? HIGH:LOW); //tx header non inverted
                 break;
        case 1 : 
                 if ((invert&0x02)==0x02)
                 digitalWrite(pinTXD, (bufferConv[byteCount] >> (7-(bitCount)))&0x1 ? LOW:HIGH); //tx buffer inverted
                 else
                 digitalWrite(pinTXD, (bufferConv[byteCount] >> (7-(bitCount)))&0x1 ? HIGH:LOW); //tx buffer non inverted
                 
                 break;
      }
      bitCount++;
      if (bitCount == 8) {
        bitCount = 0;
        byteCount++; 
      }
    }
    
}

//change mode  1 :tx, 0:rx
void Txmode(byte mode)   
{
if (mode==1){            // 1 : tx 
   //Serial.println("tx on");
    detachInterrupt(0);
   //ajoute la commande ptt on ici 
   digitalWrite(pinPTT, HIGH);
   delay(500);
    attachInterrupt(1, txINT, RISING); 
   // TXenable=1;
  }
  else
  {
  TXenable=0;            // 0 : rx
  //ajoute la commande ptt off ici 
  delay(100);
  digitalWrite(pinPTT, LOW);
  //Serial.println("tx off");
  detachInterrupt(1); 
  attachInterrupt(0, rxINT, FALLING);
  bitCount=0;
  byteCount=0;
  }

}

// wait txing data is complete
void txingData(byte nbuf, int dataSize)
{
TXenable=1;
nBuffer=nbuf;
bitCount=0;
byteCount=0;
while(byteCount < dataSize) {};    //wait txing complete;
TXenable=0;
}


// left bit shift into header rx and check frame synchronization 
void LshiftBit(byte *data,int dataSize,byte bitArrived)  //msb 1st
{
byte bitToshift;
int n;
for (n=0;n<dataSize-1;n++)
{
 bitToshift = (data[n+1] & 0x80) ? 1 : 0;
 data[n]<<=1;
 data[n]|=bitToshift;
}
data[dataSize-1]<<=1;
data[dataSize-1]|=bitArrived;
if (bitSynchro(data,dataSize)==true) {
                                     stateRxSync=1;                //sync & Frame detected
                                     digitalWrite(pinPLLACQ, LOW);
                                     //Serial.print("S");            //debug
                                     digitalWrite(pinDebugR, HIGH); 
                                     bitCount=0;
                                     byteCount=0;
                                     }
}
                                  
// compare frame sequence headertx with headerrx
boolean bitSynchro(byte *data,int dataSize)
{
int n;
boolean result=true;
for (n=0;n<dataSize;n++)
{
if (headerRx[n]!=headerTx[n+4]) result=false;    //utilisation de data* alors ?  
}
return result;  
}

// left bytes shift into rx buffer until buffer full 
void LshiftByte(byte *data,int dataSize,byte bitArrived)
{

bitWrite(data[byteCount],7-bitCount,bitArrived);  //msb 1st
bitCount++;
if (bitCount==8) {
                  //Serial.print("i");    //debug
                  bitCount=0;
                  byteCount++;
                  if (byteCount>dataSize) {
                 
                  //Serial.print("d");    //debug
                   digitalWrite(pinPLLACQ, HIGH); 
                   done=1;
                    stateRxSync=0;                       //all byte saved rx done return to sync next frame
                  }
                  }
}


//print char array in hex, char or raw
void print_data(byte *data, int dataSize,byte mode)
{
 int n;
for (n=0;n<dataSize;n++)
{
switch(mode)
{
case 0:  Serial.print(data[n],HEX); 
         Serial.print(" ");
         break;
case 1:  Serial.print((char) data[n]);
         break;
case 2:  Serial.write(data[n]);
         break;
}         
}
if (mode&2==0) Serial.println(); 
}



