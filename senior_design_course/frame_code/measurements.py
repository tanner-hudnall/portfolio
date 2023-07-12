#Created by Reva Vaidya 2/20/23
#Last updated  1:50pn CST 2/22/23

#Create dependencies
import os
import sys
import time
import pandas as pd
import matplotlib.pyplot as plt
import subprocess
import csv
from time import sleep
from watchdog.observers import Observer
from watchdog.events import LoggingEventHandler

#Parse tegrastats https://developer.ridgerun.com/wiki/index.php/Xavier/JetPack_5.0.2/Performance_Tuning/Evaluating_Performance
#declare variables to print
gpu_temp = 0  #GPU@
cpu_temp = 0  #CPU@
output_file = 'results.csv'


dataframe = pd.DataFrame(list())
dataframe.to_csv(output_file)
with open(output_file, 'w', newline='') as file:
 writer = csv.writer(file)
 writer.writerow(["RAM 1903/1569mal@32.9C PMIC@100C GPU@40 CPU 210/216 SOC 860/864 CV 0/0 VDDRQ 140/144 SYS5V 1880/1889"])
 writer.writerow(["RAM 1903/1569mal@32.9C PMIC@100C GPU@23 CPU 210/216 SOC 860/864 CV 0/0 VDDRQ 140/144 SYS5V 1880/1889"])
 writer.writerow(["RAM 2222/1569mal@32.9C PMIC@100C GPU@90 CPU 210/216 SOC 860/864 CV 0/0 VDDRQ 140/144 SYS5V 1880/1889"])
tegrastats_info = "tegrastats --interval 1000  --logfile results.csv &" #print every 1000 ms
#repeat this task while the Nano is running
#subprocess.Popen(tegrastats_info, shell=True) #write to command and open tegrastats in backgroudn
num_sec = 0
f = open(output_file, 'r')
fig = plt.figure()
x, y = [], []
z = plt.plot(x,y)

# Set up plot to call animate() function periodically
while True:
 new_output = (f.readline())
 temp_idx = new_output.find('GPU@')
 gpu_temp = new_output[temp_idx+4:temp_idx+6]
 temp_idx = new_output.find('CPU@')
 cpu_temp = new_output[temp_idx+4:temp_idx+6]
 num_sec = num_sec + 1
 x.append(gpu_temp)
 y.append(num_sec)
 plt.draw(z)
 sleep(1)

