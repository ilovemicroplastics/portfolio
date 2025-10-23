import math
from sympy.geometry import Point


def translate_coordinate_around_center(pivot_x, pivot_y, length, direction):
    x = pivot_x
    y = pivot_y + length

    ox = pivot_x
    oy = pivot_y

    theta = math.radians(direction)

    translated_x = x - ox
    translated_y = y - oy

    new_x = translated_x * math.cos(theta) - translated_y * math.sin(theta)
    new_y = translated_x * math.sin(theta) + translated_y * math.cos(theta)

    new_x += ox
    new_y += oy

    return Point(new_x, new_y)
