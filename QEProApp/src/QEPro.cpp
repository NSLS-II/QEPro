/**
 * Main source file for the QEPro EPICS driver
 *
 * Author: Jakub Wlodek
 *
 * Copyright (c) : Brookhaven National Laboratory, 2022
 *
 */

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>

// EPICS includes
#include <epicsExit.h>
#include <epicsExport.h>
#include <epicsStdio.h>
#include <epicsString.h>
#include <epicsThread.h>
#include <epicsTime.h>
#include <iocsh.h>

// Include Seabreeze api wrapper
#include "api/SeaBreezeWrapper.h"

// Error message formatters
#define ERR(msg)                                                                                 \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "ERROR | %s::%s: %s\n", driverName, functionName, \
              msg)

#define ERR_ARGS(fmt, ...)                                                              \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "ERROR | %s::%s: " fmt "\n", driverName, \
              functionName, __VA_ARGS__);

// Warning message formatters
#define WARN(msg) \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "WARN | %s::%s: %s\n", driverName, functionName, msg)

#define WARN_ARGS(fmt, ...)                                                            \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "WARN | %s::%s: " fmt "\n", driverName, \
              functionName, __VA_ARGS__);

// Log message formatters
#define LOG(msg) \
    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s: %s\n", driverName, functionName, msg)

#define LOG_ARGS(fmt, ...)                                                                       \
    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s: " fmt "\n", driverName, functionName, \
              __VA_ARGS__);

// QEPro include
#include "QEPro.hpp"

// Add any additional namespaces here
using namespace std;

const char* driverName = "QEPro";

/**
 * @brief External configuration function for QEPro.
 *
 * Envokes the constructor to create a new QEPro object
 * This is the function that initializes the driver, and is called in the IOC startup script
 *
 * NOTE: When implementing a new driver with ADDriverTemplate, your camera may use a different
 * connection method than a const char* connectionParam. Just edit the param to fit your device, and
 * make sure to make the same edit to the constructor below
 *
 */
extern "C" int QEProConfig(const char* portName, int deviceIndex, int debugEnable) {
    new QEPro(portName, deviceIndex, debugEnable);
    return (asynSuccess);
}

/**
 * @brief Generate error message string from error code
 *
 * @param errorCode Seabreeze error code
 * @return const char*
 */
static const char* getErrorString(int errorCode) {
    static char buffer[32];
    seabreeze_get_error_string(errorCode, buffer, sizeof(buffer));
    return buffer;
}

/**
 * @brief Callback function called when IOC is terminated.
 *
 * @param pPvt Pointer to QEPro object
 */
static void exitCallbackC(void* pPvt) {
    QEPro* pQEPro = (QEPro*)pPvt;
    delete pQEPro;
}

/**
 * @brief Wrapper function that spawns main acquisition thread
 *
 * @param pPvt Pointer to QEPro object
 */
static void threadWorker(void* pPvt) {
    QEPro* qepro = (QEPro*)pPvt;
    qepro->getSpectrumThread(qepro);
}

/**
 * @brief Simple function that prints all information about a connected device
 */
void QEPro::printConnectedDeviceInfo() {
    printf("--------------------------------------\n");
    printf("Connected to QEPro device\n");
    printf("--------------------------------------\n");
    // Add any information you wish to print about the device here

    printf("--------------------------------------\n");
}

void QEPro::errLogToStatus(const char* msg, const char* functionName) {
    ERR(msg);
    setIntegerParam(QEProStatus, 1);
    setStringParam(QEProStatusMsg, msg);
    callParamCallbacks();
}

void QEPro::logToStatus(const char* msg, const char* functionName) {
    LOG(msg);
    setIntegerParam(QEProStatus, 0);
    setStringParam(QEProStatusMsg, msg);
    callParamCallbacks();
}

/**
 * @brief Function that is used to initialize and connect to the device.
 *
 * @return asynStatus
 */
asynStatus QEPro::connectToDeviceQEPro() {
    const char* functionName = "connectToDeviceQEPro";
    bool connected = false;

    LOG_ARGS("Opening spectrometer with index %d...", this->deviceIndex);
    flag = seabreeze_open_spectrometer(this->deviceIndex, &(this->errorCode));
    LOG_ARGS("Result is (%d) [%s]", flag, getErrorString(this->errorCode));

    if (flag == 0) {
        setIntegerParam(QEProConnected, 1);
        callParamCallbacks();
        return asynSuccess;
    } else {
        errLogToStatus("Failed to connect to device", functionName);
        return asynError;
    }
}

/**
 * @brief Function that disconnects from any connected device
 *
 * First checks if is connected, then if it is, it frees the memory
 * for the info and the camera
 *
 * @return asynStatus
 */
asynStatus QEPro::disconnectFromDeviceQEPro() {
    const char* functionName = "disconnectFromDeviceQEPro";

    // Free up any data allocated by driver here, and call the vendor libary to disconnect

    printf("Closing spectrometer with index %d...\n", this->deviceIndex);
    this->flag = seabreeze_close_spectrometer(this->deviceIndex, &(this->errorCode));
    printf("Result is [%s]\n", getErrorString(this->errorCode));
    setIntegerParam(QEProConnected, 0);
    callParamCallbacks();

    seabreeze_shutdown();

    return asynSuccess;
}

