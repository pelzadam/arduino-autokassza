#include <HX711.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <Ethernet.h>
#include <string.h>

// Árukat tároló adatstruktúra
struct ware{
  char* name;
  char* barcode;
  int price;
  double weight;
};

//Érméket tároló adatstruktúra
struct coin{
  int value;
  double weight;  
};

//Függvény az áruk inicializálására
ware newWare(char* n, char* b, int p, double we){
  ware w;  
  w.name = new char[strlen(n)];
  w.name = n;
  w.barcode = new char[strlen(b)];
  w.barcode = b;
  w.price = p;
  w.weight = we;

  return w;
}

//Függvény az érmék inicializálására
coin addCoin(int v, double w){
  coin c;
  c.value=v;
  c.weight=w;

  return c;
}

//Érmék súlyának számítása az ár alapján
double priceWeight(int p, coin* c){
  double w = 0.0;
  int i = 0, val; 

  while (p > 0 && i < 6){
    val = c[i].value;
    if(p >= val){
      w += c[i].weight;
      p -= val;
      Serial.print(val);
    }else{
      i++;      
    }
  }

  return w;    
}

//Vonalkódok összehasonlítása
bool comparison(char* str1, char* str2){
  int i=0;

  while(i<6 && str1[i]==str2[i]){
    i++;    
  }

  if(i<6){
    return true;
  }else{
    return false;
  }
  
}

//Hálózathoz való csatlakozáshoz szükséges adatok
byte mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
byte ip[] = {192, 168, 0, 3};
byte gateway[] = {192, 168, 0, 1};
byte subnet[] = {255, 255, 255, 0};
byte server[] = {192, 168, 0, 2};

ware wares[5];
coin forints[6];
double measure, weight=0.0;
char barcode[6], dump[1];
int barcode_char=0, price=0, i;
bool wrongWeight = false, payment = false;

//Pinek inicializálása
const int LOADCELL_DOUT_PIN = A0;
const int LOADCELL_SCK_PIN = A1;
const int SCANNER_RX = A2;
const int SCANNER_TX = A3;
const int BUTTON = 7;
const int GREEN_LED = A4;
const int RED_LED = A5;
const int RS = 8, EN = 9, D4 = 2, D5 = 3, D6 = 4, D7 = 5; 

//OSztályok meghívása
HX711 scale;
LiquidCrystal lcd(RS,EN,D4,D5,D6,D7);
SoftwareSerial scanner(SCANNER_RX,SCANNER_TX);
EthernetClient client;

void setup() {
  //Eszközök osztályainak inicializálása   
  Serial.begin(9600);
  scanner.begin(9600);
  Ethernet.init(10);
  Ethernet.begin(mac, ip, {}, gateway, subnet);
  pinMode(7,INPUT);
  lcd.begin(16, 2);

  //Kezdéshez a vonalkódot ki kell nullázni,
  //Hozzáadni az érméket, az árukat
  for(i=0;i<6;i++){
    barcode[i]='0';
  }

  forints[0] = addCoin(200,9.2);
  forints[1] = addCoin(100,8.1);
  forints[2] = addCoin(50,7.8);
  forints[3] = addCoin(20,7.1);
  forints[4] = addCoin(10,6.2);
  forints[5] = addCoin(5,4.3);

  wares[0] = newWare("Calculator", "AAA001", 200, 142.1);
  wares[1] = newWare("Pendrive", "AAA002", 100, 9.6);
  wares[2] = newWare("Highlighter", "AAB001", 50, 18.7);
  wares[3] = newWare("Cell phone", "AAA003", 400, 153.3);
  wares[4] = newWare("Ruler", "AAB004", 20, 31.5);

  double testweight = 115;
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  scale.set_scale();
  scale.tare();
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Place the weight");
  lcd.setCursor(0,1);
  lcd.print("on the scale!");

  Serial.println("Place the weight on the scale, then press the button.");
  while(digitalRead(BUTTON)==LOW);

  measure=scale.get_units(10);
  Serial.print("The measured unit is ");
  Serial.println(measure);
  Serial.print("The calibration factor will be ");
  Serial.println(measure/testweight);
  
  //scale.set_scale(460);
  scale.set_scale(measure/testweight);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Calibration done,");
  lcd.setCursor(0,1);
  lcd.print("empty the scale!");
  
  analogWrite(GREEN_LED,255);
  Serial.println("Calibration done. Please empty the scale and press the buton.");
  while(digitalRead(BUTTON)==LOW);
  scale.tare();

  analogWrite(GREEN_LED,255);
}

