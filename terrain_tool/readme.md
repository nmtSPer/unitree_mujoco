# Terrain Generation Tool

This tool generates simple MuJoCo terrain scenes such as boxes, stairs, rough
ground, and height fields. It is usually used to create or update
`unitree_robots/<robot>/scene_terrain.xml`.

## Usage

1. Install dependencies:

```bash
pip3 install noise opencv-python numpy 
```

2. Open `terrain_generator.py` and modify the initial configuration near the
   top. Example for Go2:

```python
# Robot directory
ROBOT = "go2"
# Input scene file
INPUT_SCENE_PATH = "./scene.xml"
# Output
OUTPUT_SCENE_PATH = "../unitree_robots/" + ROBOT + "/scene_terrain.xml"
```

3. Run the generator:

```bash
cd terrain_tool
python3 ./terrain_generator.py
```

The program writes the terrain scene to
`unitree_robots/go2/scene_terrain.xml`.

To run it with the current start/tmuxp workflow:

```bash
./start.sh robot=go2 scene=scene_terrain.xml
```

Or set the simulator default in `simulate/config.yaml`:

```yaml
robot_scene: "scene_terrain.xml"
```

If using the Python simulator, modify `simulate_python/config.py`:

```python
ROBOT_SCENE = "../unitree_robots/" + ROBOT + "/scene_terrain.xml" 
```

## Function Reference

`terrain_generator.py` provides these helper functions.
### `AddBox`

Add a cube, parameters:
```python
position=[1.0, 0.0, 0.0] # Center position
euler=[0.0, 0.0, 0.0] # Orientation
size=[0.1, 0.1, 0.1] # Size, length x width x height
``` 
### `AddGeometry`

Add a geometry, parameters:
```python
position=[1.0, 0.0, 0.0] # Center position
euler=[0.0, 0.0, 0.0] # Orientation
size=[0.1, 0.1, 0.1] # Size, some geometries only require the first two parameters
geo_type="cylinder" # Geometry type, supports "plane", "sphere", "capsule", "ellipsoid", "cylinder", "box"
``` 
### `AddStairs`

Add stairs, parameters:
```python
init_pos=[1.0, 0.0, 0.0] # Position of the stair near the ground
yaw=0.0 # Stair orientation
width=0.2 # Stair width
height=0.15 # Stair height
length=1.5 # Stair length
stair_nums=10 # Number of stairs
```
### `AddSuspendStairs`

Add floating stairs, parameters:
```python
init_pos=[1.0, 0.0, 0.0] # Position of the stair near the ground
yaw=0.0 # Stair orientation
width=0.2 # Stair width
height=0.15 # Stair height
length=1.5 # Stair length
gap=0.1 # Floating gap
stair_nums=10 # Number of stairs
```
### `AddRoughGround`

Add rough terrain by randomly arranging cubes, parameters:
```python
init_pos=[1.0, 0.0, 0.0] # Position of the first cube
euler=[0.0, -0.0, 0.0], # Terrain orientation relative to the world
nums=[10, 10], # Number of cubes in x and y directions
box_size=[0.5, 0.5, 0.5], # Cube size
box_euler=[0.0, 0.0, 0.0], # Cube orientation
separation=[0.2, 0.2], # Cube separation in x and y directions
box_size_rand=[0.05, 0.05, 0.05], # Random increment of cube size
box_euler_rand=[0.2, 0.2, 0.2], # Random increment of cube orientation
separation_rand=[0.05, 0.05] # Random increment of cube separation
```

### `AddPerlinHeighField`

Generate terrain based on Perlin noise, parameters:
```python
position=[1.0, 0.0, 0.0],  # Terrain center position
euler=[0.0, 0.0, 0.0],  # Terrain orientation relative to the world
size=[1.0, 1.0],  # Terrain length and width
height_scale=0.2,  # Maximum terrain height
negative_height=0.2,  # Negative height in the z-axis direction
image_width=128,  # Terrain height map image pixel size
image_height=128,
smoothness=100.0,  # Noise smoothness
perlin_octaves=6,  # Perlin noise parameters
perlin_persistence=0.5,
perlin_lacunarity=2.0,
output_heightmap_image="height_field.png"  # Output height map image name
```

### `AddHeighFieldFromImage`

Generate terrain based on a given image, parameters:
```python
position=[1.0, 0.0, 0.0] # Terrain center position
euler=[0.0, 0.0, 0.0],  # Terrain orientation relative to the world
size=[2.0, 1.6],  # Terrain length and width
height_scale=0.02,  # Maximum terrain height
negative_height=0.1,  # Negative height in the z-axis direction
input_image_path="./unitree_robot.jpeg" # Input image path
output_heightmap_image="height_field.png", # Output height map image name
image_scale=[1.0, 1.0],  # Image scaling factors
invert_grayscale=False # Invert pixel
```