/**
 * @brief Override of superclass connect function
 *
 * @param pasynUser
 * @return asynStatus
 */
asynStatus QEPro::connect(asynUser* pasynUser) { return connectToDeviceQEPro(); }

/**
 * @brief Override of superclass disconnect function
 *
 * @param pasynUser
 * @return asynStatus
 */
asynStatus QEPro::disconnect(asynUser* pasynUser) { return disconnectFromDeviceQEPro(); }

/**
 * @brief Function that updates PV values with camera information
 *
 * Gets the model, serial number, supported features, and limits
 *
 * @return asynStatus
 */
asynStatus QEPro::getDeviceInformation() {
    const char* functionName = "getDeviceInformation";
    asynStatus status = asynSuccess;
    LOG("Collecting device information");

    char type[16];
    seabreeze_get_model(this->deviceIndex, &(this->errorCode), type, sizeof(type));
    setStringParam(QEProModel, type);

    // Done twice to avoid lockup (not sure if needed, was in SDK example)
    char serial_number[32];
    this->flag = seabreeze_get_serial_number(this->deviceIndex, &(this->errorCode), serial_number,
                                             sizeof(serial_number));
    serial_number[31] = '\0';
    // this->flag = seabreeze_get_serial_number(this->deviceIndex, &(this->errorCode),
    // serial_number, sizeof(serial_number)); serial_number[31] = '\0';
    setStringParam(QEProSerial, serial_number);

    if (checkFeature(HAS_LIGHTSOURCE_FEATURE)) {
        int light_source_count =
            seabreeze_get_light_source_count(this->deviceIndex, &(this->errorCode));
        setIntegerParam(QEProLightSourceCount, light_source_count);
    } else
        setIntegerParam(QEProLightSourceCount, 0);

    long minIntegrationTime, maxIntegrationTime;
    minIntegrationTime =
        seabreeze_get_min_integration_time_microsec(this->deviceIndex, &(this->errorCode));
    maxIntegrationTime =
        seabreeze_get_max_integration_time_microsec(this->deviceIndex, &(this->errorCode));
    setDoubleParam(QEProMinIntegrationTime, (float)(minIntegrationTime / 1000));
    setDoubleParam(QEProMaxIntegrationTime, (float)(maxIntegrationTime / 1000));

    unsigned long capacity = 0;
    unsigned long maxCapacity = 0;
    unsigned long minCapacity = 0;
    unsigned long count = 0;
    if (checkFeature(HAS_BUFFER_FEATURE)) {
        minCapacity = seabreeze_get_buffer_capacity_minimum(this->deviceIndex, &(this->errorCode));
        maxCapacity = seabreeze_get_buffer_capacity_maximum(this->deviceIndex, &(this->errorCode));
        capacity = seabreeze_get_buffer_capacity(this->deviceIndex, &(this->errorCode));
        count = seabreeze_get_buffer_element_count(this->deviceIndex, &(this->errorCode));
    }
    setIntegerParam(QEProMaxBuffCapacity, (int)maxCapacity);
    setIntegerParam(QEProMinBuffCapacity, (int)minCapacity);
    setIntegerParam(QEProBuffCapacity, (int)capacity);
    setIntegerParam(QEProBuffElementCount, (int)count);

    if (checkFeature(HAS_EDC_FEATURE)) {
        this->darkPixelCount = seabreeze_get_electric_dark_pixel_indices(
            this->deviceIndex, &(this->errorCode), this->darkPixelIndices, MAX_DARK_PIXELS);
    }

    int formattedLen =
        seabreeze_get_formatted_spectrum_length(this->deviceIndex, &(this->errorCode));
    // int unformattedLen = seabreeze_get_unformatted_spectrum_length(this->deviceIndex,
    // &(this->errorCode));
    setIntegerParam(QEProFormattedSpectLen, formattedLen);

    callParamCallbacks();

    return status;
}

/**
 * @brief Function that checks spectrometer features
 *
 * Sets features PV as a bit array, with each bit representing an individual feature
 *
 */
void QEPro::checkDeviceFeatures() {
    const char* functionName = "checkDeviceFeatures";
    int features = 0;
    seabreeze_set_tec_enable(this->deviceIndex, &(this->errorCode), 0);
    if (this->errorCode == 0)
        features = features | HAS_TEC_FEATURE;
    else
        WARN("TEC feature not supported");

    int minCapacity = seabreeze_get_buffer_capacity_minimum(this->deviceIndex, &(this->errorCode));
    if (this->errorCode == 0)
        features = features | HAS_BUFFER_FEATURE;
    else
        WARN("Buffer feature not supported");

    int test;
    int supported =
        seabreeze_get_electric_dark_pixel_indices(this->deviceIndex, &(this->errorCode), &test, 1);
    if (0 == supported)
        WARN("Electric dark correction is not supported for this device.");
    else
        features = features | HAS_EDC_FEATURE;

    int has_irrad = seabreeze_has_irrad_collection_area(this->deviceIndex, &(this->errorCode));
    if (has_irrad == 0)
        WARN("IRRAD collection area not stored on device");
    else
        features = features | HAS_IRRAD_COLLECT_AREA;

    int light_source_count =
        seabreeze_get_light_source_count(this->deviceIndex, &(this->errorCode));
    if (this->errorCode == 0)
        features = features | HAS_LIGHTSOURCE_FEATURE;
    else
        WARN("Light source feature not supported");

    int copied;
    int slotIndex;
    unsigned char eepromBytes[24] = {0};
    for (int i = 0, slotIndex = 6; i < 8; i++, slotIndex++) {
        copied = seabreeze_read_eeprom_slot(this->deviceIndex, &(this->errorCode), slotIndex,
                                            eepromBytes, 24);
        if (copied == 0) {
            WARN("Non-Linearity correction feature not supported.");
            break;
        }
        eepromBytes[copied] = '\0';
        this->nonLinearityCoeffs[i] = atof((char*)eepromBytes);
        // If we get to the last iteration of the loop without breaking, we have NLC
        if (i == 7) features = features | HAS_NONLINEARITY_CORRECTION;
    }

    setIntegerParam(QEProFeatures, features);
}

