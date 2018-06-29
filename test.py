#!/usr/bin/python

import numpy
import sys
from keras.models import load_model

output_names = ['circle', 'triangle', 'rest']
model = load_model('model.h5')

training_x = []
with open('testing_data.csv') as fp:
    data = []
    for i in range (10):
        input = []
        for j in range(10):
            line_raw = fp.readline()
            input.append(map(float,line_raw[:len(line_raw)-1].split(',',256)))
        data.append(numpy.asmatrix(input).transpose())

training_x.append(data)
training_x = numpy.asarray(training_x)

results = model.predict(training_x, batch_size=32, verbose=0)

print('====================================\n')
print('Sample: [P(circle), P(triangle), P(rest)]')
print('Result: ' + str(results) + '\n')
print ('predicted motion: ' + output_names[numpy.argmax(results)] + '\n')