void loop() {
  int wares_amount = sizeof(wares)/sizeof(wares[0]);

  analogWrite(GREEN_LED, 0);

  if((weight+0.2 < scale.get_units(5) || weight-0.2 > scale.get_units(5)) && !payment){
    Serial.print("Weight ");
    Serial.print(scale.get_units(5));
    Serial.print(" is not the expected weight(");
    Serial.print(weight);
    Serial.println(")!");
    
    lcd.clear();  
    lcd.setCursor(0,0);
    lcd.print("Unknown or ");
    lcd.setCursor(0,1);
    lcd.print("missing product!");
    
    wrongWeight = true;
    analogWrite(RED_LED, 255);
  }else if((weight+0.2 < scale.get_units(5) || weight-0.2 > scale.get_units(5)) && payment){
    Serial.print("Weight ");
    Serial.print(scale.get_units(5));
    Serial.print(" is not the expected weight(");
    Serial.print(weight);
    Serial.println(")!");

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Place ");
    lcd.print(price);
    lcd.print(" HUF");    
    lcd.setCursor(0,1);
    lcd.print("on the scale!");

    wrongWeight = true;
    analogWrite(RED_LED, 0);
  }else{
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Price:");
    lcd.setCursor(0,1);
    lcd.print(price);

    if (payment){
      analogWrite(GREEN_LED, 255);      
    }
    
    analogWrite(RED_LED, 0);
    wrongWeight = false;
  }

  if(digitalRead(BUTTON)==HIGH && !wrongWeight && payment){
    analogWrite(GREEN_LED, 255);
    Serial.println("Successful payment!");

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Payment done!");
    lcd.setCursor(0,1);
    lcd.print("Remove the wares!");

    while(0.2 < scale.get_units(5) || -0.2 > scale.get_units(5));

    price=0;
    weight = 0.0;    
    payment=false;
  }

  if(!wrongWeight && !payment){
    if(scanner.available() && scanner.available()!=8){
      barcode_char=0;
      Serial.println("Wrong barcode format!");
      Serial.print("Length ");
      Serial.print(scanner.available());
      Serial.println(" is not the correct length!");
      analogWrite(RED_LED,255);

      while(scanner.available()){
        if(barcode_char<6){
          barcode[barcode_char]=scanner.read();
          barcode_char++;
        }else{
          dump[0]=scanner.read();
        }
      }

      Serial.println(barcode);
      
    }else if(scanner.available()==8){
      barcode_char=0;
      while(scanner.available()){
        if(barcode_char<6){  
          barcode[barcode_char]=scanner.read();
          barcode_char++;
        }else{
          dump[0]=scanner.read();
        }
      }

      i=0;
      while(i < wares_amount && comparison(barcode,wares[i].barcode)){
        i++;
      }

      if(i>=wares_amount){
        Serial.println("Wrong barcode. Ware does not exist.");
        
        lcd.clear();  
        lcd.setCursor(0,0);
        lcd.print("Wrong barcode!");
        
        analogWrite(RED_LED,255);      
      } else{
        Serial.print("Succesfull scan. Ware \"");
        Serial.print(wares[i].name);
        Serial.println("\" has been added to the bought products.");
        price += wares[i].price;
        weight += wares[i].weight;
        
        analogWrite(GREEN_LED,255);
      }

      Serial.println("The barcode is: ");
      Serial.println(barcode);

    }
  }

  if(digitalRead(BUTTON)==HIGH && !wrongWeight && !payment){
    payment = true;
    weight = weight + priceWeight(price,forints);
    analogWrite(GREEN_LED, 255);
  }

  Serial.println(scale.get_units(5));
  delay(10);
}
