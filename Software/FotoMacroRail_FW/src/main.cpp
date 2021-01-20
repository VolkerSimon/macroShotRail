#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

void Menu_Main();
void Menu_Run();
void ManualStepping(uint32_t tSteps,uint32_t tDelay);
void FOW(uint32_t tSteps,uint32_t tDelay);
void REV(uint32_t tSteps,uint32_t tDelay);
void ManualSteppingSingle(uint32_t tSteps,uint32_t tDelay);
void ShowDisplay(String tHeader,String tInfo);
void ChangeMotorEnableState(int tState);


int WasPressed=0;

enum Modes
{
  Mode_Menu_Main
  ,Mode_Menu_Run
  ,Mode_Run
  ,Mode_Run_Cont
  ,Mode_Run_Step00062
  ,Mode_Run_Step00312
  ,Mode_Run_Step00625
  ,Mode_Run_Step01250
  ,Mode_Run_Step02500
  ,Mode_Run_Step05000
  ,Mode_Run_Step07500
  ,Mode_Run_Step10000
  ,Mode_NoMode
};

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

//Definition for Arduino CNC shield V4
#define STP_En 8
#define STP_Dir_1 2
#define STP_Step_1 5
#define MicroSteps 16
#define BTN_Left A3
#define BTN_Right A2
#define BTN_Sel A1

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int Menu_Run_ICount=10;
String Menu_Run_Items[Menu_Run_ICount]={"Cont. Run","Step 0.0062","Step 0.0312mm","Step 0.0625mm","Step 0.125mm","Step 0.25mm","Step 0.50mm","Step 0.75mm","Step 1mm","Main Menu"};
int Menu_Run_Mode[Menu_Run_ICount]={Mode_Run_Cont,Mode_Run_Step00062,Mode_Run_Step00312,Mode_Run_Step00625,Mode_Run_Step01250,Mode_Run_Step02500,Mode_Run_Step05000,Mode_Run_Step07500,Mode_Run_Step10000,Mode_Menu_Main};
int Menu_Run_ActItem=0;
int Menu_Run_OldItem=-1;

const int Menu_Main_ICount=3;
String Menu_Main_Items[Menu_Main_ICount]={"Run Mode","Focus Mode", "Settings"};
int Menu_Main_Mode[Menu_Main_ICount]={Mode_Menu_Run,Mode_Menu_Run,Mode_Menu_Run};
int Menu_Main_ActItem=0;
int Menu_Main_OldItem=-1;

unsigned long MotorEnabledStateTimer=0;

unsigned long MotorDisableTime=500;

int ActMode=Mode_Menu_Main;


