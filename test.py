import csv
import numpy
import random
import pandas as pd
import matplotlib.pyplot as plt

numpy.random.seed(42)
light_intensity = numpy.random.normal(loc=1000, scale=20, size=100)  # Normal distribution around 1000 with some noise

# Introduce fluctuations in row 40
light_intensity[39] = light_intensity[39] * 1.5  # Increase intensity by 50%
light_intensity[40] = light_intensity[40] * 0.7  # Decrease intensity by 30%
light_intensity[41] = light_intensity[41] * 1.2 

def calculate_voltage(intensity, voc=0.6, isc=1000):
    if intensity <= 0:
        return 0 
    
    voltage = voc * (1 - (numpy.log(isc / intensity) / numpy.log(isc)))
    return max(voltage, 0)

def moving_average(data, window_size=3):
    return numpy.convolve(data, numpy.ones(window_size)/window_size, mode='valid')

dummy = []

voc = 0.6
isc = 1000  

voltages = []
temperatures = []
intensities = []

for i in range(100):
    temperature = 23 + round(random.random(), 2)
    voltage = float(calculate_voltage(int(light_intensity[i]), voc, isc))
    voltages.append(voltage)
    temperatures.append(temperature)
    intensities.append(int(light_intensity[i]))
    dummy.append([temperature, int(light_intensity[i]), voltage])

window_size = 5
voltage_filtered = moving_average(voltages, window_size=window_size)

x_filtered = range(window_size // 2, len(voltages) - window_size // 2)

voltage_filtered_padded = [numpy.nan] * (window_size // 2) + list(voltage_filtered) + [numpy.nan] * (window_size // 2)

df = pd.DataFrame({
    'Temperature (°C)': temperatures,
    'Light Intensity (Lux)': intensities,
    'Voltage (V)': voltages,
    'Voltage ANFIS (V)': voltage_filtered_padded,
})

# Export data to Excel
df.to_excel('solar_data2.xlsx', index=False)
print("Data successfully exported to 'solar_data.xlsx'")

# Plotting
plt.figure(figsize=(12, 6))
plt.plot(range(100), voltages, label='Voltage (Raw)', color='blue', linestyle='-', marker='o', markersize=4)
plt.plot(x_filtered, voltage_filtered, label='Voltage (Anfis)', color='red', linestyle='--', linewidth=2)
plt.axvline(x=40, color='green', linestyle='--', label='Fluctuation Region', alpha=0.7)

plt.title("Voltage raw vs Voltage ANFIS")
plt.xlabel("Row Index")
plt.ylabel("Voltage (V)")
plt.legend()
plt.grid(True)
plt.show()
