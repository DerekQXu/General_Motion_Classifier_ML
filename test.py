#!/usr/bin/python

import numpy
import sys
from statistics import mean
from keras.models import load_model

SAMPLE_SIZE = 50
model = load_model('model.h5')
output_names = ['circle', 'triangle', 'rest']

with open('testing_data.csv') as fp:
    acc_x = []
    acc_y = []
    acc_z = []

    for j in range(SAMPLE_SIZE):
        line_raw = fp.readline()
        acc_all = map(float,line_raw[:len(line_raw)-1].split(',',256))
        acc_x.append(acc_all[0])
        acc_y.append(acc_all[1])
        acc_z.append(acc_all[2])

    acc_x_mean = mean(acc_x)
    acc_y_mean = mean(acc_y)
    acc_z_mean = mean(acc_z)

    acc_x[:] = [x - acc_x_mean for x in acc_x]
    acc_y[:] = [x - acc_y_mean for x in acc_y]
    acc_z[:] = [x - acc_z_mean for x in acc_z]

    data = []
    for i in range(SAMPLE_SIZE - 2):
        window_start = i
        window_end = i + 3
        data.append(numpy.asmatrix([acc_x[window_start:window_end], \
                                    acc_y[window_start:window_end], \
                                    acc_z[window_start:window_end]]))

data = numpy.asarray(data)
data = numpy.asarray([data])

results = model.predict(data, batch_size=32, verbose=0)
print (results)
print ('predicted motion: ' + output_names[numpy.argmax(results)])
