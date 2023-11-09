##################################################################
#              DEPENDENCIES STUFF                                #
##################################################################
# install the required packages
sudo apt-get update
sudo apt-get install -y python3 python3-pip imagemagick libmagick++-dev mosquitto libmosquitto-dev cmake
sudo cp mosquitto.conf /etc/mosquitto/mosquitto.conf
sudo systemctl restart mosquitto.service
sudo pip3 install flask paho-mqtt
sudo ldconfig

##################################################################
#              Allied Vision STUFF                               #
##################################################################
cd ../
git clone git@github.com:DaleGia/AlliedVisionVimbaX.git
cd AlliedVisionVimbaX
bash -x install.sh

cd ../Lunar-Flashes

bash -x install.sh