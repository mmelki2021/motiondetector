# Motion Detector Program
Motion dectection implementation cmake based using c++17
Same code sample implemented using Python 3.12

## How to build

:white_check_mark:  C++ code (you need to install clang and c++ librariries)
chmod +x build.sh

./build.sh

:white_check_mark:  Python Code (to install Python 3.12 release in linux)
cd /usr/src
sudo curl -O https://www.python.org/ftp/python/3.12.0/Python-3.12.0.tgz
sudo tar -xf Python-3.12.0.tgz
cd Python-3.12.0
sudo ./configure --enable-optimizations
sudo make -j$(nproc)
sudo make altinstall

then you can execute script
chmod +x MotionDetector.py




## How to run
executable is generated under ./build_test folder
in linux distribution you can run it as below

./build_test/MotionDetector

## How to execute test Scenario
three scenarios are implemented under flags

DISPLAY_ONLY : random generation of video frames and we display generated frames only

DISPLAY_WITH_DETECTOR : random generation of video frames, detect specific pattern in generated frames(motion) and we display generated frames only

DEFAULT : random generation of video frames, detect specific pattern in generated frames(motion) and we display generated frames only using asychronous queue

C++ Design pattern chain of responsability used to implement solution
Status : we found one pattern in one exactly frame
Next Step : 
1. Find a way to generate randomly frames with insertion of mouvements inside subsequents frames : :fire:
2. find and implement an heuristic to detect motion pattern among multiple frames (display alert Popup for mouvement) :fire: :fire:


