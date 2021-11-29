#ifndef VIRALINK_OTAUPDATE_TPP
#define VIRALINK_OTAUPDATE_TPP

//region .h

#include <Arduino.h>
#if defined(ESP8266)
#include <Updater.h>
#elif defined(ESP32)
#include <Update.h>
#endif

#define TOO_LESS_SPACE              (-100)
#define SERVER_FAULTY_MD5           (-105)
#define NO_PARTITION                (-108)

using OTAUpdateStartCB = std::function<void()>;
using OTAUpdateEndCB = std::function<void(bool)>;
using OTAUpdateErrorCB = std::function<void(int)>;
using OTAUpdateProgressCB = std::function<void(int, int)>;

class OTAUpdateClass {
public:
    OTAUpdateClass(void);

    ~OTAUpdateClass(void);

    void rebootOnUpdate(bool reboot) {
        _rebootOnUpdate = reboot;
    }

    bool startUpdate(const uint32_t size, const String &md5);

    bool writeUpdateChunk(uint8_t *data, size_t len);

    bool endUpdate();

    // Notification callbacks
    void onStart(OTAUpdateStartCB cbOnStart) { _cbStart = cbOnStart; }

    void onEnd(OTAUpdateEndCB cbOnEnd) { _cbEnd = cbOnEnd; }

    void onError(OTAUpdateErrorCB cbOnError) { _cbError = cbOnError; }

    void onProgress(OTAUpdateProgressCB cbOnProgress) { _cbProgress = cbOnProgress; }

    int getLastError(void);

    String getLastErrorString(void);

    bool isUpdating() const;

protected:
    bool handleStartUpdate(uint32_t size, const String &md5);

    bool startUpdateProcess(uint32_t size, const String &md5);

    // Set the error and potentially use a CB to notify the application
    void _setLastError(int err) {
        _lastError = err;
        if (_cbError) {
            _cbError(err);
        }
    }

private:

    // Callbacks
    OTAUpdateStartCB _cbStart;
    OTAUpdateEndCB _cbEnd;
    OTAUpdateErrorCB _cbError;
    OTAUpdateProgressCB _cbProgress;

    int _lastError;
    bool _rebootOnUpdate = true;
    size_t _size;
    bool updating;
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_HTTPUPDATE)
extern OTAUpdateClass OTAUpdate;
#endif

//endregion

//region .cpp

#include <StreamString.h>

OTAUpdateClass::OTAUpdateClass(void) {
}

OTAUpdateClass::~OTAUpdateClass(void) {
}

bool OTAUpdateClass::startUpdate(const uint32_t size, const String &md5) {
    return handleStartUpdate(size, md5);
}

int OTAUpdateClass::getLastError(void) {
    return _lastError;
}

String OTAUpdateClass::getLastErrorString(void) {

    if (_lastError == 0) {
        return String(); // no error
    }

    // error from Update class
    if (_lastError > 0) {
        StreamString error;
        Update.printError(error);
        error.trim(); // remove line ending
        return String("Update error: ") + error;
    }

    switch (_lastError) {
        case TOO_LESS_SPACE:
            return "Not Enough space";
        case SERVER_FAULTY_MD5:
            return "Wrong MD5";
        case NO_PARTITION:
            return "Partition Could Not be Found";
    }

    return String();
}

bool OTAUpdateClass::handleStartUpdate(const uint32_t size, const String &md5) {


        int sketchFreeSpace = ESP.getFreeSketchSpace();
        if (!sketchFreeSpace) {
            _lastError = NO_PARTITION;
            return false;
        }

        if (size > sketchFreeSpace) {
            _lastError = TOO_LESS_SPACE;
            return false;
        }

    return startUpdateProcess(size, md5);
}

bool OTAUpdateClass::endUpdate() {

    StreamString error;
    if (!Update.end()) {
        _lastError = Update.getError();
        Update.printError(error);
        error.trim(); // remove line ending
        if (_cbError) _cbError(_lastError);
        return false;
    }

    updating = false;
    if (_cbEnd) _cbEnd(true);

    if (_rebootOnUpdate) {
        ESP.restart();
    }

    return true;
}

bool OTAUpdateClass::startUpdateProcess(uint32_t size, const String &md5) {

    _size = size;

    StreamString error;

    if (_cbProgress) Update.onProgress(_cbProgress);

    if (!Update.begin(size, U_FLASH)) {
        _lastError = Update.getError();
        Update.printError(error);
        error.trim(); // remove line ending
        return false;
    }

    if (md5.length()) {
        if (!Update.setMD5(md5.c_str())) {
            _lastError = SERVER_FAULTY_MD5;
            return false;
        }
    }

    updating = true;
    if (_cbStart) _cbStart();
    return true;
}

bool OTAUpdateClass::writeUpdateChunk(uint8_t *data, size_t len) {
    StreamString error;

    if (updating && Update.write(data, len) != len) {
        updating = false;
        _lastError = Update.getError();
        Update.printError(error);
        error.trim(); // remove line ending
        if (_cbError) _cbError(_lastError);
        return false;
    }
    return true;
}

bool OTAUpdateClass::isUpdating() const {
    return updating;
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_HTTPUPDATE)
OTAUpdateClass OTAUpdate;
#endif
//endregion

#endif