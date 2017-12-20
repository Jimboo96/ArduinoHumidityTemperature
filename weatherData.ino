#include <math.h>
#define HUMIDITYPIN A0                                // Luetaan H25K5A-kosteusanturin lukemaa tästä pinnistä.
#define TEMPERATUREPIN A1                             // Luetaan NTCLE100E3103JB0-lämpötilasensorin lukemaa tästä pinnistä.
#define SAMPLES 125                                   // Otettavien näytteiden lukumäärä.
#define MAXNUMBEROFLOOPS 30                           // Läpikäytävien looppien lukumäärä.

// Lämpötilasensorin muuttujien määrittelyjä.
volatile uint16_t temperatureSampleArray[SAMPLES];    // Lämpötilasensorin arvojen näytetaulukko.
boolean temperatureInterruptReady = false;            // Lämpötilakeskeytyksen flägi.
boolean temperatureReadyToLoop = false;               // Lämpötilaloopin flägi.
volatile float temperatureAverage;                    // Keskiarvo lämpötilanäytteistä.
float temperature;                                    // AD-arvon keskiarvosta muunnettu lämpötila.
int temperatureLoopCounter = 0;                       // Läpi käytyjen looppien määrä, jonka saavutettua 30, lasketaan 60s keskiarvo arvoista.
float temperatureAverages;                            // Keskiarvot minuutin ajalta.                   
  
// H25K5A-kosteusanturin muuttujien määrittelyä.
boolean humidityInterruptReady = true;                // Kosteuskeskeytyksen flägi.
boolean humidityReadyToLoop = false;                  // Kosteusloopin flägi.
volatile int humiditySampleArray[SAMPLES];            // Kosteusnäytteiden taulukko.
volatile int humidityMaxSampleValue;                  // Kosteusanturin mittaamista AD-arvoista saatu suurin arvo.
volatile int humidityAnalogValue;                     // Kosteusanturin mittaama AD-arvo.
volatile int humiditySampleCount;                     // Kosteusnäytteiden laskuri.
volatile int humidityValue;                           
int ADHumidityValue[4]={1018,996,460,260};            // AD-muuntimen arvo.
int humidityPercentage[4]={20,40,60,80};              // Vastaava kosteusprosentti.
float humidity;                                       // AD-arvon keskiarvosta muunnettu kosteusprosentti.
int humidityLoopCounter = 0;                          // Keskiarvot 60 sekunnin ajalta. 
float humidityAverages;                               // Läpi käytyjen looppien määrä, jonka saavutettua 30, lasketaan minuutin keskiarvo arvoista.

void setup(void) {
   noInterrupts();                                    // Alustetaan ja määritellään Arduinon rekistereitä.
   ADMUX &= B11011111;                                // Clear ADLAR in ADMUX (0x7C) to right-adjust the result ADCL will contain lower 8 bits, ADCH upper 2 (in last two bits).
   ADMUX |= B01000000;                                // Set REFS1..0 in ADMUX (0x7C) to change reference voltage to the proper source (01).
   ADMUX &= B11110000;                                // Clear MUX3..0 in ADMUX (0x7C) in preparation for setting the analog input.     
   ADCSRA |= B10000000;                               // Set ADEN in ADCSRA (0x7A) to enable the ADC. Note, this instruction takes 12 ADC clocks to execute.    
   ADCSRA |= B00000000;                               // Set ADATE in ADCSRA (0x7A) to enable auto-triggering.  
   ADCSRA |= B00000111;                               // Set the Prescaler to 128 (16000KHz/128 = 125KHz). Above 200KHz 10-bit results are not reliable.
   ADCSRA |= B00001000;                               // Set ADIE in ADCSRA (0x7A) to enable the ADC interrupt. Without this, the internal interrupt will not trigger.
   ADCSRA |= B01000000;                               // Set ADSC in ADCSRA (0x7A) to start the ADC conversion.
   interrupts();
   
   Serial.begin(9600); 
   analogWrite(5,128);                                // 980Hz PWM, pinni numero 5.
   analogReference(EXTERNAL);
}
 
void loop(void) {
    ADCSRA |= B01000000;                             // Rekisterin nollaus.
    if(humidityReadyToLoop == true) {
      humidityLoop();
      humidityLoopCounter++;
    }
    if(temperatureReadyToLoop == true) {
      temperatureLoop();
      temperatureLoopCounter++;
    }
}

