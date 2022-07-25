//Libraries

//If statement for both ESP32 and EP8266 boards
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <Firebase_ESP_Client.h>
#include <SPI.h>
#include <MFRC522.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

//Pins defined for the RFID reader
#define SS_PIN 4  //D2
#define RST_PIN 5 //D1

//Define the WiFi credentials
#define WIFI_SSID "ssid"
#define WIFI_PASSWORD "password"

//Define the API Key
#define API_KEY "AIzaSyD54Mc7MkXhSxzDPJn1YvaLjMJhSF50HrE"

//Define the project ID
#define FIREBASE_PROJECT_ID "smart-bag-tracking-syste-89fc6"

//Define the user Email and password that alreadey registerd or added in your project
#define USER_EMAIL "nodemcu@gmail.com"
#define USER_PASSWORD "123456789"

// Create MFRC522 instance.
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

//Variable for skip points change it according to the skip point number
int node = 1;

//Setup Function
void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    
    Serial.begin(9600);
    SPI.begin();      // Initiate  SPI bus
    mfrc522.PCD_Init();   // Initiate MFRC522
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    //Assign the api key (required)
    config.api_key = API_KEY;

    //Assign the user sign in credentials
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    //Assign the callback function for the long running token generation task
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

#if defined(ESP8266)
    // In ESP8266 required for BearSSL rx/tx buffer for large data handle, increase Rx size as needed.
    fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 2048 /* Tx buffer size in bytes from 512 - 16384 */);
#endif

    // Limit the size of response payload to be collected in FirebaseData
    fbdo.setResponseSize(2048);

    Firebase.begin(&config, &auth);

    Firebase.reconnectWiFi(true);
}

void loop()
{

    // Firebase.ready() should be called repeatedly to handle authentication tasks.
    if ( ! mfrc522.PICC_IsNewCardPresent()) {
      return;//got to start of loop if there is no card present
    }
    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()) {
      return;//if read card serial(0) returns 1, the uid struct contians the ID of the read card.
    }

    //Getting the RFID number
    String CardID ="";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      CardID += mfrc522.uid.uidByte[i];
    }
    Serial.println("Card ID is: "+CardID);

    // For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create.ino
    FirebaseJson content;

    // RFID is the collection id, CardID is the document id in the Firebase.
    String documentPath = "RFID/"+CardID;

    //Upadate the Document in the Firebase
    content.clear();
    content.set("fields/node/integerValue", node);
    
    Serial.print("Update a document... ");
    
    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "node" /* updateMask */)){
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());\

        //Buzzer or LED indication when data sucessfully updated
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
    }
    else{
        Serial.println(fbdo.errorReason());
    }
}
