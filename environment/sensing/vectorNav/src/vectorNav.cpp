#include "vectorNav.hpp"
#include "LifeCycle.hpp"

vectorNav::vectorNav(int argc, char** argv) : Sensor(argc, argv) {
	if(argc >1){
		initializeParameters(argc, argv);
	}
	else{
		initializeParameters();
	}
	ros::init(argc, argv, node_name.c_str());
    node_handle = new ros::NodeHandle();
	setupCommunications();
}

vectorNav::~vectorNav() {
}

bool vectorNav::connect() {
	vn200_connect(&vn200, COM_PORT.c_str(), BAUD_RATE);
    return true;
}

bool vectorNav::disconnect() {
	vn200_disconnect(&vn200);
    return true;
}

bool vectorNav::fetch() {
	unsigned short gpsWeek, status;
    unsigned char gpsFix, numberOfSatellites;
    float speedAccuracy, timeAccuracy, attitudeUncertainty, positionUncertainty, velocityUncertainty, temperature, pressure;
    double gpsTime, latitude, longitude, altitude;
    VnVector3 magnetic, acceleration, angularRate, ypr, latitudeLognitudeAltitude, nedVelocity, positionAccuracy;
    
	vn200_getGpsSolution(&vn200, &gpsTime, &gpsWeek, &gpsFix, &numberOfSatellites, &latitudeLognitudeAltitude, &nedVelocity, &positionAccuracy, &speedAccuracy, &timeAccuracy);
    ROS_INFO("Triangulating from %d satellites",numberOfSatellites);
    vn200_getInsSolution(&vn200, &gpsTime, &gpsWeek, &status, &ypr, &latitudeLognitudeAltitude, &nedVelocity, &attitudeUncertainty, &positionUncertainty, &velocityUncertainty);
    vn200_getCalibratedSensorMeasurements(&vn200, &magnetic, &acceleration, &angularRate, &temperature, &pressure);
    
    tf_angles.setEuler(ypr.c0, ypr.c1, ypr.c2);
    
    _imu.orientation.x = tf_angles.x();
    _imu.orientation.y = tf_angles.y();
    _imu.orientation.z = tf_angles.z();
    _imu.orientation.w = tf_angles.w();
    _imu.angular_velocity.x = angularRate.c0;
    _imu.angular_velocity.y = angularRate.c1;
	_imu.angular_velocity.z = angularRate.c2;
	
	_twist.linear.x = nedVelocity.c0;
	_twist.linear.y = nedVelocity.c1;
	_twist.linear.z = nedVelocity.c2;
	_twist.angular.x = angularRate.c0;
	_twist.angular.y = angularRate.c1;
	_twist.angular.z = angularRate.c2;
	

    _gps.latitude = latitudeLognitudeAltitude.c0;
    _gps.longitude = latitudeLognitudeAltitude.c1;
    _gps.altitude = latitudeLognitudeAltitude.c2;
    
	return true;
}

void vectorNav::publish(int frame_id) {
    imu_pub.publish(_imu);
    gps_pub.publish(_gps);
    twist_pub.publish(_twist);
}

void vectorNav::initializeParameters() {
    node_name = std::string("sensors_vectorNav_0");
    gps_topic_name=std::string("sensors/gps/0");
    imu_topic_name=std::string("sensors/imu/0");
    twist_topic_name=std::string("sensors/twist/0");
    message_queue_size = 10;
    COM_PORT = std::string("/dev/serial/by-id/usb-FTDI_USB-RS232_Cable_FTVJUC0O-if00-port0");
    BAUD_RATE= 115200;
}

void vectorNav::initializeParameters(int argc, char** argv) {
	node_name = std::string("sensors_vectorNav_") + std::string(argv[1]);
    gps_topic_name=std::string("sensors/gps/") + std::string(argv[1]);
    imu_topic_name=std::string("sensors/imu/") + std::string(argv[1]);
    twist_topic_name=std::string("sensors/twist/") + std::string(argv[1]);
    COM_PORT = std::string(argv[2]);
    message_queue_size = 10;
    BAUD_RATE= 115200;
}

void vectorNav::setupCommunications() {
	gps_pub = node_handle->advertise<sensor_msgs::NavSatFix>(gps_topic_name.c_str(), message_queue_size);
	imu_pub = node_handle->advertise<sensor_msgs::Imu>(imu_topic_name.c_str(), message_queue_size);
    twist_pub = node_handle->advertise<geometry_msgs::Twist>(twist_topic_name.c_str(), message_queue_size);
}

int main(int argc, char** argv) {
	if(argc>1 && argc <=3){
		printf("Usage : <name> <id_no> <COM_PORT>\n");
	}
	double loop_rate = 10;
	int frame_id;
	frame_id = 0;
    vectorNav *vectornav = new vectorNav(argc, argv);
	vectornav->connect();
    
    ROS_INFO("VectorNav succesfully connected. \n");
	
	ros::Rate rate_enforcer(loop_rate);
	while (ros::ok()) {
        vectornav->fetch();
        vectornav->publish(frame_id);
        ros::spinOnce();
        rate_enforcer.sleep();
    }
    vectornav->disconnect();
return 0;
}
