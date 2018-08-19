import numpy as np

from keras.models import Sequential
from keras.layers import Dense
from keras.layers import LSTM
from keras.layers import TimeDistributed
from keras.layers import Flatten
from keras.layers.convolutional import Conv1D
from keras.layers.convolutional import MaxPooling1D

class Data_Obj():
    def __init__(self, input_data = [[]], output_data = []):
        self.in_data = input_data
        self.out_data = output_data


def data_func(raw_data):
    # Data Manipulation (CNN+LSTM)
    keras_X = []
    keras_Y = []
    temp = []
    filter_size = 3
    for data in raw_data:
        temp.append(np.asmatrix(data.in_data).transpose())
        if data.out_data != None:
            keras_Y.append(np.asarray(data.out_data))
    for temp_X in temp:
        real_X = []
        for i in range(temp_X.shape[1]-filter_size+1):
            real_X.append(temp_X[:,0+i:filter_size+i])
        keras_X.append(np.asarray(real_X))

    keras_X = np.asarray(keras_X)
    keras_Y = np.asarray(keras_Y)

    return (keras_X, keras_Y)

    '''
    # Data Manipulation (LSTM)
    keras_X = []
    keras_Y = []
    for data in raw_data:
        keras_X.append(np.asmatrix(data.in_data).transpose())
        if data.out_data != None:
            keras_Y.append(np.asarray(data.out_data))
    keras_X = np.asarray(keras_X)
    keras_Y = np.asarray(keras_Y)
    return (keras_X, keras_Y)
    '''

def train_func(keras_training_X, keras_training_Y, training_size, classif_num, queue_size, channels = 3):
    # CNN LSTM Network
    filter_size = 3
    cnn = Sequential()
    cnn.add(Conv1D(filters=1, kernel_size=3, activation='relu', padding='same', input_shape=(channels, filter_size)))
    cnn.add(MaxPooling1D(pool_size=channels))
    cnn.add(Flatten())
    model = Sequential()
    model.add(TimeDistributed(cnn, input_shape=(queue_size-filter_size+1, channels, filter_size)))
    model.add(LSTM(100))
    model.add(Dense(classif_num, activation='sigmoid'))
    model.compile(loss='binary_crossentropy', optimizer='adam', metrics=['accuracy'])
    model.fit(keras_training_X, keras_training_Y, epochs = 3, batch_size = 64)

    # save model
    model.save('model.h5')

    '''
    # basic LSTM Network
    model = Sequential()
    model.add(LSTM(100, input_shape=(channels, queue_size)))
    model.add(Dense(classif_num, activation = 'sigmoid'))
    model.compile(loss='binary_crossentropy', optimizer='adam', metrics=['accuracy'])
    model.fit(keras_training_X, keras_training_Y, epochs = 5, batch_size = 64)

    # save model
    model.save('model.h5')
    '''
