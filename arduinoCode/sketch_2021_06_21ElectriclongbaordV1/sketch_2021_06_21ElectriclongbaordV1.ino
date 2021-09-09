/* author: Alex Semenov
  Version 2 started using Butter Scotch look at the V4 Bluetooth Shenenagans app for more information
*/

#define ADDR_LENG 0 // address of the length identifier
#define ADDR_ID 1 // address of the ID
#define ADDR_TYPE 2 // address of the message type
#define ADDR_CONT 3 // address of the contents of the message
#define SIZE_CONST 5 // the size of the everything but the message
#define SIZE_CHECKSUM 2 // the size of the checksum
#define ID 0x30 // device ID
#define IA_EL 0 // instead of a hashmap I will let the complier handle this one
#define ET_IV 1 // dito

#define SAFTY_PIN 2 // pint that reads the state of the bluetooth modual

volatile int p = 0; // if information has been resently recived p=0
volatile int j; // amount of serial events

char message [100];  // the whole message we want to send
char received [100]; // the whole received message
char reads [100]; // reading buffer
int partner [4]; // contains decoded info about the partner
int values [100]; //stores current values of things
char receivedLast = 0x00; // last message recived? 0x01 : 0x00
int lookupSizes [] = {11,9};
int lookupType [] = {0,1};
String errorCode;

int analog = 147;
int r = 0;
int b = 0;
int g = 0;
int ledState = 0;
int mode = 0;
int trans = 0;

//Writers -----------------------------------------------------
void writeIntegerValue(int value, int index) {
  values[index] = value;
}
void writeFloatValue(float value, int index){
  values[index] = (int) value; // writes everything left of the decimal as an int and truncates the rest
  values[index+1] = (int) ((value-(float)values[index])*10000.0); // writes everything right of the decimal like an int in the thousands
}
void writeCharValue(char value, int index){
   values[index] = value;
}
void writeStringValue(char *string, int index, int leng){
  for(int i=0; i<leng; i++){
    writeCharValue(string[i], index+i);
  }
}
//Readers ---------------------------------------------------
int readIntegerValue(int index, int size){
  return (int) bytes2Int(received, size+index+ADDR_CONT, index+ADDR_CONT);
 }
float readFloatValue(int index, int size){ // passed the decimal the size is always 2 bytes
   float right = (float) bytes2Int(received, size-2+index+ADDR_CONT, index+ADDR_CONT);
   float left = (float) bytes2Int(received, size+index+ADDR_CONT, index+ADDR_CONT+size-2);
   left = (float) (left/10000.0);
   return right + left;
}
char readCharValue(int index){
   return (char) received[index+ADDR_CONT];
}
String readStringValue(int index, int size){
   char chars [size];
   for (int i=0; i<size; i++){
      chars[i] = readCharValue(index);
   }
   return String(chars);
}

int getType(){
  return lookupType[partner[ADDR_TYPE]];
}

String readInstruction(char *bytes, int length){
  String toReturn = String("");
  for (int i=0; i<length; i++){ // reads the bytes buffer into the received message
     received[i] = bytes[i];
  }

  if (length == (int) bytes2Int(received,1+ADDR_LENG,ADDR_LENG)){
     toReturn = toReturn + "Length Passed";
  } else {
     toReturn = toReturn + ", Length Failed: " + bytes2Int(received,1+ADDR_LENG,ADDR_LENG) + '/' + length;
     receivedLast = 0x00;
     return toReturn;
  }
  if (cSum(received,length-SIZE_CHECKSUM) == bytes2Int(received,length,length-SIZE_CHECKSUM)){ // checks checksum
     toReturn = toReturn + ", Checksum Passed";
  } else {
     toReturn = toReturn + "Checksum Failed: " + cSum(received,length-SIZE_CHECKSUM) + '/' + bytes2Int(received,2+length-SIZE_CHECKSUM,length-SIZE_CHECKSUM);
     receivedLast = 0x00;
     return toReturn;
  }
  receivedLast = 0x01;

  // gets all the easy stuff
  partner[ADDR_LENG] = length;
  partner[ADDR_ID] = (int) bytes2Int(received, 1+ADDR_ID, ADDR_ID);
  partner[ADDR_TYPE] = (int) bytes2Int(received, 1+ADDR_TYPE, ADDR_TYPE);
  partner[ADDR_CONT] = cSum(received,length-SIZE_CHECKSUM);
  return toReturn;
}

void changeInstruction (int instruction){
  long temp;
  int j;
  int size = lookupSizes[instruction];
  message[ADDR_ID] = ID|receivedLast; // updates the ID byte
  int2Bytes(size, message,1, ADDR_LENG); // updates the length identifier
  int2Bytes(lookupType[instruction], message, 1, ADDR_TYPE); // updates the type identifier

  for (int i=0; i<size-SIZE_CONST; i++){
     if (values[i] > 255){  // ------------------------------------cant be 255?
     temp = 255;
     j=1;
        while (temp < (long)values[i]){ //finds the total bytes we need to store a number
           temp = temp*256;
           j++;
        }
        int2Bytes(values[i], message, j, i+ADDR_CONT); // updates the contents of the message
        i=j-1+i;
     } else {
        int2Bytes(values[i], message, 1, i+ADDR_CONT); // updates the contents of the message
      }
  }

  int2Bytes(cSum(message,size-SIZE_CHECKSUM), message, SIZE_CHECKSUM, size-SIZE_CHECKSUM); // updates the checksum
}


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

