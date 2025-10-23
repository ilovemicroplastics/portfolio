import training
import map_generator as m
import simulation
from sympy.geometry import Point, Segment

if __name__ == "__main__":
    # main could be anything, this is just a showcase of some of the functions available.
    # libraries required: sympy, TensorFlow, PyCairo, pillow, numpy, scikit-learn
    # this main runs on my computer, if you wish to run this but cannot feel free to email me
    # 25798652@students.lincoln.ac.uk

    # a map being generated
    w = [[[0, 0], [0, 360]],
        [[180,0],[180,180]], [[180,180],[90,180]], [[90,180],[90,210]], [[90,210],[180,210]], [[180,210],[180,360]]]
    labels = [[60,90,0],[90,140,2],[60,300,0]]
    barriers = [Segment(Point(0,120),Point(180,120)), Segment(Point(0,240),Point(180,240))]
    m.generate_map(w, "showcase", 30, labels, barriers)

    # rounding up pre-generated training data
    files_0 = ["72_data_dead_end1_0", "72_data_dead_end2_0", "72_data_dead_end3_0", "72_data_dead_end5_0",
               "72_data_squeeze1_0", "72_data_squeeze2_0", "72_data_squeeze3_0", "72_data_squeeze4_0","72_data_squeeze5_0"]
    files_1 = ["72_data_dead_end1_1","72_data_dead_end2_1","72_data_dead_end3_1","72_data_dead_end5_1",]
    files_2 = ["72_data_squeeze1_2","72_data_squeeze2_2","72_data_squeeze3_2","72_data_squeeze4_2","72_data_squeeze5_2"]

    read_data_0, read_data_1, read_data_2 = [], [], []

    for f in files_0:
        read_data_0.append(training.read_data(f))
    for f in files_1:
        read_data_1.append(training.read_data(f))
    for f in files_2:
        read_data_2.append(training.read_data(f))

    data0 = training.combine_data(read_data_0)
    data1 = training.combine_data(read_data_1)
    data2 = training.combine_data(read_data_2)

    data_list = [data0, data1, data2]

    for data in data_list:
        for d in data[0]:
            for i in range(36):
                if d[i] == -1:
                    d[i] = 800
                if d[i] > 800:
                    d[i] = 800

    for i in range(len(data_list)):
        data_list[i] = training.normalise_data(data_list[i])
        print(data_list[i])
        
    # training a CNN
    training.train_cnn(data_list, True, "test")

    # running a simple simulation
    walls = m.read_map("dead_end1")
    s = simulation.Sim(28, walls[0], 350, 150, 90, "test")

    for i in range(30):
        if i == 17:
            s.update(10, 90)
        else:
            s.update(10, 0)
