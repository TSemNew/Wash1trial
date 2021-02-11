
/// Washing Machine finsished Extended alarming device base on Adafruit Feather Sense
// SW Version : 0.1
// 

// Change History : 
// Rev 15: 
// Rev 16 & 17: 1. Change song from HappyBday to Lookin' Out My Backdoor. Change USer parameters for realistic washer usage. FIx adding time between song repeats.
// Rev 18: 1. Fixed song repeat bug. Tested, Appears to work. Working release.
// Rev 19 : 1. Adjusted detect thresholds to 10.3 and 10.7. BLueart print onlly max g level, so can plot 
// Rev 20: 1. Modify so can continue collecting Gforce data when washer is finished and playing song. 2. WasherOFF calibration porcess added with function DoWasherOffCalibration(). Change WasherStillTimeBeforeDeemedOff_sec from 120 to 360.
// Rev TBD: Attempt to do washer off g-force level calibration Holds down diable button for 5 seconds straight while washer off device , and use that as a more sensitive baseline for detecting washer activity. 

// To DOs: 
// 1. Add an autocalibration for the resting g-forcce when the washer is off
// 2. Verify if really need  FirstTimeCheckingStarted & FirstTimeFinishedStarted.
// 3.
#include <Arduino.h>

// Debugging 
#define NORMAL_TIMER_DURS  1 // uncomment this line  to set timer constants back to real time operation 
//#define DEBUG_TIMER_DURS  1 // uncomment this line out to shorten the timer constants to speed up real time eval
#define ENABLE_DEBUG1  1 // commnent this line out to turn off Serial.prints from debug1 

// #include header files
#include <Arduino.h>
#include <Adafruit_SensorLab.h>
//#include <Adafruit_LIS3MDL.h>  //magnetomter header
//#include <Adafruit_LSM6DS33.h> /// points to the Ada...DS33 header file
//#include <Adafruit_Sensor.h>
#include <bluefruit.h>

// Create a variable type called WasherState. Used to  make a variable called CurrentWasherState that defines the present state , so can printout for debugging
enum WasherState { Off , Active , Finished, Alarming,  SwitchCancelled, ButtonDisabled, Calibrating  } ; 
WasherState CurrentWasherState=Off; 

// Class calls ? What is the name of this group of items ??? 
//Adafruit_LIS3MDL lis3mdl;   // magnetometer
Adafruit_LSM6DS33 lsm6ds33; // accelerometer, gyroscope.
   // BLE Service
  BLEDfu  bledfu;  // OTA DFU service
  BLEDis  bledis;  // device information
  BLEUart bleuart; // uart over ble
  BLEBas  blebas;  // battery over ble

//Song Notes Data
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415 
//
#define NOTE_A4   440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988

#ifdef DEBUG_TIMER_DURS
#define WHOLE         300       // Length of time in milliseconds of a whole note (i.e. a full bar).// Orginal was 2200, not 1100
#define DHALF         225 //(uint16_t) WHOLE*0.75
#define HALF          150 //(uint16_t) WHOLE*0.5
#define DQUARTER      111 //(uint16_t) WHOLE*0.37
#define QUARTER       75   //(uint16_t) WHOLE*0.25
#define EIGHTH        38  //(uint16_t) QUARTER*0.5
#define EIGHTH_TRIPLE 12  //(uint16_t) QUARTER*0.3
#define SIXTEENTH     19  //(uint16_t) EIGHTH*0.5
#endif

#ifdef NORMAL_TIMER_DURS
#define WHOLE        1000 //(uint16_t) 900       // Length of time in milliseconds of a whole note (i.e. a full bar).// Orginal was 2200, not 1100
#define DHALF         750 //(uint16_t) WHOLE*0.75
#define HALF          500 //(uint16_t) WHOLE*0.5
#define DQUARTER      375 //(uint16_t) WHOLE*0.37
#define QUARTER       250 //(uint16_t) WHOLE*0.25
#define EIGHTH        125 //(uint16_t) QUARTER*0.5
#define EIGHTH_TRIPLE 41 //(uint16_t) QUARTER*0.3
#define SIXTEENTH     63 //(uint16_t) EIGHTH*0.5
#endif

