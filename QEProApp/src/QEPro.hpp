#ifndef QEPRO_H
#define QEPRO_H
#include <asynPortDriver.h>
#include <epicsExport.h>
#include <iocsh.h>

#include "api/SeaBreezeWrapper.h"

// Device information
#define QEProSerialString "QEPRO_SERIAL"
#define QEProModelString "QEPRO_MODEL"

// Device connected & features
#define QEProConnectedString "QEPRO_CONNECTED"
#define QEProFeaturesString "QEPRO_FEATURES"

// Integration time
#define QEProIntegrationTimeString "QEPRO_INTEGRATION_TIME"
#define QEProMinIntegrationTimeString "QEPRO_MIN_INT_TIME"
#define QEProMaxIntegrationTimeString "QEPRO_MAX_INT_TIME"

// Various togglable modes
#define QEProStrobeString "QEPRO_STROBE"
#define QEProShutterString "QEPRO_SHUTTER"
#define QEProEDCString "QEPRO_EDC"
#define QEProNLCString "QEPRO_NLC"

// Thermo-Electric Cooler
#define QEProTECTempString "QEPRO_TEC_TEMP"
#define QEProTECString "QEPRO_TEC"
#define QEProCurrTECTempString "QEPRO_CURR_TEC_TEMP"

// Light sources feature
#define QEProLightSourceString "QEPRO_LIGHT_SOURCE"
#define QEProLightSourceCountString "QEPRO_LIGHT_SOURCE_CNT"
#define QEProLightSourceIntensityString "QEPRO_LIGHT_SOURCE_INTENSITY"

// Buffer Feature
#define QEProMinBuffCapacityString "QEPRO_MIN_BUFF_CAPACITY"
#define QEProMaxBuffCapacityString "QEPRO_MAX_BUFF_CAPACITY"
#define QEProBuffCapacityString "QEPRO_BUFF_CAPACITY"
#define QEProBuffElementCountString "QEPRO_BUFF_ELEMENT_CNT"

// Acquisiton settings
#define QEProCollectModeString "QEPRO_COLLECT_MODE"
#define QEProTriggerModeString "QEPRO_TRIGGER_MODE"
#define QEProCollectString "QEPRO_COLLECT"
#define QEProSubtractFormatString "QEPRO_SUB_FORMAT"
#define QEProCorrectionString "QEPRO_CORRECTION"
#define QEProSpectrumTypeString "QEPRO_SPECTRUM_TYPE"

// Various spectra arrays and info
#define QEProFormattedSpectLenString "QEPRO_FORMATTED_SPECT_LEN"
#define QEProXAxisFormatString "QEPRO_X_AXIS_FORMAT"
#define QEProXAxisAvailableString "QEPRO_X_AXIS_AVAILABLE"
#define QEProXAxisString "QEPRO_X_AXIS"
#define QEProDarkAvailableString "QEPRO_DARK_AVAILABLE"
#define QEProDarkString "QEPRO_DARK_SPECT"
#define QEProRefAvailableString "QEPRO_REF_AVAILABLE"
#define QEProReferenceString "QEPRO_REF_SPECT"
#define QEProSampleSpectrumString "QEPRO_SAMPLE_SPECT"
#define QEProOutputSpectrumString "QEPRO_OUTPUT_SPECT"

// Counter for spectra collected
#define QEProNumSpectraString "QEPRO_NUM_SPECTRA"
#define QEProSpectraCollectedString "QEPRO_SPECTRA_COLLECTED"

// Status/error state records
#define QEProCheckStatusString "QEPRO_CHECK_STATUS"
#define QEProStatusString "QEPRO_ERR"
#define QEProStatusMsgString "QEPRO_STATUS"

typedef enum QEProFeature {
    HAS_NONLINEARITY_CORRECTION = 32,
    HAS_LIGHTSOURCE_FEATURE = 16,
    HAS_EDC_FEATURE = 8,
    HAS_BUFFER_FEATURE = 4,
    HAS_TEC_FEATURE = 2,
    HAS_IRRAD_COLLECT_AREA = 1
} QEProFeature_t;

typedef enum QEProCorrection {
    QEPRO_CORRECTION_NONE = 0,
    QEPRO_CORRECTION_DARK = 1,
    QEPRO_CORRECTION_REF = 2
} QEProCorrection_t;

typedef enum QEProAcquisitionMode {
    QEPRO_ACQUISITION_SINGLE = 0,
    QEPRO_ACQUISITION_AVERAGE = 1,
    QEPRO_ACQUISITION_CONTINUOUS = 2
} QEProAcquisitionMode_t;

