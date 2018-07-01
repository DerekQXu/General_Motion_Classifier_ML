#!/usr/bin/python

import numpy
from statistics import mean
from keras.models import Sequential
from keras.layers import Dense
from keras.layers import TimeDistributed
from keras.layers import LSTM
from keras.layers import Flatten
from keras.layers.convolutional import Conv1D
from keras.layers.convolutional import MaxPooling1D

SAMPLE_SIZE = 50
TOTAL_SIZE = 1000
training_x = []
training_y = []

with open('training_data.csv') as fp:
    # collect training data
    n_samples = TOTAL_SIZE - SAMPLE_SIZE

    labels = [[1,0,0],[0,1,0],[0,0,1]]

    for i  in range(3):
        line_raw = fp.readline()
        if not line_raw:
            break

        acc_x = []
        acc_y = []
        acc_z = []

        for j in range(TOTAL_SIZE):
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

        for j in range (n_samples):
            data = []
            for k in range(SAMPLE_SIZE - 2):
                window_start = j + k
                window_end = j + k + 3
                data.append(numpy.asmatrix([acc_x[window_start:window_end], \
                                            acc_y[window_start:window_end], \
                                            acc_z[window_start:window_end]]))
            training_x.append(numpy.asarray(data))
            training_y.append(numpy.asarray(labels[i]))

    training_x = numpy.asarray(training_x)
    training_y = numpy.asarray(training_y)

    print(training_x.shape)
    print(training_y.shape)

cnn = Sequential()
cnn.add(Conv1D(filters=1, kernel_size=3, activation='relu', padding='same', input_shape=(3, 3)))
cnn.add(MaxPooling1D(pool_size=3))
cnn.add(Flatten())
model = Sequential()
model.add(TimeDistributed(cnn, input_shape=(48, 3, 3)))
model.add(LSTM(100))
model.add(Dense(3, activation='sigmoid'))
model.compile(loss='binary_crossentropy', optimizer='adam', metrics=['accuracy'])
print(model.summary())
model.fit(training_x, training_y, epochs=3, batch_size=64)
# Final evaluation of the model

model.save('model.h5')
print('Program exit successful')