void setup()
{

  //Setup Pins
  pinMode(BTN_Sel, INPUT_PULLUP); 
  pinMode(BTN_Left, INPUT_PULLUP); 
  pinMode(BTN_Right, INPUT_PULLUP); 
  pinMode(STP_En, OUTPUT); //Enable
  pinMode(STP_Step_1, OUTPUT); //Step
  pinMode(STP_Dir_1, OUTPUT); //Direction

  //Set motor state to OFF
  ChangeMotorEnableState(HIGH);


// Setup Display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 
  { 
    //We need Serial only the the case of this Error
    Serial.begin(9600);
    delay(200);
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Prepare Display and Text
  display.display();
  delay(2000);                          
  display.clearDisplay();
  display.setTextSize(1);               
  display.setTextColor(SSD1306_WHITE);  
  display.setCursor(0,0);               
}

//*********************************************************************
void loop()
{
  
  if (ActMode==Mode_Menu_Main) Menu_Main();
  if (ActMode==Mode_Menu_Run) Menu_Run();
  if (ActMode==Mode_Run_Cont) ManualStepping(200,25);
  if (ActMode==Mode_Run_Step00062) ManualSteppingSingle(1*MicroSteps,500);//0,625um
  if (ActMode==Mode_Run_Step00312) ManualSteppingSingle(5*MicroSteps,500);//3,125um
  if (ActMode==Mode_Run_Step00625) ManualSteppingSingle(10*MicroSteps,500);//6,25um
  if (ActMode==Mode_Run_Step01250) ManualSteppingSingle(20*MicroSteps,500);//12,5um
  if (ActMode==Mode_Run_Step02500) ManualSteppingSingle(40*MicroSteps,500);//25um
  if (ActMode==Mode_Run_Step05000) ManualSteppingSingle(80*MicroSteps,500);//50um
  if (ActMode==Mode_Run_Step07500) ManualSteppingSingle(120*MicroSteps,500);//75um
  if (ActMode==Mode_Run_Step10000) ManualSteppingSingle(160*MicroSteps,500);//1mm
  
  //We shut off the motor after a Time to avoid heating problems
  if (millis()>(MotorEnabledStateTimer+MotorDisableTime))
  {
    ChangeMotorEnableState(HIGH);  
  }
}

//*********************************************************************
void Menu_Main()
{
  if (!digitalRead(BTN_Left))
  {
    Menu_Main_ActItem++;
  }else if (!digitalRead(BTN_Right))
  {
    Menu_Main_ActItem--;
  }else if (!digitalRead(BTN_Sel))
  {
    ActMode=Menu_Main_Mode[Menu_Main_ActItem];
    delay(250);
  }

  if (Menu_Main_ActItem==Menu_Main_ICount)
  {
    Menu_Main_ActItem=0;
  } else if (Menu_Main_ActItem==-1)
  {
    Menu_Main_ActItem=Menu_Main_ICount-1;
  }

  if (Menu_Main_ActItem!=Menu_Main_OldItem)
  {
    ShowDisplay("Main Menu",Menu_Main_Items[Menu_Main_ActItem]);
    delay(250);
  }

  Menu_Main_OldItem=Menu_Main_ActItem;
}

//*********************************************************************
void Menu_Run()
{
  if (!digitalRead(BTN_Left))
  {
    Menu_Run_ActItem++;
  }else if (!digitalRead(BTN_Right))
  {
    Menu_Run_ActItem--;
  }else if (!digitalRead(BTN_Sel))
  {
    ActMode=Menu_Run_Mode[Menu_Run_ActItem];
    ShowDisplay("Run",Menu_Run_Items[Menu_Run_ActItem]);
    delay(250);
    //if (ActMode!=Mode_Menu_Main)ChangeMotorEnableState(LOW);//digitalWrite(STP_En, LOW);
  }

  if (Menu_Run_ActItem==Menu_Run_ICount)
  {
    Menu_Run_ActItem=0;
  } else if (Menu_Run_ActItem==-1)
  {
    Menu_Run_ActItem=Menu_Run_ICount-1;
  }

  if (Menu_Run_ActItem!=Menu_Run_OldItem)
  {
    ShowDisplay("Run Menu",Menu_Run_Items[Menu_Run_ActItem]);
    delay(250);
  }

  Menu_Run_OldItem=Menu_Run_ActItem;
}

//*********************************************************************
void ShowDisplay(String tHeader,String tInfo)
{
  display.clearDisplay();
    display.setTextSize(1);             
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println(tHeader);
    display.setCursor(0,16);
    display.println(tInfo);
    display.display();
}

//*********************************************************************
void ManualSteppingSingle(uint32_t tSteps,uint32_t tDelay)
{
  if (!digitalRead(BTN_Left) && WasPressed==0)
  {
    FOW(tSteps,tDelay);
    delay(250);
    WasPressed=1;
    
  }else if (!digitalRead(BTN_Right)&& WasPressed==0)
  {
    REV(tSteps,tDelay);
    delay(250);
    WasPressed=1;
  }else if (!digitalRead(BTN_Sel)&& WasPressed==0)
  {
    ActMode=Mode_Menu_Run;
    Menu_Run_OldItem=256;
    //We are Leaving the RUN menu so we can stop the motor if still on
    ChangeMotorEnableState(HIGH);
    delay(250);
  }else{
    WasPressed=0;
  }

}

//*********************************************************************
void ManualStepping(uint32_t tSteps,uint32_t tDelay)
{
  
  if (!digitalRead(BTN_Left))
  {
    FOW(tSteps,tDelay);
  }else if (!digitalRead(BTN_Right))
  {
    REV(tSteps,tDelay);
  }else if (!digitalRead(BTN_Sel))
  {
    Menu_Run_OldItem=256;
    //Back to the Run Menu
    ActMode=Mode_Menu_Run;
    //We are Leaving the RUN menu so we can stop the motor if still on
    ChangeMotorEnableState(HIGH);
    delay(250);
   }
}


//*********************************************************************
void FOW(uint32_t tSteps,uint32_t tDelay)
{
    ChangeMotorEnableState(LOW);
    digitalWrite(STP_Dir_1, HIGH);
    for (uint32_t Index = 0; Index < tSteps; Index++)
    {
      digitalWrite(STP_Step_1, HIGH);
      delayMicroseconds(tDelay);
      digitalWrite(STP_Step_1, LOW);
      delayMicroseconds(tDelay);
    }
}

//*********************************************************************
void REV(uint32_t tSteps,uint32_t tDelay)
{
    ChangeMotorEnableState(LOW);
    digitalWrite(STP_Dir_1, LOW);
    for (uint32_t Index = 0; Index < tSteps; Index++)
    {
      digitalWrite(STP_Step_1, HIGH);
      delayMicroseconds(tDelay);
      digitalWrite(STP_Step_1, LOW);
      delayMicroseconds(tDelay);
    }
}

//*********************************************************************
void ChangeMotorEnableState(int tState)
{
  digitalWrite(STP_En, tState);
  if (tState==HIGH) 
  {
    MotorEnabledStateTimer=0;

  }else{
    MotorEnabledStateTimer=millis();
  }
}
