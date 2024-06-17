This is a fork of [QtSerialMonitor](https://github.com/mich-w/QtSerialMonitor). All credit goes to Michal.

I am using it to practice Qt and create an interface to my project [CondorRudderPedal_ESP8622](https://github.com/flyfuri/CondorRudderPedal_ESP8622).

I just ported it to Qt6 and will add some futures. If something useful results, I'll either push-request them to Michal or fully document this Version/Repo.

Stay tuned...


## update 24.4.2024
It looks like it is going to be a major refactoring project as I get more and more ideas the more I dive into the code...

## update 17.5.2024
My priorities have shifted. The project is on hold at least for half a year...
 

## What is already useful:
If you want to know what it does and/or are looking for the original source code for Qt5 got to the original [QtSerialMonitor](https://github.com/mich-w/QtSerialMonitor).

Otherwise one of the following branches could be used (all Qt6):

|branch |description|status|
|-------|------------------|--------------------------|
|"Qt6"| same as the original but for Qt6|seems to work (only tested with Serial)|
|"CustomBaudDialog"| improved GUI for custom baudrate |seems to work (only tested with Serial)|
|TsourceImproved|improved and added timestamp sources|only works with Serial, rest will be done later|

branch main contains all the latest changes with status "seems to work"



## Test programm for ESP8266 (should also work for Arduinos and ESPs)

Output parameters can be chosen by sending commands to the ESP. See the commands in the comments of the code below:

```cpp
#include "arduino.h"

#define DEBGOUT 1 //activate/deactivate the temporary print stuff

#if DEBGOUT > 0  
  #define dbugprint(x) Serial.print(x)
  #define dbugprintln(x) Serial.println(x)
#else
  #define dbugprint(x) 
  #define dbugprintln(x) 
#endif

float signals[6] = {0,0,0,0,0,0};
unsigned long target_intervall = 2000; //micros
unsigned int work_micros;

unsigned long t_lastcycl, t_now, t_cycletime, t_cyclt_print; //measure cycle time
//log settings
#define LOG_OPTIONS_SIZE 140
String log_separator = ",";
String log_command = "";
bool log_options[LOG_OPTIONS_SIZE] = {0,1,1,1,1,1,0};
float log_tempmem[LOG_OPTIONS_SIZE];
String log_labels[LOG_OPTIONS_SIZE];
enum LOGMODE{
  DEVAULT = 0,  //same as VAL (get rid of 0 index)
  VAL, //just value separated by separetor  expl.: "1.23 4.56" (Graph 0 and Graph 1)
  LABEL_COLON_SPACE_VAL_TAB,  //  expl.: "Roll = 1.23 Pitch = 45.6"
  LABEL_EQUAL_VAL_SPACE  // expl.: "Voltage: 1.23 (tabulator) Output: 4.56"
} log_mode;


void debuglogs();  //declaration at the end of this file
void setdebuglabels();  //declaration at the end of this file

void setup() {
  setdebuglabels();
  //Initialize serial and wait for port to open:
  Serial.begin(460800);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
}

float thisByte = 0.0f;

void loop() {

  //work simulated
  delayMicroseconds(work_micros);

  signals[1] = (sin(thisByte)*100.0f);
  signals[2] = (cos(thisByte)*100.0f);
  signals[3] =(sin(thisByte)*50.0f);
  signals[4] = (cos(thisByte)*50.0f);
  signals[5] = (sin(thisByte));
  
  thisByte += 0.1f;
  if (thisByte >= 360.0f) {    // you could also use if (thisByte == '~') {
    thisByte = 0.0f;
  }

  //debug info -----------------------------------------------------------------------------------------------------------

      while(Serial.available() > 0){
        log_command=Serial.readString();
        log_command.trim();
      }
      
      debuglogs();

  //print cycletime if chosen and wait to make cycle time to a fix value

      t_lastcycl = t_now;
      t_now = micros();
      t_cycletime = t_now - t_lastcycl;
      t_cyclt_print = t_cycletime;
      if (log_options[0]){Serial.println(t_cycletime);}
      else{Serial.println("");}
      while(t_cycletime < target_intervall){
        t_now = micros();
        t_cycletime = t_now - t_lastcycl;
      }      
}

//------------------------------------------------------------------------------------------------------------------------------------
// send debug information to analyze signals 
// commands can be sent to the ESP../Arduino to activate/diactivate certain debug informations to be sent
// unfortunately the monitor filter sentOnEnter seems not to work in VSCode use another terminal like CoolTerminal 
// commands:
//   t    activate/disactivate cycletime (should be lower than 2000 micros)
//   r    deactivate all debug infos
//   ?    list acivated infos (not implemented completly)
//   i200..i1000000 set target interval to micros
//   i?    print target_intervall
//   m1..m3 set logmode
//   m?  print logmode
//   d1..d1000000  work delay of in micros
//   d or d0  reset (diactivates) work delay
//   d? print workdelay
//   1..  any positive number activates the corresponding information to be sent (options check code below)
//   -1.. any negative number disactivates the corresponding information 

void debuglogs(){
  long intcmd = 0;
  if (log_command.startsWith("i") && !log_command.startsWith("i?")){  
    log_command.remove(0,1);
    intcmd = log_command.toInt();
    log_command = "i";
  }
  else if (log_command.startsWith("m") && !log_command.startsWith("m?")){
    log_command.remove(0,1);
    intcmd = log_command.toInt();
    log_command = "m";
  }
  else if (log_command.startsWith("d") && !log_command.startsWith("d?") && !log_command.startsWith("d0")){ 
    log_command.remove(0,1);
    intcmd = log_command.toInt();
    log_command = "d";
  }
  else{
    intcmd = log_command.toInt();
    if(intcmd !=0){
      log_command = "";
    }
  }

  if(log_command.isEmpty()){
    if(intcmd > 0 && intcmd < LOG_OPTIONS_SIZE){
      log_options[intcmd] = true;
    }
    else if(intcmd < 0 && abs(intcmd) < LOG_OPTIONS_SIZE){
      log_options[abs(intcmd)] = false;
    }
  }
  else if(log_command == "?"){
    Serial.print("labels :");
    for(int i = 1; i < LOG_OPTIONS_SIZE; i++){
        if(log_options[i]){
            Serial.print(i);
            Serial.print(": ");
            Serial.print(log_labels[i]);
            Serial.print(log_separator);
        }
    }
    delay(3000);
    if(log_options[0]){
      Serial.print(" ");
      Serial.println(log_separator);
    }
  }
  else if(log_command == "r"){
    for(int i = 1; i < LOG_OPTIONS_SIZE; i++){
      log_options[i]=false;
    }
  }
  else if(log_command == "t"){
    log_options[0] = !log_options[0];
  }
  else if(log_command == ";" || log_command == ","){
    log_separator = log_command;
  }
  else if(isSpace(log_command[0])){
    log_separator = "  ";
  }
  else if(log_command == "i" && intcmd >= 200 && intcmd <= 1000000){
    target_intervall = intcmd;
    delay(3000);
  }
  else if(log_command == "i?"){
      Serial.print(" t_interval: ");
      Serial.println(target_intervall);
      delay(3000);
  }
  else if(log_command == "d" && intcmd >= 1 && intcmd <= 1000000){
    work_micros = intcmd;
  }
  else if(log_command == "d0"){
    work_micros = 0;
  }
  else if(log_command == "d?"){
      Serial.print(" work_daly_micros: ");
      Serial.println(work_micros);
      delay(3000);
  }
  else if(log_command == "m" && intcmd >= 1 && intcmd <= 3){ 
    if (intcmd == 1){log_mode = VAL;}
    else if (intcmd == 2){log_mode = LABEL_COLON_SPACE_VAL_TAB;}
    else if (intcmd == 3){log_mode = LABEL_EQUAL_VAL_SPACE;}
  }
  else if(log_command == "m?"){
      Serial.print(" logmode: ");
      if (log_mode == LOGMODE::VAL){Serial.print("1");}
      else if (log_mode == LOGMODE::LABEL_COLON_SPACE_VAL_TAB){Serial.print("2");}
      else if (log_mode == LOGMODE::LABEL_EQUAL_VAL_SPACE){Serial.print("3");}
      delay(3000);
  }
  
  log_command = "";  //reser log command after reacting to it

  //send choosen options

  String lastReadTmp = "";
  for(int i = 0; i < LOG_OPTIONS_SIZE; i++){
      if(log_options[i]){
        if(log_mode == LOGMODE::LABEL_COLON_SPACE_VAL_TAB){
          Serial.print(log_labels[i]);
          Serial.print(": ");
        } 
        else if(log_mode == LOGMODE::LABEL_EQUAL_VAL_SPACE){
          Serial.print(log_labels[i]);
          Serial.print(" = ");
        } 
        switch(i){
          case 1: 
            Serial.print(signals[1]);
            break;
          case 2: 
            Serial.print(signals[2]);
            break;
          case 3: 
            Serial.print(signals[3]);
            break;
          case 4: 
            Serial.print(signals[4]);
            break;
          case 5: 
            Serial.print(signals[5]);
            break;
          case 6: 
            Serial.print(t_cyclt_print);
            break;
        }
        if(log_mode == LOGMODE::LABEL_COLON_SPACE_VAL_TAB){
          Serial.print("\t");
        } 
        else if(log_mode == LOGMODE::LABEL_EQUAL_VAL_SPACE){
          Serial.print(" ");
        } 
        else{
          Serial.print(log_separator);
        }
      }
    }
    
}

void setdebuglabels(){
    log_labels[0] = "time";
    log_labels[1] = "sig1";
    log_labels[2] = "sig2";
    log_labels[3] = "sig3";
    log_labels[4] = "sig4";
    log_labels[5] = "sig5";
    log_labels[6] = "sig6";
}

```
