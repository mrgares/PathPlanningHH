#include <fstream>
#include <math.h>
#include <uWS/uWS.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include "json.hpp"
/****************Added***************/
#include "map.h"
#include "vehicle.h"
#include "road.h"
#include "planner.h"
/****************Added_End***************/	

using namespace std;

// for convenience
using json = nlohmann::json;

// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
string hasData(string s) {
  auto found_null = s.find("null");
  auto b1 = s.find_first_of("[");
  auto b2 = s.find_first_of("}");
  if (found_null != string::npos) {
    return "";
  } else if (b1 != string::npos && b2 != string::npos) {
    return s.substr(b1, b2 - b1 + 2);
  }
  return "";
}

/**Main Code**/
int main() {
  uWS::Hub h;
/****************Added***************/
//map file
string map_file_ = "../data/highway_map.csv";

int event_counter = 0;

// Map & Road & Vehicle & Planner instances
Map map (map_file_);
Road road;
Vehicle car;
Planner planner;

h.onMessage([
&map,
&road,
&car,
&planner,
&event_counter](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
/****************Added_End***************/	

 
 	// "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event
    //auto sdata = string(data).substr(0, length);
    //cout << sdata << endl;
    if (length && length > 2 && data[0] == '4' && data[1] == '2') {

      auto s = hasData(data);

      if (s != "") {
        auto j = json::parse(s);
        
        string event = j[0].get<string>();
        
        if (event == "telemetry") {
          // j[1] is the data JSON object
/****************Added***************/
		  event_counter++;
          cout << "-------------------------------" << endl;
          cout << "EVENT: " << event_counter << endl;
/****************Added_End***************/	

        	// Main car's localization Data
          	double car_x = j[1]["x"];
          	double car_y = j[1]["y"];
          	double car_s = j[1]["s"];
          	double car_d = j[1]["d"];
          	double car_yaw = j[1]["yaw"];
          	double car_speed = j[1]["speed"];

          	// Previous path data given to the Planner
          	auto previous_path_x = j[1]["previous_path_x"];
          	auto previous_path_y = j[1]["previous_path_y"];
          	// Previous path's end s and d values 
          	double end_path_s = j[1]["end_path_s"];
          	double end_path_d = j[1]["end_path_d"];

          	// Sensor Fusion Data, a list of all other cars on the same side of the road.
          	auto sensor_fusion = j[1]["sensor_fusion"];

          	json msgJson;

          	vector<double> next_x_vals;
          	vector<double> next_y_vals;
/****************Added***************/
			/*****************
			******CONTEXT*****
			*****************/

			cout << "-------------CAR--------------" << endl;
			cout << "Car S: " << car_s << " " << "Car speed: " << car_speed << endl;

			car.update_vehicle_values(car_x, car_y, car_speed, car_s, car_d, car_yaw);

			vector<Vehicle> left_lane;
			vector<Vehicle> center_lane;
			vector<Vehicle> right_lane;

			/* Get SF info to create the Road -> Sensor Fusion Elements (Just cars) */
			for (int i = 0; i < sensor_fusion.size(); ++i) {

				int id = sensor_fusion[i][0];
				double x = sensor_fusion[i][1];
				double y = sensor_fusion[i][2];
				double vx = sensor_fusion[i][3];
				double vy = sensor_fusion[i][4];
				double s = sensor_fusion[i][5];
				double d = sensor_fusion[i][6];
				double v = sqrt(vx*vx + vy*vy);

				Vehicle vehicle (id, x, y, v, s, d);
				LANE vehicle_lane = vehicle.lane();

				if (vehicle_lane == LANE::LEFT) {
				left_lane.push_back(vehicle);//pushing all vehicles data to the left lane vector of vehicles 
				} else if (vehicle_lane == LANE::CENTER) {
				center_lane.push_back(vehicle);
				} else {
				right_lane.push_back(vehicle);
				}
			}

			// Update road
			road.update_road(left_lane, center_lane, right_lane);

			/*****************
			******CORE*****
			*****************/
			
			// Previous path
			int n = previous_path_x.size();
			for(int i = 0; i < n; i++) {
				next_x_vals.push_back(previous_path_x[i]);//next_x_vals is empty on each iteration
				next_y_vals.push_back(previous_path_y[i]);
			}

			/*****************
			******DRIVE*******
			*****************/
			vector<vector<double>> trajectory = {next_x_vals, next_y_vals};
			planner.create_trajectoryHH(map, road, car, trajectory); 
								
			// Update next point
			msgJson["next_x"] = trajectory[0];
			msgJson["next_y"] = trajectory[1];


/****************Added_End***************/	
          	
          	auto msg = "42[\"control\","+ msgJson.dump()+"]";

          	//this_thread::sleep_for(chrono::milliseconds(1000));
          	ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
          
        }
      } else {
        // Manual driving
        std::string msg = "42[\"manual\",{}]";
        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
      }
    }
}); //end of "h.onMessage"

  // We don't need this since we're not using HTTP but if it's removed the
  // program
  // doesn't compile :-(
  h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data,
                     size_t, size_t) {
    const std::string s = "<h1>Hello world!</h1>";
    if (req.getUrl().valueLength == 1) {
      res->end(s.data(), s.length());
    } else {
      // i guess this should be done more gracefully?
      res->end(nullptr, 0);
    }
  });

  h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
    std::cout << "Connected!!!" << std::endl;
  });

  h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code,
                         char *message, size_t length) {
    ws.close();
    std::cout << "Disconnected" << std::endl;
  });

  int port = 4567;
  if (h.listen(port)) {
    std::cout << "Listening to port " << port << std::endl;
  } else {
    std::cerr << "Failed to listen to port" << std::endl;
    return -1;
  }
  h.run();
}
