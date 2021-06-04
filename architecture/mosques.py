# Originally developed as part of a final for Col Steeves at the 
# end of the 2020-2021 Spring Semester.
# It's honestly pretty bad. Like it's cool, but it's also bad and 
# was made under a timecrunch. There's so much more I wish that I
# could have done with it.

import bpy
import bmesh
import random
from math import pi, sin, cos, sqrt
from mathutils import Euler

# Clear all objects
def clear_model():
    objs = bpy.data.objects
    for obj in objs:
        objs.remove(obj, do_unlink=True) 

def create_light(location, brightness):
    light_data = bpy.data.lights.new(name="my-light-data", type='POINT')
    light_data.energy = brightness
    
    light_object = bpy.data.objects.new(name="my-light", object_data=light_data)
    bpy.context.collection.objects.link(light_object)
    light_object.location = location
    
def create_camera(location, rotation):
    camera_data = bpy.data.cameras.new(name='Camera')
    camera_object = bpy.data.objects.new('Camera', camera_data)
    bpy.context.scene.collection.objects.link(camera_object)
    camera_object.location = location
    camera_object.rotation_euler = Euler(rotation, 'XYZ')

def create_object(name, verts, faces, offset):
    mesh = bpy.data.meshes.new(name)
    object = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(object)
    object.location = offset
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)

def apply_offset(verts, offset):
    for i, vert in enumerate(verts):
        new_vert = [vert[0], vert[1], vert[2]]
        new_vert[0] += offset[0] # x
        new_vert[1] += offset[1] # y
        new_vert[2] += offset[2] # z
        
        verts[i] = tuple(new_vert)
        
    return verts

# verts: 0-3: bottom of rect
#        4-7: top of rect
# faces: 0: bottom
#        1: top
#        2, 4: y sides
#        3, 5: x sides
def make_rect(l, w, h, center=False):
    # 0         1          2          3
    # 4         5          6          7
    verts = [(0, 0, 0), (l, 0, 0), (l, w, 0), (0, w, 0), # bottom
             (0, 0, h), (l, 0, h), (l, w, h), (0, w, h)] # top
    # 0     1
    # 2     3       4       5
    faces = [(0, 1, 2, 3), (4, 5, 6, 7),
             (0, 4, 5, 1), (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0)]
             
    if center:
        verts = apply_offset(verts, (-l / 2, -w / 2, 0))
             
    return verts, faces

def shift_faces(faces, shift):
    for i, face in enumerate(faces):
        new_face = []
        for j, vert in enumerate(face):
            new_face.append(vert + shift)
        faces[i] = tuple(new_face)
    
    return faces

def seq(start, stop, step=1):
    l = [start]
    curr = start + step
    while curr <= stop:
        l.append(curr)
        curr += step
        
    return l

# goes counter clockwise, angles in radians
# orientation: 0: xy; 1: yz; 2: zx
def getCircVerts(loc, orientation, rad, angleBegin, angleEnd):
    places = 2
    
    x = lambda t: round(rad * sin(t), places)
    y = lambda t: round(rad * cos(t), places)
    z = lambda t: round(0           , places)
    
    if orientation == 0:
        point = lambda t: (x(t) + loc[0], y(t) + loc[1], z(t) + loc[2])
    elif orientation == 1:
        point = lambda t: (z(t) + loc[0], x(t) + loc[1], y(t) + loc[2])
    elif orientation == 2:  
        point = lambda t: (y(t) + loc[0], z(t) + loc[1], x(t) + loc[2])
    
    verts = []
    
    for angle in seq(angleBegin, angleEnd, pi / resolution):
        verts.append(point(angle))
        
    verts.append(point(angleEnd))
  
    return verts
 
# ===================== Constants
universal_offset = (0, 0, 0)
resolution = 25

clear_model()

# ===================== Prayer Hall
    # Made in hypostyle form, will use columns in support on the inside

phL = random.randint(10, 40)
phW = random.randint(int(phL / 2), phL)
phH = random.randint(5, 15)

phVerts, phFaces = make_rect(phL, phW, phH)