/**
 * @brief Checks if feature is supported by the spectrometer
 *
 * Simple '&' bitwise operation checks if the feature is supported
 *
 * @param feature Feature code
 * @return bool
 */
bool QEPro::checkFeature(QEProFeature_t feature) {
    const char* functionName = "checkFeature";
    int features;
    getIntegerParam(QEProFeatures, &features);
    return (feature & features) == feature;
}

/**
 * @brief Sets the integration time for the spectra
 *
 * @param integrationTime integration time in ms, up to 3 decimal points
 * @return asynStatus
 */
asynStatus QEPro::setIntegrationTime(double integrationTime) {
    const char* functionName = "setIntegrationTime";
    double minIntegrationTime, maxIntegrationTime;
    getDoubleParam(QEProMinIntegrationTime, &minIntegrationTime);
    getDoubleParam(QEProMaxIntegrationTime, &maxIntegrationTime);
    asynStatus status = asynSuccess;

    if (integrationTime < minIntegrationTime || integrationTime > maxIntegrationTime) {
        ERR_ARGS("Integration time must be between %lf and %lf ms!", minIntegrationTime,
                 maxIntegrationTime);
        status = asynError;
    } else {
        seabreeze_set_integration_time_microsec(this->deviceIndex, &(this->errorCode),
                                                (unsigned long)(integrationTime * 1000));
        if (this->errorCode == 0) {
            LOG_ARGS("Set integration time to %lf ms", integrationTime);
        } else {
            ERR_ARGS("Failed to set integration time to %lf ms", integrationTime);
            status = asynError;
        }
    }
    return status;
}

/**
 * @brief Sets the spectrometer buffer capacity
 *
 * @param capacity New target capacity, between 1 and the max capacity
 * @return asynStatus
 */
asynStatus QEPro::setBufferCapacity(int capacity) {
    const char* functionName = "setBufferCapacity";
    int minCapacity, maxCapacity, currCapacity;
    getIntegerParam(QEProMinBuffCapacity, &minCapacity);
    getIntegerParam(QEProMaxBuffCapacity, &maxCapacity);
    getIntegerParam(QEProBuffCapacity, &currCapacity);
    asynStatus status = asynSuccess;

    if (capacity < minCapacity || capacity > maxCapacity) {
        ERR_ARGS("Buffer capacity must be between %d and %d!", minCapacity, maxCapacity);
        status = asynError;
    } else {
        // Clear the buffers first
        seabreeze_clear_buffer(this->deviceIndex, &(this->errorCode));
        if (capacity == currCapacity) {
            LOG_ARGS("Buffer capacity already set to %d", currCapacity);
        } else {
            seabreeze_set_buffer_capacity(this->deviceIndex, &(this->errorCode), capacity);
            if (this->errorCode == 0) {
                LOG_ARGS("Set buffer capacity time to %d", capacity);
            } else {
                ERR_ARGS("Failed to set buffer capacity to %d", capacity);
                status = asynError;
            }
        }
    }

    return status;
}

/**
 * @brief Function ran periodically to collect status info from the spectrometer
 *
 * @return asynStatus
 */
asynStatus QEPro::checkStatus() {
    const char* functionName = "checkStatus";

    double temp = 0;
    if (checkFeature(HAS_TEC_FEATURE))
        temp = seabreeze_read_tec_temperature(this->deviceIndex, &(this->errorCode));
    setDoubleParam(QEProCurrTECTemp, temp);
    int bufferElementCount =
        seabreeze_get_buffer_element_count(this->deviceIndex, &(this->errorCode));
    setIntegerParam(QEProBuffElementCount, bufferElementCount);
    return asynSuccess;
}

/**
 * @brief Function for allocating buffers for all spectra once at startup
 *
 */
void QEPro::allocateBuffers() {
    const char* functionName = "allocateBuffers";
    int connected;
    getIntegerParam(QEProConnected, &connected);
    if (connected == 1) {
        int formattedLen;
        // int unformattedLen;
        getIntegerParam(QEProFormattedSpectLen, &formattedLen);
        // getIntegerParam(QEProUnformattedSpectLen, &unformattedLen);

        this->wavelengths = (double*)calloc(formattedLen, sizeof(double));
        this->dark = (double*)calloc(formattedLen, sizeof(double));
        this->reference = (double*)calloc(formattedLen, sizeof(double));
        this->sample = (double*)calloc(formattedLen, sizeof(double));
        this->output = (double*)calloc(formattedLen, sizeof(double));
        this->averaged = (double*)calloc(formattedLen, sizeof(double));
        this->averagedSample = (double*)calloc(formattedLen, sizeof(double));
    }
}

