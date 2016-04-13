#include <LiquidCrystal.h>
#include <SPI.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <Temboo.h>

#define TEMBOO_ACCOUNT "____"           //complete these parameters
#define TEMBOO_APP_KEY_NAME "____"
#define TEMBOO_APP_KEY "____"
#define DropboxAppSecret "___"          //developer credentials of Dropbox
#define DropboxAppKey "___"
#define AuthToken "___"                 //developer credentials of Twilio
#define AccountSID "___"  
#define SMSFromNumber "___"
#define SMSToNumber "___"


byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 0, 177);
EthernetClient client;

void initializeEnvironment();
void initializeDropboxAuth();
void authSMS();
void finalizeDropboxAuth();
void WarningOutputLCD();
void CorrectOutputLCD();
void TimeLimitSMS();
void getTimeStamp();
void getDateFromTimeStamp();
void encodingMessageBase64();
void uploadFileInDropbox();


String Authorization_URL;
String OAuth_Token_Secret;
String Callback_ID;
String Access_Token;
String Access_Token_Secret;
String datacorrente;
String timestamp;
String message;
float temperature;

int numRuns = 1;                      // Execution count, so this doesn't run forever
int maxRuns = 1;                      // Maximum number of times the Choreo should be executed
                                      // This project is a test, so we execute only one time

const int sensorPin = A0;             // Name assigned to analogic input
const float highLimitTemp= 25.0;
const float lowLimitTemp= 5.0; 

byte degree[8] = {                    // °
  0b00000,
  0b01110,
  0b01010,
  0b01110,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};


LiquidCrystal lcd(2, 3, 4, 5, 6, 7);  // Initialized with the number of pins used

void setup() {
  if (Ethernet.begin(mac) == 0) {
    while(true);
  }
  
  initializeEnvironment();

  initializeDropboxAuth();
 
  lcd.clear();
  lcd.setCursor(0, 0);                          
  lcd.print("Sending SMS for");
  lcd.setCursor(0, 1);    
  lcd.print("autentication...");
      
  authSMS();

  delay(10000);
  lcd.clear();
  lcd.setCursor(0, 0);                          
  lcd.print("Please allow me");
  lcd.setCursor(0, 1);
  lcd.print("on your phone");
  delay(15000);

  lcd.clear();
  lcd.setCursor(0, 0);                          
  lcd.print("Thank you");
  lcd.setCursor(0, 1);
  lcd.print("You're logged in");
  delay(4000);
  
  finalizeDropboxAuth();
}



void loop() {
  float analogSignal = analogRead(sensorPin);     // Value of analogic voltage
  float voltage = (analogSignal/1024.0)*5.0;    // The respective value of real voltage
  temperature = (voltage - .5)*100;               // Voltage in temperature

  if(temperature <= lowLimitTemp || temperature >= highLimitTemp){
    
    WarningOutputLCD();
      
    if (numRuns <= maxRuns) {       //test effettuato solo 1 volta
      numRuns++;

      lcd.clear();                                      
      lcd.setCursor(0, 0);    
      lcd.print("Sending SMS...");

      TimeLimitSMS();

      delay(4000);
      lcd.clear();                                      
      lcd.setCursor(0, 0);    
      lcd.print("Done!");
      delay(2000);
  
      getTimeStamp();
      getDateFromTimeStamp();
      encodingMessageBase64();
      uploadFileInDropbox();
    }
  }
  else{
    CorrectOutputLCD();
  }
  delay(1000);
}



void initializeEnvironment(){
  Serial.begin(9600);                 // Start a connection between Genuino and PC
                                      // Genuino communicates at 9600 bit per second
  lcd.createChar(0, degree);   

  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);
  
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);
  
  lcd.begin(16, 2);                   // 16x2 is the display format
  lcd.print("Welcome!");
  delay(5000);
}