typedef enum QEProSpectrumType {
    QEPRO_SPECTRUM_DARK = 0,
    QEPRO_SPECTRUM_REFERENCE = 1,
    QEPRO_SPECTRUM_CORRECTED_SAMPLE = 2,
    QEPRO_SPECTRUM_ABSORBTION = 3
} QEProSpectrumType_t;

typedef enum QEProCollectionStatus { QEPRO_IDLE = 0, QEPRO_COLLECTING = 1 } QEProCollectionStatus_t;

#define MAX_DARK_PIXELS 32

class QEPro : public asynPortDriver {
   public:
    QEPro(const char* portName, int deviceIndex, int debugEnable);
    ~QEPro();

    /* These are the methods that we override from asynPortDriver */
    // virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    virtual asynStatus writeInt32(asynUser* pasynUser, epicsInt32 value);
    // virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
    virtual asynStatus writeFloat64(asynUser* pasynUser, epicsFloat64 value);
    // virtual asynStatus readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t
    // *nActual, int *eomReason);
    virtual asynStatus connect(asynUser* pasynUser);
    virtual asynStatus disconnect(asynUser* pasynUser);
    virtual void report(FILE* fp, int details);

    /* Thread function to read spectra from device */
    // virtual void getSpectrumThread(void *);
    // Function running on seperate thread that performs async spectrum collection
    virtual void getSpectrumThread(void*);

   protected:
    int QEProSerial;
#define FIRST_QEPRO_PARAM QEProSerial
    int QEProModel;
    int QEProFeatures;

    // Integration Time
    int QEProIntegrationTime;
    int QEProMaxIntegrationTime;
    int QEProMinIntegrationTime;

    int QEProStrobe;

    int QEProConnected;

    // Thermal Electric Cooler
    int QEProTECTemp;
    int QEProTEC;
    int QEProCurrTECTemp;

    // Light Source Feature
    int QEProLightSource;
    int QEProLightSourceCount;
    int QEProLightSourceIntensity;

    int QEProFormattedSpectLen;
    int QEProXAxis;
    int QEProXAxisAvailable;

    int QEProSpectrumType;
    int QEProDarkAvailable;
    int QEProRefAvailable;
    int QEProDark;
    int QEProReference;
    int QEProSample;
    int QEProOutput;
    int QEProCorrection;
    int QEProSubtractFormat;

    int QEProEDC;
    int QEProNLC;
    // Collection Specific Params
    int QEProCollect;
    int QEProCollectMode;
    int QEProXAxisFormat;

    int QEProMinBuffCapacity;
    int QEProMaxBuffCapacity;
    int QEProBuffCapacity;
    int QEProBuffElementCount;

    int QEProNumSpectra;
    int QEProSpectraCollected;

    int QEProTriggerMode;
    int QEProShutter;
    int QEProCheckStatus;
    int QEProStatus;
    int QEProStatusMsg;
#define LAST_QEPRO_PARAM QEProStatusMsg
   private:
    int deviceIndex;
    int flag;
    int errorCode;

    int darkPixelIndices[MAX_DARK_PIXELS];
    int darkPixelCount;

    float nonLinearityCoeffs[8] = {0};

    // Buffers (allocated once on startup for better perf)
    double* wavelengths;
    double* dark;
    double* reference;
    double* sample;
    double* output;
    double* averaged;
    double* averagedSample;

    // Functions for allocating memories for the various spectrum buffers
    void allocateBuffers();
    void clearBuffers();

    // ID for thread created to perform spectrum collection
    epicsThreadId spectrumThread = NULL;

    // Utility functions
    asynStatus getDeviceInformation();
    void printConnectedDeviceInfo();

    // Functions for connecting/disconnecting device
    asynStatus connectToDeviceQEPro();
    asynStatus disconnectFromDeviceQEPro();

    // Collects information on device features into bit array
    void checkDeviceFeatures();
    bool checkFeature(QEProFeature_t feature);

    // Function for performing async periodic status checks every second
    asynStatus checkStatus();

    void subtractSpectra(double* outputData, const double* rawData, const double* correction,
                         int formattedLen, bool abs);
    void addSpectrumToAverage(double* spectrum, int formattedLen);
    void calculateAbsorbtion(double* outputData, const double* rawData, const double* rawDark, const double* rawReference, int formattedLen);

    void performElectricDarkCorrection(double* spectrum, int formattedLen);
    void performNonLinearityCorrection(double* spectrum, int formattedLen);

    // Spectrometer operations
    asynStatus setIntegrationTime(double integrationTime);
    asynStatus setBufferCapacity(int capacity);

    // logging utilities
    void errLogToStatus(const char* msg, const char* functionName);
    void logToStatus(const char* msg, const char* functionName);
};

#define NUM_QEPRO_PARAMS ((int)(&LAST_QEPRO_PARAM - &FIRST_QEPRO_PARAM + 1))
#endif