/**
 * @brief Function for clearing allocated spectra buffers
 *
 */
void QEPro::clearBuffers() {
    int connected;
    getIntegerParam(QEProConnected, &connected);
    if (connected == 1) {
        free(this->wavelengths);
        free(this->dark);
        free(this->reference);
        free(this->sample);
        free(this->output);
        free(this->averaged);
        free(this->averagedSample);
    }
}

/**
 * @brief Utility function for subtracting two spectra into another one
 *
 * @param outputData Pointer to output array
 * @param rawData Pointer to raw data
 * @param correction pointer to array to subtract
 * @param formattedLen length of all three arrays
 */
void QEPro::subtractSpectra(double* outputData, const double* rawData, const double* correction,
                            int formattedLen, bool abs) {
    for (int i = 0; i < formattedLen; i++) {
        outputData[i] = rawData[i] - correction[i];
        if (abs && outputData[i] < 0) outputData[i] = outputData[i] * -1;
    }
}


void QEPro::calculateAbsorbtion(double* outputData, const double* rawData, const double* rawDark, const double* rawReference, int formattedLen){
    for(int i = 0; i< formattedLen; i++){
        double ratio = (rawData[i] - dark[i]) / (rawReference[i] - rawDark[i]);
        outputData[i] = -1 * log10(ratio);
    }
}

/**
 * @brief Function for performing electric dark correction
 *
 * @param spectrum data spectrum
 * @param formattedLen length of spectrum
 */
void QEPro::performElectricDarkCorrection(double* spectrum, int formattedLen) {
    /* Device has no electric dark pixels, so bail out. */
    if (!checkFeature(HAS_EDC_FEATURE) || 0 == this->darkPixelCount) return;

    double baseline = 0;
    int i;

    for (i = 0; i < this->darkPixelCount; i++) {
        baseline += spectrum[this->darkPixelIndices[i]];
    }
    baseline /= (double)this->darkPixelCount;

    for (i = 0; i < formattedLen; i++) {
        spectrum[i] -= baseline;
    }
}

/**
 * @brief Function for performing nonlinearity correction
 *
 * @param spectrum data spectrum
 * @param formattedLen length of spectrum
 */
void QEPro::performNonLinearityCorrection(double* spectrum, int formattedLen) {
    if (!checkFeature(HAS_NONLINEARITY_CORRECTION)) return;

    if (this->nonLinearityCoeffs[0] == 0 ||
        this->nonLinearityCoeffs[0] == this->nonLinearityCoeffs[1]) {
        /* This indicates that correct coefficients were probably not programmed
         * into the device.  Rather than divide by zero below, this will just
         * bail out.
         */
        return;
    }

    int pixel;
    double power;
    int i;
    for (pixel = 0; pixel < formattedLen; pixel++) {
        /* Evaluate the 7th order polynomial for each pixel's intensity and
         * apply the nonlinearity correction.
         */
        power = spectrum[pixel];
        double y = this->nonLinearityCoeffs[0];
        for (i = 1; i < 8; i++) {
            y += power * this->nonLinearityCoeffs[i];
            power *= spectrum[pixel];
        }
        spectrum[pixel] /= y;
    }
}

/*
 * Function overwriting asynPortDriver base function.
 * Takes in a function (PV) changes, and a value it is changing to, and processes the input
 *
 * @params[in]: pasynUser       -> asyn client who requests a write
 * @params[in]: value           -> int32 value to write
 * @return: asynStatus      -> success if write was successful, else failure
 */
