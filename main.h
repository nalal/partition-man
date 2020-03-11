//C includes
#include <stdio.h>
#include <string.h>

//C++ includes
#include <experimental/filesystem>
#include <string>
#include <chrono>
#include <thread>
#include <vector>

struct partition_data{
	bool mounted;
	char block_device[255];
	char mount_point[255];
};
