#!/bin/bash

echo "Removing old SeaBreeze library installation if present..."
rm -rf SeaBreeze

echo "Downloading and building open-source SeaBreeze library..."


echo "Downloading most recent SeaBreeze source release..."
# Download most recent source release of SeaBreeze and unpack
wget https://sourceforge.net/projects/seabreeze/files/SeaBreeze/source/seabreeze-3.0.11.zip
unzip seabreeze-3.0.11.zip
mv seabreeze-3.0.11/SeaBreeze .
rm -rf seabreeze-3.0.11*

# Enter SeaBreeze Directory and perform some patches with sed (default setup is broken on modern
# compilers)
cd SeaBreeze

echo "Fixing various compiler errors in library with sed commands..."

# Use c++98 - lots of `throw` declarations that break with c++11 or higher
sed -i 's/CPPFLAGS     = $(CFLAGS_BASE)/CPPFLAGS     = $(CFLAGS_BASE) -std=c++98/g' common.mk

# Fix compiler error catching polymorphic type ‘class seabreeze::FeatureException’ by value
sed -i 's/Exception) {/Exception const\&) {/g' src/api/seabreezeapi/PixelBinningFeatureAdapter.cpp

# Fix compiler error this ‘if’ clause does not guard...
sed -i '163s/if (logFile == NULL)/ if (logFile == NULL) {/g' src/common/Log.cpp
sed -i '165s/$/    }/g' src/common/Log.cpp
sed -i '180s/$/ {/' src/common/Log.cpp
sed -i '181s/$/\n    }/' src/common/Log.cpp


# Fix compiler error writing to an object of type ‘class std::vector<unsigned char>’ with no trivial copy-assignment; use copy-assignment or copy-initialization instead
sed -i '81s/\&outBuffer\[0\]/outBuffer->data()/g' src/vendors/OceanOptics/buses/usb/BlazeUSBTransferHelper.cpp


# Fix compiler error catching polymorphic type ‘class seabreeze::FeatureException’ by value
sed -i 's/Exception fpnfe) {/Exception const\&) {/g' src/vendors/OceanOptics/features/data_buffer/DataBufferFeatureBase.cpp
sed -i 's/Exception ex) {/Exception const\&) {/g' src/vendors/OceanOptics/features/light_source/LightSourceFeatureBase.cpp
sed -i 's/Exception fpnfe) {/Exception const\&) {/g' src/vendors/OceanOptics/features/spectrum_processing/SpectrumProcessingFeature.cpp
sed -i 's/Exception fpnfe) {/Exception const\&) {/g' src/vendors/OceanOptics/features/thermoelectric/ThermoElectricFeatureBase.cpp
sed -i 's/Exception fpnfe) {/Exception const\&) {/g' src/vendors/OceanOptics/features/pixel_binning/STSPixelBinningFeature.cpp

# Remove demo executable that has some printout compiler errors instead of fixing it
rm sample-code/c/demo-pthreads.c

echo "Building SeaBreeze..."
# Build SeaBreeze
make

echo "Done."