asynStatus QEPro::writeInt32(asynUser* pasynUser, epicsInt32 value) {
    int function = pasynUser->reason;
    int status = asynSuccess;
    static const char* functionName = "writeInt32";

    if (function == QEProBuffCapacity) {
        if (checkFeature(HAS_BUFFER_FEATURE)) {
            status = setBufferCapacity(value);
        } else {
            errLogToStatus("Device does not support the buffer feature.", "setBuffCapacity");
        }
    } else if (function == QEProStrobe) {
        seabreeze_set_strobe_enable(this->deviceIndex, &(this->errorCode), value);
        if (this->errorCode != 0) status = asynError;
    } else if (function == QEProTEC) {
        if (checkFeature(HAS_TEC_FEATURE)) {
            seabreeze_set_tec_enable(this->deviceIndex, &(this->errorCode), value);
            if (this->errorCode != 0) status = asynError;
            else {
                setIntegerParam(QEProDarkAvailable, 0);
                setIntegerParam(QEProRefAvailable, 0);
            }
        } else {
            errLogToStatus("Device does not support the TEC feature.", "setTECEnable");
            status = asynError;
        }
    } else if (function == QEProNLC) {
        if (!checkFeature(HAS_NONLINEARITY_CORRECTION)) {
            errLogToStatus("Non-Linearity correction feature not supported by device.",
                           "enableNLC");
            status = asynError;
        }
    } else if (function == QEProEDC) {
        if (!checkFeature(HAS_EDC_FEATURE)) {
            errLogToStatus("Electric-Dark correction feature not supported by device.",
                           "enableNLC");
            status = asynError;
        }
    } else if (function == QEProXAxisFormat) {
        setIntegerParam(QEProXAxisAvailable, 0);
    } else if (function == QEProTriggerMode) {
        seabreeze_set_trigger_mode(this->deviceIndex, &(this->errorCode), value);
        if (this->errorCode != 0) status = asynError;
    } else if (function == QEProCheckStatus) {
        status = checkStatus();
    } else if (function == QEProShutter) {
        seabreeze_set_shutter_open(this->deviceIndex, &(this->errorCode), value);
        if (this->errorCode != 0) status = asynError;
    } else if (function == QEProLightSource && checkFeature(HAS_LIGHTSOURCE_FEATURE)) {
        int num_light_sources;
        getIntegerParam(QEProLightSourceCount, &num_light_sources);
        for (int i = 0; i < num_light_sources; i++) {
            seabreeze_set_light_source_enable(this->deviceIndex, &(this->errorCode), i, value);
        }
    } else if (function == QEProCollect && value == QEPRO_COLLECTING) {
        // If we start collecting, reset the number of spectra collected to 0.
        status = setIntegerParam(QEProSpectraCollected, 0);
    } else if (function == QEProNumSpectra) {
        if (value < 1) {
            status = asynError;
            errLogToStatus("Number of spectra cannot be less than 1!", "setNumSpectra");
        }
    } else if (function < FIRST_QEPRO_PARAM) {
        status = asynPortDriver::writeInt32(pasynUser, value);
    }

    if (status) {
        ERR_ARGS("ERROR status=%d, function=%d, value=%d, msg=%s", status, function, value,
                 getErrorString(this->errorCode));
        return asynError;
    } else if (function != QEProCheckStatus) {  // Don't log period checkStatus PV processing
        status = setIntegerParam(function, value);
        LOG_ARGS("function=%d value=%d", function, value);
    }
    callParamCallbacks();
    return asynSuccess;
}

/*
 * Function overwriting asynPortDriver base function.
 * Takes in a function (PV) changes, and a value it is changing to, and processes the input
 * This is the same functionality as writeInt32, but for processing doubles.
 *
 * @params[in]: pasynUser       -> asyn client who requests a write
 * @params[in]: value           -> int32 value to write
 * @return: asynStatus      -> success if write was successful, else failure
 */
asynStatus QEPro::writeFloat64(asynUser* pasynUser, epicsFloat64 value) {
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static const char* functionName = "writeFloat64";
    if (function == QEProIntegrationTime) {
        status = setIntegrationTime((double)value);
    } else if (function == QEProTECTemp && checkFeature(HAS_TEC_FEATURE)) {
        seabreeze_set_tec_temperature(this->deviceIndex, &(this->errorCode), value);
        if (this->errorCode != 0) status = asynError;
    } else if (function == QEProLightSourceIntensity && checkFeature(HAS_LIGHTSOURCE_FEATURE)) {
        int num_light_sources;
        getIntegerParam(QEProLightSourceCount, &num_light_sources);
        for (int i = 0; i < num_light_sources; i++) {
            seabreeze_set_light_source_intensity(this->deviceIndex, &(this->errorCode), i, value);
        }
    } else if (function < FIRST_QEPRO_PARAM) {
        status = asynPortDriver::writeFloat64(pasynUser, value);
    }

    if (status) {
        ERR_ARGS("ERROR status=%d, function=%d, value=%f, msg=%s", status, function, value,
                 getErrorString(this->errorCode));
    } else {
        status = setDoubleParam(function, value);
        LOG_ARGS("function=%d value=%f", function, value);
    }

    callParamCallbacks();
    return status;
}

/*
 * Function used for reporting ADUVC device and library information to a external
 * log file. The function first prints all libuvc specific information to the file,
 * then continues on to the base asynPortDriver 'report' function
 *
 * @params[in]: fp      -> pointer to log file
 * @params[in]: details -> number of details to write to the file
 * @return: void
 */
void QEPro::report(FILE* fp, int details) {
    const char* functionName = "report";
    int height;
    int width;
    LOG("Reporting to external log file");
    if (details > 0) {
        fprintf(fp, " Connected Device Information\n");

        asynPortDriver::report(fp, details);
    }
}

