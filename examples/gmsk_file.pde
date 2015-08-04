/**
simulation démission de fichier pour modem GMSK

taille data = (mulpiple de 8) - 1 -3
48-4=44
64-4=64
72-4=72
80-4=76
96-4=92
104-4=100
112-4108
120-4=116
128-4=124



protocole basic :
identifiant trame  / donnees (taille a definir) / l'arduino ajoute le crc + un octet vide

I : initialize
R : Ready to send / destinataire
A : reponse positive 

F : fichier à envoyer / nom du fichier 8+3 , taille du fichier (32 bits), nombre de paquets 16bits
D : data  / numero du paquet (16 bits) , donnees du paquet
  insérer des pauses entre chaque TX
C : checkup bilan donnes transmises
R : pourcentage recus

E : fin de fichier /  (facultatif) a voir


T: texte / call / data (supprimer le char 10) dans la trame ci c'est pas déja fait

G: gps / call / data

H: hour / call / data

commande de ptt ? on/off


serveur web , messagerie


5846  26s  224.84

26843  152s avec 200ms 

59015  380  avec 232ms test sur trx ras
162514  736 avec 200ms

4.63 s/ko
5.25 s/Ko



 */


import processing.serial.*;

Serial myPort;  // Create object from Serial class
int val;      // Data received from the serial port
int taille;
int Nb_lignes;
int Reste;
int duree_tx;

//String file="file.jpg";
String file="massif2.jpg";
//String file="essai.png";
//String file="essai.txt";

//int tempo=200;
int tempo=232;  //testé pour gros fichier ras

String name="";

int size_tx=48-3;            //test 45 octets
char buffer[] = new char[size_tx];
char[] inBuffer = new char[size_tx];
int ptr_din;
int stateRx;




byte[] saved_buffer = new byte[1024*512];    //512Ko
  
void setup() 
{
  size(200, 200);
  // I know that the first port in the serial list on my mac
  // is always my  FTDI adaptor, so I open Serial.list()[0].
  // On Windows machines, this generally opens COM1.
  // Open whatever port is the one you're using.
  String portName = Serial.list()[1];
  myPort = new Serial(this, portName, 115200);
  
    
// Writes the bytes to a file
//byte[] nums = { 0, 34, 5, 127, 52};
//saveBytes("numbers.dat", nums);

 ptr_din=0; 
 stateRx=0;
 sketchPath("config.xml");      //pas besoin ?

frameRate(200); 
 
}





void draw()
{


}




void print_buffer(byte b[])
{
  // Print each value, from 0 to 255 
for (int i = 0; i < b.length; i++) { 
  // Every tenth number, start a new line 
  if ((i % size_tx) == 0) { 
    println(); 
  } 
  // bytes are from -128 to 127, this converts to 0 to 255 
  int a = b[i] & 0xff; 
  print(hex(a) + ","); 
} 
// Print a blank line at the end 
println(); 
}



void keyPressed() 
{
  if ( key == 'F' )  {
                       myPort.write(0x8c);
                       delay(1000);
                       Send_file(file);
                       delay(1000);    
                       myPort.write(0x88);
                       delay(1000);
                      myPort.write(0x80);
                     delay(1000); 
  } 
}

/*
tables de simulation terminal

Routines de réception de données
46,66,69,6C,65,2E,6A,70,67,00,00,00,00,00,00,68,DB,02,70,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00
44,00,00,FF,D8,FF,E0,00,10,4A,46,49,46,00,01,01,01,00,48,00,48,00,00,FF,E1,13,A4,45,78,69,66,00,00,4D,4D,00,2A,00,00,00,08,00,07,01,0F
44,00,01,02,00,00,00,5C,00,00,00,62,01,10,00,02,00,00,00,5C,00,00,00,BE,01,1A,00,05,00,00,00,01,00,00,01,1A,01,1B,00,05,00,00,00,01,00
taille : 26843
Nb_lignes : 624
Reste : 11

essai.txt  une ligne
46,65,73,73,61,69,2E,74,78,74,00,00,00,00,00,00,20,00,01,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00
44,00,00,62,6F,6E,6A,6F,75,72,20,63,27,65,73,74,20,61,6C,65,20,71,75,69,20,76,6F,75,73,20,70,61,72,6C,65,00,00,00,00,00,00,00,00,00,00

essai.txt  trois lignes
46,65,73,73,61,69,2E,74,78,74,00,00,00,00,00,00,69,00,03,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,
44,00,00,62,6F,6E,6A,6F,75,72,20,63,27,65,73,74,20,61,6C,65,20,71,75,69,20,76,6F,75,73,20,70,61,72,6C,65,20,65,6E,20,64,69,72,65,63,74,
44,00,01,20,64,65,20,6C,27,61,74,65,6C,69,65,72,20,61,75,20,70,72,65,6D,69,65,72,20,E9,74,61,67,65,0D,0A,64,65,20,6C,61,20,6D,61,69,73,
44,00,02,6F,6E,20,E0,20,54,E9,6C,6F,63,68,E9,20,37,32,32,32,30,2E,20,20,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,

 
*/

//routines de réception

void serialEvent(Serial p) {
  
char din = p.readChar();
//print(din);  
  inBuffer[ptr_din]=din;
  ptr_din++;
  if(ptr_din==size_tx) {
                      ptr_din=0;
                     /* for (int n=0;n<size_tx;n++) {                                    
                                            //print((char) buffer[n]+",");
                                            //print(hex(inBuffer[n])+",");
                                            }
                                           println();*/
                      //traitement en fonction de type                                           
                      traitement();                     
                      }
} 

