#!/bin/env python3

"""
Utility script for generating Poisson disk samples for PCF filtering.

Based on the O(n) algorithm by Robert Bridson. See his paper "Fast Poisson
Sampling in Arbitrary Dimensions" for more information.
"""
from pyray import *
import math
import random

class Point:
	def __init__(self, x, y):
		self.x = x
		self.y = y

	def distance(self, other):
		dx = other.x - self.x
		dy = other.y - self.y
		return math.sqrt(dx*dx + dy*dy)

	def __add__(self, other):
		return Point(self.x + other.x, self.y + other.y)

	def __sub__(self, other):
		return Point(self.x - other.x, self.y - other.y)

	def __mul__(self, scalar):
		return Point(self.x * scalar, self.y * scalar)

	def __str__(self):
		return f"({self.x:.7f},{self.y:.7f})"

def insert_into_grid(p):
	i = math.floor(p.x * grid_size)
	j = math.floor(p.y * grid_size)
	grid[i][j] = p
	points.append(p)
	active_list.append(p)

def is_adequate(p):
	if p.distance(Point(0.5, 0.5)) > 0.5:
		return False
	for y in range(-2, 3):
		for x in range(-2, 3):
			i = math.floor(p.x * grid_size) + x
			j = math.floor(p.y * grid_size) + y
			if (0 <= i < grid_size) and (0 <= j < grid_size):
				if grid[i][j] and p.distance(grid[i][j]) < min_r:
					return False
	return True

def random_ring_point(origin, r, R):
	theta = random.random() * 2 * math.pi
	dist = math.sqrt(random.random() * (R*R - r*r) + r*r)
	p = Point(math.cos(theta), math.sin(theta))
	return origin + p * dist

grid_size = 15
grid = [[None]*grid_size for y in range(grid_size)]
points = []
active_list = []
cell_size = 1 / grid_size
min_r = cell_size * math.sqrt(2)
max_tries = 30

insert_into_grid(random_ring_point(Point(0.5, 0.5), 0, 0.5))

while len(active_list) > 0:
	index = random.randrange(0, len(active_list))
	selected = active_list[index]
	inserted = False
	for i in range(max_tries):
		p = random_ring_point(selected, min_r, 2 * min_r)
		if is_adequate(p):
			insert_into_grid(p)
			inserted = True
			break
	if not inserted:
		# Remove element from the active list.
		active_list[index], active_list[-1] = active_list[-1], active_list[index]
		active_list.pop()

for p in points:
	print(f"vec2{p - Point(0.5, 0.5)},")
print(f"A total of {len(points)} points were generated.")

width = 500
set_trace_log_level(999) # Disable annoying raylib logging.
init_window(width, width, "Poisson Disk Sampling")
set_target_fps(60)

while not window_should_close():
	begin_drawing()
	clear_background(RAYWHITE)
	draw_circle(width//2, width//2, width/2, LIGHTGRAY)
	for x in range(1, grid_size):
		xpos = math.floor(x * (width / grid_size))
		draw_line(xpos, 0, xpos, width, BLACK)
	for y in range(1, grid_size):
		ypos = math.floor(y * (width / grid_size))
		draw_line(0, ypos, width, ypos, BLACK)
	for p in points:
		x = math.floor(p.x * width)
		y = math.floor(p.y * width)
		draw_circle(x, y, 3, DARKBLUE)
	end_drawing()
close_window()
