import cairo
import helper as h
from PIL import Image


class Visual:
    def __init__(self, width, height):
        self.surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        self.c = cairo.Context(self.surface)
        self.c.set_source_rgb(1, 1, 1)
        self.c.paint()

        self.frames = []

    def spawn_line(self, x1, y1, x2, y2, red):
        self.c.move_to(x1, y1)
        self.c.line_to(x2, y2)
        self.c.set_source_rgb(red, 0, 0)
        self.c.set_line_width(2)
        self.c.stroke()

    def spawn_circle(self, x, y, r):
        self.c.arc(x, y, r, 0, 2 * 3.14159)
        self.c.set_source_rgb(1, 0, 0)
        self.c.set_line_width(2)
        self.c.stroke()

    def add_circle(self, c, direction):
        x1 = c.args[0][0]
        y1 = c.args[0][1]
        r = c.args[1]
        self.spawn_circle(x1, y1, r)

        p = h.translate_coordinate_around_center(x1, y1, 35, direction)
        new_x = p[0]
        new_y = p[1]

        self.spawn_line(x1, y1, new_x, new_y, 1)

    def add_walls(self, lines):
        for o in lines:
            x1 = o.args[0][0]
            x2 = o.args[1][0]
            y1 = o.args[0][1]
            y2 = o.args[1][1]
            self.spawn_line(x1, y1, x2, y2, 0)

    def display_frame(self, walls, circ, direction, guess):
        self.c.set_source_rgb(1, 1, 1)
        self.c.paint()

        self.add_walls(walls)
        self.add_circle(circ, direction)

        self.add_text(20, 20, guess)

        data = self.surface.get_data()
        pil_image = Image.frombuffer('RGBA', (self.surface.get_width(), self.surface.get_height()), data, 'raw', 'BGRA', 0, 1)
        return pil_image

    def add_text(self, x, y, text):
        font_size = 20
        font_face = "Arial"
        self.c.select_font_face(font_face, cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
        self.c.set_font_size(font_size)
        self.c.set_source_rgb(0, 0, 0)  # Set text color to black
        self.c.move_to(x, y)
        self.c.show_text(f"state: {str(text)}")

    def save_frame(self, walls, circle, direction, prediction=0):
        self.frames.append(self.display_frame(walls, circle, direction, prediction))

    def finalise_animation(self):
        self.frames[0].save("gif.gif", save_all=True, append_images=self.frames[1:], duration=300, loop=0)
