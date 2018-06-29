#!/usr/bin/python

import numpy
from keras.models import Sequential
from keras.layers import Dense
from keras.layers import TimeDistributed
from keras.layers import LSTM
from keras.layers import Flatten
from keras.layers.convolutional import Conv1D
from keras.layers.convolutional import MaxPooling1D

SAMPLE_SIZE = 100
TOTAL_SIZE = 1000
training_x = []
training_y = []

with open('training_data.csv') as fp:
    # collect training data
    classif_num = 0
    n_hidden = 256
    n_samples = TOTAL_SIZE - SAMPLE_SIZE
    n_timesteps = SAMPLE_SIZE

    while 1:
        line_raw = fp.readline()
        if not line_raw:
            break

        classif_num += 1
        input = []
        output = map(int,line_raw[:len(line_raw)-1].split(',',256))

        for i in range(TOTAL_SIZE):
            line_raw = fp.readline()
            input.append(map(float,line_raw[:len(line_raw)-1].split(',',256)))
        for i in range (n_samples):
            data = []
            for j in range(10):
                window_start = i+j*10
                window_end = i+(j+1)*10
                data.append(numpy.asmatrix(input[window_start:window_end]).transpose())
            training_x.append(data)
            training_y.append(numpy.asarray(output))

    training_x = numpy.asarray(training_x)
    training_y = numpy.asarray(training_y)

    print(training_x.shape)
    print(training_y.shape)

cnn = Sequential()
cnn.add(Conv1D(filters=1, kernel_size=6, activation='relu', padding='same', input_shape=(3, 10)))
cnn.add(MaxPooling1D(pool_size=3))
cnn.add(Flatten())
model = Sequential()
model.add(TimeDistributed(cnn, input_shape=(10, 3, 10)))
model.add(LSTM(100))
model.add(Dense(3, activation='sigmoid'))
model.compile(loss='binary_crossentropy', optimizer='adam', metrics=['accuracy'])
print(model.summary())
model.fit(training_x, training_y, epochs=6, batch_size=64)
# Final evaluation of the model

model.save('model.h5')
print('Program exit successful')
