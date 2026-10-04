#include <string>
#include <nimBLEDevice.h>
using namespace std;

extern volatile bool NEW_NOTIFICATION;
extern volatile bool NEW_TIME;
extern string NOTIFICATION_RAW_DATA;
extern int TIME_SYNC[];

class notif{
public:
    notif(){
        this->app = "?";
        this->header = "?";
        this->body = "?";
        this->readStatus = false;    
    }
    notif(string app, string header, string body, bool read_status = false) {
    this->app = app;
    this->header = header;
    this->body = body;
    this->readStatus = read_status;
}
    void setApp(string app);
    void setBody(string body);
    void setHeader(string header);
    void setReadStatus(bool read_status);
    string getApp();
    string getBody();
    string getHeader();
    bool getReadStatus();
private:
    string app;
    string body;
    string header;
    bool readStatus;
};  

void notif::setApp(string app){
    this->app = app;
}

void notif::setBody(string body){
    this->body = body;  
}

void notif::setHeader(string header){
    this->header = header;
}

void notif::setReadStatus(bool read_status){
    this->readStatus = read_status;
}

string notif::getApp(){
    return app;
}

string notif::getHeader(){
    return header;
}

string notif::getBody(){
    return body;
}

bool notif::getReadStatus(){
    return readStatus;
}

class ServerCallbacks : public NimBLEServerCallbacks {
 void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
        Serial.printf("Client connected: %s\n", connInfo.getAddress().toString().c_str());
        Serial.println("Client connected!");
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
        Serial.printf("Client disconnected: %s, reason: %d\n", connInfo.getAddress().toString().c_str(), reason);
    }
};
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *pCharacteristic,NimBLEConnInfo& connInfo) override {
        string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0) {
            Serial.print("Write called for characteristic: ");
            Serial.println(rxValue.c_str());
            string temp = string(rxValue.c_str());
            if(temp[0] == '0'){
                NOTIFICATION_RAW_DATA = temp; 
                NEW_NOTIFICATION = true; 
            }else if(temp[0] == '1'){
                int currPlace = 0;
                int prevPlace = 1;
                int num;
                string toNum;
                for(int i = 0; i < 6; i++){
                    currPlace = temp.find(':',prevPlace);
                    if(currPlace == string::npos){
                        toNum = temp.substr(prevPlace, temp.length() - prevPlace);
                    }else{
                        toNum = temp.substr(prevPlace, currPlace - prevPlace);
                    }
                    prevPlace = currPlace + 1;
                    num = stoi(toNum);
                    TIME_SYNC[i] = num;
                }
                NEW_TIME = true;
            }else if(temp[0] == '2'){
                Serial.println("Setting change");
            }
        }
    }
};