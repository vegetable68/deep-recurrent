import matplotlib.pyplot as plt
import numpy as np
with open('dhhf.out', 'r') as f:
    array = [[],[],[]]
    i = 0
    for line in f:
        if (i == 3): 
            i = 0
            continue
        array[i].append(float(line))
        i = i + 1
for i in range(0, 3):
    plt.plot(array[2 - i])
plt.legend(("third layer", "second layer", "first layer"))
plt.title("Average of norm of dhhff")
plt.xlabel("number of backward process")
plt.savefig("dhhf.png")