void QEPro::getSpectrumThread(void* pPvt) {
    const char* functionName = "getSpectrumThread";

    LOG("Starting Spectrum acquisition thread...");
    int deviceConnected;
    getIntegerParam(QEProConnected, &deviceConnected);

    // Loop forever while we are connected
    while (deviceConnected == 1) {
        int collectionMode;
        int spectrumType = 0;
        int collectStatus;
        int raman;
        int correction;
        int darkAvailable, refAvailable;
        int spectraCollected;
        int numSpectraToAverage;
        int edcCorrection, nonLinearityCorrection;
        int abs;
        bool performAbs = false;

        getIntegerParam(QEProCollect, &collectStatus);
        getIntegerParam(QEProCollectMode, &collectionMode);
        getIntegerParam(QEProXAxisFormat, &raman);
        getIntegerParam(QEProCorrection, &correction);
        getIntegerParam(QEProSpectrumType, &spectrumType);
        getIntegerParam(QEProDarkAvailable, &darkAvailable);
        getIntegerParam(QEProRefAvailable, &refAvailable);
        getIntegerParam(QEProSpectraCollected, &spectraCollected);
        getIntegerParam(QEProNumSpectra, &numSpectraToAverage);
        getIntegerParam(QEProEDC, &edcCorrection);
        getIntegerParam(QEProNLC, &nonLinearityCorrection);
        getIntegerParam(QEProSubtractFormat, &abs);
        if (abs == 1) performAbs = true;


        if (collectStatus == QEPRO_COLLECTING) {
            int formattedLen;
            getIntegerParam(QEProFormattedSpectLen, &formattedLen);

            // Case 1, we are collecting dark frame
            if (spectrumType == QEPRO_SPECTRUM_DARK) {
                LOG("Collecting dark spectrum...");
                seabreeze_get_formatted_spectrum(this->deviceIndex, &(this->errorCode), this->dark,
                                                 formattedLen);

                if (edcCorrection == 1 && checkFeature(HAS_EDC_FEATURE))
                    performElectricDarkCorrection(this->dark, formattedLen);

                if (nonLinearityCorrection && checkFeature(HAS_NONLINEARITY_CORRECTION))
                    performNonLinearityCorrection(this->dark, formattedLen);

                doCallbacksFloat64Array(this->dark, formattedLen, QEProDark, 0);
                setIntegerParam(QEProDarkAvailable, 1);
                logToStatus("Collected dark spectrum", functionName);

            }

            // Case 2, collect reference frame
            else if (spectrumType == QEPRO_SPECTRUM_REFERENCE) {
                // Subtract the dark spectrum from the reference one.
                
                seabreeze_get_formatted_spectrum(this->deviceIndex, &(this->errorCode),
                                                 this->reference, formattedLen);

                if (edcCorrection == 1 && checkFeature(HAS_EDC_FEATURE))
                    performElectricDarkCorrection(this->reference, formattedLen);

                if (nonLinearityCorrection && checkFeature(HAS_NONLINEARITY_CORRECTION))
                    performNonLinearityCorrection(this->reference, formattedLen);

                doCallbacksFloat64Array(this->reference, formattedLen, QEProReference, 0);
                setIntegerParam(QEProRefAvailable, 1);
                logToStatus("Collected reference spectrum.", functionName);

            }
            // Case 3, collecting sample spectra. If desired, compute average and absorbtion
            else {
                
                // Check if correction spectra are available, if yes, subtract them to get our final
                // formatted spectrum
                if ((correction == QEPRO_CORRECTION_REF || spectrumType == QEPRO_SPECTRUM_ABSORBTION) &&
                    (darkAvailable != 1 || refAvailable != 1)) {
                    errLogToStatus(
                        "Reference and dark spectra required for selected mode!",
                        functionName);
                    setIntegerParam(QEProCollect, QEPRO_IDLE);
                } else if (correction == QEPRO_CORRECTION_DARK && darkAvailable != 1) {
                    errLogToStatus("Dark spectrum required for sleected mode!",
                                   functionName);
                    setIntegerParam(QEProCollect, QEPRO_IDLE);
                } else {
                    // Collect our sample spectrum
                    seabreeze_get_formatted_spectrum(this->deviceIndex, &(this->errorCode),
                                                     this->sample, formattedLen);

                    // Perform EDC and NLC corrections if supported and selected
                    if (edcCorrection == 1 && checkFeature(HAS_EDC_FEATURE))
                        performElectricDarkCorrection(this->sample, formattedLen);

                    if (nonLinearityCorrection && checkFeature(HAS_NONLINEARITY_CORRECTION))
                        performNonLinearityCorrection(this->sample, formattedLen);

                    // Perform any selected dark/reference correction or absorbtion calculations
                    if (spectrumType == QEPRO_SPECTRUM_ABSORBTION){
                        calculateAbsorbtion(this->output, this->sample, this->dark, this->reference, formattedLen);
                    } else if (correction == QEPRO_CORRECTION_REF) {
                        // For reference correction we have:
                        // final =  raw - dark - ( rawReference - dark )
                        subtractSpectra(this->output, this->sample, this->reference,
                                        formattedLen, true);
                    } else if (correction == QEPRO_CORRECTION_DARK) {
                        // For dark correction subtract the dark spectrum
                        subtractSpectra(this->output, this->sample, this->dark,
                                        formattedLen, true);
                    } else {
                        // If we don't apply corrections, just feed out the raw spectrum
                        memcpy(this->output, this->sample, formattedLen * sizeof(double));
                    }

                    // Increment our spectra collected counter by one
                    spectraCollected++;
                    setIntegerParam(QEProSpectraCollected, spectraCollected);

                    // In single mode, or if the num spectra to average is one, just output the
                    // final spectrum In average or continuous modes, compute average first, then
                    // output.
                    if (collectionMode == QEPRO_ACQUISITION_SINGLE || numSpectraToAverage == 1) {
                        logToStatus("Wrote out spectrum.", functionName);
                        doCallbacksFloat64Array(this->sample, formattedLen,
                                                QEProSample, 0);
                        doCallbacksFloat64Array(this->output, formattedLen, QEProOutput, 0);
                    } else if (collectionMode == QEPRO_ACQUISITION_AVERAGE ||
                               collectionMode == QEPRO_ACQUISITION_CONTINUOUS) {
                        // Add collected spectrum to average
                        for (int i = 0; i < formattedLen; i++) {
                            this->averaged[i] += this->output[i];
                            this->averagedSample[i] += this->sample[i];
                        }

                        if (spectraCollected == numSpectraToAverage) {
                            // calculate the final averages for each value
                            for (int i = 0; i < formattedLen; i++) {
                                this->averaged[i] =
                                    this->averaged[i] / numSpectraToAverage;
                                this->averagedSample[i] = 
                                    this->averagedSample[i] / numSpectraToAverage;
                            }

                            doCallbacksFloat64Array(this->averaged, formattedLen,
                                                    QEProOutput, 0);
                            doCallbacksFloat64Array(this->averagedSample, formattedLen,
                                                    QEProSample, 0);
                            logToStatus("Wrote out averaged output spectrum.", functionName);
                            // Reset average buffer back to 0s
                            for (int i = 0; i < formattedLen; i++) {
                                this->averaged[i] = 0;
                                this->averagedSample[i] = 0;
                            }

                            // In continuous mode set number of spectra back to 0 for averaging
                            // purposes.
                            if (collectionMode == QEPRO_ACQUISITION_CONTINUOUS)
                                setIntegerParam(QEProSpectraCollected, 0);
                        }
                    }
                }
                   
            }

//                int iwavelengthsAvailable;
 //               getIntegerParam(QEProXAxisAvailable, &wavelengthsAvailable);

   //             if (wavelengthsAvailable == 0 && collectedSpectrum == 1){
                    // Get our wavelengths array 
                    seabreeze_get_wavelengths(this->deviceIndex, &(this->errorCode),
                                              this->wavelengths, formattedLen);

                    // X-axis callback, convert to Raman if requested, otherwise push out
                    // wavelengths (in nm)
                    if (raman == 1) {
                        // Convert wavelength nanometers to raman shift
                        for (int i = 0; i < formattedLen; i++) {
                            // TODO 522 "LASER" shouldn't be hard-coded.
                            this->wavelengths[i] = (1. / 522 - 1. / this->wavelengths[i]) * 10e7;
                        }
                    }
                    doCallbacksFloat64Array(this->wavelengths, formattedLen, QEProXAxis, 0);
      //              setIntegerParam(QEProXAxisAvailable, 1);
     //           }

            // In single mode, or if we collected a dark/reference frame, stop after one grab
            // In average mode, stop after we collect one average set.
            // In continuous mode we don't stop until the user specifies
            if (collectionMode == QEPRO_ACQUISITION_SINGLE || (spectrumType != QEPRO_SPECTRUM_CORRECTED_SAMPLE && spectrumType != QEPRO_SPECTRUM_ABSORBTION)) {
                setIntegerParam(QEProCollect, QEPRO_IDLE);
            } else if (spectraCollected == numSpectraToAverage &&
                       collectionMode == QEPRO_ACQUISITION_AVERAGE) {
                setIntegerParam(QEProCollect, QEPRO_IDLE);
            }
        }

        callParamCallbacks();

        // Check again if device is connected before loop again
        getIntegerParam(QEProConnected, &deviceConnected);

        // TODO - do we need this sleep here?
        epicsThreadSleep(0.15);
    }
    LOG("Exiting spectrum acquisition thread...");
}

