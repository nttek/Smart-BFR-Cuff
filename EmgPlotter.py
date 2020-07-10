from itertools import count

import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

index = count()


def animate(i):
    emg_data = pd.read_csv('emg_data.csv')
    x = emg_data['time']
    y = emg_data['volts']
    print(x, y)
    plt.cla()  # clear the plots and update the them
    plt.plot(x, y)
    plt.margins(x=0.025, y=0.75)
    plt.title('Emg Signal')
    plt.xlabel('Time(s)')
    plt.ylabel('Amplitude(mv)')
    plt.tight_layout()


ani = FuncAnimation(plt.gcf(), animate,
                    interval=1500)  # FuncAnimation calls the animate function passed every interval seconds e.g 1500
plt.tight_layout()
plt.show()