//Lookin' Out My Backdoor song specific data
const uint8_t  NumNotesSong1=46; 
uint16_t HB_NoteFreq[]={NOTE_D4,NOTE_D4,NOTE_D4,NOTE_D4,   NOTE_E4,NOTE_E4,NOTE_D4,  NOTE_G4,NOTE_G4,NOTE_G4,NOTE_G4,  NOTE_B4,NOTE_G4,   NOTE_E5,NOTE_E5,NOTE_E5,  NOTE_D5,NOTE_B4,NOTE_G4,  440,440,NOTE_B4,         440,1,NOTE_D4,       NOTE_D4,NOTE_D4,NOTE_D4,NOTE_D4,  NOTE_E4,NOTE_D4, NOTE_G4,NOTE_G4,NOTE_G4,NOTE_G4,  NOTE_B4,NOTE_G4, NOTE_E5,NOTE_E5, NOTE_D5,NOTE_B4,NOTE_G4,  NOTE_B4,NOTE_B4,440,     NOTE_G4};
uint16_t HB_NoteDur[]={ QUARTER,QUARTER,QUARTER,QUARTER,   DQUARTER,EIGHTH,HALF,     QUARTER,QUARTER,QUARTER,QUARTER,   QUARTER,DHALF ,     QUARTER,HALF,QUARTER,    HALF,QUARTER,QUARTER,     HALF,QUARTER,QUARTER,    HALF,QUARTER,QUARTER,  QUARTER,QUARTER,QUARTER,QUARTER,  QUARTER,DHALF,   QUARTER,QUARTER,QUARTER,QUARTER,  QUARTER,DHALF,   HALF,HALF,       HALF, QUARTER,QUARTER,    QUARTER,HALF, QUARTER,    WHOLE };
uint8_t HB_NoteHold[]={ false,false,false,false,           false,false,false,        false,false,false,false,          false,false,         false,false,false,        false,false,false,         false,false,false,     false,false,false,     false,false,false,false,          false,false,     false,false,false,false,          false,false,     false,false,     false,false,false,        false,false,false,        false };

//Happy Birthday song specific data
//const uint8_t  NumNotesSong1=26; 
//uint16_t HB_NoteFreq[]={294,294,NOTE_E4, NOTE_D4,NOTE_G4,NOTE_FS4,  NOTE_D4, NOTE_D4, NOTE_E4, NOTE_D4, NOTE_A4,NOTE_G4,   NOTE_D4,NOTE_D4,NOTE_D5,NOTE_B4, NOTE_G4,NOTE_G4, NOTE_FS4,NOTE_E4,   NOTE_C5,NOTE_C5,NOTE_B4,NOTE_G4, NOTE_A4, NOTE_G4  };
//uint16_t HB_NoteDur[]={ EIGHTH,EIGHTH,QUARTER,QUARTER,QUARTER,HALF,  EIGHTH,EIGHTH,QUARTER,QUARTER,QUARTER,HALF,  EIGHTH,EIGHTH,QUARTER,QUARTER,EIGHTH,EIGHTH,QUARTER,HALF, EIGHTH,EIGHTH,QUARTER,QUARTER,QUARTER,WHOLE  };
//uint8_t HB_NoteHold[]={true,false,false,false,false,false,  true,false,false,false,false,false, true,false,false,false,false,false,false,  true,false,false,false,false,false };

// User Prefernece Adjustable Timing Constants defined:
#ifdef NORMAL_TIMER_DURS  
const uint16_t delay_betweenWasherStartChecks_sec=10 ; //60 sec default. 
const uint16_t delay_betweenWasherFinishChecks_sec=30 ; // 60 sec default.
const uint16_t WasherStillTimeBeforeDeemedOff_sec= 360 ;  // 120 default
const uint16_t TimeBetweenSongPlaying_sec= 420; // 120 default
const uint16_t delay_betweenAlarmEndToRestartWashOnChecking_sec= 120  ; // 120 default
const uint8_t MaxAlarmRepeats =20 ; // 20 default ( max =255)
#endif

#ifdef DEBUG_TIMER_DURS
const uint16_t delay_betweenWasherStartChecks_sec=5 ; //60 sec default. 
const uint16_t delay_betweenWasherFinishChecks_sec=5 ; // 60 sec default.
const uint16_t WasherStillTimeBeforeDeemedOff_sec= 10 ;  // 120 default
const uint16_t TimeBetweenSongPlaying_sec= 10; // 120 default
const uint16_t delay_betweenAlarmEndToRestartWashOnChecking_sec= 20  ; // 120 default
const uint8_t MaxAlarmRepeats =3 ; // 20 default ( max =255)                          
#endif 

// User  Adjustable Washer Characteristic Constants defined:
//const float WasherOffGForceLevel = 10.3;  // threshold of magnitude GForce when washer is off
float WasherOffGForceLevel =10; // this level is derived by reading the accel when the use initiates a calibration by placing the magnet on top when the washer  is off. Starting default set to 10.
// NOTE: a partially loaded spinning dryer next to the off washer caused the washerOff g's to increase by ~ +0.1 G's.  
const float WasherOnGForceLevel = (WasherOffGForceLevel +0.3);     ///threshold of magnitude GForce when washer is ON. Set ~0.3g above off so its sensstive to light cycles. // This varies per machine and might have to be adjusted by user.

