# QEPro

EPICS support for the Ocean Optics QEPro spectrometer, using the SeaBreeze open-source library.

### Installation

The QEPro driver depends on EPICS and asyn, so those must be installed and available prior to building this driver. You will also need to build the SeaBreeze open source library. The library as it is available to download from the default sourceforge site seems to have a lot of issues building with modern compilers, and as such, I have made a fork that resolves these, and also adds a few more useful API calls. To clone and install it, simply navigate to the `QEProSupport` directory, and run the provided helper script:

```
cd QEProSupport
./fetch-modified-seabreeze.sh
```

Note that the `fetch-seabreeze.sh` script will build SeaBreeze directly from SourceForge using `sed` commands to fix the problematic lines of code - but it will not work with the driver due to the missing API calls.

From there, navigate to this top-level driver directory, and build the QEPro driver by simply running `make`.

Finally, to use the device you will need to ensure that it is accessible to the user running the IOC. Typically this would be accomplished with the help of `udev` rules.