void humidityLoop() {
     if (humidityValue>= ADHumidityValue[0]) humidity=humidityPercentage[0]-(humidityPercentage[0]-0)*(humidityValue-ADHumidityValue[0])/(1023-ADHumidityValue[0]);
     else if (humidityValue<ADHumidityValue[0] && humidityValue>=ADHumidityValue[1]) humidity=humidityPercentage[1]-(humidityPercentage[1]-humidityPercentage[0])*(humidityValue-ADHumidityValue[1])/(ADHumidityValue[0]-ADHumidityValue[1]);
     else if (humidityValue<ADHumidityValue[1] && humidityValue>=ADHumidityValue[2]) humidity=humidityPercentage[2]-(humidityPercentage[2]-humidityPercentage[1])*(humidityValue-ADHumidityValue[2])/(ADHumidityValue[1]-ADHumidityValue[2]);
     else if (humidityValue<ADHumidityValue[2] && humidityValue>=ADHumidityValue[3]) humidity=humidityPercentage[3]-(humidityPercentage[3]-humidityPercentage[2])*(humidityValue-ADHumidityValue[3])/(ADHumidityValue[2]-ADHumidityValue[3]);
     else humidity = 100 - (100-humidityPercentage[3])*(humidityValue-0)/(ADHumidityValue[3]-0);
     humidityAverages += humidity;                                      // Kerätään minuutin ajalta arvoja tähän muuttujaan, josta lasketaan sitten niiden keskiarvo.
     
     if(humidityLoopCounter == MAXNUMBEROFLOOPS) {                      // Kun kolmaskymmenes kierros alkaa, tulostetaan sarjamonitorille minuutin keskiarvo kosteudesta.
          humidityAverages = humidityAverages / MAXNUMBEROFLOOPS;
          humidity = humidityAverages;
          Serial.print(humidity);
          Serial.println("Havg");
          humidityAverages = 0;
          humidityLoopCounter = 0;
     }
     Serial.print(humidity);
     Serial.println("H");
     humidityReadyToLoop = false;
     temperatureInterruptReady = true;
     delay(100);
}

void temperatureLoop() {
      temperature = (-35.39 * log(temperatureAverage) + 245);           // Lasketaan lämpötila kalibrointiyhtälön avulla AD-arvon keskiarvosta. 
      temperatureAverages += temperature;                               // Kerätään minuutin ajalta arvoja tähän muuttujaan, josta lasketaan sitten niiden keskiarvo.
      if(temperatureLoopCounter == MAXNUMBEROFLOOPS) {                  // Kun kolmaskymmenes kierros alkaa, tulostetaan sarjamonitorille minuutin keskiarvo lämpötilasta.
          temperatureAverages = temperatureAverages / MAXNUMBEROFLOOPS;
          temperature = temperatureAverages;
          Serial.print(temperature);
          Serial.println("Cavg");
          temperatureAverages = 0;
          temperatureLoopCounter = 0;
      }
      Serial.print(temperature);
      Serial.println("C");
      temperatureReadyToLoop = false;
      humidityInterruptReady = true;
      delay(100);
}

void humidityInterrupt() {
     uint8_t i;
     humidityAnalogValue = ADCL | (ADCH << 8);                          // Luetaan kosteusanturin antama AD-arvo.
     humiditySampleArray[humiditySampleCount] = humidityAnalogValue;    // Lisätään näytetaulukkoon AD-arvo.
     humiditySampleCount++;                                             // Nostetaan laskurin arvoa yhdellä.
     if(humiditySampleCount==SAMPLES) {                                 // Kun taulukossa on 125 näytettä, suoritetaan suurimman AD-arvon etsimimen taulukosta.
       humidityMaxSampleValue=humiditySampleArray[0];
       for(i=0;i<SAMPLES;i++)                                           // Käydään läpi kaikki näytteet taulukossa ja korvataan AD-arvo, jos taulukosta löytyy suurempi arvo.
       {
          if (humidityMaxSampleValue<humiditySampleArray[i+1]) humidityMaxSampleValue=humiditySampleArray[i+1];
       }
       ADMUX &= B11110000;                                              // Clear MUX3..0 in ADMUX (0x7C) in preparation for setting the analog input.
       ADMUX |= 1;                                                      // AD-muuntimen kanava 1.  
       humidityValue = humidityMaxSampleValue;                          // Asetetaan kosteusanturin AD-arvo suurimmaksi mitatuksi arvoksi.
       humiditySampleCount=0;                                           // Nollataan näytteiden laskuri.
       humidityInterruptReady = false;
       humidityReadyToLoop = true;
     }
}

void temperatureInterrupt() {
    uint8_t i;
    for (i=0; i < SAMPLES; i++) {                                       // Otetaan 125 näytettä.
       temperatureSampleArray[i] = ADCL | (ADCH << 8);                  // Luetaan lämpötilasensorin antama AD-arvo ja lisätään se näytetaulukkoon.
    }
    for (i=0; i < SAMPLES; i++) {                                       // Otetaan keskiarvo näytteistä.
       temperatureAverage += temperatureSampleArray[i];
    } 
    temperatureAverage /= SAMPLES;
    ADMUX &= B11110000;                                                 // Clear MUX3..0 in ADMUX (0x7C) in preparation for setting the analog input.
    ADMUX |= 0;                                                         // AD-muuntimen kanava 0.  
    temperatureInterruptReady = false;
    temperatureReadyToLoop = true;
}

ISR(ADC_vect)
{
    if(humidityInterruptReady == true) humidityInterrupt();            // Vuorotellaan kosteus- ja lämpötilasensorin keskeytyksiä flägien avulla.
    if(temperatureInterruptReady == true) temperatureInterrupt();
}
