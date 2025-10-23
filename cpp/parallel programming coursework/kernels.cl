
// takes image A as input, outputs histogram
kernel void histogram(global const uchar* A, global int* histogram, const int bin_size, global const int* bin_lookup) {
	int gID = get_global_id(0);


    // takes each intensity
	uchar pixel_value = A[gID];

    // bin_index is the value of the closest bin
    // if = 1, this section is skipped as that is the default behaviour
    int bin_index = 0;
    if (bin_size == 1){
        bin_index = pixel_value;
    }
    //otherwise, count down until a bin boundary is hit
    //e.g. i = 14, when bin_size = 7
    else {
		for (int i = pixel_value; i > 0; i--) {
			if ((i % bin_size == 0)) {
				bin_index = i;
				break;
			}
		}
    }

    // alternative way to calculate, disabled
    //int bin_index = bin_lookup[pixel_value];

    // then divide by bin_value to place in correct index
	atomic_inc(&histogram[bin_index/bin_size]);
	}




// a hillis steele inclusive scan that is used to cumulate the histogram (NOT FROM WORKSHOP)
kernel void cumulative_histogram(global int* histogram, const int bin_size) {
    int gID = get_global_id(0);
    
    // offset iterates over powers of 2
    for (int offset = 1; offset < 256/bin_size; offset <<= 1) {
        int left = histogram[gID - offset];

        barrier(CLK_GLOBAL_MEM_FENCE);
        
        // add the left gID to the current gID
        if (gID >= offset) {
            histogram[gID] += left;
        }

        barrier(CLK_GLOBAL_MEM_FENCE);
    }
}




// normalise the histogram to 255, this is a map pattern
kernel void normalise_histogram(global int* histogram, global int* normal_histogram, const int bin_size) {
	int gID = get_global_id(0);


    // normalise indexes, with 255 as the highest value
    normal_histogram[gID] = (int)(histogram[gID] * 255.0f / (histogram[255/bin_size]));
	}




// maps the obtained histogram values onto a provided image, also takes the histogram as an input
kernel void map_image(global const uchar* A, global uchar* B, global int* histogram, const int bin_size) {
    int gID = get_global_id(0);

    // get intensity from input
    uchar pixel_value = A[gID];

    // sort into bin
    int bin_index = 0;
    if (bin_size == 1){
        bin_index = pixel_value;
    }
    else {
		for (int i = pixel_value; i > 0; i--) {
			if ((i % bin_size == 0)) {
				bin_index = i;
				break;
			}
		}
        bin_index = bin_index/bin_size;
    }
    

    // use the histogram as a look up table
    uchar mapped_value = (uchar)histogram[bin_index];

    // set intensity on output
    B[gID] = mapped_value;
}