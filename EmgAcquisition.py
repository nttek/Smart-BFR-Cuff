import time
import matplotlib.pyplot as plt
from bitalino import BITalino
import pandas as pd
from datetime import datetime

# The macAddress variable on Windows can be "XX:XX:XX:XX:XX:XX" or "COMX"
# while on Mac OS can be "/dev/tty.BITalino-XX-XX-DevB" for devices ending with the last 4 digits of the MAC address or "/dev/tty.BITalino-DevB" for the remaining
# check this by going to devices
macAddress = "COM31"

runningTime = 10  # time in sec
batteryThreshold = 30
acqChannels = [0, 1, 2, 3, 4, 5]  # read signal from all the channels
samplingRate = 1000
nSamples = 1  # read n samples at a time ...returns a list of lists  eg. if nSample =2, output is [[x,x,x,x,x,x,][x,x,x,x,x]] i.e 2 lists in a list
digitalOutput = [1, 1]

EMG_CHANNEL = 0
EMG2_CHANNEL = 1
ECG_CHANNEL = 2
PPG_CHANNEL = 3
ACC_CHANNEL = 4
fNIRS_CHANNEL = 5

# # Connect to Bitalino
device = BITalino(macAddress)  # port com number can also be used e.g com31

# Set battery threshold
device.battery(batteryThreshold)
print(device.version())


# Some Parameters passed have default values, but different values can be passed when calling the function
def read_signals(running_time, sampling_rate=1000, ac_channels=[0, 1, 2, 3, 4, 5], n_samples=10, digital_output=[1, 1]):
    # Start acquiring data from the Bitalino
    device.start(sampling_rate, ac_channels)
    emg_data = pd.DataFrame(columns=["date", "time", "A0", "A1", "A2", "A3", "A4", "A5"])
    date = datetime.today()
    start = time.time()
    end = time.time()
    time_elapsed = end - start
    sample = []
    while time_elapsed < running_time:  # Acquire signals as long as time is not up
        # Get/read n samples at a time
        sample = device.read(n_samples)
        print(sample)
        # get current time
        end = time.time()
        # update time elapsed
        time_elapsed = end - start
        time_elapsed = round(time_elapsed, 2)  # round time to 2 decimal places
        # store the list of samples in sensor_value Note: if nSamples is greater...
        # than 1 device.read(n_samples) returns n list of samples with in a big list
        for i in range(n_samples):
            channel_1 = sample[i][EMG_CHANNEL + 5]
            channel_2 = sample[i][EMG2_CHANNEL + 5]
            channel_3 = sample[i][ECG_CHANNEL + 5]
            channel_4 = sample[i][PPG_CHANNEL + 5]
            channel_5 = sample[i][ACC_CHANNEL + 5]
            channel_6 = sample[i][fNIRS_CHANNEL + 5]

            # row stores info to be made as pandas series and write to a csv file
            row = [date, time_elapsed, channel_1, channel_2, channel_3, channel_4, channel_5, channel_6]
            # print(row)  # to check the values
            # Append row i.e time elapsed and sensor value  as series to emg data which will be writen to emg_data.csv file
            a_series = pd.Series(row, index=emg_data.columns)
            emg_data = emg_data.append(a_series, ignore_index=True)
            emg_data.to_csv('emg_data.csv', mode='a', header=False,
                            index=False)  # save data in csv format in 'a'(append) mode

    print(emg_data.head(20))  # see data in clear format

    # Turn the LED on Bitalino
    device.trigger(digital_output)
    # Stop reading
    device.stop()
    # Close channel
    device.close()


# Calling function
read_signals(runningTime, samplingRate, acqChannels, nSamples, digitalOutput)
