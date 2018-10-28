
/*
  TURN-TABLE  
  Stepper Motor Speed Control - one revolution in an predifined time
  This program drives the Unipolar Stepper Motor 28-BYJ48.
  The motor will step one step at a time, very slowly.

  GUI-Panel:
                        Button
                        +10min
  Button     Button              Button    Button
    GO     Grundwerte             r / l     RESET
                        Button
                        + 1min

  Created 08.08.2018
  by Rainer Pospich

*/

#include <Stepper.h>
#include <LiquidCrystal.h>

// Motor pin definitions
#define motorPin1  13    // IN1 on the ULN2003 driver 1
#define motorPin2  12    // IN2 on the ULN2003 driver 1
#define motorPin3  11    // IN3 on the ULN2003 driver 1
#define motorPin4  3     // IN4 on the ULN2003 driver 1


// global var's

int progStatus = 1;        // Programmstatus setzen  1= Eingabemodus  2=GO
float minuten = 1;           // Einstellwert für die Mintuen (1-180min erlaubt)
int stepperDirection = -1;  // Drehrichtung mit dem uhrzeigersinn
int stepperRawDirection = 0;  // Drehrichtung gegen den uhrzeigersinn
int stepperInf = 0;        // Dreh dich nicht unendlich lange
int is_minute = 0;        // 0: time in minutes, 1: time in seconds, 2: time in hours

unsigned long startMillis;    // Timer0- variable
unsigned long currentMillis;  // Timer0- variable
bool stepperStop = false;     // flag for: one revolution ist complete

float period = 29;   //milliseconds / every step
// 1000 millisec/Schritt = 1000 * 2050 = 2050.000 sec/Um
//   29                  = 29   * 2050 =   59.450 = 1min
//  293                  = 293  * 2050 =  600.650 = 10min
// 1756                  = 1756 * 2050 = 3599.800 = 1h
// 3512                  = 3512 * 2050 = 7199.600 = 2h
//    3                  = 5268 * 2050 =10799.400 = 3h

// Einstellung in der GUI: von 1min - 180min
// --> Formel:  minuten *60 *1000 /stepsPerRevolution
// -->       :  minuten * formelWert

const int stepsPerRevolution = 2050;  // change this to fit the number of steps per revolution
const int formelWert = 60000 / stepsPerRevolution;

unsigned int stepCount = 0;  // number of steps the motor has taken


// LCD DISPLAY ---------------------------------------------------------------------------------------
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); //Angabe der erforderlichen Pins
// erstellen einiger Variablen
int Taster = 0;
int Analogwert = 0;
#define Tasterrechts 0
#define Tasteroben 1
#define Tasterunten 2
#define Tasterlinks 3
#define Tasterselect 4
#define KeinTaster 5

String lcdstr1;
String textZeile1;
String lcdstr2;
long lcdposition;


byte customChar0[8] = {
  0b00000,
  0b00000,
  0b11111,
  0b10001,
  0b10001,
  0b10001,
  0b11111,
  0b00000
};
byte customCharPL[8] = {
  0b00000,
  0b00000,
  0b10000,
  0b10011,
  0b10100,
  0b11000,
  0b11111,
  0b00000
};
byte customCharL[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00011,
  0b00100,
  0b01000,
  0b10000,
  0b00000
};
byte customCharMInf[8] = {
  0b00000,
  0b00000,
  0b11111,
  0b00000,
  0b00000,
  0b11111,
  0b10101,
  0b11111
};
byte customCharM[8] = {
  0b00000,
  0b00000,
  0b11111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00100
};
byte customCharR[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b11000,
  0b00100,
  0b00010,
  0b00001,
  0b00000
};
byte customCharPR[8] = {
  0b00000,
  0b00000,
  0b00001,
  0b11001,
  0b00101,
  0b00011,
  0b11111,
  0b00000
};

// function
int Tasterstatus()
{
  Analogwert = analogRead(A0); // Auslesen der Taster am Analogen Pin A0.
  delay(10);
  if (Analogwert > 1000) return KeinTaster;
  if (Analogwert < 50) return Tasterrechts;
  if (Analogwert < 195) return Tasteroben;
  if (Analogwert < 380) return Tasterunten;
  if (Analogwert < 555) return Tasterlinks;
  if (Analogwert < 790) return Tasterselect;

  return KeinTaster; // Ausgabe wenn kein Taster gedrückt wurde.
}

// eigene String-Funktionen -------------------------------------------------------------------------