//----------------------------------------------------------------------------
// QEPro Constructor/Destructor
//----------------------------------------------------------------------------

QEPro::QEPro(const char* portName, int deviceIndex, int debugEnable)
    : asynPortDriver(
          portName, 1, /* maxAddr */
          (int)NUM_QEPRO_PARAMS,
          asynInt32Mask | asynFloat64Mask | asynFloat64ArrayMask | asynDrvUserMask |
              asynOctetMask, /* Interface mask */
          asynInt32Mask | asynFloat64Mask | asynFloat64ArrayMask |
              asynOctetMask, /* Interrupt mask */
          0, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
          1, /* Autoconnect */
          0, /* Default priority */
          0) /* Default stack size*/
{
    static const char* functionName = "QEPro";

    this->deviceIndex = deviceIndex;

    if (debugEnable == 1) seabreeze_set_logfile(NULL, 0);

    createParam(QEProSerialString, asynParamOctet, &QEProSerial);
    createParam(QEProModelString, asynParamOctet, &QEProModel);
    createParam(QEProConnectedString, asynParamInt32, &QEProConnected);
    createParam(QEProFeaturesString, asynParamInt32, &QEProFeatures);
    createParam(QEProStrobeString, asynParamInt32, &QEProStrobe);

    // Light Source params
    createParam(QEProLightSourceString, asynParamInt32, &QEProLightSource);
    createParam(QEProLightSourceCountString, asynParamInt32, &QEProLightSourceCount);
    createParam(QEProLightSourceIntensityString, asynParamFloat64, &QEProLightSourceIntensity);

    // Integration Time
    createParam(QEProIntegrationTimeString, asynParamFloat64, &QEProIntegrationTime);
    createParam(QEProMaxIntegrationTimeString, asynParamFloat64, &QEProMaxIntegrationTime);
    createParam(QEProMinIntegrationTimeString, asynParamFloat64, &QEProMinIntegrationTime);

    // Buffers
    createParam(QEProMinBuffCapacityString, asynParamInt32, &QEProMinBuffCapacity);
    createParam(QEProMaxBuffCapacityString, asynParamInt32, &QEProMaxBuffCapacity);
    createParam(QEProBuffCapacityString, asynParamInt32, &QEProBuffCapacity);
    createParam(QEProBuffElementCountString, asynParamInt32, &QEProBuffElementCount);

    // Thermo Electric Cooler
    createParam(QEProTECString, asynParamInt32, &QEProTEC);
    createParam(QEProTECTempString, asynParamFloat64, &QEProTECTemp);
    createParam(QEProCurrTECTempString, asynParamFloat64, &QEProCurrTECTemp);

    // Spectra Params
    createParam(QEProSampleSpectrumString, asynParamFloat64Array, &QEProSample);
    createParam(QEProOutputSpectrumString, asynParamFloat64Array, &QEProOutput);
    createParam(QEProFormattedSpectLenString, asynParamInt32, &QEProFormattedSpectLen);
    createParam(QEProCorrectionString, asynParamInt32, &QEProCorrection);
    createParam(QEProXAxisString, asynParamFloat64Array, &QEProXAxis);
    createParam(QEProSpectrumTypeString, asynParamInt32, &QEProSpectrumType);
    createParam(QEProDarkAvailableString, asynParamInt32, &QEProDarkAvailable);
    createParam(QEProRefAvailableString, asynParamInt32, &QEProRefAvailable);
    createParam(QEProDarkString, asynParamFloat64Array, &QEProDark);
    createParam(QEProReferenceString, asynParamFloat64Array, &QEProReference);
    createParam(QEProXAxisAvailableString, asynParamInt32, &QEProXAxisAvailable);
    createParam(QEProSubtractFormatString, asynParamInt32, &QEProSubtractFormat);

    createParam(QEProEDCString, asynParamInt32, &QEProEDC);
    createParam(QEProNLCString, asynParamInt32, &QEProNLC);

    createParam(QEProNumSpectraString, asynParamInt32, &QEProNumSpectra);
    createParam(QEProSpectraCollectedString, asynParamInt32, &QEProSpectraCollected);

    createParam(QEProCollectModeString, asynParamInt32, &QEProCollectMode);
    createParam(QEProCollectString, asynParamInt32, &QEProCollect);
    createParam(QEProXAxisFormatString, asynParamInt32, &QEProXAxisFormat);

    createParam(QEProTriggerModeString, asynParamInt32, &QEProTriggerMode);

    createParam(QEProCheckStatusString, asynParamInt32, &QEProCheckStatus);
    createParam(QEProShutterString, asynParamInt32, &QEProShutter);

    createParam(QEProStatusString, asynParamInt32, &QEProStatus);
    createParam(QEProStatusMsgString, asynParamOctet, &QEProStatusMsg);

    asynStatus connected = connectToDeviceQEPro();
    if (connected == asynSuccess) {
        LOG("Acquiring device information");
        checkDeviceFeatures();
        getDeviceInformation();
        // printConnectedDeviceInfo();
        allocateBuffers();
        logToStatus("Device connected.", functionName);
    }

    // when epics is exited, delete the instance of this class
    epicsAtExit(exitCallbackC, (void*)this);

    // Create our thread options struct
    epicsThreadOpts opts;
    opts.priority = epicsThreadPriorityMedium;
    opts.stackSize = epicsThreadGetStackSize(epicsThreadStackMedium);
    opts.joinable = 1;

    // Spawn our data collection thread. Make it joinable
    this->spectrumThread =
        epicsThreadCreateOpt("QEProSpectrumThread", (EPICSTHREADFUNC)threadWorker, this, &opts);
}

