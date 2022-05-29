#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <ledfunctions.h>
#include <usart.h>
#include <util/delay.h>
#include <potentio.h>
#include <display.h>
#include <timer.h>
#include <buzzer.h>

#include <extra.h>

#include <obstacleDetector.h>

//CONSTANTS
#define BUTTON_DELAY 100

//System related constants
#define PASSWORD_LENGTH 6 
#define SECONDS_TILL_ALARM 20


//System related vars
volatile int isObstacleDetected = 0;
int password[PASSWORD_LENGTH];
int isPasswordSet = 0;
int secondsTillAlarm = SECONDS_TILL_ALARM;
int isAlarmSet = 0;
volatile int isAlarmTriggered = 0;



//flow related vars
int currentNumberOfButtonsPressedForPasswordSetup = 0;
int confirmedShouldSetupSecondsTillAlarm = 0;
int shouldSetupSecondsTillAlarm = 0;

//log
int loggerSize = 160;
int logCounter = 0;

//Sound related vars
#define C5 523.250
#define D5 587.330
#define E5 659.250
#define F5 698.460
#define G5 783.990
#define A5 880.000
#define B5 987.770
#define C6 1046.500

typedef struct{
  int seconds;
  int *passcode;  
}ALARM;

typedef struct{
  int turnedOff;
  int tries;
  int inSeconds;
}LOG;

LOG *logger ;

ALARM alarm;


//functions
void setPassword(){
  printf("Setting password:\n");
  while (!isPasswordSet)
  {
    writeString("CODE");

    if(currentNumberOfButtonsPressedForPasswordSetup<PASSWORD_LENGTH){
      if (isButtonPressed(0))
      {
        _delay_ms(BUTTON_DELAY);
        if (isButtonPressed(0)){
          printf("Button 1 pushed\n");
          password[currentNumberOfButtonsPressedForPasswordSetup] = 0;
          currentNumberOfButtonsPressedForPasswordSetup++;
        }
      }
      if (isButtonPressed(1))
      {
         _delay_ms(BUTTON_DELAY);
        if (isButtonPressed(1)){
          printf("Button 2 pushed\n");
          password[currentNumberOfButtonsPressedForPasswordSetup] = 1;
          currentNumberOfButtonsPressedForPasswordSetup++;
        }
      }
      if (isButtonPressed(2))
      {
        _delay_ms(BUTTON_DELAY);
        if (isButtonPressed(2)){
          printf("Button 3 pushed\n");
          password[currentNumberOfButtonsPressedForPasswordSetup] = 2;
          currentNumberOfButtonsPressedForPasswordSetup++;

        }
      }

      
    }else{
      isPasswordSet = 1;
      printf("Password config is done.\n");
      clearDisplay();
    }
  }
  
}

void waitForPotentioSetupWizard(){
  while (!confirmedShouldSetupSecondsTillAlarm)
  {
    writeString("SECS");
     if (isButtonPressed(0))
      {
        _delay_ms(BUTTON_DELAY);
        if (isButtonPressed(0)){
          //nee is default, dus geen waarde wijziging
          confirmedShouldSetupSecondsTillAlarm = 1;
        }
      }  
     if (isButtonPressed(1))
      {
        _delay_ms(BUTTON_DELAY);
        if (isButtonPressed(1)){
          shouldSetupSecondsTillAlarm = 1;
          confirmedShouldSetupSecondsTillAlarm = 1;
        }
      }  
  }
  clearDisplay();
  
}

void setupSecondsTillAlarm(){
  printf("Please use your potentiometer to adjust the seconds till the alarm goes off.\n");
  printf("To confirm, press on button 1.\n");

  while (1)
  {
    int currentSeconds = readPotentioValue()%100;
    writeNumberToSegment(0,firstDigit(currentSeconds));
    writeNumberToSegment(1,currentSeconds%10);
    //S van seconden, maar lijkt een 5 op de display.
    writeCharToSegment(3,'S');

    if (isButtonPressed(0))
    {
      _delay_ms(BUTTON_DELAY);
      if (isButtonPressed(0)){
        if(currentSeconds>60){
          printf("Please select a number till 60\n");
          setupSecondsTillAlarm();
        }
        secondsTillAlarm = currentSeconds;
        alarm.seconds = secondsTillAlarm;

        printf("Your alarm will activate after %d seconds.\n",currentSeconds);

        clearDisplay();

        return;
      }
    }
    
  }
  

}


void printPassword(){
  printf("This is your password: ");
  for (size_t i = 0; i < PASSWORD_LENGTH; i++)
  {
    printf("%d ", alarm.passcode[i]+1);
  }
  printf("\n");
  
}

void triggerBuzzer(){
  buzzerOn();
  
    lightUpLed(3);

    playTone(A5,250);
    playTone(C6,250);
   
  
}

int checkEnteredPassword(int *attemptedPasswordArray){
  int correctSequence = 1;
  for (size_t i = 0; i < PASSWORD_LENGTH; i++)
  {
    if (!correctSequence)
    {
      return 0;
    }
    
    if(attemptedPasswordArray[i]!=alarm.passcode[i])correctSequence = 0;
  }

  return 1;
  
}

