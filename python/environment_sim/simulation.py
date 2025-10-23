import helper as h
import visual as vis
from sympy.geometry import Point, intersection, Segment, Circle
import math
import sympy

import numpy as np
import tensorflow as tf


class Sim:
    def __init__(self, tick_limit, walls, start_x, start_y, start_d, model_name="def", height=720, width=720):
        self.tick = 0
        self.tick_limit = tick_limit

        self.walls = walls

        self.x = start_x
        self.y = start_y
        self.d = start_d

        self.vis = vis.Visual(height, width)

        self.model = tf.keras.models.load_model(f"{model_name}.keras")

    def create_circle(self, x, y, r):
        p7 = Point(x, y)
        c = Circle(p7, r)
        return c

    def get_lidar_data(self):
        final_sensor = self.d + 360
        sensor_direction = self.d
        origin = Point(self.x, self.y)
        scan = []
        while sensor_direction != final_sensor:
            p = h.translate_coordinate_around_center(self.x, self.y, 800, sensor_direction)
            l1 = Segment(origin, p)

            intersections = []

            for o in self.walls:
                intersections.append(intersection(l1, o))

            d = 999999999
            if intersections:
                for i in intersections:
                    if i:
                        for j in i:
                            if type(j) is sympy.Point2D:
                                d_calc = math.sqrt((self.x - j[0]) ** 2 + (self.y - j[1]) ** 2)
                                if d_calc < d:
                                    d = d_calc

            if d == 999999999:
                d = -1
            scan.append(int(round(d, 0)))

            sensor_direction += 5
        return scan

    def selected_lidar_scan(self, x, y, d):
        self.x = x
        self.y = y
        self.d = d
        return self.get_lidar_data()

    def classify(self, lidar):
        for i in range(72):
            if lidar[i] == -1:
                lidar[i] = 800
            if lidar[i] > 800:
                lidar[i] = 800
        lidar = np.array(lidar)
        min_val = np.min(lidar)
        max_val = np.max(lidar)
        scaled_input_arr = (lidar - min_val) / (max_val - min_val)
        input_arr = scaled_input_arr.reshape(1, 72, 1)

        predictions = self.model.predict(input_arr)
        return predictions[0]

    def update(self, v, vr):
        if self.tick > self.tick_limit:
            print("Cannot advance, tick limit has been reached.")
            lidar = []
            self.vis.finalise_animation()
        else:
            print(f"Advancing tick to {self.tick}")

            lidar = self.get_lidar_data()
            predictions = self.classify(lidar)
            if np.max(predictions) > 0.95:
                guess = np.argmax(predictions)
            else:
                guess = "?"

            circle = self.create_circle(self.x, self.y, 30)
            self.vis.save_frame(self.walls, circle, self.d, guess)

            self.d += vr

            new_pos = h.translate_coordinate_around_center(self.x, self.y, v, self.d)
            self.x, self.y = new_pos[0], new_pos[1]

            self.tick += 1

        return lidar
