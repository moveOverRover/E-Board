/* author: Alex Semenov
  Third resonable working version
  Reads more data at a lower resolution and along with the checksum this greatly improves the speed (by about 4x)
  Dosent use all data recived so that the remote can send every input it has and only some need to be used by the reciver
  Uses a checksum to determine the integrity of hte message
  Kill swith is integrated
  Made the intterupt contain siginicantly less code and switch varriables moddifed in the intterupt to volatle
*/

#include <ServoTimer2.h>

#define D1 3
#define D2 4
#define D3 5
#define T1 6
#define T2 7
#define T3 8
#define HORN 11
#define LIGHTS 10
#define MUSIC 12
#define X_UPPER 175
#define X_LOWER 80
#define Y_UPPER 175
#define Y_LOWER 80
#define SAFTY_PIN 2

int x = 130;
int y = 130;
int z = 0;
char horn = 'b';
char lights = 'd';
char music = 'f';
volatile int p = 0;
char cmd[8];
char srt;
char cha[2];
volatile int j;
ServoTimer2 ESC;
int offset;

int power(int a, int b){ // a = base, b = exponent
  if(b==0){
    return 1;
  }
  int og = a;
  for(int q=0; q<b-1; q++){
    a = a*og;
  }
  return a;
}

byte int2Byte(int x, boolean yesMap){
  if (yesMap){
    x = map(x, 0, 1023, 0, 255); //must fit in one byte 
  }
  return (byte)x;
}

unsigned long int getSum(char *msg, int leng){ // makes a serires of bytes into a binary number which is returned as an integer
 byte mask;
 unsigned long int sum = 0;
  
  for (int i=leng; i>0; i--){ // repetes untill every byte has been prosessed start at last byte
    //if (msg[i-1] == 0x00){
    //  return 0;
    //}
    mask = 0x01;
    for (int j=0; j<8; j++){ // start at first bit
      if(msg[i-1] & mask){ // if bit is 1
       sum += power(2,j+(8*(leng-i))); // add bit
      }
      mask <<= 1; // bit shift mask to the left
    }
  }
  return sum;
}

void byteSum(int checkSum, char* msg){ // writes a 2 bit binary number to be used as the checksum
  msg[6] = 0x00;
  msg[7] = 0x00;
  
  if (checkSum < 256){ // if the check sum is small we only need one byte so we can write the other one to be 0
    msg[6] = 0x00;
    msg[7] = (char)int2Byte(checkSum,false);
    return;
  }
  
  if (checkSum%2 != 0){ // if checkSum is odd then we know the first bit is a 1
    cmd[7] = cmd[7] | 0x01;
  }
  
  int shift;
  byte mask;
  int tempSum = checkSum;
  
  while ((shift=int2Bit(tempSum)) != 0){ // while there is bits left to add
    tempSum = tempSum - power(2,shift); // removes previous bit
    mask = 0x01;
    if (shift > 7){ // if bit is in first byte
      shift -=8;
      mask <<= shift;
      msg[6] = msg[6] | mask; // adds a bit to the msg
    } else {
      mask <<= shift;
      msg[7] = msg[7] | mask; // adds a bit to the msg
    }
  }
}

int int2Bit(int tempSum){ // determines bigest bit position of checksum
  int k =0;
  while((tempSum =tempSum/2) >= 1){
   k++; 
  }
  return k;
}

int cSum(char* msg, int leng){ // calcualtes integer checksum
  int sum =0;
  for (int k=0; k<leng; k++){
    sum+=getSum(&msg[k],1);
  }
  return sum;
}

void serialEvent(){
  p=0; // good connection
  TCNT1  = 0x00; //should reset timer
  j++;
}

char* takeIn(char *msg){
  int i=0;
  int flag = 0;
  while((srt = Serial.read()) != -1){ // read entire message to memory
    delay(2);
    if(i<7){
      msg[i++] = srt;
    }
  }
  if (i==7){
    return msg;
  } else {
    return NULL;
  }
}

ISR(TIMER1_COMPA_vect){//timer1 interrupt 10Hz: if timer is ever aloud to complete we have a bad connection
  p = 1; // bad connection
}

void debuggin(){
  /*
  Serial.print("cmd: ");
    for(int i=0;i<7;i++){
      Serial.write(cmd[i]);
    }
    Serial.write('\n');
  
    Serial.print("a_x: ");
    Serial.println(getSum(&cmd[0],1));
    Serial.print("a_y: ");
    Serial.println(getSum(&cmd[1],1));
    Serial.print("b_x: ");
    Serial.println(getSum(&cmd[2],1));
    Serial.print("b_y: ");
    Serial.println(getSum(&cmd[3],1));
    Serial.print("z: ");
    Serial.println(z);
    Serial.print(cSum(cmd,5));
    Serial.print(" = ");
    Serial.println(getSum(cha,2));
    */

    Serial.print("0: ");
    Serial.println(x);
    Serial.print("1: ");
    Serial.println(getSum(&cmd[1],1));
    Serial.print("2: ");
    Serial.println(getSum(&cmd[2],1));
    Serial.print("3: ");
    Serial.println(y);
    
}

void setup(){ // compare match register = [16,000,000Hz/(prescaler*desired interupt frequency)]-1
  ESC.attach(6);
  //for(int k=3; k<13; k++){ // set all pins but 2 to be output
  //  if (k!=6){ // pin 6 has an internal short on current board
  //     pinMode(k,OUTPUT);
  //  }
    //digitalWrite(k, LOW);
 // }
  Serial.begin(38400);
  pinMode(SAFTY_PIN, INPUT);
  pinMode(13,OUTPUT);
  j=0; // Very important to make zero

  cli(); //stop interrupts
  //set timer1 to 10Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 10hz increments
  //OCR1A = 1500;// = (16*10^6) / (10*1024) - 1 (must be <65536) ***VERY IMPORTANT*** -LIKES TO BE A NUMBER THAT ENDS IN AT LEAST ONE '0', I HAVE NO IDEA WHY JUST LEAVE IT BE!
  OCR1A = 3000; // double the pervious time becuase it runs better this way
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();//allow interrupts

  ESC.write(2400);
  delay(3000);
}

void loop() {
  
  if(j>7){ //once enough bytes have been recived
    j=0;  // reset the war clock
    if (takeIn(cmd) != NULL){ // take in the bytes
      //takeIn(cmd);
      cha[0] = cmd[5]; // prepare a char* with the check sum
      cha[1] = cmd[6];
      
      if (cSum(cmd,5) == getSum(cha,2)){ // if the check sum checks out then we can change stuff
        y = getSum(&cmd[0],1);
        x = getSum(&cmd[3],1);
        z = getSum(&cmd[4],1);
      }
      //debuggin();
    }
  }
  
 if (p == 0){ // good conection *** saftey pin set low for testing
   digitalWrite(13,HIGH);
   ESC.write(map(y,255,0,2400,900));
 } else { // bad conection
   digitalWrite(13,LOW);
    ESC.write(1550);
 }
 
}