void addToLog(LOG *logpoint,int seconds, int tries, int turnedOff){
  logpoint->inSeconds = seconds;
  logpoint->tries = tries;
  logpoint->turnedOff = turnedOff;
}

void printLog(){
  printf("Your alarm setting:\n\tSeconds till alarm goes off\t: \t%d.\n\tIs password set\t: \t%d.\n",alarm.seconds,isPasswordSet);
  for (size_t i = 0; i < logCounter; i++)
  {
    printf("Was the alarm turned off\t:\t%c\n\tAmount of attempts: %d\n\tAlarm had %d seconds left.\n",logger[i].turnedOff==1?'y':'n',logger[i].tries,(alarm.seconds-logger[i].inSeconds)<0?0:(alarm.seconds-logger[i].inSeconds));
  }



  printf("Your logger will now be cleared.\n");
  free(logger);
  
}

void waitToTriggerBuzzer(int *timeTillBuzzer){
  startTimer();

  int lastSecond = 0;
  int wrongAttempt = 0;

  int playAlarm = 0;

  

 

  int passwordSequenceCounter = 0;
  int passwordAttemptArr[PASSWORD_LENGTH];
  while (isAlarmTriggered)
  {
    if(getIsTimerActive()&&getSeconds()!=lastSecond){
      
      printf("You have %d seconds left\n",alarm.seconds-getSeconds());
    }
    lastSecond = getSeconds();

    //if attempt to enter password
    if (passwordSequenceCounter==PASSWORD_LENGTH)
    {
      printf("Checking password.\n");
      int isPasswordCorrect = checkEnteredPassword(&passwordAttemptArr[0]);
      if (!isPasswordCorrect){
        printf("Incorrect password!\n\tTry Again.\n");
        passwordSequenceCounter = 0;
        wrongAttempt++;

        
        
        
      }else{
        printf("Correct password.\n");
        wrongAttempt=0;
        playAlarm = 0;

        addToLog(&logger[logCounter++],getSeconds(),wrongAttempt,1);
        silenceAlarm();
        return;
      }     
    }
    

    if (isButtonPressed(0))
    {
      _delay_ms(BUTTON_DELAY);
      if (isButtonPressed(0)){
        passwordAttemptArr[passwordSequenceCounter++] = 0;
        
      }
    }
    
    if (isButtonPressed(1))
    {
      _delay_ms(BUTTON_DELAY);
      if (isButtonPressed(1)){
        passwordAttemptArr[passwordSequenceCounter++] = 1;
        
      }
    }
    
    if (isButtonPressed(2))
    {
      _delay_ms(BUTTON_DELAY);
      if (isButtonPressed(2)){
        passwordAttemptArr[passwordSequenceCounter++] = 2;

      }
    }
    
   
    if (getSeconds()>=alarm.seconds || wrongAttempt>=3||playAlarm==1)
    {
      playAlarm = 1;
      stopTimer();
      triggerBuzzer();
    }
    
    


  }
 

  
}

void triggerAlarm(int secondsToWait){
  dimLed(1,70,150);

  
  printf("Alarm triggered\n");
  isAlarmTriggered = 1;
  printf("You have %d seconds till the alarm goes off.\n",secondsToWait);

  //pass by reference
  waitToTriggerBuzzer(secondsToWait);
}

void waitForAlarm(){
  
    turnOnObstacleDetector();

  printf("Press button 1 to view the log.\n");
  while (!isObstacleDetected)
  {
    writeString("ACTV");
    dimLed(0,50,250);
    if(isButtonPressed(0)){
      printLog();
    }
  }
  //alarm triggered
  //pass by value
  triggerAlarm(alarm.seconds);
  
}

void silenceAlarm(){
  isObstacleDetected = 0;
  isAlarmTriggered = 0;
  buzzerOff();
  stopTimer();
  
  waitForAlarm();

  
}


ISR(PCINT2_vect) {  
  
  if(isAlarmSet&&isAlarmTriggered==0&&!isObstacleDetected){
    turnOffObstacleDetector();
    isObstacleDetected = 1;
  }
}

ISR(TIMER2_COMPA_vect) {
  runTickCheck();
    
}


int main(void){
  initUSART();  
  initDisplay();
  enableAllButtons();
  enableAllleds();
  enableBuzzer();
  lightDownAllLeds();
  enableAllleds();
  lightDownAllLeds();
  initObstacleDetector();
  initTimer();
  initADC();

  sei();




  printf("Secure your house!\n");

  alarm.seconds = secondsTillAlarm;
  alarm.passcode = &password[0];

  printf("Please enter a combination of %d buttons to setup your passcode.\n",PASSWORD_LENGTH);
setPassword();

printPassword();

  printf("Do you want to change the amount of seconds till your alarm goes off? currently you have %d seconds. \n",SECONDS_TILL_ALARM);
  printf("Click on\n \tButton 1\t:\tNo\n\tButton 2\t:\tYes\n");

  waitForPotentioSetupWizard();

  if(shouldSetupSecondsTillAlarm==1)setupSecondsTillAlarm();

  printf("Your alarm is active.\n");




  logger = malloc(loggerSize * sizeof(LOG));

  //Activate alarm
  isAlarmSet = 1;
  waitForAlarm();

  





  
  return 0;
}