// System fixed Constants defined: 
const uint8_t IO_AlarmOffButton = 9; // digital pin 9 used for Alarm off button. Momentary ON. External 10k pullup to 3v3 , with an series 330Ohm into pin for debounce (RC).
const uint8_t SpeakerIO=11;   // digital pin D11 used for speaker output generator signal to Amp
const uint8_t IO_DisableSwitch = 12; // digital pin 12 used for Alarm Disable slide switch. External 10k pullup to 3v3.
    //const uint16_t IntervalToCheckWasherState_sec= 60 ;
    //const uint16_t WasherFinishedLoopDelay_sec= 60 ; 
// const uint16_t WasherOffLoopDelay_sec= 5 ; // 60 default

// Global Variables ( defined outside of setup(), main() , and functions()
float Debug_accel_mag=0; // used to capture g-force data during any interbval over BLEuart for threshold adjustment

//float accel_x, accel_y, accel_z , accel_mag; /// varaiables used for acceleration sample, 
//uint16_t WasherStillTime=0;
boolean WasherStarted=false ; boolean WasherOff=true ; 
boolean WasherFinished=false;
boolean AlarmOffButtonPressed = false;
boolean ExceededAlarmRepeats =false;
boolean AlarmCancelled=false;
boolean Disabled=false; 
boolean FirstTimeCheckingStarted =true ; 
boolean FirstTimeCheckingFinished =true ;
boolean ToneWasAlreadyMadeWhenDisableSwitchActivated=false ; 

// Counters
uint8_t n=0;  // note counter
uint8_t AlarmRepeats=0 ;  // counter for number of time the alarm song has played
unsigned long previousMillis =0; 
unsigned long currentMillis =0; 
unsigned long PreviousSongStartTime=0; 
unsigned long DebugStoreMillis=0; 

//FUNCTION declarations. Definition should be after main. Function declaration format : ReturnType FunctionName ( PassedParameterType PassedParameterName , " ) ; 
  boolean CheckIfWasherStarted();
  boolean CheckIfWasherOff();
  boolean CheckIfWasherFinished ();
  boolean CheckIfAlarmOffButtonPressed (); 
  boolean CheckIfExceededAlarmRepeats (uint8_t); 
  boolean CheckIfSwitchDisabled();
  void PlayNextNote (int frequency , int duration , bool hold=false) ; 
  //void PlayNextNote (int frequency [], int duration [], bool hold=false) ; 
  void StopAlarm();
  void DelayWhileCheckingButtonPress(uint16_t myDelaySec );
  float GetMaxAccelOf10Samples();
  float GetAvgWasherOffGforceLevel(); 
  void DoWasherOffCalibration() ; 
  
  //void prph_connect_callback(); 
  //void prph_disconnect_callback();
  //void startAdv(void);

   

