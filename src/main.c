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
int isObstacleDetected = 0;
int password[PASSWORD_LENGTH];
int isPasswordSet = 0;
int secondsTillAlarm = SECONDS_TILL_ALARM;
int isAlarmSet = 0;
int isAlarmTriggered = 0;



//flow related vars
int currentNumberOfButtonsPressedForPasswordSetup = 0;
int confirmedShouldSetupSecondsTillAlarm = 0;
int shouldSetupSecondsTillAlarm = 0;

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

void silenceAlarm(){
  //todo
  buzzerOff();
  stopTimer();
  isAlarmTriggered = 0;
  isObstacleDetected = 0;
  waitForAlarm();
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

void printLog(){
  //TODO
}

void waitToTriggerBuzzer(int *timeTillBuzzer){
  printf("Time till Buzzer is %d\n", timeTillBuzzer);
  startTimer();
  int lastSecond = 0;
  int wrongAttempt = 0;

 

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
        silenceAlarm();
        waitForAlarm();
        return;
      }     
    }
    

    if (isButtonPressed(0))
    {
      _delay_ms(BUTTON_DELAY);
      if (isButtonPressed(0)){
        printf("Button 0 pressed\n");
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
    
   
    if (getSeconds()>=alarm.seconds || wrongAttempt==3)
    {
      stopTimer();
      triggerBuzzer();
    }
    
    


  }
 

  
}

void triggerAlarm(int secondsToWait){
  dimLed(1,70,150);

  
  printf("Alarm triggered\n");
  isAlarmTriggered = 1;
  printf("You have %d seconds till the alarm goes off.",secondsToWait);

  //pass by reference
  waitToTriggerBuzzer(secondsToWait);
}

void waitForAlarm(){

  printf("Press button 1 to view the log.\n");
  while (!isObstacleDetected)
  {
    dimLed(0,50,250);
    writeString("ACTV");
    if(isButtonPressed(0)){
      _delay_ms(BUTTON_DELAY);
      printLog();
    }
  }

  //alarm triggered
  //pass by value
  triggerAlarm(alarm.seconds);
  
}

ISR(PCINT2_vect) {  
  if(isAlarmSet)isObstacleDetected = 1;
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


  printf("Uw huis, beveiligd!\n");

  alarm.seconds = secondsTillAlarm;
  alarm.passcode = &password[0];

  printf("Gelieve een combinatie van %d knoppen te drukken om het wachtwoord in te stellen\n",PASSWORD_LENGTH);
setPassword();

printPassword();

  printf("Wil je de tijdsduur tot de activatie van de alarm wijzigen? Momenteel heb je %d seconden.\n",SECONDS_TILL_ALARM);
  printf("Druk op\n \tknop 1\t:\tNee\n\tKnop 2\t:\tJa\n");

  waitForPotentioSetupWizard();

  if(shouldSetupSecondsTillAlarm==1)setupSecondsTillAlarm();

  printf("Your alarm is active.\n");



  //Activate alarm
  isAlarmSet = 1;
  waitForAlarm();







  printf("hello world");

  while (1) {
   continue;
   
  }
  

  return 0;
}