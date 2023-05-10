# UIConnect
Qt5 AX.25 UI frame only chatting application

Linux users should install libraries:

Debian and derivatives:
````
sudo apt install qt5base-dev libqt5serialport5-dev qt5-qmake
````
and there may be others...report on discord as you find needs please.

Other Linuxes you have to figure out the package names.

Generally, Linux compilation is:
````
mkdir ~/src
cd src
git clone https://github.com/guitarpicva/UIConnect.git
cd UIConnect
mkdir build
cd build
qmake ..
make 
````
Executable is "UIConnect" in the build folder

Copy to a convenient folder and run it from the command line, or create a 
system shortcut if you like.
````
cd ~
mkdir UIConnect
cp src/UIConnect/build/UIConnect .
cd ~/UIConnect
./UIConnect
````
If all goes well, it should run!  Use the File menu to show the configuration items.
They are pretty obvious for configuration tasks.  Choose serial or tcp KISS connection
and supply the appropriate values for either choice.


***Mac users 
My only Mac is a mini that is a dev box, so I have the entire Qt development suite and IDE installed.
Mac deployment is a pain.  Godspeed