////////////////////////////////////////////////////////////////////////////////////////
void setup() {          // put your setup code here, to run once:
  void startAdv(void);
  
  Serial.begin(115200);
    
  //lis3mdl.begin_I2C(); // initialize the mag sensor
  lsm6ds33.begin_I2C(); // initialize the accel sensor

   //#define RedLED D13 // my macro to define the status LED pin 
  pinMode(LED_BUILTIN , OUTPUT) ; /// my setting of D13 for the onboard RED LED as an output pin. 
  pinMode(SpeakerIO , OUTPUT) ; /// my setting of pin D11 as an output pin for the Tune driver to Amp input. 
  pinMode(IO_AlarmOffButton , INPUT) ; /// use digital IO pin # X with an external pullup & debounce RC to detect if button pressed.
  pinMode(IO_DisableSwitch , INPUT) ; /// use digital IO pin # 12 with an external pullup & debounce RC to detect if button pressed.
  // setup ADC module in nRF5240 for Battery voltage measurement
  analogReference(AR_INTERNAL_3_0);  /// set up the reference as the internal ref choice.  This sets ref to (0.6V Ref * 5 = 0..3.0V)
  analogReadResolution(12);  delay(2);   // Set to 12 bit. Can be 8, 10, 12 or 14. 

  Bluefruit.autoConnLed(true);
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.begin();
  Bluefruit.setTxPower(8); 
  Bluefruit.setName("BluefruitWasherApp"); //Bluefruit.setName("Bluefruit52");
  //Bluefruit.Periph.setConnectCallback(prph_connect_callback);  // commented out since continues to create compile errors. Need ? 
  //Bluefruit.Periph.setDisconnectCallback(prph_disconnect_callback); // commented out since continues to create compile errors. Need ? 
  bledfu.begin();
   // Configure and Start Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather52");
  bledis.begin();

  // Configure and Start BLE Uart Service
  bleuart.begin();

  // Start BLE Battery Service
  blebas.begin();
  blebas.write(100);

  // Set up and start advertising
  startAdv();
  Serial.println("Please use Adafruit's Bluefruit LE app to connect in UART mode");
  Serial.println("Once connected, enter character(s) that you wish to send");
  delay (1000);

}
/////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
void loop() 
{
  // auto calibrate function here, to determine the WasherOff G force. Use this to adjust the parameter "WasherOffGForceLevel" on the fly ( drift over time or installation )!!! Consider impact if dryer is rumbling next to washer while washer is off. 

  // millis() rollover prevention function here : Not neccessary , since both current & prevMillis are usigned long , they both roll over together at ~ 50 days, and at the boundary when currentM=1  and preM=~4.3b, still (prevM-currentM)will = 2. 
  Disabled=CheckIfSwitchDisabled();

  currentMillis = millis(); // get current timer value , on first time thru loop, and store it in  currentMillis
      //Serial.print("  CurrentMillis=");Serial.print( currentMillis);Serial.print("  CurrentMillis- previousMillis=");Serial.println( currentMillis- previousMillis);
  
  while ( (!WasherStarted || FirstTimeCheckingStarted) && !Disabled )  // check if washer started  once every "delay_betweenWasherStartChecks_sec"   minutes. Except if disable switched. THIS IS BLOCKING, ( stays stuck in this loop until washer starts)
  { 
    AlarmOffButtonPressed =CheckIfAlarmOffButtonPressed ();
    if (AlarmOffButtonPressed) { DoWasherOffCalibration(); } 
     //Serial.println("...in  while ( (!WasherStarted || FirstTimeCheckingStarted)  ... loop");
    currentMillis = millis(); // get current timer value , and store it in  currentMillis and froce to be unsigned long
    
    //Serial.print(" currentMillis)=");Serial.print(currentMillis); Serial.print(" previousMillis)=");Serial.print(previousMillis);
     //Serial.print(" (currentMillis- previousMillis)=");Serial.print((currentMillis- previousMillis)); 
    if ( (currentMillis- previousMillis) >= delay_betweenWasherStartChecks_sec*1000)
    {  //Serial.println("...( (currentMillis- previousMillis) WAS >= delay_betweenWasherStartChecks_sec*1000)");
     WasherStarted=CheckIfWasherStarted();
      Serial.println("***************************************************************************");
      Serial.print("(IN  if ( (currentMillis- previousMillis) >= delay_betweenWasherStartChecks_");Serial.print( currentMillis- previousMillis);Serial.print("  delay_betweenWasherStartChecks_sec*1000=");Serial.print(delay_betweenWasherStartChecks_sec*1000);
     previousMillis=currentMillis;
      Serial.print("previousMillis)=");Serial.println (previousMillis);

       
                           #ifdef ENABLE_DEBUG1
                           Serial.print ( " Checked if washer started :  WasherStarted= " );Serial.println (  WasherStarted );
                           Serial.println("***************************************************************************"); 
                           #endif 
     FirstTimeCheckingStarted =false ;
     //bleuart.print("CheckIfWasherStarted");
     }  /// end  if ( (currentMillis- previousMillis)
     Disabled=CheckIfSwitchDisabled(); 
  } /// end    while ( !WasherStarted || FirstTimeCheckingStarted                

                           
  while (  (!WasherFinished || FirstTimeCheckingFinished ) && !Disabled    )   // THIS IS BLOCKING ( stays stuck in this loop until washer finishes) 
  {
        //Serial.println ("  while: (!WasherFinished && (   (( currentMillis- previousMillis) >= delay_betweenWasherFinishChecks_sec*1000)|| FirstTimeCheckingFinished )    ) Was TRUE"  ); 
     currentMillis = millis(); // get current timer value , and store it in  currentMillis
        //Serial.print("(IN while (WasherStarted): currentMillis- previousMillis)=");Serial.print( currentMillis- previousMillis);Serial.print("  delay_betweenWasherFinishChecks_sec*1000=");Serial.println(delay_betweenWasherFinishChecks_sec*1000);                    
     if ( ( currentMillis- previousMillis) >= delay_betweenWasherFinishChecks_sec*1000)    // check if washer finished once every "delay_betweenWasherStartChecks_sec"   minutes
     {
      WasherFinished=CheckIfWasherFinished ();
      // WasherStillTimeBeforeDeemedOff_sec
                          #ifdef ENABLE_DEBUG1
                          Serial.print ( " Checked if washer finished : returned, WasherFinished= " );Serial.println (  WasherFinished ); //WasherOff=CheckIfWasherOff();  // ? not need this function
                          #endif 
      previousMillis=currentMillis;  
      //bleuart.print("Checking If Washer finished. "); 
     }
     FirstTimeCheckingFinished=false ;
     Disabled=CheckIfSwitchDisabled();
  } // end : while (!WasherFinished && (   (( currentMillis- previousMi

  
  while ( WasherFinished && !AlarmOffButtonPressed  && !ExceededAlarmRepeats && !Disabled  )  
  {    
        Serial.println ( " Start Into :  while ( WasherFinished && !AlarmOffButtonPressed  && !ExceededAlarmRepeats && !Disabled  )" );
  Serial.print("WasherFinished=");Serial.print( WasherFinished);Serial.print(" !AlarmOffButtonPressed=");Serial.print(!AlarmOffButtonPressed );Serial.print(" !ExceededAlarmRepeats=");Serial.print(!ExceededAlarmRepeats);Serial.print(" !Disabled=");Serial.println(!Disabled );
          if ( millis() > (DebugStoreMillis + 2000)) { Debug_accel_mag = GetMaxAccelOf10Samples(); delay(5); DebugStoreMillis=millis(); } /// continue collecting G-force samples at 2 sec rate during interval after decided WasherFinsihed, so can monitor on BLEuart g-force levels for debug & threshold calibration
          //bleuart.println("Washer finished and && !AlarmOffButtonPressed  && !ExceededAlarmRepeats && !Disabled ");
          //while(!AlarmCancelled &&  (!AlarmOffButtonPressed || !ExceededAlarmRepeats) )
          //{
         //Serial.println ( " Into : while(!AlarmCancelled &&  (!AlarmOffButtonPressed || !ExceededAlarmRepeats) )" );
    AlarmOffButtonPressed = CheckIfAlarmOffButtonPressed();  // check if user requested to turn off the alarm after the Washer finished, but before the AlarmsStarted
    Disabled=CheckIfSwitchDisabled();
      Serial.print("(millis()-PreviousSongStartTime)=" ); Serial.print((millis()-PreviousSongStartTime) );Serial.print(" millis=" );Serial.print(millis() ); Serial.print(" PreviousSongStartTime=") ;Serial.println(PreviousSongStartTime) ;
      //bleuart.print("(millis()-PreviousSongStartTime)=" ); bleuart.println((millis()-PreviousSongStartTime) ); 
    if ( (millis()-PreviousSongStartTime) > (TimeBetweenSongPlaying_sec*1000) )
    {     
      for (n=0; n<(NumNotesSong1); n++)      /// play song 1 time, checking for button press during song
      {     if ( n==0){ Serial.println("Song started..................." );  } 
        //PlayNextNote ( HB_NoteFreq, HB_NoteDur, HB_NoteHold ); // This is blocking during each note , and can't hear button press until the end of the note.
        PlayNextNote ( HB_NoteFreq[n],  HB_NoteDur[n], HB_NoteHold [n]);
        delay(5);
        AlarmOffButtonPressed = CheckIfAlarmOffButtonPressed(); // check for alarm off button press between each note
        Disabled=CheckIfSwitchDisabled();                       // check is disableswitch  activated between each note   
        if ( (n==11)||(n==21) ) { Debug_accel_mag = GetMaxAccelOf10Samples();  } /// continue collecting G-force samples, during interval when these notes are being played
        if ( AlarmOffButtonPressed==1 || Disabled==1 ) // active low signal
        { AlarmCancelled= true;
                          #ifdef ENABLE_DEBUG1
                          Serial.print ( "************************AlarmCancelled=  " );Serial.println ( AlarmCancelled  ); 
                          #endif  
           n=NumNotesSong1;  // set "n" to pass last note, so will stop playing notes immeadiately
           ExceededAlarmRepeats= true;  /// set to true so will stop playing itemrations of songs
           tone ( SpeakerIO, 2000, 3000) ; //  A auditory feedback when pressing the Alarm Cancell button. (pin , freq, duration msec)
           noTone ( SpeakerIO) ; // turn off speaker
         }  // end if ( AlarmOffB || ExceededAlarmRepeats) 
         if ( n==(NumNotesSong1-1))
         { Serial.print("Song completed..........." ); Serial.print("millis()=" );Serial.println(millis() );    
              PreviousSongStartTime=millis();  /// store present time once  the song is  finsihed
              AlarmRepeats ++ ;Serial.println(" AlarmRepeats was incremented after song");
          }             
       } /// end for (n=0: n< ...     /// finished playing song
      
      }  // end if (millis()
          
      //if (!AlarmOffButtonPressed && !Disabled) {AlarmRepeats ++ ;Serial.print("if()Incremented AlarmRepeats=");Serial.print( AlarmRepeats); } // After playing the song , increment the counter that tracks how many time the song has played. Unless the alarm button of max songs occured.                     
     ExceededAlarmRepeats= CheckIfExceededAlarmRepeats ( AlarmRepeats );     
          //currentMillis = millis(); // timer snapshot for while loop start 
     if ( ExceededAlarmRepeats==true ) {Serial.println ( "ExceededAlarmRepeats :End of Alarm " );} 
      Serial.println ( " Reached end :  while ( WasherFinished && !AlarmOffButtonPressed  && !ExceededAlarmRepeats && !Disabled  )" );
      Serial.print("WasherFinished=");Serial.print( WasherFinished);Serial.print(" !AlarmOffButtonPressed=");Serial.print(!AlarmOffButtonPressed );Serial.print(" !ExceededAlarmRepeats=");Serial.print(!ExceededAlarmRepeats);Serial.print(" !Disabled=");Serial.println(!Disabled );

   } //end while ( WasherFinished && !AlarmOffButtonPressed &&
     
  //   // RESET all  , so can start checking for washer runnning again.   
  WasherStarted=false ;  WasherFinished=false ;  AlarmOffButtonPressed=false; ExceededAlarmRepeats=false;  AlarmCancelled=false; FirstTimeCheckingStarted=true ; FirstTimeCheckingFinished=true ; //boolean WasherOff=true ;// RESET all  
  AlarmRepeats=0; 
  
  delay (delay_betweenAlarmEndToRestartWashOnChecking_sec*1000); // hard blocking delay where all other input is ignored. Don't want a fasle alarm for taking out clothes bumping device. OK ??
  

} // end void loop()
                                
  
/////////////////////////////////////////////////////////////////

