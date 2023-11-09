# Lunar-Flashes
LunarFlashes is a software project that enables high frame rate capture and saving of Allied Vision Alvium imagery. 

# What is this software?
A web interface provides a way to connect to an Allied Vision Alvium camera, start/stop capturing imagery, start/stop saving imagery, and configure difference camera settings such as the camera frame rate, preview frame rate, sensor gain, exposure period, and image format. It is also possible to save and load a previous configuration. When imagery is being captured, preview images will display in real-time, and a histogram of the image will be available along with other statistics.

![image](https://github.com/DaleGia/Lunar-Flashes/assets/31597257/262ae5a6-44cb-4ac1-a552-666d0b90dd26)

# How do I access this web interface
After installation, type '''http://0.0.0.0:1000''' into your browser.

# How does it work?
A simple flask web application provides the interface for the user. This is configured as a systemd service. The backend is a C++ program that is also run as a systemd service. The web interface and backend communicate with eachother through MQTT messages. Once the connect button on the web interface is pushed, it starts the backend services, which looks for cameras to connect to, and ensures a harddrive name '''dfn''' exists. When recording, the backend saves images together in image blobs of 400 images directly to the connected '''dfn''' harddrive. These have to be unpacked at a later date. Preview images and other statistical data are sent from the backend to the web interface via MQTT.

# Installation
Clone this repository and run '''sh install-all.sh'''

# What is Installed when I do this?
VimbaX, the allied vision SDK for the alvium series camera will be installed (via a repositiory located at https://github.com/DaleGia/AlliedVisionVimbaX.git). Other installed packages include:

- python3
- python3-pip
- imagemagick
- libmagick++-dev
- mosquitto
- libmosquitto-dev
- cmake

and python packages 
- flask
- paho-mqtt

