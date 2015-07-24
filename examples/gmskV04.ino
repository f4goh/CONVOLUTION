/*
GMSK for Arduino message text V0.4

TX and RX message with convolution library

manque la commande ptt

et expérimentations

sentence format:
1 octet             ,  6 octets ,  32-1-6-3=22, 2octets , 1 octet
nb octets call+data ,   call :  ,     datas   ,      crc, octet null 0 

prochaine version adaptation avec une tablette et module wifi


*/

#include <CONVOLUTION.h>



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
#define pinDebug A2     //output led1
#define led2 A3     //output led3

#define bitSync 0xcc            //bitsync format and 16 bits SyncFrame
#define SyncFrameMsb 0xec
#define SyncframeLsb 0xa3

//#define INVERTTX //enable to change TX level, according to TX model
//#define INVERTRX //enable to change RX level, according to RX model 

char callsign[]={'F','4','G','O','H',':'};


byte headerRx[6];
byte headerTx[]={bitSync,bitSync,bitSync,bitSync,bitSync,bitSync,bitSync,bitSync,SyncFrameMsb,SyncframeLsb};

#define BUFSIZE 48          //size of primary buffer

byte bufferTxRx[BUFSIZE];      //data to encode or decoded data

byte bufferConv[BUFSIZE*2];      //buffer RX and convolued,randomize and interleaved data 
byte history[BUFSIZE*8];    //buffer for viterbi decoding (large buffer need)


int ptrTXRaw=1;
byte car;


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
  Serial.flush();
 Serial.print("Free Ram: ");
 Serial.println(freeRam(),DEC);

// set all the pin modes and initial states
  pinMode(pinRXD, INPUT);    
  pinMode(pinTXD, OUTPUT);    
  pinMode(pinPTT, OUTPUT);    
  pinMode(pinPLLACQ, OUTPUT);
  pinMode(pinRXDCACQ, OUTPUT);
  pinMode(pinCOSLED, OUTPUT);
  digitalWrite(pinPTT, LOW);
  digitalWrite(pinCOSLED, LOW);
  
  digitalWrite(pinPLLACQ, HIGH);
  digitalWrite(pinRXDCACQ, HIGH);

 Convolve.size_buffer=BUFSIZE;    //need for coding and decoding
 
  
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


void loop(void)
{
 if (Serial.available()>0){
                           car=Serial.read();
                           //Serial.print((char) car);
                           if ((ptrTXRaw==sizeof(bufferTxRx)-sizeof(callsign)-3)||(car==0x0a)) {      //moins 6 pour call moins 3 pour crc dernier octet a 0
                                                              //Serial.println("/");
                                                              Serial.print("TX:");
                                                              // insert call
                                                              for(int n=0;n<sizeof(callsign);n++) bufferTxRx[n+1]=callsign[n];
                                                              bufferTxRx[0]=ptrTXRaw+sizeof(callsign);  //nb real used data into buffer
                                                              //print_data(bufferTxRx,sizeof(bufferTxRx),0);
                                                               print_data(bufferTxRx+1,bufferTxRx[0]-1,1); //n'affiche pas le nb d'octets
                                                              convol();                       //convolution here
                                                              Txmode(1);                      //change irq to TX
                                                              txingData(0,sizeof(headerTx));   //send header
                                                              txingData(1,sizeof(bufferConv));   // send data
                                                              Txmode(0);                      //change irq to RX
                                                              ptrTXRaw=1;                     //set ptr to 1 for next sentence
                                                              }
                                              if (car!=0x0a)  {
                                                              bufferTxRx[ptrTXRaw+sizeof(callsign)]=car;
                                                              ptrTXRaw++;
                                                              }
                          }
if (done==1){
     decode();     //decode viterbi here   
      //print_data(bufferConv,sizeof(bufferConv),0); 
     if (Convolve.check_crc(bufferTxRx)==1)  {
                                             Serial.print("RX:");
                                             print_data(bufferTxRx+1,bufferTxRx[0]-1,1);    //n'affiche pas le nb d'octets
                                             }
                                           else
                                           {
                                           Serial.print("Nb errors :");
                                           Serial.println(Convolve.acc_error[1][0]);    //print nb errors    
                                           print_data(bufferTxRx,sizeof(bufferTxRx),0);                                           
                                           }
     done=0;
   }
     
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
     #ifdef INVERTRX
     LshiftBit(headerRx,sizeof(headerRx),(~digitalRead(pinRXD))&1);     // save the received bit in bit Lshift buffer inverted
     #else
      LshiftBit(headerRx,sizeof(headerRx),digitalRead(pinRXD));        // save the received bit in bit Lshift buffer non inverted
     #endif
     break;
case 1 : 
     #ifdef INVERTRX
     LshiftByte(bufferConv,sizeof(bufferConv),(~digitalRead(pinRXD))&1);        // save the received bit in byte Lshift buffer inverted
     #else
     LshiftByte(bufferConv,sizeof(bufferConv),digitalRead(pinRXD));        // save the received bit in byte Lshift buffer non inverted
     #endif
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
                 #ifdef INVERTTX
                 digitalWrite(pinTXD, (headerTx[byteCount] >> (7-(bitCount)))&0x1 ? LOW:HIGH); //tx header inverted
                 #else
                 digitalWrite(pinTXD, (headerTx[byteCount] >> (7-(bitCount)))&0x1 ? HIGH:LOW); //tx header non inverted
                 #endif
                 
                 break;
        case 1 : 
                 #ifdef INVERTTX
                 digitalWrite(pinTXD, (bufferConv[byteCount] >> (7-(bitCount)))&0x1 ? LOW:HIGH); //tx buffer inverted
                 #else
                 digitalWrite(pinTXD, (bufferConv[byteCount] >> (7-(bitCount)))&0x1 ? HIGH:LOW); //tx buffer non inverted
                 #endif                 
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
    detachInterrupt(0);
   //ajoute la commande ptt on ici 
    attachInterrupt(1, txINT, RISING); 
    TXenable=1;
  }
  else
  {
  TXenable=0;            // 0 : rx
  //ajoute la commande ptt off ici 
  detachInterrupt(1); 
  attachInterrupt(0, rxINT, FALLING);
  bitCount=0;
  byteCount=0;
  }

}

// wait txing data is complete
void txingData(byte nbuf, int dataSize)
{
nBuffer=nbuf;
bitCount=0;
byteCount=0;
while(byteCount < dataSize) {};    //wait txing complete;
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


//print char array in hex or char
void print_data(byte *data, int dataSize,byte hexorchar)
{
 int n;
for (n=0;n<dataSize;n++)
{
if (hexorchar==0) {
                  Serial.print(data[n],HEX); 
                  Serial.print(" ");
                  }
                  else Serial.print((char) data[n]);
}
Serial.println(); 
}



