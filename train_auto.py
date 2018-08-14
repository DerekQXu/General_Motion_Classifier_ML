#!/usr/bin/python

# ignore frivolous warnings
import warnings
warnings.simplefilter(action='ignore', category=FutureWarning)
import tensorflow as tf
tf.logging.set_verbosity(tf.logging.ERROR)

from random import shuffle
from keras.models import Sequential
from keras.layers import Dense
from keras.layers import TimeDistributed
from keras.layers import LSTM
from keras.layers import Flatten
from keras.layers.convolutional import Conv1D
from keras.layers.convolutional import MaxPooling1D
import numpy as np
import csv

class Training_Data():
    def __init__(self, input_data = [[]], output_data = []):
        self.in_data = input_data
        self.out_data = output_data

def main():
    filepath = "training_data.csv"
    reader = csv.reader(open(filepath))

    #extract header
    header = next(reader)
    training_size = int(header[0])
    classif_num = int(header[1])
    queue_size = int(header[2])
    classif_mnemo = []
    for name in header[3:]:
        classif_mnemo.append(name)

    # extract raw training data
    raw_data = []
    for i in range(classif_num):
        out_sample = [int(val) for val in next(reader)]
        in_sample = []
        for j in range(training_size):
            in_sample.append([float(val) for val in next(reader)])
        raw_data.append(Training_Data(in_sample, out_sample))
    raw_data = raw_data[1::2]

    # window raw training data
    proc_data = []
    for data in raw_data:
        out_sample = data.out_data
        for i in range(training_size - queue_size):
            in_sample = data.in_data[i:i + queue_size]
            proc_data.append(Training_Data(in_sample, out_sample))
    shuffle(proc_data)

    # format arguments for keras
    keras_training_X = []
    keras_training_Y = []
    for data in proc_data:
        keras_training_X.append(np.asmatrix(data.in_data).transpose())
        keras_training_Y.append(np.asarray(data.out_data))
    keras_training_X = np.asarray(keras_training_X)
    keras_training_Y = np.asarray(keras_training_Y)
    print keras_training_X.shape

    # run algorithms
    channels = 3
    filter_size = 3
    cnn = Sequential()
    cnn.add(Conv1D(filters=1, kernel_size=3, activation='relu', padding='same', input_shape=(channels, filter_size)))
    cnn.add(MaxPooling1D(pool_size=channels))
    cnn.add(Flatten())
    model = Sequential()
    model.add(TimeDistributed(cnn, input_shape=(queue_size, channels, filter_size)))
    model.add(LSTM(100))
    model.add(Dense(3, activation='sigmoid'))

    model.compile(loss='binary_crossentropy', optimizer='adam', metrics=['accuracy'])
    model.fit(keras_training_X, keras_training_Y, epochs = 3, batch_size = 64)

    # save model
    model.save('model.h5')
    print 'successful program exit.'

if __name__ == '__main__':
    main()
