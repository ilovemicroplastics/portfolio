#include <iostream>
#include <vector>

#include "Utils.h"
#include "CImg.h"

using namespace cimg_library;

void print_help() {
	std::cerr << "Application usage:" << std::endl;

	std::cerr << "  -p : select platform " << std::endl;
	std::cerr << "  -d : select device" << std::endl;
	std::cerr << "  -l : list all platforms and devices" << std::endl;
	std::cerr << "  -f : input image file (default: test.ppm)" << std::endl;
	std::cerr << "  -h : print this message" << std::endl;
}


int main(int argc, char** argv) {
	//Part 1 - handle command line options such as device selection, verbosity, etc.
	int platform_id = 0;
	int device_id = 0;
	string image_filename = "test_large.pgm";

	for (int i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "-p") == 0) && (i < (argc - 1))) { platform_id = atoi(argv[++i]); }
		else if ((strcmp(argv[i], "-d") == 0) && (i < (argc - 1))) { device_id = atoi(argv[++i]); }
		else if (strcmp(argv[i], "-l") == 0) { std::cout << ListPlatformsDevices() << std::endl; }
		else if ((strcmp(argv[i], "-f") == 0) && (i < (argc - 1))) { image_filename = argv[++i]; }
		else if (strcmp(argv[i], "-h") == 0) { print_help(); return 0; }
	}

	//// turns off library messages
	cimg::exception_mode(0);

	//detect any potential exceptions
	try {
		CImg<unsigned char> image_input(image_filename.c_str());
		CImgDisplay disp_input(image_input, "input");

		//Part 3 - host operations
		//3.1 Select computing devices
		cl::Context context = GetContext(platform_id, device_id);

		//display the selected device
		std::cout << "Running on " << GetPlatformName(platform_id) << ", " << GetDeviceName(platform_id, device_id) << std::endl;

		//create a queue to which we will push commands for the device
		cl::CommandQueue queue(context, CL_QUEUE_PROFILING_ENABLE);


		//3.2 Load & build the device code
		cl::Program::Sources sources;

		AddSources(sources, "kernels.cl");

		cl::Program program(context, sources);


		//build and debug the kernel code
		try {
			program.build();
		}
		catch (const cl::Error& err) {
			std::cout << "Build Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
			std::cout << "Build Options:\t" << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
			std::cout << "Build Log:\t " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(context.getInfo<CL_CONTEXT_DEVICES>()[0]) << std::endl;
			throw err;
		}

		//Part 4 - device operations

		///////////////////////////////  MY CODE STARTS HERE ///////////////////////////////


		// vectors for profiling
		std::vector<const char*> event_names;
		std::vector<int> event_memory_transfer;


		cl::Buffer dev_image_input(context, CL_MEM_READ_ONLY, image_input.size());
		cl::Buffer dev_image_output(context, CL_MEM_READ_WRITE, image_input.size());
		cl::Buffer dev_histogramBuffer(context, CL_MEM_WRITE_ONLY, 256 * sizeof(int));
		cl::Buffer dev_normal_histogram(context, CL_MEM_READ_WRITE, 256 * sizeof(int));
		cl::Buffer dev_bin_lookup(context, CL_MEM_READ_ONLY, 256 * sizeof(int));


		///////////////////////////////

		// Set the bin size for the histogram
		int bin_size = 1;

		// create look-up-table for bins so bins can run in parallel
		std::vector<int> bin_look_up;
		for (int i = 0; i < 256; ++i) {
			bin_look_up.push_back(i / bin_size);
		}

		// this has no profiling, but it doesn't need to
		// it is insignificant since its just a failed experiment. left as evidence of optimising
		queue.enqueueWriteBuffer(dev_bin_lookup, CL_TRUE, 0, 256 * sizeof(int), bin_look_up.data());

		///////////////////////////////


		cl::Event e0;
		// calculate image size in bytes for profiling
		int image_bytes = image_input.size() * image_input.spectrum() * sizeof(unsigned char);

		// input image data
		queue.enqueueWriteBuffer(dev_image_input, CL_TRUE, 0, image_input.size(), &image_input.data()[0], NULL, &e0);

		event_names.push_back("input image_input");
		event_memory_transfer.push_back(image_bytes);
		

		///////////////////////////////


		// set kernel to "histogram", it places a histogram in dev_histogramBuffer
		cl::Kernel kernel = cl::Kernel(program, "histogram");
		kernel.setArg(0, dev_image_input);
		kernel.setArg(1, dev_histogramBuffer);
		kernel.setArg(2, bin_size);
		kernel.setArg(3, dev_bin_lookup);


		///////////////////////////////


		cl::Event e1;
		// execute kernel
		queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(image_input.size()), cl::NullRange, NULL, &e1);

		event_names.push_back("execute event_histogram");
		// no host-device transfers so memory transfer = 0
		event_memory_transfer.push_back(0);


		///////////////////////////////


		// set kernel to "cumulative_histogram", it runs a cumulative sum on the histogram in dev_histogramBuffer
		kernel = cl::Kernel(program, "cumulative_histogram");
		kernel.setArg(0, dev_histogramBuffer);
		kernel.setArg(1, bin_size);

		///////////////////////////////


		cl::Event e2;
		// execute kernel
		queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(256), cl::NullRange, NULL, &e2);

		event_names.push_back("execute cumulative_histogram");
		event_memory_transfer.push_back(0);


		///////////////////////////////


		// set kernel to "normalise_histogram", it normalises the histogram scaling the highest value to 255
		kernel = cl::Kernel(program, "normalise_histogram");
		kernel.setArg(0, dev_histogramBuffer);
		kernel.setArg(1, dev_normal_histogram);
		kernel.setArg(2, bin_size);


		///////////////////////////////


		cl::Event e3;
		// execute kernel
		queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(256), cl::NullRange, NULL, &e3);

		event_names.push_back("execute normalise_histogram");
		event_memory_transfer.push_back(0);


		///////////////////////////////


		// set kernel to "map_image", it uses the histogram as a look-up-table to scale it's pixel's intensities
		kernel = cl::Kernel(program, "map_image");
		kernel.setArg(0, dev_image_input);
		kernel.setArg(1, dev_image_output);
		kernel.setArg(2, dev_normal_histogram);
		kernel.setArg(3, bin_size);


		///////////////////////////////


		cl::Event e4;
		// execute kernel
		queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(image_input.size()), cl::NullRange, NULL, &e4);

		event_names.push_back("execute map_image");
		event_memory_transfer.push_back(0);


		///////////////////////////////
		

		cl::Event e5;
		// transfers output image from buffer to host, into output_buffer
		vector<unsigned char> output_buffer(image_input.size());

		queue.enqueueReadBuffer(dev_image_output, CL_TRUE, 0, output_buffer.size(), &output_buffer.data()[0], NULL, &e5);

		event_names.push_back("output image_output");
		// since output has same properties as input, recalculating size is unnecessary
		event_memory_transfer.push_back(image_bytes);


		///////////////////////////////  profiling data   ///////////////////////////////


		int ns_end;
		int ns_start;
		const char* name;
		int mem;

		int total_mem = 0;
		int total_events = 0;
		int total_time = 0;

		// wait for all events to finish, events need to be finished to analyse
		queue.finish();

		std::vector<cl::Event> events;
		events.push_back(e0);
		events.push_back(e1);
		events.push_back(e2);
		events.push_back(e3);
		events.push_back(e4);
		events.push_back(e5);


		// the vectors get queued in the same order so the indexes in the vectors contain related data
		// i.e. i = 0, all data is related to e0
		for (int i = 0; i <= 5; ++i) {
			mem = 0;
			ns_end = events[i].getProfilingInfo<CL_PROFILING_COMMAND_END>();
			ns_start = events[i].getProfilingInfo<CL_PROFILING_COMMAND_START>();
			name = event_names[i];
			mem = event_memory_transfer[i];

			total_mem += mem;
			total_events += 1;
			total_time += ns_end - ns_start;

			std::cout << std::endl 
				<< name << std::endl
				<< mem << std::endl 
				<< "execution time [ms (ns)]: " << float(ns_end - ns_start) / float(1000000) << "ms (" << ns_end - ns_start << "ns)" << std::endl;
		}
		

		std::cout << std::endl
			<< "total events: " << total_events << std::endl
			<< "total memory transfer between host and device: " << total_mem << std::endl
			<< "total execution time [ms (ns)]: " << float(total_time) / float(1000000) << "ms (" << total_time << "ns)" << std::endl;


		/////////////////////////////// MY CODE ENDS HERE ///////////////////////////////


		CImg<unsigned char> output_image(output_buffer.data(), image_input.width(), image_input.height(), image_input.depth(), image_input.spectrum());
		CImgDisplay disp_output(output_image, "output");

		while (!disp_input.is_closed() && !disp_output.is_closed()
			&& !disp_input.is_keyESC() && !disp_output.is_keyESC()) {
			disp_input.wait(1);
			disp_output.wait(1);
		}


	}
	catch (const cl::Error& err) {
		std::cerr << "ERROR: " << err.what() << ", " << getErrorString(err.err()) << std::endl;
	}
	catch (CImgException& err) {
		std::cerr << "ERROR: " << err.what() << std::endl;
	}

	return 0;
}
