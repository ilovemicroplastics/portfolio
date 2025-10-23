import pickle
from sympy.geometry import Point, Segment


# create wall segments from list [[p1,p2],[p2,p3], ... etc]
def create_sim_walls(walls):
    segments = []
    for w in walls:
        p1 = Point(w[0][0], w[0][1])
        p2 = Point(w[1][0], w[1][1])
        s = Segment(p1, p2)
        segments.append(s)

    return segments


# generates map of valid locations to choose from when generating training data
def valid_locations(walls, circle_radius, labels, barriers, debug=False):

    grid = []

    br = walls[0].args[0]

    bounds = walls.copy()
    for w in bounds:
        for p in w.args:
            if p.x >= br.x and p.y >= br.y:
                br = p

    for i in range(0, br.y//5):
        grid.append([])
        print(f"{i}/{(br.y//5)-1}")
        for j in range(0, br.x//5):
            grid[i].append("")
            grid[i][j] = -1
            for w in walls:
                distance = w.distance(Point((j*5), (i*5)))
                if distance <= circle_radius:
                    grid[i][j] = 999
                    break

    # add labels
    for l in labels:
        label_x = l[0]
        label_y = l[1]
        label = l[2]
        grid[label_y//5][label_x//5] = label


    save = []
    for b in barriers:
        p = b.args
        if p[1].x -p[0].x != 0:
            for i in range((p[1].x - p[0].x)//5):
                save.append([grid[p[0].y//5][i+(p[0].x//5)], p[0].y//5, i+(p[0].x//5)])
                grid[p[0].y//5][i+(p[0].x//5)] = 999
        else:
            for i in range((p[1].y - p[0].y)//5):
                save.append([grid[i+(p[0].y//5)][p[0].x//5], i+(p[0].y//5), p[0].x//5])
                grid[i+(p[0].y//5)][p[0].x//5] = 999

    go = True
    b_y = (br.x // 5)
    b_x = (br.y // 5)

    while go:
        go = False
        for i in range(0, br.y//5):
            for j in range(0, br.x//5):

                if grid[i][j] == -1:
                    edge = []
                    if i - 1 > -1:
                        e = grid[i-1][j]
                        if e != -1:
                            edge.append(e)
                    if i + 1 < b_x:
                        e = grid[i+1][j]
                        if e != -1:
                            edge.append(e)
                    if j - 1 > -1:
                        e = grid[i][j-1]
                        if e != -1:
                            edge.append(e)
                    if j + 1 < b_y:
                        e = grid[i][j+1]
                        if e != -1:
                            edge.append(e)

                    change = False
                    for l in labels:
                        if l[2] in edge:
                            change = True
                        if change:
                            grid[i][j] = l[2]
                            go = True
                            break

    for i in save:
        grid[i[1]][i[2]] = i[0]

    if debug:
        for x in grid:
            for y in x:
                print(y, end="\t")
            print()

    return grid


# generate map, optionally valid placement map
def generate_map(wall_points, file_name, radius=0, labels=None, barrier=[Segment(Point(0, 0), Point(0, 1))]):
    if labels is None:
        labels = [0, 0, 0]
    walls = create_sim_walls(wall_points)
    if radius == 0:
        output = walls
    else:
        valid_map = valid_locations(walls, radius, labels, barrier)
        output = [walls, valid_map]

    with open(f"{file_name}.pkl", "wb") as f:
        pickle.dump(output, f)


# input file name, return map.pkl
def read_map(file_name):
    with open(f"{file_name}.pkl", "rb") as f:
        map_data = pickle.load(f)
    return map_data


# unrefined, untested
def define_demo(map_file_name, auto_trajectory):
    map_data = read_map(map_file_name)
    map_data.append(auto_trajectory)
    print("Trajectory added to map...")
    print(auto_trajectory)
    with open(f"{map_file_name}_demo.pkl", "wb") as f:
        pickle.dump(map_data, f)

# label:category 0:normal 1:dead-end 2:tight-squeeze

# how maps in the folder were created

## squeeze 5
#w = [[[0,0],[0,360]],
#     [[180,0],[180,180]], [[180,180],[90,180]], [[90,180],[90,210]], [[90,210],[180,210]], [[180,210],[180,360]]]
#labels = [[60,90,0],[90,140,2],[60,300,0]]
#barriers = [Segment(Point(0,120),Point(180,120)), Segment(Point(0,240),Point(180,240))]
#generate_map(w, "squeeze5", 30, labels, barriers)

## squeeze 4
#w = [[[0,0],[120,0]], [[120,0],[120,180]], [[120,180],[180,180]], [[180,180],[180,210]], [[180,210],[120,210]], [[120,210],[120,420]],
#     [[330,0],[330,180]], [[330,180],[270,180]], [[270,180],[270,210]], [[270,210],[330,210]], [[330,210],[330,420]]]
#labels = [[240,60,0],[240,240,2],[240,360,0]]
#barriers = [Segment(Point(120,120),Point(330,120)),Segment(Point(120,270),Point(330,270))]
#generate_map(w, "squeeze4", 30, labels, barriers)

## squeeze 3
#w = [[[0,0],[120,0]], [[120,0],[60,300]], [[60,300],[180,360]], [[180,360],[120,420]], [[120,420],[60,420]], [[60,420],[60,540]],
#     [[480,0],[480,240]], [[480,240],[360,240]], [[360,240],[300,360]], [[300,360],[360,360]], [[360,360],[360,420]], [[360,420],[480,420]], [[480,420],[480,540]]]
#labels = [[300,120,0],[240,330,2],[300,480,0]]
#barriers = [Segment(Point(60,300),Point(360,300)),Segment(Point(60,420),Point(420,420))]
#generate_map(w, "squeeze3", 30, labels, barriers)

## de5
#w = [[[0,0],[180,0]], [[180,0],[180,180]], [[180,180],[0,180]], [[0,180],[0,360]], [[0,360],[180,360]], [[180,360],[180,540]],
#     [[360,0],[360,180]], [[360,180],[540,180]], [[540,180],[540,360]], [[540,360],[360,360]], [[360,360],[360,540]],
#     [[540,360],[540,540]]]
#labels = [[270,100,0],[60,270,1],[480,270,1]]
#barriers = [Segment(Point(120,180),Point(120,360)),Segment(Point(420,180), Point(420,360))]
#generate_map(w, "dead_end5", 30, labels, barriers)

## de4
#w = [[[0,0],[180,0]], [[180,0],[360,180]], [[360,180],[420,180]], [[420,180],[600,240]], [[600,240],[660,420]], [[660,420],[540,480]], [[540,480],[420,420]], [[420,420],[420,600]],
#     [[0,180],[180,360]], [[180,360],[180,600]],
#     [[660,420],[660,600]]]
#labels = [[540,360,1],[300,300,0]]
#barriers = [Segment(Point(420,180),Point(420,420))]
#generate_map(w, "dead_end4", 30, labels, barriers)

## squeeze2
#w = [[[0,0],[0,240]], [[0,240],[60,300]], [[60,300],[0,360]], [[0,360],[0,600]],
#     [[180,0],[180,240]], [[180,240],[150,300]], [[150,300],[180,360]], [[180,360],[180,600]]]
#labels = [[100,90,0],[100,270,2],[100,480,0]]
#barriers = [Segment(Point(0,240),Point(180,240)),Segment(Point(0,360),Point(180,360))]
#generate_map(w, "squeeze2", 30, labels, barriers)

## squeeze1
#w = [[[0,0],[35,0]], [[35,0],[275,240]], [[275,240],[275,300]], [[275,300],[335,300]], [[335,300],[395,360]], [[395,360],[515,360]], [[515,360],[515,540]],
#     [[0,180],[120,300]], [[120,300],[60,360]], [[60,360],[180,360]], [[180,360],[180,540]]]
#labels = [[210,300,2],[120,180,0],[360,480,0]]
#barriers = [Segment(Point(60,240), Point(300,240)),Segment(Point(180,360),Point(420,360))]
#generate_map(w, "squeeze1", 30, labels, barriers)


## de3
#w = [[[0,0],[300,0]], [[300,0],[300,240]], [[300,240],[360,240]], [[360,240],[360,0]], [[360,0],[480,0]], [[480,0],[480,120]], [[480,120],[600,240]], [[600,240],[540,300]], [[540,300],[660,420]], [[660,420],[720,360]], [[720,360],[600,240]],
#     [[0,240],[240,420]], [[240,420],[240,480]], [[240,480],[0,660]],
#     [[540,720],[420,600]], [[420,600],[480,540]], [[480,540],[600,660]], [[600,660],[600,720]]]
#labels = [[480,240,1],[150,150,0]]
#barriers = [Segment(Point(360,240),Point(360,300)),Segment(Point(360,300),Point(540,300))]
#generate_map(w, "dead_end3", 30, labels, barriers)

## de2
#w = [[[0,0],[360,0]], [[360,0],[360,120]], [[360,120],[540,360]], [[540,360],[660,240]],
#     [[0,0],[0,120]], [[0,120],[240,360]], [[240,360],[240,480]], [[240,480],[0,480]],
#     [[400,700],[400,600]], [[400,600],[660,480]], [[660,480],[700,600]]]
#labels = [[180,60,1],[240,300,0]]
#barrier = [Segment(Point(0,120),Point(360,120))]
#generate_map(w, "dead_end2", 30, labels, barrier)

## de1
#w = [[[0,0],[0,200]], [[0,200],[400,200]], [[100,0],[100,100]],
#         [[100,100],[130,100]], [[130,100],[110,0]], [[100,0],[260,0]],
#         [[260,0],[230,100]], [[230,100],[400,100]]]
#labels = [[150,50,1], [150,150,0]]
#
#generate_map(w, "test_dead_end1", 30, labels, [Segment(Point(130,100), Point(230,100))])