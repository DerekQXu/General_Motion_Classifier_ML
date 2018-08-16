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

import numpy as np
import sys
import os
import csv
import time
from keras.models import load_model

def main():
    classif_mnemo = sys.argv[1].split(':')
    filepath = "testing_data.csv"
    collect_flag = 0

    model = load_model('model.h5')

    while True:
        while not os.path.exists(filepath):
            time.sleep(0.05)

        reader = csv.reader(open(filepath))

        # extract raw training data
        raw_data = []
        for line in reader:
            raw_data.append([float(val) for val in line])
        proc_data = [Data_Obj(raw_data, None)]

        (keras_testing_X, temp) = data_func(proc_data)

        results = model.predict(keras_testing_X, batch_size=32, verbose=0)
        print (results)
        print ('predicted motion: ' + classif_mnemo[np.argmax(results)])

        os.remove(filepath)

if __name__ == '__main__':
    main()
