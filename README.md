# Kinect-Rendering
Project to explore the mapping and rendering of raw data retrieved using a Microsoft Kinect sensor. 

### The following are the goals of this project in order:
- [Completed] Visualize the point cloud retrieved from the Kinect depth information with respect to the frustum of the virtual camera
- [Completed] Use the accumulated point cloud data to calculate density and color information in a voxel grid
- [Completed] Use marching cubes implementation from a previous project to construct a triangle mesh out of the voxelized data
- Attempt to implement photogrammetry methods to track camera movement between frames and align data from multiple camera positions
- Attempt to construct coherent 3D models by scanning objects by hand using the Kinect

### Note: 
The Kinect Fusion library has already implemented comprehensive photogrammetry and SLAM techniques far outshadowing the scope of this project. The goal of this project is only to humbly learn and replicate a few of these techniques.

### Relevant papers:
- https://www.microsoft.com/en-us/research/wp-content/uploads/2016/02/ismar2011.pdf

## Progress:

### Update 4:

Made the red voxels bounding box into a selection interface to determine what subset of the point cloud data to construct into a mesh. Added some render flags to control from input what type of data is being rendered (raw camera, point cloud, geometry, etc.). Here, my acoustic guitar is selected with the red box and only the points within the bounding box are used to construct a mesh. Note this is useful because the entire resolution of the voxel field goes directly towards the area of interest in the scene. 

![Image 11](https://github.com/nithinp7/Kinect-Rendering/blob/main/Screenshots/segmentation1.png)

![Image 12](https://github.com/nithinp7/Kinect-Rendering/blob/main/Screenshots/segmentation2.png)

![Image 13](https://github.com/nithinp7/Kinect-Rendering/blob/main/Screenshots/segmentation3.png)

### Update 3:

Implemented density and color smoothing function to convert from point cloud to voxel density and color field and added color to marching cubes. Notice the smoother geometry that is created and the optional coloring of the resulting mesh. So far, only a single frame (and therefore single viewpoint) of color and depth data is used to generate the mesh. This makes the mesh somewhat onesided, the occluded geometry cannot be constructed. I am hoping to speed up the voxelization by reimplementing part of it on the GPU. The next big goal is to implement automatic camera tracking so multiple frames of data can be aligned together to stochastically map the space and help generate more complete geometry.

![Image 6](https://github.com/nithinp7/Kinect-Rendering/blob/main/Screenshots/Kinect%20SLAM%2011_20_2020%2010_33_57%20PM.png)

![Image 7](https://github.com/nithinp7/Kinect-Rendering/blob/main/Screenshots/Kinect%20SLAM%2011_20_2020%209_55_32%20PM.png)

![Image 8](https://github.com/nithinp7/Kinect-Rendering/blob/main/Screenshots/Kinect%20SLAM%2011_20_2020%2011_03_40%20PM.png)

![Image 9](https://github.com/nithinp7/Kinect-Rendering/blob/main/Screenshots/Kinect%20SLAM%2011_20_2020%2011_03_56%20PM.png)

![Image 10](https://github.com/nithinp7/Kinect-Rendering/blob/main/Screenshots/Kinect%20SLAM%2011_20_2020%2011_08_34%20PM.png)

### Update 2:

Basic integration of marching cubes. So far, the program transfers density from the point cloud into a simple boolean voxel field (notice blocky geometry).

![Image 5](https://github.com/nithinp7/Kinect-Rendering/blob/main/Screenshots/Kinect%20SLAM%2011_18_2020%208_49_48%20PM.png)


### Update 1:

So far, the program aligns the color and depth cameras, creates a point cloud from the fused data, and renders a point cloud in realtime.

#### YouTube Link: 
https://youtu.be/TrRJwSSkSFM

![Image 1](https://github.com/nithinp7/Kinect-Rendering/blob/main/Screenshots/out-001.jpg)

![Image 2](https://github.com/nithinp7/Kinect-Rendering/blob/main/Screenshots/out-003.jpg)

![Image 3](https://github.com/nithinp7/Kinect-Rendering/blob/main/Screenshots/out-004.jpg)

![Image 4](https://github.com/nithinp7/Kinect-Rendering/blob/main/Screenshots/out-006.jpg)

