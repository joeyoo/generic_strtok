import re
import sys

time = 0.0
cpu = 0.0
num_iter = 0

for line in sys.stdin:
    matches = re.findall("^.+_median\s+((?:-|\+)\d+\.\d+)\s+((?:-|\+)\d+\.\d+)", line)
    if matches:
        group = matches[0]
        time += float(group[0])
        cpu += float(group[1])
        num_iter += 1

avg_time = time / num_iter
avg_cpu = cpu / num_iter

str_end = "%"
if avg_time < 0.0:
    str_end += " faster"
else:
    str_end += " slower"

print ("avg_time = " + str(abs(round (avg_time * 100, 2))) + str_end)
print ("avg_cpu = " + str(abs(round (avg_cpu * 100, 2))) + str_end)