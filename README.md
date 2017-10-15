========================================================================
    CarCount Application
========================================================================
car counter using opencv
- processes web cam input
- segments motion
- tracks observations and counts cars

/// TODO
[2017-10-15] 
load params from sqlite file out of home dir
params: framesize(x,y), background subtractor params,
max # of tracks, max # of vehicles,
window (velocity and position for associating blobs with tracks,
track confidence
file: config.sqlite
read from $HOME
if file not present or no parameters set --> use standard