String formatStr(String inString, byte len)
{
  // String in definierter Länge von 1 bis 16 Zeichen linksbündig zurückgeben
  // wenn inString länger wie len ist wird abgeschnitten...
  String result;
  result = inString + "                ";
  result = result.substring (0, len);
  return result;
}

String formatStrR(String inString, byte len)
{
  // String in definierter Länge von 1 bis 16 Zeichen rechtsbündig zurückgeben
  // wenn inString länger wie len ist wird abgeschnitten...
  String result;
  result = "                ";
  result = result.substring(0, (len - inString.length()));
  result = result + inString;
  return result;
}


String formatStrRmm(long inValue, byte len)
{
  // String in definierter Länge von 1 bis 16 Zeichen rechtsbündig zurückgeben
  // dezimalpunkt für 1/100 wird eingefügt
  // wenn inValue > 99999 oder kleiner als -9999 ist, sllet noch ne Fehlerbehandlung rein ?
  String result;
  float flat;

  flat = (float)inValue;
  flat = flat / 100;     // Dezimalpunkt für 1/100 mm einfügen
  result = "                ";
  result = result + (String)flat;
  result = result.substring(result.length() - len);

  return result;
}

String getStr(String zeichen, byte len)
{
  // String in definierter Länge von len gefüllt mit zeichen zurückgeben
  String result = "";
  for (int i = 0; i <= len; i++) {
    result = result + zeichen;
  }
  return result;
}



// ----------------------------------------------------------------------------------------------------------

// initialize the stepper library on pins 8 through 11, ATTENTION - the order must be exactl so:
Stepper myStepper(stepsPerRevolution, motorPin1, motorPin3, motorPin2, motorPin4);



void setup() {
  Serial.begin(9600);  // initialize the serial port for debug-outputs
  // Ansteuerfrequenz : Angabe bei der stepper Bibliothek durch die Angabe der Umdrehungen
  // pro Minute (RPM). Im Datenblatt findet sich ein empfohlener Frequenzbereich von 600-1000Hz
  // bezogen auf Halbschritte.
  // 1000Hz / 4096 Schritte = 0,244Hz
  // Also 4 Sekunden für eine Umdrehung oder 15 RPM.
  myStepper.setSpeed(15);  // Ansteuerfrequenz

  startMillis = millis();  //initial start time

  // create a new custom character for LCD
  lcd.createChar(0, customChar0);
  lcd.createChar(1, customCharPL);
  lcd.createChar(2, customCharL);
  lcd.createChar(3, customCharM);
  lcd.createChar(4, customCharR);
  lcd.createChar(5, customCharPR);
  lcd.createChar(6, customCharMInf);
  lcd.begin(16, 2); // Starten der LCD-Programmbibliothek.
  lcd.clear();
  lcd.clear();
  lcd.setCursor(0, 0); // Angabe des Cursorstartpunktes oben links.
  lcd.print(" TURN-TABLE v2.0"); // Ausgabe des Textes "Nachricht".
}

