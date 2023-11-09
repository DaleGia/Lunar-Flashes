##################################################################
#              INTERFACE STUFF                                   #
##################################################################

# Kill any existing instances of LunarFlashesWebInterface
sudo systemctl stop LunarFlashesWebInterface.service
sudo pkill -f "LunarFlashesWebInterface"
sudo systemctl disable LunarFlashesWebInterface.service
sudo rm -r /var/lib/LunarFlashesWebInterface


# Put all the interface stuff in var
sudo mkdir /var/lib/LunarFlashesWebInterface
sudo cp -R webpage/static /var/lib/LunarFlashesWebInterface/static
sudo cp -R webpage/templates /var/lib/LunarFlashesWebInterface/templates
sudo cp webpage/app.py /var/lib/LunarFlashesWebInterface/app.py

# Copy over the systemd service file
sudo cp LunarFlashesWebInterface.service  /etc/systemd/system/LunarFlashesWebInterface.service

# Setup the systemd service
sudo systemctl daemon-reload
sudo systemctl enable LunarFlashesWebInterface.service
sudo systemctl start LunarFlashesWebInterface.service

##################################################################
#              PROGRAM STUFF                                     #
##################################################################


# Kill any existing instances of HighSpeedAlliedVision
sudo systemctl stop LunarFlashes.service
sudo pkill -f "LunarFlashes"
sudo systemctl disable LunarFlashes.service
sudo rm /usr/local/bin/LunarFlashes

# Build the software and move it to /usr/loca/bin
rm -r build
mkdir build
cd build
cmake ..
cmake --build .
sudo cp LunarFlashes /usr/local/bin/LunarFlashes

cd ..
# Copy over the systemd service file
sudo cp LunarFlashes.service  /etc/systemd/system/LunarFlashes.service

# Setup the systemd service
sudo systemctl daemon-reload
# Enable but do not start the service
sudo systemctl enable LunarFlashes.service