///FUNCTION DEFINITIONS
float GetMaxAccelOf10Samples() {

  float accel_x, accel_y, accel_z , accel_mag=0, Getmax_accel_mag=0;
  uint8_t i=0;
                          #ifdef ENABLE_DEBUG1
                          //Serial.println ( "Entered function: GetMaxAccelOf10Sample " );
                          #endif  
  for ( i=0; i<10 ; i++){  // get 10 accel mag samples, and store the greatest  & see if greater than the threshold
    sensors_event_t accel;  // wakeup event from acceleraometer ?? 
    sensors_event_t gyro;
    sensors_event_t temp;
    lsm6ds33.getEvent(&accel, &gyro, &temp); // this points to the attribute "get event" in the defined class lsm6ds33. What does it do ?
    accel_x = accel.acceleration.x;  accel_y = accel.acceleration.y;  accel_z = accel.acceleration.z;
    accel_mag= sqrt( accel_x*accel_x + accel_y*accel_y + accel_z*accel_z); /// calc the magnitude for the sampled accel vector , independent of direction, using Pythagorean .
    delay (20);//******** EXECUTION GETS STUCK HERE  unless add this delay !!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //Serial.print ( " In loop ofGAM"); Serial.print(" accel_mag="); Serial.print(accel_mag);Serial.print("   Getmax_accel_mag="); Serial.println(Getmax_accel_mag);/// serial prints. WHY ?? 
    if ( accel_mag> Getmax_accel_mag)  
      { Getmax_accel_mag=accel_mag; delay (10); 
      }
    else delay (10); 
    //Serial.println("Accel_mag WAS> Getmax_accel_mag");   } 
  }
                          #ifdef ENABLE_DEBUG1
                          Serial.print ( "Returned value will be :Getmax_accel_mag   =  " );Serial.println ( Getmax_accel_mag );
                          #endif
        bleuart.print(Getmax_accel_mag );bleuart.print("," );bleuart.print(9); bleuart.println( ); //bleuart.print(" Getmax_accel_mag=" ); bleuart.print(Getmax_accel_mag );
  return ( Getmax_accel_mag); 
}

