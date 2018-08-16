#!/usr/bin/python

# ignore frivolous warnings
import warnings
warnings.simplefilter(action='ignore', category=FutureWarning)
import tensorflow as tf
tf.logging.set_verbosity(tf.logging.ERROR)

# get the model and the data reorganization
from custom import Data_Obj
from custom import data_func
from custom import train_func

from random import shuffle
from keras.models import Sequential
from keras.layers import Dense
from keras.layers import LSTM
import csv

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
        raw_data.append(Data_Obj(in_sample, out_sample))
    # raw_data = raw_data[1::2] . . . Why do we purposely handicap ourselves?

    # window raw training data
    proc_data = []
    for data in raw_data:
        out_sample = data.out_data
        for i in range(training_size - queue_size):
            in_sample = data.in_data[i:i + queue_size]
            proc_data.append(Data_Obj(in_sample, out_sample))
    shuffle(proc_data)

    # format arguments for keras
    (keras_training_X, keras_training_Y) = data_func(proc_data)

    # run algorithms
    train_func(keras_training_X, keras_training_Y, training_size, classif_num, queue_size)

    print 'successful program exit.'

if __name__ == '__main__':
    main()
