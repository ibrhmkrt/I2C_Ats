#include <avr/io.h>
#include <avr/interrupt.h>
#include <Wire.h>
unsigned  int sayac = 0;//Hedefin vurulduğu Toplam sayı

int ilk_siddet = 0;
int ortam_siddeti = 100;//1,5 saniye ortam gürültü ortalaması, daha sonra 1 önceki analog kanal değeri
bool ortam_dinleme = true;//false değeri için hedef atışa hazırdır
bool yeni_atis = false;//yeni bir dogru atış tespitinde true değerini alır bir sonraki atış bekleme için false yapılır
bool hazir = false;//atış tespitinde atış algılanmasında sonra gerekli olan sürenin geçmesi ve analog kalaın değerlendirmeye hazır olma durumunu tutar(true)
int fark;
int esik = 100;
byte SlaveReceived = 0;
byte SayacSend = 0;
int ByteSayisi = 0;
byte SetMode = 1;
byte SetModeGet = 1;
byte ResetOnay = 31;
byte SetOnay = 32;
byte GecersizRequest = 41;
boolean ByteHata = false;
byte ByteHataMessage = 51;
boolean EsikSetHata = false;
byte EsikSetHataMessage = 61;
byte Ready = 34;

int farkCikarma = 0;
int esikSeviye = 1;
byte EsikSend = 0;

void setup() {
  Serial.begin(115200);//com port a
  //pinMode(13, OUTPUT);//13. pin atış tespitinde kullanılacak buzzer pini
  pinMode(8, OUTPUT); //ı2c Master tetikleme
  Wire.begin(1);
  Wire.onReceive(receiveEvent);           //Function call when Slave receives value from master
  Wire.onRequest(requestEvent);           //Function call when Master request value from Slave
  delay(1000);

  ADCSRA = 0x8F;      // Enable the ADC and its interrupt feature
  //and set the ACD clock pre-scalar to clk/128
  //ADMUX = 0xE0;     // Select internal 2.56V as Vref, left justify

  //ADC MODUL AYARLARI
  ADMUX |= (1 << REFS0); //AREF seçimi,REFS1 "0" kalır ve besleme gerilimi olan 5V referans alınır
  ADMUX &= 0xF0;//ADC kanalı seçimi(A0)
  ADCSRA |= (1 << ADSC); //ADC dönüşüm başlatma bayragını aktif etme, dönüşüm bittiğinde "0" olur.

  TCNT1 = 64285; // 1 saniye 15625 için >> 65535-15625, 64285 ~80 ms
  TCCR1A = 0x00;//Normal mode
  TCCR1B &= (0 << CS11); // 1024 prescler
  TCCR1B |= (1 << CS10); // 1024 prescler
  TCCR1B |= (1 << CS12); // 1024 prescler
  TIMSK1 |= (1 << TOIE1); // Timer1 taşma kesmesi aktif

  sei(); //Global kesmeler devrede

  //Nucleo 1 tetikleme yaparak I2C haberleşme başlatıldı.
  digitalWrite(8, HIGH);//Merkezi tetikleyen pin aktif
  TCNT1 = 64285;// 65535-64285(1024 prescaler)=~80 ms --- 1 sn 15625
  TIFR1 |= (1 << TOV1) ;//timer1 taşma bayragı sıfırlanır.
  TIMSK1 |= (1 << TOIE1) ;// Timer1 taşma kesmesi aktif
  hazir = false;

}

