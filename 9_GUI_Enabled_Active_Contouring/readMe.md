# Active Contouring using Graphical User Interface

A Graphical User Interface is developed that provides user an option to give initial set of points to perform active contour shaping around an object of their desire.

Currently the application can load PNM and PPM images. When RGB images are loaded necessary conversions to make it greyscale image is also been implemented. 

For demo purposes hawk.ppm image is used.

Two models of active contour is implemented
1. Rubber band model
	This is similar to "5_Active_Contouring" , wherein the user will left click at a point outside the object and start moving it around the object to form a loop and again left click to indicate that they are done. Now the points are resampled and now the user choosen points appear as red "+" marks. After this the points start to move to new location based on the least energy spot and settle around the user desired object.
2. Ballon model
	In this mode the user right clicks inside the object , it comes in handy when there are objects that are close by and the user has to delicately give the points between them, but this way it becomes much easier. There are three energy terms here , namely
	1. Internal Energy 1 - distance of each contour point from the centroid of all the points
	2. Internal Energy 2 - difference in distance between current point with previous and next point.
	3. External Energy - due to the edges of the image , dervied used Sobel convolution.

 


