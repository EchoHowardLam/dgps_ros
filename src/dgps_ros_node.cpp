#include <ros/ros.h>
#include <sensor_msgs/NavSatFix.h>
#include <sensor_msgs/NavSatStatus.h>
#include <thread>
#include <string>
#include "ntrip.h"


#define STATUS_NO_FIX -1
#define STATUS_FIX 0
#define STATUS_GBAS_FIX 2
#define SERVICE_GPS 1
#define COVARIANCE_TYPE_DIAGONAL_KNOWN  2

double NMEA2float(std::string s)
{
    double d = std::stod(s)/100.0;
    d = floor(d) + (d - floor(d))*10/6;
    return d;
}

boost::array<double,9> covariance={1,0,0,
                     0,1,0,
                     0,0,1 };

void fillSatMessage(sensor_msgs::NavSatFix& sat, Location& loc )
{
    sat.header.frame_id="gps";
    sat.header.stamp = ros::Time::now();

    if( loc.fix == "0" )
        sat.status.status = STATUS_NO_FIX;
    else if( loc.fix == "1" )
        sat.status.status = STATUS_FIX;
    else
        sat.status.status = STATUS_GBAS_FIX;
    sat.status.service = SERVICE_GPS;

    sat.latitude = NMEA2float(loc.lat);
    sat.longitude = NMEA2float(loc.lon);
    sat.altitude = std::stod(loc.alt);
   
    std::copy(covariance.begin(), covariance.end(),sat.position_covariance.begin());
    sat.position_covariance[0]  = std::stod(loc.hdop);
    sat.position_covariance[4]  = std::stod(loc.hdop);
    sat.position_covariance[8]  = 10;


    sat.position_covariance_type = COVARIANCE_TYPE_DIAGONAL_KNOWN;
    
}


int main(int argc, char **argv)
{
    ros::init(argc, argv, "dgps_node");
    ros::NodeHandle nh("~");


    std::string serverName, serverPort, mountpoint, userName, password, initialGPGGA, serialPort;
    nh.param<std::string>("server",        serverName,   "unk_server");
    nh.param<std::string>("port",          serverPort,   "unk_port");
    nh.param<std::string>("mountpoint",    mountpoint,   "unk_mountpt");
    nh.param<std::string>("username",      userName,     "unk_user");
    nh.param<std::string>("password",      password,     "unk_pw");
    nh.param<std::string>("initial_GPGGA", initialGPGGA, "unk_GGA");
    nh.param<std::string>("serialPort",    serialPort,   "");


    struct Args args;
    args.server =   serverName.c_str();
    args.port =     serverPort.c_str();
    args.data =     mountpoint.c_str();
    args.user =     userName.c_str();
    args.password = password.c_str();
    args.nmea =     initialGPGGA.c_str();
    args.bitrate = 0;
    args.proxyhost = 0;
    args.proxyport = "2101";
    args.mode = NTRIP1;
    args.initudp = 0;
    args.udpport = 0;
    args.protocol = SPAPROTOCOL_NONE;
    args.parity = SPAPARITY_NONE;
    args.stopbits = SPASTOPBITS_1;
    args.databits = SPADATABITS_8;
    args.baud = SPABAUD_9600;
    if( serialPort.empty())
        args.serdevice = 0;//"/dev/ttyUSB0";  
    else
        args.serdevice = serialPort.c_str();
    args.serlogfile = 0;
    args.stop = false;

    
    ROS_INFO("Username = %s",args.user); 
    ROS_INFO("password = %s",args.password); 

    const std::string topic = "dgps";
    ros::Publisher pub = nh.advertise<sensor_msgs::NavSatFix>(topic,10);

    
    std::thread ntrip_thread(ntrip_client,&args);
    ros::Rate loop_rate(10);
    while (ros::ok())
    {
        /*Location loc = getGNGGA();
        if( loc.lat.empty() )
        {
        }
        else
        {
           // cout<<s;
            sensor_msgs::NavSatFix sat;
            fillSatMessage(sat,loc);
            pub.publish(sat);
            ROS_INFO("Talker_____:GPS:x = %s",loc.nmea.c_str()); 
        }*/
        ros::spinOnce();
        loop_rate.sleep();
    }
    args.stop = true;
    ROS_INFO("Waiting to Quit");
    ntrip_thread.join();
    
}

