/*
* Program to be used with a Pololu DRV8835 dual motor driver kit for the Raspberry Pi, in conjunction with the NI Mate software to control a dc motor.
* The DC motor we used is a vibrating motor meant for toys but in our case we mount it to an Asus Xtion Pro Live camera to improve depth images when using more than one camera.
* The point cloud produced by one camera will not affect the point could of the other cameras due to the vibrations. (According to the "Shake and sense" research)

* The program works in the following way:
	- Program starts and you are asked to input the IP address and port number of the NI Mate software / websocket server
	- After entering the IP and port number according to the on-screen example, press enter
	- The program will now connect to NI Mate / the websocket server and send the initial message with the device information
	- Once the message has been sent the program will sleep and wait for a start message to be received from NI Mate / the server
	- After receiving the message, the motor is ready to be turned on and off. The program will send a heartbeat message every second
	- To stop / close the program, simply send a stop message or turn off the server.
	- If the server is closed, the program will try to reconnect but after a number of retries it will close the program and turn off the motor

* Things to note:
	- This program can be modified to run any python code by just editing the function on line 339
	- Program needs to be run as sudo, due to having to access the GPIO pins on the raspberry pi
	- The program sends a heartbeat message every second to check that the server is still receiving messages
*/

///General Header files///
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <map>
#include <new>
#include <math.h>

///Websocket++ Header files///
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

///Boost Header files///
#include <boost/thread/thread.hpp>

///JSON Header files///
#include "json.hpp"
using json = nlohmann::json;

///Define///
#define MAX_RETRIES 5				//Nr of time the program will try to reconnect to the websocket server before closing
#define WAIT_TIME 8					//Define wait time for limiting the number of messages sent per second
#define TIME_BETWEEN_RECONNECT 5	//Defined the time in seconds we wait between each reconnect try

///Websocket++ typedef///
typedef websocketpp::client<websocketpp::config::asio_client> client;

///Websocket++ Variable///
bool disconnected = true;			//If we are disconnected from server
int retries = 0;					//If connection to server is lost we save the number of retries, exit program after MAX_RETRIES
bool start = false;					//Variable that is checked before we start to send data
bool stop = false;					//Variable to stop the program
int speed_param = 0;				//The parameter of the motor speed, used for when the "start_motor" function is called

///Time varibles///
boost::posix_time::ptime time_start;		//When to start counting towards the wait time
boost::posix_time::ptime time_end;			//Used to compare/determine how long time has passed
boost::posix_time::time_duration diff;		//The time difference between time_start and time_end

///Declaring functions///
void start_motor(int speed);

///Classes for websocket communication///
/*
* We use Websocket++ (Websocketpp) to connect to a websocket server
* Classes are from the Websocket Utils example but have been modified to our own needs
*/

class connection_metadata {

public:
    typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

    connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
      : m_id(id)
      , m_hdl(hdl)
      , m_status("Connecting")
      , m_uri(uri)
      , m_server("N/A")
    {}

    void on_open(client * c, websocketpp::connection_hdl hdl) {
        m_status = "Open";

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
    }

    void on_fail(client * c, websocketpp::connection_hdl hdl) {
        m_status = "Failed";

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
        m_error_reason = con->get_ec().message();
    }

	void on_close(client * c, websocketpp::connection_hdl hdl) {
    m_status = "Closed";
    client::connection_ptr con = c->get_con_from_hdl(hdl);
    std::stringstream s;
    s << "close code: " << con->get_remote_close_code() << " (" 
      << websocketpp::close::status::get_string(con->get_remote_close_code()) 
      << "), close reason: " << con->get_remote_close_reason();
    m_error_reason = s.str();
	}

