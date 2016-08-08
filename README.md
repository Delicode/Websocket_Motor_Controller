##Controlling Pololu DRV8835 Dual Motor Driver Kit for Raspberry Pi with NI Mate websockets

###Project goals
The main goal of the project was at first to use a vibrating motor to vibrate the Microsoft Kinect or Asus Xtion Live camera at a specific frequency ([Shake & Sense](https://www.microsoft.com/en-us/research/publication/shakensense-reducing-interference-for-overlapping-structured-light-depth-cameras/)). The motor would be controlled with a Raspberry Pi + Pololu DRV8835 motor driver from the Pi itself. The project evolved into an easier to control system, now being able to control the speed of the motor using NI Mate over the network.

###Devices used
- Raspberry Pi 3
- [Pololu DRV8835 Dual Motor Driver Kit for Raspberry Pi](https://www.pololu.com/product/2753/resources)
- Vibrating DC motor (We used [this](https://www.amazon.co.uk/Vibration-Vibrating-Electric-1-5-6V-5200RPM/dp/B00NQCOOKQ/ref=sr_1_4?ie=UTF8&qid=1470293135&sr=8-4&keywords=vibrating+motor) one)

###Software  / Libraries used
- Raspbian GNU / Linux 8 (Jessie)
- [Boost (1.61)](http://www.boost.org/)
- [Websocket++](https://github.com/zaphoyd/websocketpp)
- [Pololu Python library](https://github.com/pololu/drv8835-motor-driver-rpi)
- [Nlohmann Json](https://github.com/nlohmann/json)
- g++ 4.9 or higher

###Initial configuration
The Pololu will need to be assembled before it can be used, unless it was pre-assembled of course. One small modification can be made to the Pololu while assembling to make it a bit cleaner and that is to add pins to 5V and ground on the board. This will allow us to use the Raspberry Pi's 5v supply to power our motor. If you have a motor which requires more power, then this modification can be skipped, unless you want to be able to get 5v from the board.

After the Pololu has been assembled and connected to our Raspberry Pi we can start to install the necessary libraries.

We start by updating the raspberry pi OS with the following commands:

- `sudo apt-get update`
- `sudo apt-get upgrade`
- `sudo reboot`

We also update the raspberry pi firmware:

- `sudo apt-get install rpi-update`
- `sudo rpi-update`
- `sudo reboot`

After that we will install our libraries by following the installation instructions online:

- [Websocket++](https://github.com/zaphoyd/websocketpp/wiki/Build-on-debian) (Installs with boost)
- [Pololu](https://github.com/pololu/drv8835-motor-driver-rpi) (Follow installation instructions)
- Boost: `sudo apt-get install libboost-all-dev` (Also good to have)
- Download the websocket++ repo from github and extract it to `/home/pi/`
- [Nlohmann Json](https://github.com/nlohmann/json), download and place the json.hpp file into our project folder.

Link the libraries, add the following lines to `/etc/ld.so.conf.d/libc.conf`:

- First open the file:`sudo nano /etc/ld.so.conf.d/libc.conf` then add the following lines:
- `/usr/local/boost_1_61_0/libs` (If newer version was installed, change name accordingly)
- `/usr/local/boost_1_61_0/libbin` (If newer version was installed, change name accordingly)

Add the following line to `/etc/ld.so.conf.d/local.conf`

- Open the file: `sudo nano /etc/ld.so.conf.d/local.conf`
- `/usr/local/boost_1_61_0/libbin/lib` (If newer version was installed, change name accordingly)
- run `sudo ldconfig`

After this we can download all the code that we need for the project from [here](). We need the following files:

- `websocket_motor.cpp`
- `motor_control.py`
- `Makefile`	(The makefile might need to be modified in case you don't use the username **pi**)

To compile we run the make command while in the same directory as our code:

- `make`

Once the program has been compiled we can run it with the command:

- `sudo ./websocket_motor` (sudo due to access to GPIO pins)

####Using the program
- Using the program is very straightforward.
- After starting the program it will ask for an IP address and port number.
- Enter the IP address and port number of the server / NI Mate, according to the example and press **_"enter"_**.
- The motor can now be controlled from **NI Mate** by first inputting the desired speed of the motor and then pressing **_"start_motor"_**
- The motor is controlled using PWM on a scale from 0 to 100, as such the frequency of the motor's vibration will depend on both which motor is being used and the speed of the motor.
- To accurately determine the frequency of the vibration, we used an accelerometer to calculate the frequency. The code we used for that can also be found on the Github page.