boolean CheckIfSwitchDisabled()
{
  // read the value on an IO pin to see if it is LOW
   uint8_t SwitchRead; 
                          #ifdef ENABLE_DEBUG1
                          //Serial.println ( "Entered function: CheckIfSwitchDisabled" );
                          #endif  
   SwitchRead=digitalRead(IO_DisableSwitch);
   //Serial.print ( " SwitchRead=" ); Serial.print ( SwitchRead );
   if (SwitchRead==0)
   {  
      if ( !ToneWasAlreadyMadeWhenDisableSwitchActivated) // Only produce Tone when activating Disable switch once  
      {
       tone ( SpeakerIO, 1500, 1000) ;   // give some sound feedback tone if user pushes to AlarmOff button
       digitalWrite(LED_BUILTIN, HIGH); // turn on onboard LED when the disabale switch is active
       ToneWasAlreadyMadeWhenDisableSwitchActivated= true; 
      }
                          #ifdef ENABLE_DEBUG1
                          Serial.println ( "Disable Alarm switch was turned on " );
                          #endif
        
   return (true);
   }
    //Serial.println ( "Switch NOT in disable position " );
    ToneWasAlreadyMadeWhenDisableSwitchActivated= false;  // reset this so next time DisableSwitch activated, it sounds
    digitalWrite(LED_BUILTIN, LOW); // turn OFF onboard LED when disable switch is deactivated
   return (false); // if button not pressed  
}

boolean CheckIfWasherStarted()
{ 
  float ONmax_accel_mag=0; 
                          #ifdef ENABLE_DEBUG1
                          Serial.println ( "Entered function: CheckIfWasherStarted() " );
                          #endif  
  ONmax_accel_mag=GetMaxAccelOf10Samples();
                          #ifdef ENABLE_DEBUG1
                          Serial.print ( "In function CheckIfWasherStarted() & called function: GetMaxAccelOf10Samples).Then ONmax_accel_mag=" );Serial.print (ONmax_accel_mag);  
                          Serial.print ( "And WasherOnGForceLevel= " );Serial.println (WasherOnGForceLevel);
                          #endif 
  if (ONmax_accel_mag > WasherOnGForceLevel  ) 
  { 
                          #ifdef ENABLE_DEBUG1
                          Serial.println ( "THe max_accel_mag  was > WasherOnGForceLevel " );
                          #endif 
     return (true) ; // Washer is ON  
  } 
  else
  {
                          #ifdef ENABLE_DEBUG1
                          Serial.println ( "The max_accel_mag  was<WasherOnGForceLevel " );
                          #endif 
  return (false) ;  // Washer is NOT on ( anymore) 
  } 
}

//boolean CheckIfWasherOff()  // NOT used
//{return (false ) ;  }

boolean CheckIfWasherFinished ()    /// NOTE : this is blocking process , and will not do other functions when in this functions !!!!! OK ? 
{
  float Fmax_accel_mag=0, sum_max_accels=0 , avg_max_accels=0;
  uint8_t j=0 ;
  uint8_t SecondsBetweenSamples = 1; // default 10 ? 
                          #ifdef ENABLE_DEBUG1
                          Serial.println ( "Entered function: CheckIfWasherFinished " );
                          #endif  
  for ( j=0; j< (WasherStillTimeBeforeDeemedOff_sec/SecondsBetweenSamples) ; j++)
  {
   Fmax_accel_mag = GetMaxAccelOf10Samples();
   sum_max_accels = sum_max_accels + Fmax_accel_mag ; 
   Serial.print(sum_max_accels);Serial.println(" ");
   delay ( 1000 * SecondsBetweenSamples);   // This is a BLOCKING delay. If push AlarmOff button during this interval , then it will not be seen. 
  }
  Serial.print(" (WasherStillTimeBeforeDeemedOff_sec/SecondsBetweenSamples) = " );Serial.println(WasherStillTimeBeforeDeemedOff_sec/SecondsBetweenSamples);
  avg_max_accels = sum_max_accels /(WasherStillTimeBeforeDeemedOff_sec/SecondsBetweenSamples) ;
  Serial.print(" Calc  avg_max_accels  =");Serial.println(avg_max_accels); 

                          #ifdef ENABLE_DEBUG1
 Serial.print("WasherStillTimeBeforeDeemedOff check: avg_max_accels was=");Serial.print(avg_max_accels);Serial.print (" WasherOffGForceLevel="); Serial.print (WasherOffGForceLevel);Serial.print (" true?= ");Serial.println (avg_max_accels < WasherOffGForceLevel);
                          #endif
  if (avg_max_accels < (WasherOffGForceLevel+0.15 )  )  // Added 0.15 for some hysteresis above the noise
  { 
                          #ifdef ENABLE_DEBUG1
                          Serial.print ( "avg_max_accels was < WasherOffGForceLevel  " );Serial.println ( "  Washer IS finihsed. " );
                          #endif
     WasherStarted=false ;   /// set global WasherStarted varaible to false , once washer cycle if finished.
     return (true) ;   // Washer deeemed off
  }
  else if (avg_max_accels >= (WasherOffGForceLevel+0.15 )  )
  {
                          #ifdef ENABLE_DEBUG1
                          Serial.println ( "Washer NOT finished. Return " );
                          #endif                        
   return (false) ;  // Washer deeemed still on
  }
  return (false);
}  // end function 

boolean CheckIfAlarmOffButtonPressed ()
 {
  // read the value on an IO pin to see if it is LOW
   uint8_t ButtonPressRead; 
                          #ifdef ENABLE_DEBUG1
                          Serial.print ( "Entered function: CheckIfAlarmOffButtonPressed" );
                          #endif  
   ButtonPressRead=digitalRead(IO_AlarmOffButton);
   //Serial.print ( "ButtonPressRead=" ); Serial.println ( ButtonPressRead );
   if (ButtonPressRead==0) // active low press
   {
       tone ( SpeakerIO, 2000, 1000) ;   // give some sound feedback tone if user pushes to AlarmOff button
                          #ifdef ENABLE_DEBUG1
                          Serial.println ( "AlarmOFF button was pressed " );
                          #endif  
   return (true);
   }
    Serial.println ( "AlarmOFF button was NOT pressed " );
   return (false); // if button not pressed
}

boolean  CheckIfExceededAlarmRepeats (uint8_t AlarmRepeats) {
                          #ifdef ENABLE_DEBUG1
                          boolean debug_test=(   AlarmRepeats >= MaxAlarmRepeats );
                          Serial.print ( "Entered function: CheckIfExceededAlarmRepeats " );
                          Serial.print ( "AlarmRepeats value=" );Serial.print ( AlarmRepeats );Serial.print ( "MaxAlarmRepeats=" );Serial.println ( MaxAlarmRepeats );
                          Serial.print ( "(  AlarmRepeats >= MaxAlarmRepeats)=" );Serial.println (  debug_test);
                          #endif 
  if (  AlarmRepeats >= MaxAlarmRepeats) 
  { Serial.print( "AlarmRepeats=" ); Serial.print( AlarmRepeats ); Serial.println( "  TRUE" );
    return( true); 
  } 
  else 
  {Serial.print( "AlarmRepeats=" ); Serial.println( AlarmRepeats );
  return (false);
  }
}

