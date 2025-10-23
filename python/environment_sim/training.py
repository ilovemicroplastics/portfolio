from tensorflow.keras import models, layers
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import MinMaxScaler
import numpy as np
import pickle
import random
import simulation as sim


# generates lidar data to train on randomly from simulation
def generate_data(scenario_walls, possible_locations, label, iterations, file_name):
    x_data, y_data = [], []

    p_l = []
    for i in range(len(possible_locations[0])):
        for j in range(len(possible_locations)):
            if possible_locations[j][i] == label:
                p_l.append([i*5, j*5])
    p_r = range(0, 360, 2)

    s = sim.Sim(1, scenario_walls, 0, 0, 0)

    curr = 0
    while curr < iterations:
        print(f"{curr+1}/{iterations}")
        l = random.choice(p_l)
        d = random.choice(p_r)
        lidar = s.selected_lidar_scan(l[0], l[1], d)
        x_data.append(lidar)
        y_data.append(label)
        curr += 1

    data = [x_data, y_data]
    with open(f'{file_name}.pkl', 'wb') as f:
        pickle.dump(data, f)


def train_cnn(data_list, save=False, file_name=""):

    s1 = np.zeros((0, 72))
    s2 = np.zeros((0,))

    # splitting the dataset
    x_train, y_train, x_val, y_val, x_test, y_test = s1, s2, s1, s2, s1, s2
    for i in range(len(data_list)):
        data = data_list[i]

        x_data, y_data = data[0], data[1]
        x_train_temp, x_temp_temp, y_train_temp, y_temp_temp = train_test_split(np.array(x_data),  np.array(y_data), test_size=0.2,random_state=40)
        x_val_temp, x_test_temp, y_val_temp, y_test_temp = train_test_split(x_temp_temp, y_temp_temp, test_size=0.5, random_state=40)

        x_train = np.concatenate((x_train, x_train_temp), axis=0)
        y_train = np.concatenate((y_train, y_train_temp), axis=0)
        x_val = np.concatenate((x_val, x_val_temp), axis=0)
        y_val = np.concatenate((y_val, y_val_temp), axis=0)
        x_test = np.concatenate((x_test, x_test_temp), axis=0)
        y_test = np.concatenate((y_test, y_test_temp), axis=0)

    # definition
    model = models.Sequential()
    model.add(layers.Conv1D(filters=36, kernel_size=3, activation="relu", input_shape=(72, 1)))
    model.add(layers.Conv1D(filters=256, kernel_size=9, strides=2, activation="relu"))
    model.add(layers.Conv1D(filters=512, kernel_size=3, strides=2, activation="relu"))
    model.add(layers.Conv1D(filters=512, kernel_size=3, strides=2, activation="relu"))
    model.add(layers.MaxPooling1D(pool_size=3))
    model.add(layers.Flatten())
    model.add(layers.Dropout(rate=0.1))
    model.add(layers.Dense(1024, activation='relu'))
    model.add(layers.Dropout(rate=0.1))
    model.add(layers.Dense(1024, activation='relu'))
    model.add(layers.Dense(3, activation='softmax'))

    model.compile(optimizer='adam', loss='sparse_categorical_crossentropy', metrics=['accuracy'])

    # training
    history = model.fit(x_train, y_train, epochs=15, batch_size=40, validation_data=(x_val, y_val))

    # testing
    loss, accuracy = model.evaluate(x_test, y_test)
    print("Loss:", loss)
    print("Accuracy:", accuracy)

    if save:
        model.save(f"{file_name}.keras")


def combine_data(data):
    x, y = [], []
    for d in data:
        x += d[0]
        y += d[1]
    return [x, y]


def normalise_data(data):
    x = data[0]
    s = MinMaxScaler()
    x_normal = s.fit_transform(x)
    data[0] = x_normal
    return data


def read_data(file_name):
    with open(f"{file_name}.pkl", "rb") as f:
        data = pickle.load(f)
    return data


