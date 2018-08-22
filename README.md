# General Motion Classifier
### Introduction
General Motion Classifier is software that classifies repetitive motions in real-time over BLE communication
with the STMicorelectronics Sensortile, using optional user-defined Keras + Scipy models.
<br><br>
The required libraries and packages are: Keras (Python), LiquidDSP (C), gatttool, bluetoothctl, and expect.
The software also requires the Sensortile to be running code found in UCLA STMicroelectronics Tutorial 8.
Please refer to the following link for this: http://iot.ee.ucla.edu/UCLA-STMicro/index.php/UCLA_-_STMicroelectronics .
<br><br>
The program allows the user to define their own machine learning and signal processing classification
models in the Python (See Custom Models). But, by default, the current program comes with the LSTM and
the CNN+LSTM machine learning classification models.
<br><br>
Note, models do not interact with raw sensor data. Rather, raw sensor values are first passed through a low-
pass filter and optionally bias-corrected (See Flags and Extensions). Furthermore, as of now, the program
only uses x, y, and z acceleration data from the sensor, discarding the other sensor values.
<br><br>
Note, the current program cannot run on the Beaglebone platform as the space required to store Keras is
too large (despite Keras/Theano supporting 32bit ARM systems). Rather, it is meant for prototyping valid
machine learning models before being ported onto embedded platforms. The current program also cannot
run on Windows systems due to the use of POSIX-defined multi-processing, synchronization, and IPC.
### Getting Started
To run General Motion Classifier with the default CNN+LSTM model, first modify bctl.txt such that the first
line is the MAC address of your computer and the second line is the MAC address of your Sensortile:
<br><br>
Next, run the following command:```$ make program```
<br><br>
Turn on your Sensortile using software from Tutorial 8 of the UCLA STMicroelectronics Tutorials.
<br><br>
Run the following command:```$ ./main (or $ sudo ./main)```
