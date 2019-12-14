/* 
 * Author: Austen Szypula 
 * 
 * Written for: Nitin Gadia
 * 
 * Project: Blutooth Enabled LED
 * 
 * *******************************************************************PLEASE READ BEFORE UPLOADING FOR THE FIRST TIME**************************************************************
 * The purpose of this sketch is to read input data on the Bluno "Beetle" BT-enabled microcontroller from a rotary encoder input (connected to pins D3 [encoder pin A] and D4 
 * [encoder pin B]) and serial data containing the changes in LED brightness level. The Bluno Beetle also drives a pwm output on pin D5 according the specified brightness settings. 
 * 
 * 
 * When configureing the Arduino IDE (v1.8.3+) to upload to the Bluno Beetle board, choose "Arduino/Genuino Uno" under Tools->Board and make sure the right COM port is selected under
 * Tools->Port. That's it.
 * 
 * Some tips for modifying the following code:
 * 
 *     - Change 'turnoffPwm' to control when the relay switch is activated.
 *     
 *     - Adjust pulse step size to change encoder sensitivity. i.e. increase int 'pulseStepSize1' or 'pulseStepSize2' (typical values (1-20) to make fine or coarse adjustments more or less 
 *       sensitive.
 *       
 *     - To create new serial data commands, add a case to the "Serial.available()" section according to the first letter of the command (i.e. case 'R': for Rqst command). If there 
 *       are multiple commands that start with the same letter. Use if(progressCluster.charAt(1) == secondLeter) within that case to diferentiate which command is intended. Note: 
 *       commands can be typed and sent in the app or sent automatically from within the app. Avoid commands that start with integers since those are intended for the default case.
 *       
 * *******************************************************************************************************************************************************************************************      
 * 
 */

#define inputA 3
#define inputB 4 
#define outputC 2

//STATE VARIABLES: program variables that don't have to be configured
int aState;
int aLastState;
int pwm;                               //0-255

int clockDivider = 100;                //Wanna execute every 100 cycles to keep computational power to a minimum. 
int clockDividerIndex = 100;           //Index to keep track of number of calls to Loop()
int speedIndex = 0;                    //Used to keep track of the time since the last encoder increase/decrease
int arrayLength = 4;                   //Take the average of four consecutive speed measurements
int speedIndexArray[4];                //Store the values in an int array
int avgSpeed = 0;                      //Note: larger values indicate slower rotational velocity because they have more time to grow between encoder increments
int thresholdSpeed = 5;                //Threshold for 'pulseStepSize1' and 'pulseStepSize2' (fine and coarse adjustments with the encoder)

//CONFIGURATION VARIABLES:             
int pulseStepSize1 = 1;                //Smaller step size for fine tuning
int pulseStepSize2 = 5;                //Normal step size for large adjustments
int turnoffPwm = 3;                    //Brightness setting when light comes on, changed in calibration settings to adjust brightness range

void setup() {

    pwm = 0;
    
    pinMode (inputA,INPUT);            //pinA on encoder
    pinMode (inputB,INPUT);            //pinB on encoder
    pinMode(outputC, OUTPUT);
    
    Serial.begin(115200);               //initialize the Serial
    
    aLastState = digitalRead(inputA);   //Reads the initial state of inputA
    pinMode(5, OUTPUT);                 //initialize the PWM

    
   
    //---------------------------------------------- Set PWM frequency for D5 -------------------------------
 
TCCR0B = TCCR0B & B11111000 | B00000001;    // set timer 0 divisor to     1 for PWM frequency of 62500.00 Hz                    //want max pwm frequency for low ripple dac conversion and quick response (min settling time w/ RC filter)            
//TCCR0B = TCCR0B & B11111000 | B00000010;    // set timer 0 divisor to     8 for PWM frequency of  7812.50 Hz
//TCCR0B = TCCR0B & B11111000 | B00000011;    // set timer 0 divisor to    64 for PWM frequency of   976.56 Hz (The DEFAULT)
//TCCR0B = TCCR0B & B11111000 | B00000100;    // set timer 0 divisor to   256 for PWM frequency of   244.14 Hz
//TCCR0B = TCCR0B & B11111000 | B00000101;    // set timer 0 divisor to  1024 for PWM frequency of    61.04 Hz
 

}

