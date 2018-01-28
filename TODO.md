/// TODO
[2017-01-28]
- Track::isClose --> TrackEntry
- TrackEntry::mVelocity --> Track
- Track --> avgHeight, avgWidth (similar to avgVelocity)


[2017-12-30]
Track
- define struct for parameters (maxDist, maxConfidence, trafficFlow)
- passing parameters when constructing track
- hold parameters in SceneTracker (update with observer pattern)

[2017-12-27]
Config::init
- move workDir logic to here
- store workPath in ParamList
Config
- read from / write to config only at startup and closedown

FrameHandler
- norm parameters as function of framesize: roi, blob_area, track_max_distance, group_distance 

Subject
- get rid of getDouble, getString -> getParam 
- double: stod(getParam)
- int: stoi(getParam)

Observer.h
- funktionsobjekt UpdateObserver einführen (als Ersatz für Funktionszeiger updateObserver in Config.cpp)

[2017-12-26]
FrameHandler::openCapSource
define path to video file as config date

rename: 
frame_handling.h, *.cpp -> frame_handler.h, *.cpp
tracking.h 	-> tracker.h
scene.cpp, vehicle.cpp, track.cpp -> tracker.cpp
Scene -> Tracker

[2017-11-02]
done: create sqlite3/bin /inc /lib /src for std setup (comparable to linux)
done: create all params in Config::Init
setup test cases

