# Kinect-Rendering
Project to explore the mapping and rendering of raw data retrieved using a Microsoft Kinect sensor. 

The following are the goals of this project in order:
- [Completed] Visualize the point cloud retrieved from the Kinect depth information with respect to the frustum of the virtual camera
- Use the accumulated point cloud data to calculate density and color information in a voxel grid
- Use marching cubes implementation from a previous project to construct a triangle mesh out of the voxelized data
- Attempt to implement photogrammetry methods to track camera movement between frames and align data from multiple camera positions
- Attempt to construct coherent 3D models by scanning objects by hand using the Kinect

Note: The Kinect Fusion library has already implemented comprehensive photogrammetry and SLAM techniques far outshadowing the scope of this project. The goal of this project is only to humbly learn and replicate a few of these techniques.

Relevant papers:
- https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/ismar2011.pdf

Progress:

Align color and depth cameras, create point cloud from fused data, and render point cloud.

![Image 1](https://github.com/nithinp7/Kinect-Rendering/blob/main/out-001.jpg)

![Image 2](https://github.com/nithinp7/Kinect-Rendering/blob/main/out-003.jpg)

![Image 3](https://github.com/nithinp7/Kinect-Rendering/blob/main/out-004.jpg)

![Image 4](https://github.com/nithinp7/Kinect-Rendering/blob/main/out-006.jpg)

Basic integration of marching cubes. So far, transfers density from point cloud only into a simple boolean voxel field (notice blocky geometry).

![Image 5](https://github.com/nithinp7/Kinect-Rendering/blob/main/Kinect%20SLAM%2011_18_2020%208_49_48%20PM.png)
