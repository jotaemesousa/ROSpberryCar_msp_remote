#include <ros/ros.h>
#include <cereal_port/CerealPort.h>
#include <signal.h>
#include <string.h>
#include <string>
#include <ROSpberryCar_msp_remote/car_msg.h>

#define DEBUG

using namespace cereal;
using namespace ROSpberryCar_msp_remote;
using namespace std;

#define LINEAR_ZERO 	0
#define LINEAR_MIN	 	-100
#define LINEAR_MAX 		100
#define ANGULAR_ZERO 	0
#define ANGULAR_RIGHT 	-100
#define ANGULAR_LEFT 	100

#define MAX_TIME_BETWEEN_MSGS 200000000	// nsecs

// Function prototypes
void flag_out_func(int a);
void callback_stream(basic_string<char>* msg);

// Global variables
char flag_out = 0, running = 0;
car_msg ferrari_cmd;
ros::Publisher *pub_twist_ptr;

ros::Time last_msg_time, curr_msg_time;
double init_time;

// Main function
int main(int argc, char * argv[])
{
	ros::init(argc, argv, "remote");
	ros::Time::init();
	init_time = ros::Time::now().toNSec();
	CerealPort remote;
		
	ros::Time temp;

	ros::NodeHandle ferrari_node;
	
	ferrari_cmd.linear = LINEAR_ZERO;
	ferrari_cmd.angular = ANGULAR_ZERO;
	
	if(argc == 2)
	{
		remote.open(argv[1],9600);
	}
	else
	{
		remote.open("/dev/ttyUSB0",9600);
		ROS_INFO("Opening serial port /dev/ttyUSB0\n");
	}
	
	if(!remote.portOpen())
	{
		ROS_ERROR("Serial Port busy");
		exit(1);
	}

	signal(SIGINT, flag_out_func);
	
	ros::Rate loop_rate(20);
	ros::Publisher pub_twist = ferrari_node.advertise<car_msg>("Ros_car_msg",100);
	pub_twist_ptr = &pub_twist;
	
	remote.startReadBetweenStream(callback_stream, ':', ';');
	
	int i = 0;
	
	while(ros::ok() && !flag_out)
	{
		if(!remote.portOpen())
		{
			ROS_ERROR("Serial Port not disconnected");
			flag_out = 1;
			flag_out_func(0);

		}

		double temp_time = ros::Time::now().toNSec() - curr_msg_time.toNSec();
#ifdef DEBUG
		cout << temp_time << endl;
#endif
		if(running)
		{
			if(temp_time < MAX_TIME_BETWEEN_MSGS)
			{

				//last_msg_time = ros::Time::now();
				pub_twist_ptr->publish(ferrari_cmd);
			}
			else
			{
				flag_out = 1;
				ferrari_cmd.linear = 0;
				ferrari_cmd.angular = 0;
				pub_twist_ptr->publish(ferrari_cmd);
				cout << "saiu_if" << endl;
			}
		}

		
#ifdef DEBUG
		printf("while\n");
#endif
		ros::spinOnce();
		loop_rate.sleep();
	}
	
	
	
	return 0;
}

void flag_out_func(int a)
{
	flag_out = 1;
	car_msg ferrari_cmd;
	ferrari_cmd.linear = LINEAR_ZERO;
	ferrari_cmd.angular = ANGULAR_ZERO;
	pub_twist_ptr->publish(ferrari_cmd);
#ifdef DEBUG
	printf("Saiu\n");
#endif
}

void callback_stream(basic_string<char>* msg)
{
	if(!running)
	{
		running = 1;
	}
	
	int id = -1, l = 0, a = 0;
	int a_norm = 0, l_norm = 0;
	sscanf((*msg).c_str(), ":%d %d %d;", &id, &l, &a);

	if(id == 0)
	{
		if(l < 10 && l > -10)
		{
			l = LINEAR_ZERO;
			l_norm = LINEAR_ZERO;
		}
		if(l > 0)
		{
			l_norm = l * 100 / 88;
		}
		else if(l < 0)
		{
			l_norm = l * 100 / 95;
		}

		if(l_norm > 100)	l_norm = 100;
		if(l_norm < -100)	l_norm = -100;

		if(a < 85 && a > 75)
		{
			a_norm = ANGULAR_ZERO;
		}
		else if(a < 80 && a >= 40)
		{
			a_norm = (80 - a)*100/36;
		}
		else if(a > 80 && a <= 116)
		{
			a_norm = -100 + (116 - a)*100/36;
		}
		if(a_norm < -100)	a_norm = -100;
		if(a_norm > 100)	a_norm = 100;





		ferrari_cmd.linear = l_norm;
		ferrari_cmd.angular = a_norm;
		
		curr_msg_time = ros::Time::now();
	}
#ifdef DEBUG
	printf("%d %d %d\n", id, l_norm, a_norm);
#endif
}