void initializeDropboxAuth(){
  lcd.clear();
  lcd.setCursor(0, 0);                          
  lcd.print("Dropbox:");
  lcd.setCursor(0, 1);
  lcd.print("Auth Phase");
  delay(4000);
  
  TembooChoreo InitializeOAuthChoreo(client);
  InitializeOAuthChoreo.begin();
    
  InitializeOAuthChoreo.setAccountName(TEMBOO_ACCOUNT);
  InitializeOAuthChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  InitializeOAuthChoreo.setAppKey(TEMBOO_APP_KEY);
  InitializeOAuthChoreo.addInput("DropboxAppSecret", DropboxAppSecret);
  InitializeOAuthChoreo.addInput("DropboxAppKey", DropboxAppKey);
  InitializeOAuthChoreo.setChoreo("/Library/Dropbox/OAuth/InitializeOAuth");

  unsigned int returnCode = InitializeOAuthChoreo.run();

  if (returnCode == 0) {
    while (InitializeOAuthChoreo.available()) {
      String name = InitializeOAuthChoreo.readStringUntil('\x1F');
      name.trim();
    
      String data = InitializeOAuthChoreo.readStringUntil('\x1E');
      data.trim();
    
      if (name == "AuthorizationURL") {
        Authorization_URL = data;
        Serial.println("Authorization_URL:" + data);
      }
      if (name == "OAuthTokenSecret") {
        OAuth_Token_Secret = data;
        Serial.println("OAuth_Token_Secret:" + data);
      }
      if (name == "CallbackID") {
        Callback_ID = data;
        Serial.println("Callback_ID :" + data);
      }
    }
  }
  InitializeOAuthChoreo.close();
}


void authSMS(){
  TembooChoreo SendSMSChoreo(client);
  SendSMSChoreo.begin();       

  SendSMSChoreo.setAccountName(TEMBOO_ACCOUNT);
  SendSMSChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  SendSMSChoreo.setAppKey(TEMBOO_APP_KEY);
  
  SendSMSChoreo.addInput("AuthToken", AuthToken);
  SendSMSChoreo.addInput("To", SMSToNumber);
  SendSMSChoreo.addInput("From", SMSFromNumber);
  SendSMSChoreo.addInput("Body", Authorization_URL);
  SendSMSChoreo.addInput("AccountSID", AccountSID);      
  SendSMSChoreo.setChoreo("/Library/Twilio/SMSMessages/SendSMS");
  
  SendSMSChoreo.run();                                              
  SendSMSChoreo.close();
}


void finalizeDropboxAuth(){
  TembooChoreo FinalizeOAuthChoreo(client);
  FinalizeOAuthChoreo.begin();

  FinalizeOAuthChoreo.setAccountName(TEMBOO_ACCOUNT);
  FinalizeOAuthChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  FinalizeOAuthChoreo.setAppKey(TEMBOO_APP_KEY);
  FinalizeOAuthChoreo.addInput("DropboxAppSecret", DropboxAppSecret);
  FinalizeOAuthChoreo.addInput("DropboxAppKey", DropboxAppKey);
  FinalizeOAuthChoreo.addInput("OAuthTokenSecret", OAuth_Token_Secret);
  FinalizeOAuthChoreo.addInput("CallbackID", Callback_ID);
  FinalizeOAuthChoreo.setChoreo("/Library/Dropbox/OAuth/FinalizeOAuth");

  unsigned int returnCode = FinalizeOAuthChoreo.run();  
  
  if (returnCode == 0) {
    while (FinalizeOAuthChoreo.available()) {
      String name = FinalizeOAuthChoreo.readStringUntil('\x1F');
      name.trim();
      
      String data = FinalizeOAuthChoreo.readStringUntil('\x1E');
      data.trim();
    
      if (name == "AccessToken") {
        Access_Token = data;
        Serial.println("Access_Token :" + data);
      }
      if (name == "AccessTokenSecret") {
        Access_Token_Secret = data;
        Serial.println("Access_Token_Secret :" + data);
      }
    }
  }
  FinalizeOAuthChoreo.close();
}


void WarningOutputLCD(){ 
  int n=0;
  lcd.createChar(n, degree);
  digitalWrite(8, HIGH);
  digitalWrite(9, LOW);

  lcd.clear();                                      
  lcd.setCursor(0, 0);                          
  lcd.print("Temperature");
  lcd.setCursor(0, 1);
  lcd.print("limit exceeded!");
  delay(3000);

  lcd.clear();                                      
  lcd.setCursor(0, 0);    
  lcd.print("Warning!");
  lcd.setCursor(0, 1);
  
  lcd.print(temperature);       
  lcd.print(" ");
  lcd.write(n);
  lcd.print("C");
  delay(3000);
}


void CorrectOutputLCD(){
    int n=0;
    lcd.createChar(n, degree);
    digitalWrite(8, LOW);
    digitalWrite(9, HIGH);  
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temperature:");
    lcd.setCursor(0, 1);
    lcd.print(temperature);
    lcd.print(" ");
    lcd.write(n);
    lcd.print("C");
}


void TimeLimitSMS(){
  TembooChoreo SendSMSChoreo(client);                 
  SendSMSChoreo.begin();                                             

  SendSMSChoreo.setAccountName(TEMBOO_ACCOUNT);                    
  SendSMSChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  SendSMSChoreo.setAppKey(TEMBOO_APP_KEY);
  
  SendSMSChoreo.setProfile("Twilio");                           
  SendSMSChoreo.setChoreo("/Library/Twilio/SMSMessages/SendSMS");    
  
  SendSMSChoreo.run();                                               
  SendSMSChoreo.close();
}


