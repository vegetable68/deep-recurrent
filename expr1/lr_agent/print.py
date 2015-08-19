import matplotlib.pyplot as plt
import numpy as np
with open('vWWff.out', 'r') as f:
    array = [[],[],[]]
    i = 0
    for line in f:
        if (i == 2): 
            i = 0
            continue
        array[i].append(float(line))
        i = i + 1
for i in range(0, 2):
    plt.plot(array[i])
plt.legend(("third layer", "second layer", "first layer"))
plt.title('Norm of vWWff per update');
plt.savefig("vWWff.png")