unsigned long int bytes2Int(char *bytes, int leng, int offset){ // makes a series of bytes into a binary number which is returned as an integer
 byte mask;
 unsigned long int sum = 0;
  
  for (int i=leng; i>offset; i--){ // repetes untill every byte has been prosessed start at last byte
    mask = 0x01;
    for (int k=0; k<8; k++){ // start at first bit
      if(bytes[i-1] & mask){ // if bit is 1
       sum += power(2,k+(8*(leng-i))); // add bit
      }
      mask <<= 1; // bit shift mask to the left
    }
  }
  return sum;
}

void int2Bytes(int num, char* bytes, int leng, int offset){ // writes a binary number from an int
  for (int i=offset; i<leng+offset; i++){ //sets all bits in message to 0
    bytes[i] = 0x00;
  }
  
  if (num%2 != 0){ // if num is odd then we know the first bit is a 1
    bytes[leng+offset-1] = bytes[leng+offset-1] | 0x01;
  }
  
  int shift, pos;
  byte mask;
  int tempNum = num;
  
  while ((shift=int2Bit(tempNum)) != 0){ // while there is bits left to add
    tempNum = tempNum - power(2,shift); // removes previous bit
    mask = 0x01; // resets the mask
    mask <<= shift%8; // moves the mask to the current bit
    pos = bytePos(leng, shift); //finds the current byte
    bytes[pos+offset] = bytes[pos+offset] | mask; // adds the bit to the byte
  }
}

int int2Bit(int tempNum){ // determines bigest bit position of checksum
  int k =0;
  while((tempNum =tempNum/2) >= 1){
   k++; 
  }
  return k;
}

int bytePos(int leng, int shift){ // counts the byte we are on based on the shift
  int pos = 0;
  while (shift>7){
    shift -=8;
    pos++;
  }
  return leng-pos-1; // -1 because arrays start at 0
}

int cSum(char* bytes, int leng){ // calcualtes integer checksum
  int sum =0;
  for (int k=0; k<leng; k++){
    sum+=bytes2Int(&bytes[k],1,0);
  }
  return sum;
}

void actions () {
  
}

void serialEvent(){
  p=0; // good connection
  j++; // counts a serial event
  TCNT1  = 0x00; // should reset timer
}

String takeIn(char *bytes){
  char srt;
  int i=0;
  int flag = 0;
  while((srt = Serial.read()) != -1){ // read entire message to memory
    delay(2); // could by shorter?
    bytes[i++] = srt;
  }
  return readInstruction(bytes,i);
}

ISR(TIMER1_COMPA_vect){//timer1 interrupt 5Hz: if timer is ever aloud to complete we have a bad connection
  p = 1; // bad connection
}

void debuggin(){
    Serial.println("Code: " + errorCode);
    Serial.print("analog: ");
    Serial.println(analog);
    Serial.print("r: ");
    Serial.println(r);
    Serial.print("g: ");
    Serial.println(g);
    Serial.print("b: ");
    Serial.println(b);
    Serial.print("ledState: ");
    Serial.println(ledState);
    Serial.print("mode: ");
    Serial.println(mode);
    //Serial.print("stuff: ");
    //for(int i=0; i<6; i++){
    //  Serial.print(partner[i]);
    //  Serial.print(", ");
    //}
    Serial.println('\n');
}

void setup(){ // compare match register = [16,000,000Hz/(prescaler*desired interupt frequency)]-1
  for(int k=3; k<13; k++){ // set all pins but 2 to be output
    pinMode(k, OUTPUT);
  }
  pinMode(SAFTY_PIN, INPUT);
  pinMode(13,OUTPUT);
  Serial.begin(38400);
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
}

void loop() {
  if (j>11){
    errorCode = takeIn(reads);
    if (errorCode.equals("Length Passed, Checksum Passed")){ // good message
      
      //Serial.println(getType());
      if (getType() == IA_EL ){ // type for control message 
        analog = readIntegerValue (0, 1);
        r = readIntegerValue (1, 1);
        b = readIntegerValue (2, 1);
        g = readIntegerValue (3, 1);
        ledState = readIntegerValue (4, 1);
        mode = readIntegerValue(5,1);
        trans = readIntegerValue(6,1);
        
        
      } else { // idk
        
      }
      
    } else { // bad message
    
    }
    // return a message
    
    writeIntegerValue(20, 0);
    writeFloatValue(22.2, 1);
    changeInstruction(ET_IV);
    for(int i=0; i<lookupSizes[ET_IV]; i++){
      Serial.write(message[i]);
    }
    
    //debuggin();
    j=0;
  }
  
 if (p == 0){ // good conection *** saftey pin set low for testing
   digitalWrite(13,HIGH);
   actions();
 } else { // bad conection
   digitalWrite(13,LOW);
        analog = 127;
 }
}