void getTimeStamp(){
  TembooChoreo GetTimestampChoreo(client);
  GetTimestampChoreo.begin();
  
  GetTimestampChoreo.setAccountName(TEMBOO_ACCOUNT);
  GetTimestampChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  GetTimestampChoreo.setAppKey(TEMBOO_APP_KEY);
  GetTimestampChoreo.setChoreo("/Library/Utilities/Dates/GetTimestamp");
  
  unsigned int returnCode = GetTimestampChoreo.run();

  if (returnCode == 0) {
    while (GetTimestampChoreo.available()) {
      String name = GetTimestampChoreo.readStringUntil('\x1F');
      name.trim();
      
      String data = GetTimestampChoreo.readStringUntil('\x1E');
      data.trim();
        
      if (name == "Timestamp") {
        timestamp = data;
        Serial.println("timestamp:" + data);
      }
    }
  }
  
  GetTimestampChoreo.close();
}


void getDateFromTimeStamp(){
  TembooChoreo GetDateChoreo(client);
  GetDateChoreo.begin();

  GetDateChoreo.setAccountName(TEMBOO_ACCOUNT);
  GetDateChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  GetDateChoreo.setAppKey(TEMBOO_APP_KEY);
  GetDateChoreo.addInput("Timestamp", timestamp);
  GetDateChoreo.setChoreo("/Library/Utilities/Dates/GetDate");
    
  unsigned int returnCode = GetDateChoreo.run();
  
  if (returnCode == 0) {
    while (GetDateChoreo.available()) {
      String name = GetDateChoreo.readStringUntil('\x1F');
      name.trim();
    
      String data = GetDateChoreo.readStringUntil('\x1E');
      data.trim();
    
      if (name == "FormattedDate") {
        datacorrente = data;
        Serial.println("datacorrente:" + data);
      }
    }
  }
  
  GetDateChoreo.close();
}


void encodingMessageBase64(){
  String testo = "Temperature limit exceed on this date\n\nTemperature of warning is: " + String(temperature) + " C";
  TembooChoreo Base64EncodeChoreo(client);
  Base64EncodeChoreo.begin();
  Serial.println("Text for encoding:" + testo);
  Base64EncodeChoreo.setAccountName(TEMBOO_ACCOUNT);
  Base64EncodeChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  Base64EncodeChoreo.setAppKey(TEMBOO_APP_KEY);
  Base64EncodeChoreo.addInput("Text", testo);
  Base64EncodeChoreo.setChoreo("/Library/Utilities/Encoding/Base64Encode");

  unsigned int returnCode = Base64EncodeChoreo.run();

  if (returnCode == 0) {
    while (Base64EncodeChoreo.available()) {
      String name = Base64EncodeChoreo.readStringUntil('\x1F');
      name.trim();
    
      String data = Base64EncodeChoreo.readStringUntil('\x1E');
      data.trim();
    
      if (name == "Base64EncodedText") {
        Serial.println(message);
        message = data;
      }
    }
  }
  
  Base64EncodeChoreo.close();
}


void uploadFileInDropbox(){
  lcd.clear();                                      
  lcd.setCursor(0, 0);                          
  lcd.print("Uploading txt");
  lcd.setCursor(0, 1);
  lcd.print("on Dropbox...");
  delay(4000);

  String nomefile = "" + datacorrente + ".txt";
  TembooChoreo UploadFileChoreo(client);
  UploadFileChoreo.begin();
  Serial.print("name of file:" + nomefile);

  UploadFileChoreo.setAccountName(TEMBOO_ACCOUNT);
  UploadFileChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  UploadFileChoreo.setAppKey(TEMBOO_APP_KEY);
  
  UploadFileChoreo.addInput("AccessTokenSecret", Access_Token_Secret);
  UploadFileChoreo.addInput("AccessToken", Access_Token);
  UploadFileChoreo.addInput("FileContents", message);
  UploadFileChoreo.addInput("AppKey", DropboxAppKey);
  UploadFileChoreo.addInput("FileName", nomefile);
  UploadFileChoreo.addInput("AppSecret", DropboxAppSecret);
  UploadFileChoreo.setChoreo("/Library/Dropbox/FilesAndMetadata/UploadFile");
  
  UploadFileChoreo.run();
  
  lcd.clear();                                      
  lcd.setCursor(0, 0);                          
  lcd.print("Done!");
  delay(4000);
  UploadFileChoreo.close();
}