	void on_message(websocketpp::connection_hdl, client::message_ptr msg) {
		//std::cout << msg->get_payload().c_str() << std::endl;						//In case we need to debug the messages we receive
        if (msg->get_opcode() == websocketpp::frame::opcode::text) {

			std::string incoming_msg = msg->get_payload(); 							//Convert to string
			json json_msg = json::parse(incoming_msg);								//Parse to Json

			//Check which function is sent in the Json message
			//Extract what is needed, the speed of the controller
			//Run the function
			if (start) {
				if (json_msg.find("type") != json_msg.end()) {						//Check Json message if it contains the type of message received
					std::string type_str = json_msg["type"].get<std::string>();		//Get the type of message
					if (type_str == "parameters") {									//Check to see if we received a parameters message
						if (json_msg.find("value") != json_msg.end()) {
							std::string temp_str = incoming_msg;
							std::string temp_str2;
							int found = temp_str.find("\"value\":");				//Find where the parameter values start
							int found2 = temp_str.find('}');						//Find where the parameter values end
							if (found > 0) {
								temp_str2 = temp_str.substr(found+8, found2-found-7);
								json json_tmp = json::parse(temp_str2);				//Parse our values to Json for easier access
								if (json_tmp.find("speed") != json_tmp.end()) {		//If we find the "Speed" parameter
									speed_param = json_tmp["speed"].get<int>();		//Get the speed and save it to the speed_param variable
									//std::cout << speed_param << std::endl;		//For debugging the speed that is received
								}
							}
						}
					}
					else if (type_str == "command") {						//Check to see if we received a command message
						if (json_msg.find("value") != json_msg.end()) {		//Find the value of the command message
							std::string cmd_str = json_msg["value"].get<std::string>();	//Save the command
							if (cmd_str == "start motor") {					//Check if the command was our "start_motor" command
								start_motor(speed_param);					//Run the function that starts the motor at a certain speed, according to our speed parameter
							}
						}
					}
					else if (type_str == "stop") { 							//Stop message from the server which will be sent to the client when the server wants to stop the program
							stop = true;
					}
				}
			}
			if (json_msg.find("type") != json_msg.end()) {
				std::string start_cmd = json_msg["type"].get<std::string>();
				if (start_cmd == "start") { //Start message from the server which will be sent to the client when the server is ready to receive data
					start = true;
				}
			}
			incoming_msg.clear();
			json_msg.clear();
        } 
		else {
            //m_messages.push_back("<< " + websocketpp::utility::to_hex(msg->get_payload())); //If the message is not a string or similar, we only send JSON/Strings
        }
    }

    websocketpp::connection_hdl get_hdl() const {
        return m_hdl;
    }

    int get_id() const {
        return m_id;
    }

    std::string get_status() const {
        return m_status;
    }

    void record_sent_message(std::string message) {
        //m_messages.push_back(">> " + message);	//We don't need to record messages, use if you want
    }

    friend std::ostream & operator<< (std::ostream & out, connection_metadata const & data);

private:
    int m_id;
    websocketpp::connection_hdl m_hdl;
    std::string m_status;
    std::string m_uri;
    std::string m_server;
    std::string m_error_reason;
    std::vector<std::string> m_messages;
};

std::ostream & operator<< (std::ostream & out, connection_metadata const & data) {
    out << "> URI: " << data.m_uri << "\n"
        << "> Status: " << data.m_status << "\n"
        << "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
        << "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason) << "\n";
    out << "> Messages Processed: (" << data.m_messages.size() << ") \n";

    std::vector<std::string>::const_iterator it;
    for (it = data.m_messages.begin(); it != data.m_messages.end(); ++it) {
        out << *it << "\n";
    }

    return out;
}

