##################################################################
#              INTERFACE STUFF                                   #
##################################################################

# Kill any existing instances of HighSpeedAlliedVisionInterface
sudo systemctl stop HighSpeedAlliedVisionInterface.service
sudo pkill -f "HighSpeedAlliedVisionInterface"
sudo systemctl disable HighSpeedAlliedVisionInterface.service
sudo rm -r /var/lib/HighSpeedAlliedVisionInterface


# install the required packages
sudo apt-get update
sudo apt-get install -y python3 python3-pip
sudo pip3 install flask paho-mqtt

# Put all the interface stuff in var
sudo mkdir /var/lib/HighSpeedAlliedVisionInterface
sudo cp -R webpage/static /var/lib/HighSpeedAlliedVisionInterface/static
sudo cp -R webpage/templates /var/lib/HighSpeedAlliedVisionInterface/templates
sudo cp webpage/app.py /var/lib/HighSpeedAlliedVisionInterface/app.py

# Copy over the systemd service file
sudo cp HighSpeedAlliedVisionInterface.service  /etc/systemd/system/HighSpeedAlliedVisionInterface.service

# Setup the systemd service
sudo systemctl daemon-reload
sudo systemctl enable HighSpeedAlliedVisionInterface.service
sudo systemctl start HighSpeedAlliedVisionInterface.service

##################################################################
#              PROGRAM STUFF                                     #
##################################################################
# Kill any existing instances of HighSpeedAlliedVisionInterface
sudo systemctl stop HighSpeedAlliedVision.service
sudo pkill -f "HighSpeedAlliedVision"
sudo systemctl disable HighSpeedAlliedVision.service
sudo rm /usr/local/bin/HighSpeedAlliedVision

# install the required packages
sudo apt-get install -y libmagick++-dev mosquitto libmosquitto-dev cmake
sudo cp mosquitto.conf /etc/mosquitto/mosquitto.conf
sudo systemctl restart mosquitto.service
sudo ldconfig

cd ../
git clone git@github.com:DaleGia/AlliedVisionVimbaX.git
cd AlliedVisionVimbaX
sh install.sh
cd ../Lunar-Flashes

# Build the software and move it to /usr/loca/bin
rm -r build
mkdir build
cd build
cmake ..
cmake --build .
sudo cp HighSpeedAlliedVision /usr/local/bin/HighSpeedAlliedVision

cd ..
# Copy over the systemd service file
sudo cp HighSpeedAlliedVision.service  /etc/systemd/system/HighSpeedAlliedVision.service

# Setup the systemd service
sudo systemctl daemon-reload
# Enable but do not start the service
sudo systemctl enable HighSpeedAlliedVision.service