void loop() {
  // Steuerung über progstatus

  // Minuteneingabe und Eingabe der Drehrichtung
  // hier kann mit Delay gearbeitet werden, da der Timer0 nicht benutzt wird
  if (progStatus == 1) {

    // Zeile 2
    lcd.setCursor(0, 1);
    if (stepperRawDirection % 4 >= 2 and stepperInf % 2 == 0) {
      // gegen den Uhrzeigersinn
      stepperDirection = 1;
      lcdstr2 = (String)char(1) + (String)char(3) + (String)char(4);
    } else if (stepperRawDirection % 4 >= 2 and stepperInf % 2 == 1) {
      // im Uhrzeigersinn infinite
      stepperDirection = 1;
      lcdstr2 = (String)char(1) + (String)char(6) + (String)char(4);
    } else if (stepperRawDirection % 4 >= 0 and stepperInf % 2 == 0) {
      // im Uhrzeigersinn
      stepperDirection = -1;
      lcdstr2 = (String)char(2) + (String)char(3) + (String)char(5);
    } else if (stepperRawDirection % 4 >= 0 and stepperInf % 2 == 1) {
      // im Uhrzeigersinn infinite
      stepperDirection = -1;
      lcdstr2 = (String)char(2) + (String)char(6) + (String)char(5);
    }

    if (is_minute % 3 == 0) {
      //Serial.print ("min:  ");
      lcdstr2 = formatStrR((String)int(minuten), 3) + " min/U    " + lcdstr2;
    } else if (is_minute % 3 == 1) {
      //Serial.print ("sec:  ");
      lcdstr2 = formatStrR((String)int(minuten), 3) + " sec/U    " + lcdstr2;
    } else if (is_minute % 3 == 2) {
      //Serial.print ("hou:  ");
      lcdstr2 = formatStrR((String)int(minuten), 3) + " hou/U    " + lcdstr2;
    }
    //Serial.println (lcdstr2);
    lcd.print(lcdstr2);

    // Tastenabfrage nur bei progStatus = 1
    Taster = Tasterstatus(); //Hier springt der Loop in den oben angegebenen Programmabschnitt "Tasterstatus" und liest dort den gedrückten Taster aus.
    switch (Taster) // Abhängig von der gedrückten Taste wird in dem folgenden switch-case Befehl entschieden, was auf dem LCD angezeigt wird.
    {
      case Tasterlinks: // Wenn die rechte Taste gedrückt wurde... Werte auf Grundwert setzen
        {
          if (progStatus == 1)
          {
            is_minute++;
            minuten = 1;           // Einstellwert für die Mintuen (1-180min erlaubt)
            //stepperRawDirection = 0;  // Drehrichtung gegen den uhrzeigersinn
            //stepperInf = 0;
            delay(150);            // damit nur eimal die Taste angenommen wird
          }
          break;
        }
      case Tasterrechts:  // Wenn die linke Taste gedrückt wurde... Drehrichtung ändern
        {
          if (progStatus == 1)
          {
            stepperRawDirection++;
            stepperInf++;
            delay(250);             // damit nur eimal die Taste angenommen wird
            break;
          }
        }
      case Tasteroben:    // minuten um 10 erhöhen
        {
          if (progStatus == 1)
          {
            // um 10 erhöhen
            //if (minuten <= 170) // max 180 min
            //{
              minuten = minuten + 10;
              delay(250);   // damit nur eimal die Taste angenommen wird
            //}
          }
          break;
        }
      case Tasterunten:
        {
          if (progStatus == 1)    // minuten um 1 erhöhen
          {
            // um 1 erhöhen
            //if (minuten <= 179) // max 180
            //{
              minuten++;
              delay(250);   // damit nur eimal die Taste angenommen wird
            //}
          }
          break;
        }
      case Tasterselect:
        {
          // mit SELECT wird der aktuelle Prog-Status durchgeschatet
          //  1 = Werteeingabe mit GO zu 2
          //  2 = Go , danach wieder zu 1
          delay(150);

          if (progStatus == 1)
          {
            progStatus = 2;
            stepperStop = false;             // rotate until one revolution is complete
            if (is_minute % 3 == 0) {
              period = minuten * formelWert;   // aus eingestelltem Minutenwert period errechnen
            } else if (is_minute % 3 == 1) { // seconds
              period = minuten * formelWert / 60;
            } else if (is_minute % 3 == 2) { // hours
              period = minuten * formelWert * 6;
            }
            startMillis = millis();  //initial start time
            lcd.setCursor(11, 1);            // Anzeige eines ! = Rotation läuft
            lcd.print("!"); 
            //Serial.print ("period:  ");      // Testausgabe
            //Serial.println (period);
          }
          break;
        }
      case KeinTaster:
        {
          break;
        }
    } //switch-case Befehl beenden

  } // if Progstatus = 1 beenden
  else {
    
    // Nur wenn GO, progstatus = 2, los gehts!
    // ACHTUNG: hier kann NICHT mit Delay gearbeitet werden, da der Timer0 zur Zeitsteuerung benutzt wird!!!
    // ACHTUNG: bei der kürzesten period = 29 millis können nicht mehr beliebig viele Befehle pro durchlauf abgearbeitet werden!!!
    //          also den Code kurz halten...
    if (!stepperStop) {
      currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)

      if (currentMillis - startMillis >= period)  //test whether the period has elapsed
      {
        // step one step:
        Serial.println (stepperDirection);
        myStepper.step(stepperDirection);    // genau einen Schritt am Stepper in die vorgegebene Richung machen
        stepCount++;

        if ((stepCount == stepsPerRevolution) && (stepperInf % 2 == 0)) {
          // Die Rotation ist komplett
          stepperStop = true;
          progStatus = 1;              // Werte wieder zurücksetzen
          stepCount = 0;
          lcd.setCursor(11, 1);       // Anzeige eines ! = Rotation läuft wieder zurücksetzten
          lcd.print(" "); 
        }
        startMillis = currentMillis;  //IMPORTANT to save the start time of the current loop
      }
    }
  } //if else Progstatus = 1 beendet

}