//

void traitement()
{
if (stateRx==0){ //check command
                if (inBuffer[0]=='F') { 
                                   stateRx=1;    //file arrived
                                    name="";
                                   //retrouve le nom et la taille du fichier
                                  for (int n = 1; n < 12+1; n++) {
                                                                  if(inBuffer[n]>0) name=name +inBuffer[n]+"";
                                                                  }
                                  println(name);
                                  taille = (int)inBuffer[16] & 0xff;
                                  taille = ((int)inBuffer[15] << 8) | taille;
                                  taille = ((int)inBuffer[14] << 16) | taille;
                                  taille = ((int)inBuffer[13] << 24) | taille; 
                                  println(taille);
                                  Nb_lignes = (int)inBuffer[18] & 0xff;
                                  Nb_lignes = ((int)inBuffer[17] << 8) | Nb_lignes;
                                  println(Nb_lignes);
                                  
                                  }
               }                      

if (stateRx==1){ //file arrived
               if (inBuffer[0]=='D') {
                                     int paquet;
                                     int ptr=3;
                                     paquet = (int)inBuffer[2] & 0xff;
                                     paquet = ((int)inBuffer[1] << 8) | paquet;
                                     println(paquet);
                                     for (int n = paquet*(size_tx-3); n <  (paquet+1)*(size_tx-3); n++)  saved_buffer[n]=(byte) inBuffer[ptr++];
                                     if ((paquet+1)==Nb_lignes) {
                                                             stateRx=0;
                                                              println("roger, recu");
                                                             sauvegarde_fichier();
                                                            }
                                     }
                                     else
                                     {
                                      //erreur 
                                     }
                }    
 
  
}

void sauvegarde_fichier()
{
byte[] saved_file = new byte[taille];
for (int n = 0; n < taille; n++)  saved_file[n]=saved_buffer[n];
//saveBytes("data/test.txt", saved_file);
String filename="data/"+name;
saveBytes(filename, saved_file);
 
}



/*
Routines d'émission de données
F : fichier à envoyer / nom du fichier 8+4 , taille du fichier (32 bits) /nb de paquets 
D : data  / numero du paquet (16 bits) , donnees du paquet
  insérer des pauses entre chaque TX à vérifier expérimentalement

0046,0065,0073,0073,0061,0069,002E,0074,0078,0074,0000,0000,0000,0000,0000,0000,0069,0000,0003,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,fichier transmis
0046,0065,0073,0073,0061,0069,002E,0074,0078,0074,0000,0000,0000,0000,0000,0000,0069,0000,0003,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000


  
  */


void Send_file(String file)
{
byte file_data[] = loadBytes(file);

analyse_buffer(file_data,size_tx-2);
//print_buffer(file_data);

buffer[0]='F';
buffer[13]='/';
//file="toto.txt";
//print(file.length());
int n;
int m;

for (n = 1; n < 12+1; n++) buffer[n] = 0;    //clear field
for (n = 1; n < file.length()+1; n++) buffer[n] = file.charAt(n-1);

buffer[16]=(char)(taille & 0xff);    
buffer[15]=(char)(taille >> 8 & 0xff);
buffer[14]=(char)(taille >> 16 & 0xff);
buffer[13]=(char)(taille >> 24 & 0xff);

buffer[18]=(char)(Nb_lignes & 0xff);    
buffer[17]=(char)(Nb_lignes >> 8 & 0xff);

buffer_print_serial();
delay(tempo);

int ptr=0;
//println((char)file_data[0]);
int paquet=0;

for(n=0;n<taille;n++)
//for(n=0;n<100;n++)
                    {
                    if (ptr==0) { 
                                 for (m = 0; m < size_tx; m++) buffer[m] = 0;    //clear field
                                 buffer[0]='D'; 
                                 buffer[2]=(char)(paquet & 0xff);    
                                 buffer[1]=(char)(paquet >> 8 & 0xff);
                                 ptr+=3;
                                 println(paquet+"/"+Nb_lignes);
                                 paquet++;
                               }
                    if (ptr==size_tx) {
                                      //TX
                                      ptr=0;
                                      n--;
                                      buffer_print_serial();
                                      delay(tempo);            //temps d'attente  (10+48*2)*8/4800= 176ms plus décodage et liaison série
                                      //println("done");
                                      }
                                      else
                                      {
                                      buffer[ptr]=(char) ((int) file_data[n] & 0xff);
                                      ptr++; 
                                      }
                     if (n==taille-1)  {
                                       buffer_print_serial();
                                       }               
                      
                    }
                    
println("fichier transmis");

}


void buffer_print_serial()
{
 int n;
 //for (n=0;n<size_config;n++) port.write(config[n]);
for (n=0;n<size_tx;n++) {
                        //print((char) buffer[n]+",");
                        //print(hex(buffer[n])+",");
                        myPort.write(buffer[n]);
                        }
//println();
 
}


void analyse_buffer(byte b[],int nbOctPerLine)
{
taille =b.length;
Nb_lignes =b.length/nbOctPerLine;
Reste =b.length%nbOctPerLine;
if (Reste>0) Nb_lignes++;
duree_tx=(taille*2*8/4800); 
println("taille : " +taille);
println("Nb_lignes : " +Nb_lignes);
println("Reste : " +Reste);
println("Duree (secondes) : " +duree_tx);  
}



