import bpy
import bmesh
import colorsys
from math import sin, sqrt, pi, tau

objs = bpy.data.objects
for obj in objs:
    objs.remove(obj, do_unlink=True)

num_cubes = 16
extent = 8.0
padding = .0005

diff_ext = extent * 2

base_cube_size = (1.75 * (extent / num_cubes)) - padding
min_size = base_cube_size * .25
max_size = base_cube_size * .50
diff_size = max_size - min_size

cube_percent = 1.0 / (num_cubes - 1)

x_center = 0.0
y_center = 0.0
z_center = 0.0

max_dist = sqrt(2 * extent * extent)
inv_max_dist = 1.0 / max_dist

gamma = 2.2

count_sq = num_cubes ** 2
mesh_objs = [None] * count_sq
lin_range = range(0, count_sq, 1)

for n in lin_range:
    i = n // num_cubes
    j = n % num_cubes

    x_percent = i * cube_percent
    x = x_percent * diff_ext - extent

    y_percent = j * cube_percent
    y = y_percent * diff_ext - extent

    rise = y - y_center
    run = x - x_center
    dist_sq = rise ** 2 + run ** 2
    dist = sqrt(dist_sq)
    norm_dist = dist * inv_max_dist

    offset = -tau * norm_dist + pi

    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=base_cube_size)

    bmesh.ops.translate(
        bm,
        verts=bm.verts,
        vec=(0.0, 0.0, base_cube_size * 0.5)
    )

    meshdata = bpy.data.meshes.new("Mesh {0:0>2d}, {1:0>2d})".format(j, i))
    bm.to_mesh(meshdata)
    bm.free()

    mesh_obj = bpy.data.objects.new(meshdata.name, meshdata)
    mesh_obj.location = (x, y, 0.0)

    bpy.context.collection.objects.link(mesh_obj)

    mesh_obj["row"] = i
    mesh_obj["column"] = j
    mesh_obj["offset"] = offset

    mesh_objs[n] = mesh_obj

    mat = bpy.data.materials.new(
        name="Material ({0:0>2d}, {1:0>2d})".format(j, i))
    rgb = colorsys.hls_to_rgb(norm_dist * 0.333333, 0.525, 1.0)
    rgba = (rgb[0] ** gamma, rgb[1] ** gamma, rgb[2] * gamma, 1.0)
    mat.diffuse_color = rgba
    meshdata.materials.append(mat)


frame_rate = 30
key_frames = 15
keyframe_to_percent = 1.0 / (key_frames - 1)

scene = bpy.context.scene
frame_start = scene.frame_start
frame_end = scene.frame_end
scene.render.fps = frame_rate

lin_range_3 = range(0, count_sq * key_frames, 1)
for m in lin_range_3:
    h = m // count_sq
    n = m - h * count_sq

    mesh_obj = mesh_objs[n]
    offset = mesh_obj["offset"]

    h_percent = h * keyframe_to_percent
    curr_frame = int((1.0 - h_percent) * frame_start 
                     + h_percent * frame_end)
    scene.frame_set(curr_frame)
    
    angle = tau * h_percent
    total_angle = sin(offset + angle)

    angle_to_fac = total_angle * .5 + .5
    mesh_obj.scale[2] = min_size + angle_to_fac * diff_size

    mesh_obj.keyframe_insert(data_path="scale", index=2)
    
for mesh_obj in mesh_objs:
    anim_data = mesh_obj.animation_data
    action = anim_data.action
    fcurves = action.fcurves
    
    for fcurve in fcurves:
        fcurve.extrapolation = 'LINEAR'

        key_frames = fcurve.keyframe_points
        for key_frame in key_frames:
            key_frame.interpolation = "BEZIER"

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