void loop() {
  if (millis() < 1500) { // açılışta 1,5 saniye süresince ortamın gürültü ortalmasını hesaplanır

    ortam_siddeti = (ilk_siddet + ortam_siddeti) / 2;
  }
  else
  {
    ortam_dinleme = false;

  }

  ADCSRA |= (1 << ADSC); //ADC dönüşüm başlatma bayragını aktif etme, dönüşüm bittiğinde "0" olur.
  /*
    if (yeni_atis == true) {
    yeni_atis = false;
    }*/

  /*
    if (SlaveReceived == 21) //reset
    {
      sayac = 0;

    }
  */

  if (SlaveReceived == 22) { //set
    switch (SetMode)
    {
      case 1:
        esik = 100;
        SetModeGet = 1;
        break;

      case 2:
        esik = 200;
        SetModeGet = 2;
        break;

      case 3:
        esik = 300;
        SetModeGet = 3;
        break;

      case 4:
        esik = 400;
        SetModeGet = 4;
        break;

      case 5:
        esik = 500;
        SetModeGet = 5;
        break;

      case 6:
        esik = 600;
        SetModeGet = 6;
        break;

      case 7:
        esik = 700;
        SetModeGet = 7;
        break;

      case 8:
        esik = 800;
        SetModeGet = 8;
        break;

      case 9:
        esik = 900;
        SetModeGet = 9;
        break;

      case 10:
        esik = 1000;
        SetModeGet = 10;
        break;

      default:
        EsikSetHata = true;

    }

  }
  //Serial.println(esik);

}



int analog_degerr = 0;//analog kanaldan gelen değerin saklandığı değişken 0-1024

ISR(ADC_vect) {
  analog_degerr = ADC;
  if (ortam_dinleme == true) {
    ilk_siddet = analog_degerr;
  }

  else {
    fark = abs(analog_degerr - ortam_siddeti);
    if (fark >= esik)
    {
      if (hazir == true) {
        farkCikarma = fark;
        sayac++;
        sayac = sayac % 100; //sayac = sayac % 256;

        //Serial.write(sayac);
        //digitalWrite(13, HIGH);Buzzer Yerine merkez tetiklenecek
        digitalWrite(8, HIGH);//Merkezi tetikleyen pin aktif

        TCNT1 = 64285;// 65535-64285(1024 prescaler)=~80 ms --- 1 sn 15625
        TIFR1 |= (1 << TOV1) ;//timer1 taşma bayragı sıfırlanır.
        TIMSK1 |= (1 << TOIE1) ;// Timer1 taşma kesmesi aktif

        //yeni_atis = true;
        hazir = false;
      }
    }
    ortam_siddeti = analog_degerr;
  }
}

ISR (TIMER1_OVF_vect)    // Timer1 ISR
{
  TIMSK1 &= (0 << TOIE1) ;   // Timer1 taşma kesmesi devredışı (TOIE1)
  digitalWrite(8, LOW);      //Merkez tetikleme pin deaktif
  hazir = true;  // for 1 sec at 16 MHz
}

void receiveEvent (int howMany)                    //This Function is called when Slave receives value from master
{
  ByteSayisi = howMany;
  if (ByteSayisi <= 2) {
    if (ByteSayisi == 1) {
      SlaveReceived = Wire.read();
    }
    else if (ByteSayisi == 2) {
      SlaveReceived = Wire.read();
      SetMode = Wire.read();
    }
    else {
      ByteHata = true;
      while (Wire.available()) {
        Wire.read();
      }
    }
  }
  else  {
    ByteHata = true;
    while (Wire.available()) {
      Wire.read();
    }
  }
}

void requestEvent()                                //This Function is called when Master wants value from slave
{
  if (ByteHata) {
    ByteHata = false;
    Wire.write(ByteHataMessage); //51

  }
  else if (SlaveReceived == 24) {
    Wire.write(Ready);//34
  }
  else if (SlaveReceived == 23) {
    Wire.write(SetModeGet);//Esit Get
  }
  else if (SlaveReceived == 20) {
    SayacSend = (byte)sayac;
    esikSeviye = map(farkCikarma, 100, 1023, 1, 10);
    EsikSend = (byte)esikSeviye;
    byte data[2] = {SayacSend, EsikSend};
    Wire.write(data, 2);
    /*Wire.write(SayacSend);
      Wire.write(EsikSend);*/
  }
  else if (SlaveReceived == 21) {
    sayac = 0;
    Wire.write(ResetOnay); //31
  }
  else if (SlaveReceived == 22 && !EsikSetHata) {
    Wire.write(SetOnay); //32
  }

  else if (EsikSetHata == true) {
    Wire.write(EsikSetHataMessage); //61
    EsikSetHata = false;
  }
  else {
    Wire.write(GecersizRequest); //41
  }
}
