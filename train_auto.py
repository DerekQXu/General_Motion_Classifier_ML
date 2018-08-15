#!/usr/bin/python

# ignore frivolous warnings
import warnings
warnings.simplefilter(action='ignore', category=FutureWarning)
import tensorflow as tf
tf.logging.set_verbosity(tf.logging.ERROR)

from random import shuffle
from keras.models import Sequential
from keras.layers import Dense
from keras.layers import LSTM
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
    model = Sequential()
    model.add(LSTM(30, input_shape=(channels, queue_size)))
    model.add(Dense(classif_num, activation = 'sigmoid'))
    '''
    model.add(LSTM(25, input_shape=(channels, queue_size), dropout=0.2, recurrent_dropout=0.2, return_sequences=True))
    model.add(Dense(10))
    model.add(LSTM(25, dropout=0.2, recurrent_dropout=0.2))
    model.add(Dense(2, activation = 'sigmoid'))
    '''
    model.compile(loss='binary_crossentropy', optimizer='adam', metrics=['accuracy'])

    model.fit(keras_training_X, keras_training_Y, epochs = 5, batch_size = 32)

    # save model
    model.save('model.h5')
    print 'successful program exit.'

if __name__ == '__main__':
    main()
