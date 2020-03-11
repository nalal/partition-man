//header
#include "main.h"

namespace fs = std::experimental::filesystem;

std::vector<partition_data> drive_list;

char mount_point[255];

bool mount_new_devices = true;

char * get_free_mount_point()
{
	bool mount_found = false;
	int mount_num = 0;
	
	while(true)
	{
		sprintf(mount_point, "/mnt/esd/amount%i", mount_num);
		if(fs::exists(mount_point))
		{
			printf("mount point %s not available\n", mount_point);
			mount_num++;
		}
		else
		{
			printf("mount point %s available\n", mount_point);
			return mount_point;
		}
	}
}

int check_mounted(const char * device)
{
	char buffer[128];
	std::string result = "";
	char cmd[255];
	sprintf(cmd, "mount | grep \"%s\"", device);
	FILE* pipe = popen(cmd, "r");
	if(!pipe)
	{
		printf("Failed to open pipe for %s\n", device);
		return 0;
	}
	while (fgets(buffer, sizeof buffer, pipe) != NULL) {
		result += buffer;
	}
	pclose(pipe);
	if(result != "")
		return 1;
	else
		return 0;
}

void add_device(const char * device_name, bool mounted)
{
	partition_data new_device;
	new_device.mounted = mounted;
	strcpy(new_device.block_device, device_name);
	if(!mount_point || !mount_new_devices)
		printf("auto-mounting disabled, not saving mountpoint\n");
	else
		strcpy(new_device.mount_point, mount_point);
	drive_list.push_back(new_device);
	printf("added %s to partition list\n", device_name);
}

void remove_device(/*const char * device_name*/)
{
	printf("Printing part list\n");
	for(
		std::vector<partition_data>::size_type i = 0;
		i != drive_list.size();
		i++
	)
	{
		partition_data current_device = drive_list[i];
		printf("%s\n", current_device.block_device);
		printf("mounted: %s\n", current_device.mounted ? "true" : "false");
	}
}

bool check_against_list(const char * device)
{
	for(
		std::vector<partition_data>::size_type i = 0;
		i != drive_list.size();
		i++
	)
	{
		partition_data current_device = drive_list[i];
		//printf("comparing %s to %s\n", current_device.block_device, device);
		if(strcmp(current_device.block_device, device) == 0)
			return true;
	}
	return false;
}



void mount_device(const char * device)
{
	char * mount_point = get_free_mount_point();
	printf("mounting device %s to %s\n", device, mount_point);
	char mount_com[2550];
	sprintf(
		mount_com, 
		"mkdir -p %s && mount %s %s",
		mount_point,
		device, 
		mount_point
	);
	add_device(device, true);
	system(mount_com);
}

void prune_drives()
{
	for(
		std::vector<partition_data>::size_type i = 0;
		i != drive_list.size();
		i++
	)
	{
		partition_data current_device = drive_list[i];
		//printf("comparing %s to %s\n", current_device.block_device, device);
		if(fs::exists(current_device.block_device))
			printf("block file %s found\n", current_device.block_device);
		else
		{
			printf(
				"block file %s not found, cleaning mountpoint\n", 
				current_device.block_device
			);
			if(fs::exists(current_device.mount_point))
			{
				char clean_com[255];
				sprintf(
					clean_com,
					"umount -R %s && rm -rf %s",
					current_device.mount_point,
					current_device.mount_point
				);
				system(clean_com);
			}
		}
	}
}

void refresh_drives()
{
	for(auto& p: fs::directory_iterator("/dev/"))
	{
		if(fs::is_block_file(p.path().c_str()))
		{
			if(strlen(p.path().c_str()) > 8)
			{
				printf(
					"checking mount status for %s\n", 
					p.path().c_str()
				);
				if(check_mounted(p.path().c_str()))
				{
					printf(
						"device %s is mounted, ignoring\n", 
						p.path().c_str()
					);
				}
				else
				{
					printf(
						"device \"%s\" is not mounted\n", 
						p.path().c_str()
					);
					if(check_against_list(p.path().c_str()))
						printf("device %s found in list\n", p.path().c_str());
					else
					{
						printf(
							"device %s not found in list\n", 
							p.path().c_str()
						);
						mount_device(p.path().c_str());
					}
					//add_device(p.path().c_str(), false);
				}
			}					
		}
	}
}

void init_mountpoints()
{
	system("umount -R /mnt/esd/amount*");
	system("rm -rf /mnt/esd/amount*");
}

int init_list()
{
	init_mountpoints();
	int connected_drives = 0;
	for(auto& p: fs::directory_iterator("/dev/"))
	{
		if(fs::is_block_file(p.path().c_str()))
		{
			if(strlen(p.path().c_str()) > 8)
			{
				connected_drives++;
				printf(
					"checking mount status for %s\n", 
					p.path().c_str()
				);
				if(check_mounted(p.path().c_str()))
				{
					printf(
						"device \"%s\" is mounted\n", 
						p.path().c_str()
					);
					add_device(p.path().c_str(), true);
				}
				else
				{
					printf(
						"device \"%s\" is not mounted\n", 
						p.path().c_str()
					);
					add_device(p.path().c_str(), false);
				}
			}					
		}
	}
	return connected_drives;
}

int main()
{
	using namespace std::chrono_literals;
	bool running = true;
	int total_drives = init_list();
	while(running)
	{
		int connected_drives = 0;
		std::this_thread::sleep_for(10ms);
		for(auto& p: fs::directory_iterator("/dev/"))
		{
			if(fs::is_block_file(p.path().c_str()))
			{
				if(strlen(p.path().c_str()) > 8)
				{
					connected_drives++;
				}		
			}
		}
		if(total_drives < connected_drives)
		{
			printf("aditional drives detected, updating drive count\n");
			total_drives = connected_drives;
			refresh_drives();
		}
		else if(total_drives > connected_drives)
		{
			prune_drives();
			total_drives = connected_drives;
		}
		//printf("total partitions found: %i\n", total_drives);
		/*printf(
			"test cycle %i of %i complete\n", 
			test_cycles, 
			test_cycles_total
		);*/
	}
	printf("total partitions: %i\n", total_drives);
	return 0;
}
