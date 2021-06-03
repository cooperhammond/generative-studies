import bpy

folder = "C:\\Users\\C23Cooper.Hammond\\Documents\\dev\\Projects\\generative-studies\\moogle\\"

def clear_model():
    objs = bpy.data.objects
    for obj in objs:
        objs.remove(obj, do_unlink=True) 

def create_object(name, verts, faces, offset):
    mesh = bpy.data.meshes.new(name)
    object = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(object)
    object.location = offset
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)
    
def read_verts_and_edges(filename):
    file = open(filename, "r")
    contents = file.read().split("\n")

    string_vertices = contents[1].strip("(, ").strip(" ,), ").split(" ,), (, ")
    vertices = []

    for vertex in string_vertices:
        vertex = vertex.split(", ")
        vertex = tuple(map(lambda x: float(x), vertex))

        vertices.append(vertex)

    string_edges = contents[3].strip("(, ").strip(" ,), ").split(" ,), (, ")
    edges = []

    for edge in string_edges:
        edge = edge.split(", ")
        edge = tuple(map(lambda x: int(x), edge))

        edges.append(edge)

    file.close()

    return vertices, edges

clear_model()

verts, edges = read_verts_and_edges(folder + "umbrellamesh.csv")
#print(vertices)
#print(edges)

create_object("umbrella", verts, edges, (0, 0, 0))