QEPro::~QEPro() {
    const char* functionName = "~QEPro";
    LOG("Disconnecting QEPro device...");
    this->lock();
    disconnectFromDeviceQEPro();
    clearBuffers();
    this->unlock();
    epicsThreadMustJoin(this->spectrumThread);
    LOG("Shutdown complete.");
}

//-------------------------------------------------------------
// QEPro ioc shell registration
//-------------------------------------------------------------

/* QEProConfig -> These are the args passed to the constructor in the epics config function */
static const iocshArg QEProConfigArg0 = {"portName", iocshArgString};

// This parameter must be customized by the driver author. Generally a URL, Serial Number, ID, IP
// are used to connect.
static const iocshArg QEProConfigArg1 = {"deviceIndex", iocshArgInt};

// This parameter must be customized by the driver author. Generally a URL, Serial Number, ID, IP
// are used to connect.
static const iocshArg QEProConfigArg2 = {"debugEnable", iocshArgInt};

/* Array of config args */
static const iocshArg* const QEProConfigArgs[] = {&QEProConfigArg0, &QEProConfigArg1,
                                                  &QEProConfigArg2};

/* what function to call at config */
static void configQEProCallFunc(const iocshArgBuf* args) {
    QEProConfig(args[0].sval, args[1].ival, args[2].ival);
}

/* information about the configuration function */
static const iocshFuncDef configQEPro = {"QEProConfig", 3, QEProConfigArgs};

/* IOC register function */
static void QEProRegister(void) { iocshRegister(&configQEPro, configQEProCallFunc); }

/* external function for IOC register */
extern "C" {
epicsExportRegistrar(QEProRegister);
}