void loop()
{ /*
    if(pwm < 255){
      pwm+=5;                                     //PWM fade test
      analogWrite(5, pwm);                        //pwm on pin D5
      delay(4000);
      Serial.println("Encoder: +5");
    }else{
      pwm = 0;
      analogWrite(5, pwm);
      delay(100000);
      Serial.println("Encoder: 0");
    }                                              //Comment out everything after this line in loop() when testing pwm fade
    */
    //The on serial available command returns true when the connected device has sent data to the module. THis occurs when ...
    //    1)The device is first paired with module (case 'R'). Device sends "Rqst" automatically to request initial brightness and sync seekBar progress
    //    2)When seekBar progress changes in the app from user input (nothing is sent when progress changes from encoder input). This is the default case (starts with a number);
    //
    if(Serial.available())                                       
    {
        String progressCluster = Serial.readString();
        String str;
        
        String pwmString = String(pwm);
        
        switch(progressCluster.charAt(0)) {
          
          case 'R':
          str = "start1:"+pwmString;
          Serial.print(str);                                          /*Send starting brightness to connected device for module 1*/
          break;


          /* In the default case below, a string is sent from the paired device containg a cluster of 1-4 values per line in order depending on seekBar progress change speed. For example change 
           *  seekBar slowly and the incoming string looks like:
           *       51~
           *       53~
           *       57~
           *       58~
           *       61~
           *
           *  However, when it seekBar is moved quickly from one end to the other, the input stream will look something like:
           *       23~
           *       25~28~29~
           *       31~37~42~
           *       47~51~57~60~
           *       63~
           *
           * Using the '~' character as a delimiter, all changes are retained by parsing the input string by the '~' character and executing each value (up to maximum five values) one 
           * at a time. 
           * 
           */
           
          default:
             
            int index1 = 0;        //index1-5 holds the values of up to five '~' characters per line
            int index2 = 0;
            int index3 = 0;
            int index4 = 0;
            int index5 = 0;
            index1 = progressCluster.indexOf("~");
            if(index1 < progressCluster.length()-1){
              String sub1 = progressCluster.substring(index1+1);  
              index2 = sub1.indexOf("~") + index1 + 1;
            }
            if(index2 < progressCluster.length()-1 && index2 != 0){
              String sub2 = progressCluster.substring(index2+1);
              index3 = sub2.indexOf("~") + index2 + 1;
            }
            if(index3 < progressCluster.length()-1 && index3 != 0){
              String sub3 = progressCluster.substring(index3+1);
              index4 = sub3.indexOf("~") + index3 + 1;
            }
            if(index4 < progressCluster.length()-1 && index4 != 0){
              String sub4 = progressCluster.substring(index4+1);
              index5 = sub4.indexOf("~") + index4 + 1;
            }
            
            int a1, a2, a3, a4, a5 = -1;                                   //a1-5 is the pwm sequence to execute. Only executes if a(i) does not equal -1.
    
            a1 = progressCluster.substring(0,index1).toInt();
            pwm = a1;                                           //usable pwm range for module: turnoffPwm to 255
            Serial.print(a1);
            Serial.print("\n");
            
            if(index2!=0){
              a2 = progressCluster.substring(index1+1,index2).toInt();
              pwm = a2; 
              Serial.print(a2);
              Serial.print("\n");
            }
            
            if(index3!=0){
              a3 = progressCluster.substring(index2+1,index3).toInt();
              pwm = a3; 
              Serial.print("A: " + a3);
              Serial.print("\n");
            }
            
            if(index4!=0){
              a4 = progressCluster.substring(index3+1,index4).toInt();
              pwm = a4;     
              Serial.print(a4);
              Serial.print("\n");
            }
            
            if(index5!=0){
              a5 = progressCluster.substring(index4+1,index5).toInt();
              pwm = a5; 
              Serial.print(a5);
              Serial.print("\n");
            }

            /*
            Serial.print(index1);                             //used in debugging the '~' character indices. No longer needed since its working, but might be good to have
            Serial.print(index2);
            Serial.print(index3);
            Serial.print(index4);
            Serial.println(index5);
            
            Serial.println();
            Serial.println(progressCluster);
            Serial.println();
            */
          break;
        }       
    }

    /////////////////////////////////////////////Encoder Reading/////////////////////////////////////////////////        
    aState = digitalRead(inputA); // Reads the "current" state of the inputA
    // If the previous and the current state of the inputA are different, that means a Pulse has occured
    if (aState != aLastState){     
      // If the inputB state is different to the inputA state, that means the encoder is rotating clockwise
      if (digitalRead(inputB) != aState) { 
        if(pwm + pulseStepSize2 > 255){
          if(pwm + pulseStepSize1 <= 255){
            pwm = pwm + pulseStepSize1;
          }else{
            pwm = 255;
          }
          String pwmString1 = String(pwm);
          String upString = "E: up:" + pwmString1;
          Serial.println(upString);
        }else{
          
          ///////////////////////////////////////////ENCODER rotating CLOCKWISE///////////////////////////////////////////////////////////////////
          for(int i = 0; i<arrayLength - 1; i++){                //rotate newest speed index into the array
            speedIndexArray[i] = speedIndexArray[i+1];
          }
          speedIndexArray[arrayLength - 1] = speedIndex;
          int sum = 0;
          for(int i = 0; i< arrayLength; i++){
            sum += speedIndexArray[i];
          }
          avgSpeed = sum / arrayLength;                          //compute new average speed index (lower the number, faster the rotational velocity)          
          //Serial.println(avgSpeed);
          if(avgSpeed > thresholdSpeed){
            pwm = pwm + pulseStepSize1;                          //if avg. speed index is greater than threshold ("slow" turning speed), increase by smaller step size
          }else{
            pwm = pwm + pulseStepSize2;                          //else increase by larger step size
          }
          speedIndex = 0;
          /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          
          String pwmString1 = String(pwm);
          String upString = "E: up:" + pwmString1;               //Must match onSerialReceived method from MainActivity.java class in Android Studio project.               
          Serial.println(upString);                              //tells the device to adjust seekBar progress to match the new pwm value. Note: step size isnt always +5, but the paired device detects
                                                                 //either the "E: up:" or "E: dn:" string to tell it which direction to adjust the seekBar.  
                                                                 
        }
      }else{  //the encoder is rotating counterclockwise
                                                                 
            if(pwm - pulseStepSize2 < 0){
              if(pwm - pulseStepSize1 >= 0){
                pwm = pwm - pulseStepSize1;
              }else{
                pwm = 0;
              }
              String pwmString2 = String(pwm);
              String downString = "E: dn:" + pwmString2;
              Serial.println(downString);
            }else{
              
              ///////////////////////////////////////////ENCODER rotating COUNTERCLOCKWISE////////////////////////////////////////////////////
              for(int i = 0; i<arrayLength - 1; i++){               //rotate newest speed index into the array
                speedIndexArray[i] = speedIndexArray[i+1];
              }
              speedIndexArray[arrayLength - 1] = speedIndex;
              int sum = 0;
              for(int i = 0; i< arrayLength; i++){
                sum += speedIndexArray[i]; 
              }
              avgSpeed = sum / arrayLength;                        //compute new average speed index (lower the number, faster the rotational velocity)
              //Serial.println(avgSpeed);
              if(avgSpeed > thresholdSpeed){                       //if avg. speed index is greater than threshold ("slow" turning speed), decrease by smaller step size
                pwm = pwm - pulseStepSize1;
              }else{
                pwm = pwm - pulseStepSize2;                        //else decrease by normal step size
              }
              speedIndex = 0;                                      //reset speed index to zero
              //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
              
              String pwmString2 = String(pwm);
              String downString = "E: dn:" + pwmString2;         //Same as 'upString' comment
              Serial.println(downString);
            }
        
      }
    
      //Serial.print("Position: ");
      //Serial.println(pwm);
    
      aLastState = aState; // Updates the previous state of inputA with the current state
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }
    delay(4);                           //not sure if this delay does anything, but it's probably good to limit the frequency at which the PWM setting is updated and therefore, how fast
                                        //loop() is repeated

    if(pwm>12){                            //NPF driver flickers when turned on between 5-9 so this code was added                     
      analogWrite(5, pwm);                //writes the updated PWM setting to digital pin 5
    }else{
      analogWrite(5, 0);                 
    }

    //////////////////////////////////////////////////////////
    if(clockDividerIndex != 0){
      clockDividerIndex--;
    }else{                              //added v2a
      clockDividerIndex = clockDivider;
      if(speedIndex < clockDivider){
        speedIndex++;
      }
    }
    //Serial.println(speedIndex);
    ///////////////////////////////////////////////////////////
    //Serial.println(pwm);
    
    /*********************************************************************FROM VERSION 2****************************************************************
    if(pwm <= turnoffPwm){             //if pwm is less than turnoff, drives output low. This switches the relay off and open circuits the L wire.          
        digitalWrite(outputC, LOW);    //relay off
      }else{
        digitalWrite(outputC, HIGH);   //relay on
      }
    ***************************************************************************************************************************************************/
   
}