void PlayNextNote (uint16_t note, uint16_t  duration, uint8_t  holdNote) {
                          #ifdef ENABLE_DEBUG1
                          Serial.print ( "Entered function: PlayNextNote >" );
                          #endif  
  if (holdNote) {   tone (SpeakerIO, note, duration + duration/32);    }
  else { tone (SpeakerIO, note, duration );    }
  delay(duration + duration/16);
  Serial.print ( " note="); Serial.print(note); Serial.print(" duration="); Serial.println(duration);
  return;
 }


float GetAvgWasherOffGforceLevel()
{         // take average of 30 samples over a 3 second interval and make that the baseline WasherOff g force level
float sum=0; 
  for (uint8_t i=0; i< 30; i++)
  {
    sum= sum+ GetMaxAccelOf10Samples(); 
    //delay (100);
    tone (SpeakerIO,800,25);tone (SpeakerIO,1600,25);delay(50); noTone ( SpeakerIO) ;
  }

  return(sum/30 ) ;  
}

void DoWasherOffCalibration()    // function starts if button pressed while washer is off. Abort unless hold button longer than 1 second 
// Then waits until the disable switch is slid to OFF position AND the disable button released before starting cal. ( Tsis is needded so user finger not touching and vibrating the device during cal)  
//
{
  boolean firstCheck=false ; boolean secondCheck=false ; boolean DisableSwitchSet=false ;
  firstCheck= CheckIfAlarmOffButtonPressed (); 
  delay ( 1000); 
  secondCheck= CheckIfAlarmOffButtonPressed ();  /// use second check for debounce and accidental lockout
  if ( firstCheck && secondCheck ) 
  {
    Serial.println ( "firstCheck && secondCheck both low detected..." );
    for (int i=1;i<6  ; i++)     // check for 6 seconds if disable switch was slid to off
    {
      DisableSwitchSet=CheckIfSwitchDisabled();
      delay ( 1000); 
      Serial.print ( "Now Checking for when  DisableSwitchSet..." ); Serial.print ( i ); Serial.println ( "  th time." );
      if ( DisableSwitchSet== true) { Serial.println ( "DisableSwitch  IS Set..." ); }
    }
    delay (100); 
    if ( DisableSwitchSet== true) {
      WasherOffGForceLevel=GetAvgWasherOffGforceLevel();  // store this avg 5 second reading in the global variable WasherOffGForceLevel
      tone (SpeakerIO,400,1000);tone (SpeakerIO,800,1000);tone (SpeakerIO,1600,1000);tone (SpeakerIO,3200,1000);noTone ( SpeakerIO) ; // audible unique alarm after completed calibration measure and set
      Serial.print ( "WasherOffGForceLevel set during calibration was =" ); Serial.print( WasherOffGForceLevel) ; Serial.println("           wwwwwwwwwwwwwwwwwwwwww" ) ;Serial.println( ) ;Serial.println( ) ;Serial.println( ) ;
      delay  ( 1000);
    }

    /// sound annoying tone continuously until user slides  the disable switch back to off
    while ( DisableSwitchSet== true   )
    {
      tone ( SpeakerIO, 2500, 1000) ; //  A auditory feedback when pressing the Alarm Cancell button. (pin , freq, duration msec)
      delay(1000); noTone ( SpeakerIO) ; delay(1000);
      DisableSwitchSet=CheckIfSwitchDisabled();
      Serial.println ( "NEED to slide disable switch back NOW .... to rseume normal operation" ); 
    }
    
  }   /// end   if ( firstCheck && secondCheck ) 
  else { Serial.println (" FALSE PRESS. MUST HOLD button longer in order to initiate the calibration routine ***************************" );tone (SpeakerIO,100,1000);noTone ( SpeakerIO) ; }
   
}  // end function DoWasherOffCalibration() 

//BLE Function :   // ??? need to define ? 
void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  // Include bleuart 128-bit uuid
  Bluefruit.Advertising.addService(bleuart);
  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
} // end startAdv(void)


////Function :  callback invoked when periph  connects
void prph_connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);
  char peer_name[32] = { 0 };
  connection->getPeerName(peer_name, sizeof(peer_name));
  Serial.print("[Periph]Connected to ");
  Serial.println(peer_name);
}
//Function : 
void prph_disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;Serial.println();Serial.print("[Prph] Disconnected, reason = 0x"); Serial.println(reason, HEX);
}

 