create_object("Prayer Hall", phVerts, phFaces, universal_offset)

# ===================== Light
diff = 10
create_light((phL + diff, phW + diff, phH + diff), 6000)

# ===================== Camera
diff1 = 100
create_camera((phL + diff1, phW / 2 + diff1, phH + diff1),
              (pi / 2 - .6, 0, .6 + pi / 2))

# Entrance(s)
dL = 0.5
dW = random.randint(int(phW / 5), max(int(phW / 5) + 1, min(int(phH / 3), int(phW / 3))))
dH = random.randint(int(phH / 3), int(phH / 2))

dPadding = random.randint(1, max(1, int(dW / 3)))
dFrameWidth = random.randint(max(1, int(dW / 3)), max(1, int(dW / 2))) / 2.0
dArchType = random.randint(1, 1)
dArchRadius = random.randint(int(dW / 2), int(dW))

numberDoors = random.randint(1, max(1, int(phW / (dW + (dPadding * 2)))))

initOffset = (phW / (numberDoors + 1)) - (dW / 2)

for n in range(0, numberDoors):
        
    dOffset = (phL, initOffset + (n * (phW / (numberDoors + 1))), 0)
#    dOffset = (0, 0, 0)

    dVerts, dFaces = make_rect(dL, dW, dH, center=False)
    
    f1Verts, f1Faces = make_rect(dL, dFrameWidth, dH)
    f2Verts, f2Faces = make_rect(dL, dFrameWidth, dH)
   
    archVerts = []   
    archFaces = [] 
    rot = (0, pi / 2, 0)
    if dArchType == 1:
        loc = (dL, dW / 2, dH)
        rad = dW / 2

        for j in range(0, 2):
            tempArchVerts = []
            
            archSideOuterVerts = getCircVerts(loc, 1, rad, -pi/2, pi/2)
            archSideInnerVerts = getCircVerts(loc, 1, rad - dFrameWidth, -pi/2, pi/2)
            
            tempArchVerts += archSideOuterVerts + archSideInnerVerts            
            archVerts += tempArchVerts
            
            loc = (0, dW / 2, dH)
        
        sideSideArchShift = int(len(archVerts) / 2)
        inOutArchShift = int(len(archVerts) / 4)
        for i in range(0, sideSideArchShift - 1):
            archFaces.append((i, inOutArchShift+i, inOutArchShift+i+1, i+1))
            archFaces.append((i, sideSideArchShift+i, sideSideArchShift+i+1, i+1))
        
    elif dArchType == 2:
        loc = (dL, dArchRadius, dH)
        rad = dArchRadius
        for i in range(0, 2):
#            bpy.ops.mesh.primitive_circle_add(vertices=resolution, radius=rad, location=loc, rotation=rot)
            loc = (dL, dW - dArchRadius, dH)
    elif dArchType == 3:
        loc = (dL, 0, dH + dArchRadius)
        rad = dArchRadius
        for i in range(0, 2):
#            bpy.ops.mesh.primitive_circle_add(vertices=resolution, radius=rad, location=loc, rotation=rot)
            loc = (dL, dW, dH + dArchRadius)

   
    # open up the Doors
    del dFaces[5]
    del dFaces[3]
    del dFaces[1]
    
    f1Verts = apply_offset(f1Verts, dOffset)
    f2Verts = apply_offset(f2Verts, dOffset)
    f2Verts = apply_offset(f2Verts, (0,dW - dFrameWidth,0))
    archVerts = apply_offset(archVerts, dOffset)
    dVerts = apply_offset(dVerts, dOffset)

    dFaces += shift_faces(f1Faces, len(dVerts))
    dVerts += f1Verts
    dFaces += shift_faces(f2Faces, len(dVerts))
    dVerts += f2Verts
    dFaces += shift_faces(archFaces, len(dVerts))
    dVerts += archVerts

    create_object("Prayer Hall Entrance " + str(n), dVerts, dFaces, universal_offset)
    