class websocket_endpoint {

public:
    websocket_endpoint () : m_next_id(0) {
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.clear_error_channels(websocketpp::log::elevel::all);
        m_endpoint.init_asio();
        m_endpoint.start_perpetual();
		m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&client::run, &m_endpoint);
        //m_thread.reset(new websocketpp::lib::thread(&client::run, &m_endpoint));
    }

	~websocket_endpoint() {
		m_endpoint.stop_perpetual();

		for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
			if (it->second->get_status() != "Open") {
				// Only close open connections
				continue;
			}
			std::cout << "> Closing connection " << it->second->get_id() << std::endl;
			websocketpp::lib::error_code ec;
			m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
			if (ec) {
				std::cout << "> Error closing connection " << it->second->get_id() << ": "  
						<< ec.message() << std::endl;
			}
		}
	m_thread->join();
	}

    int connect(std::string const & uri) {
        websocketpp::lib::error_code ec;

        client::connection_ptr con = m_endpoint.get_connection(uri, ec);
        if (ec) {
            std::cout << "> Connect initialization error: " << ec.message() << std::endl;
            return -1;
        }
        int new_id = m_next_id++;
        connection_metadata::ptr metadata_ptr = websocketpp::lib::make_shared<connection_metadata>(new_id, con->get_handle(), uri);
        m_connection_list[new_id] = metadata_ptr;

        con->set_open_handler(websocketpp::lib::bind(
            &connection_metadata::on_open,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_fail_handler(websocketpp::lib::bind(
            &connection_metadata::on_fail,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_close_handler(websocketpp::lib::bind(
            &connection_metadata::on_close,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_message_handler(websocketpp::lib::bind(
            &connection_metadata::on_message,
            metadata_ptr,
            websocketpp::lib::placeholders::_1,
            websocketpp::lib::placeholders::_2
        ));

        m_endpoint.connect(con);

        return new_id;
    }

	void send(int id, std::string message) {
		websocketpp::lib::error_code ec;
		con_list::iterator metadata_it = m_connection_list.find(id);

		if (metadata_it == m_connection_list.end()) {
			std::cout << "> No connection found with id " << id << std::endl;
			return;
		}
		m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);

		if (ec) {
			std::cout << "> Error sending message: " << ec.message() << std::endl;
			std::cout << "> Trying again after " << TIME_BETWEEN_RECONNECT << " seconds \n";
			boost::this_thread::sleep(boost::posix_time::seconds(TIME_BETWEEN_RECONNECT)); //If a send fails, the server has most likely crashed, wait TIME_BETWEEN_RECONNECT seconds if this happens then try to reconnect
			retries++;		//Keep track of the number of retries
			return;
		}
		retries = 0;		//If we send our heartbeat message successfully we set our nr of retries back to 0
	}

	void close(int id, websocketpp::close::status::value code, std::string reason) {
        websocketpp::lib::error_code ec;

        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }

        m_endpoint.close(metadata_it->second->get_hdl(), code, reason, ec);
        if (ec) {
            std::cout << "> Error initiating close: " << ec.message() << std::endl;
        }
    }

	connection_metadata::ptr get_metadata(int id) const {
		con_list::const_iterator metadata_it = m_connection_list.find(id);
		if (metadata_it == m_connection_list.end()) {
			return connection_metadata::ptr();
		} else {
			return metadata_it->second;
		}
	}

private:
    typedef std::map<int,connection_metadata::ptr> con_list;

    client m_endpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

    con_list m_connection_list;
    int m_next_id;
};

void start_motor (int speed) {	          				//Function to start the motor
	std::string motor_msg = "python motor_control.py ";	//Python command plus the python code file name
	motor_msg += std::to_string(speed);					//Add the speed we want to set the motor to, into our python command
	system(motor_msg.c_str());							//Run the command
	//std::cout << motor_msg << std::endl;				//To debug the system message
	return;
}

int main(int argc, char *argv[]) {

	///Websocket++ Setup///
	std::string ip_address;
	int id = -1;
	websocket_endpoint endpoint;

	while(id < 0) {												//Loop untill we get a valid ip address and port number
		std::cout << "Enter IP address and port, example: ws://192.168.1.1:7651 \n";
		std::cin >> ip_address;
		id = endpoint.connect(ip_address); 
	}
	boost::this_thread::sleep(boost::posix_time::seconds(2));	//Wait for connection

	system("bash get_mac_addr.sh");								//Run a bash file to get the current devices MAC address (eth0 address)
	std::string mac_addr;										//MAC address of the local computer / client
	std::ifstream F_out;										//New file streamer
	F_out.open("MAC_address.txt");								//Open the file that contains the MAC address
	std::getline(F_out, mac_addr);								//Get the MAC address
	F_out.close();												//Close our file streamer

	///Initial JSON message template that will be sent to the Server///
	char Json[] = R"({
		"type": "device",
		"value": {
			"device_id": "",
			"device_type": "rpi_motor",
			"name": "rpi_motor",
			"parameters": [ 
			{
				"name": "speed",
				"type": "vec",
				"datatype": "int",
				"default": 0,
				"min": 0,
				"max": 100,
				"count": 1
			}
			],
			"commands": [ {
				"name": "start motor",
				"datatype": "bool"
			}
			]
		}
	}
)";

	std::string init_msg(Json);					//Create string from the above char message
	init_msg.insert(52, mac_addr);				//Insert the MAC address of the device into the initial message
	//std::cout << init_msg << std::endl;		//In case of bug in our initial message

	json j_complete = json::parse(init_msg);	//Parse our message to Json
	std::string start_message = j_complete.dump(); //Dump our Json code to a string which will be sent to the server / NI Mate

	endpoint.send(id, start_message);			//Send the initial message	

	///Main loop, polls data and sends it to websocket server///

	while(!start) {								//Wait for server to send start command
		boost::this_thread::sleep(boost::posix_time::seconds(1));	
	}

	while (retries < MAX_RETRIES ) {										//Close after MAX_RETRIES
		boost::this_thread::sleep(boost::posix_time::seconds(1));			//Sleep for 1 second
		endpoint.send(id, "Heartbeat");										//We send a heartbeat message every second to check connection to the server
		if (retries > 0) {													//Heartbeat message didn't go through if retries is > 0
			int close_code = websocketpp::close::status::service_restart;	//Reason why we are closing connection
			std::string reason = "Trying to re-connect";
			endpoint.close(id, close_code, reason);							//This will fail if server was closed, might be useless
			id = endpoint.connect(ip_address);								//Try to reconnect to the server, id will be old id + 1
			boost::this_thread::sleep(boost::posix_time::seconds(2)); 		//Have to wait a couple of seconds to reconnect
		}
		if (stop) {															//If we receive a stop command from the sever / NI Mate
			//start_motor(0);												//Stop the motor
			break;															//Break from the loop and quit program
		}
	}
    start_motor(0);															//In case the program quit due to a server crash or something else, we stop the motor
	
	//Websocket Cleanup//
	int close_code = websocketpp::close::status::normal;
    std::string reason = "Quit program";
    endpoint.close(id, close_code, reason);									//Close the connection with the websocket server
	std::cout << "> Program has closed \n";

	return 0;
} 