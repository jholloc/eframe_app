import numpy as np
import matplotlib
from matplotlib import pyplot as plt

matplotlib.use("module://matplotlib.backends.html5_canvas_backend")

x = np.arange(0, 10, 0.01)
y = np.sin(x)

plt.plot(x, y)
plt.show()