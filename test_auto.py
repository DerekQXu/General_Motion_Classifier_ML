#!/usr/bin/python

# ignore frivolous warnings
import warnings
warnings.simplefilter(action='ignore', category=FutureWarning)
import tensorflow as tf
tf.logging.set_verbosity(tf.logging.ERROR)

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

        keras_testing_X = np.asmatrix(raw_data).transpose()
        keras_testing_X = np.asarray([keras_testing_X])

        results = model.predict(keras_testing_X, batch_size=32, verbose=0)
        print (results)
        print ('predicted motion: ' + classif_mnemo[np.argmax(results)])

        os.remove(filepath)

if __name__ == '__main__':
    main()
