import bpy
import bmesh

objs = bpy.data.objects
for obj in objs:
    objs.remove(obj, do_unlink=True)

scene = bpy.context.scene
display_settings = scene.display_settings
view_settings = scene.view_settings
sequencer = scene.sequencer_colorspace_settings

old_display_device = display_settings.display_device
old_view_transform = view_settings.view_transform
old_look = view_settings.look
old_exposure = view_settings.exposure
old_gamma = view_settings.gamma
old_sequencer = sequencer.name

display_settings.display_device = 'None'
view_settings.view_transform = 'Standard'
view_settings.look = 'None'
view_settings.exposure = 0.0
view_settings.gamma = 1.0
sequencer.name = 'sRGB'

# Assign colors to Blender data here.

display_settings.display_device = old_display_device
view_settings.view_transform = old_view_transform
view_settings.look = old_look
view_settings.exposure = old_exposure
view_settings.gamma = old_gamma
sequencer.name = old_sequencer

center = True
gamma = 2.2

num_cubes = 7
padding = 0.025
extent = 2.0

cube_size = (extent / num_cubes) - padding
cube_percent = 1 / (num_cubes - 1)

x_orig = 0.0
y_orig = 0.0
z_orig = 0.0

x_center = 0
y_center = 0
z_center = 0

if center:
    x_center = - num_cubes * (cube_size + padding) / 2
    y_center = - num_cubes * (cube_size + padding) / 2
    z_center = - num_cubes * (cube_size + padding) / 2


for i in range(0, num_cubes):

    x_percent = i * cube_percent
    x = x_percent * extent
    x += x_center

    red = x_percent ** gamma

    for j in range(0, num_cubes):

        y_percent = j * cube_percent
        y = y_percent * extent
        y += y_center

        blue = y_percent ** gamma

        for k in range(0, num_cubes):

            z_percent = k * cube_percent
            z = z_percent * extent
            z += z_center

            green = z_percent ** gamma

            location = (x, y, z)

            bpy.ops.mesh.primitive_cube_add(
                location=location,
                size=cube_size
            )

            current = bpy.context.object

            current.name = "Cube ({0}, {1}, {2})".format(i, j, k)
            current.data.name = "Mesh ({0}, {1}, {2})".format(i, j, k)

            mat = bpy.data.materials.new(name="Material ({0}, {1}, {2})".format(k, j, i))

            mat.diffuse_color = (red, green, blue, 1.0)
            current.data.materials.append(mat)

# Add a sun above the grid.
light_data = bpy.data.lights.new("Sun", 'SUN')
light_data.energy = 1.0
light_obj = bpy.data.objects.new(light_data.name, light_data)
light_obj.location = (0.0, 0.0, extent * 1.5)
bpy.context.collection.objects.link(light_obj)


# Add an orthographic camera above the grid.
cam_data = bpy.data.cameras.new("Camera")
cam_data.type = 'ORTHO'
cam_data.ortho_scale = extent * 7.0
cam_obj = bpy.data.objects.new(cam_data.name, cam_data)
cam_obj.location = (extent * 1.414, -extent * 1.414, extent * 2.121)
cam_obj.rotation_euler = (0.785398, 0.0, 0.785398)
bpy.context.collection.objects.link(cam_obj)

# Create a ground.
bm = bmesh.new()
bmesh.ops.create_grid(
    bm,
    x_segments=1,
    y_segments=1,
    size=extent * 15.0)
mesh_data = bpy.data.meshes.new("Ground")
bm.to_mesh(mesh_data)
bm.free()

# Add ground material.
mat = bpy.data.materials.new(name="Ground")
mat.diffuse_color = (0.015, 0.015, 0.015, 1.0)
mesh_data.materials.append(mat)

mesh_obj = bpy.data.objects.new(mesh_data.name, mesh_data)
mesh_obj.location = (0.0, 0.0, -extent * 1.5)

bpy.context.collection.objects.link(mesh_obj)