# ===================== Buttresses
bW = random.randint(1, 4)
bD = random.randint(1, bW * 2) / 2
bH = random.randint(int(phH / 2), phH)
bArchH = random.randint(1, bH)
bPadding = random.randint(bW*3, bW*5)
bWNum = int(phL / (bW + bPadding))
bLNum = int(phW / (bW + bPadding))

# xz-plane buttresses
j = 0
for i in range(0, bWNum * 2):
    
    ydist = phW
    if i % 2 == 0:
        ydist = -bD
    
    bOffset = ((phL / (bWNum + 1)) - (bW / 2) + (j * (phL / (bWNum + 1))),
               ydist,
               0)
    
    bVerts, bFaces = make_rect(bW, bD, bH)
    bVerts = apply_offset(bVerts, bOffset)
    
    create_object("Buttress", bVerts, bFaces, universal_offset)
    
    if i % 2 == 1:
        j += 1
       
# zy-plane buttresses 
for i in range(0, bLNum):

    bOffset = (-bD,
               (phW / (bLNum + 1)) - (bW / 2) + (i * (phW / (bLNum + 1))),
               0)
    
    bVerts, bFaces = make_rect(bD, bW, bH)
    bVerts = apply_offset(bVerts, bOffset)
    
    create_object("Buttress", bVerts, bFaces, universal_offset)
    
# ==================== DOMES

#clear_model()

base = random.randint(0, int(phH / 4))
domePadding = 0.5
domeR = random.randint(int(min(phL, phW) / 8), int((min(phL, phW) - domePadding) / 2))
domeX = random.randint(domeR, int(phL - domeR))
domeY = phW / 2

domeLoc = (domeX, domeY, phH + base)

bpy.ops.mesh.primitive_uv_sphere_add(
    radius=domeR,
    enter_editmode=False, 
    align='WORLD', 
    location=domeLoc, 
#    rotation=(0, -pi / 2, 0),
    segments=resolution,
    ring_count=resolution
)
baseLoc = (domeX, domeY, phH)
bpy.ops.mesh.primitive_cylinder_add(
    radius=domeR,
    depth=base * 2,
    enter_editmode=False, 
    align='WORLD', 
    location=baseLoc,
)

if random.randint(0, 1):
    print("THIS")
    bpy.ops.mesh.primitive_cylinder_add(
        radius=random.randint(1, max(2, int(domeR / 6))) / 10,
        depth=domeR * random.randint(2, 4),
        enter_editmode=False, 
        align='WORLD', 
        location=domeLoc,
    )

bpy.ops.object.select_all(action='DESELECT')

# ===================== Minaret
mStyle = random.randint(0, 0);
mW = random.randint(max(2, int(phW / 8)), max(3, int(phW / 6)))
mH = random.randint(int(phH * 1.5), int(phH * 2.5))
mPadding = random.randint(7, 10)

mLocs = 0

distFromDome = lambda x, y: sqrt((domeX - x) ** 2 + (domeY - y) ** 2)

mVerts = []
mFaces = []

if mStyle == 0:
    layers = random.randint(2, 4)
    diff = min(mW / layers, random.randint(1, max(2, int(mW / 2))) / 3)
    
    for layer in range(0, layers):
        if mW - (layer * diff) < 2:
            break
        
        layerVerts, layerFaces = make_rect(mW - (layer * diff), 
                                           mW - (layer * diff), 
                                           mH / layers)
        layerVerts = apply_offset(layerVerts, (diff / 2 * layer, 
                                               diff / 2 * layer, 
                                               mH / layers * layer))
        layerFaces = shift_faces(layerFaces, len(mVerts))
        
        mVerts += layerVerts
        mFaces += layerFaces

mVerts = apply_offset(mVerts, (0, 0, phH))

mx = 0
my = 0
tries = 0
while tries <= 100:
    mx = random.randint(0, phL - mW)
    my = phW / 2 - mW / 2
#    print("dist:", distFromDome(mx, my), "min:", domeR, "+", mPadding, "=", domeR + mPadding)
    if distFromDome(mx, my) >= domeR + mPadding:
        mVerts_ = apply_offset(mVerts, (mx, my, 0))
        create_object("Minaret", mVerts_, mFaces, universal_offset)
        break
    tries += 1