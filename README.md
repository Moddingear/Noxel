# Noxel

What is it ? It's a vehicle creation framework. Noxel is a neologism of node and voxel, where instead of using cubes, you use polygons whit thickness, that connect to each other. That allows for things like odd angle placement, penetration simulation and accurate armor modeling, aerodynamics, buoyancy, and direct mesh to vehicle conversion.

## Getting started

Clone the RMC (RuntimeMeshComponent/RealtimeMeshComponent) version v4.5-ue5 to Plugins/RuntimeMeshComponent, then you can open the game in UE4.27 or later.

## Open source

Yes, this is open source. Doesn't mean it's free, but don't fret. 

You're free to do whatever with the code, but it remains my property, so if you want to use part of it as-is for a commercial project, get in touch. 

This is mostly meant as a free, large-scale example of the RMC, but it's far from production code, and I lost interest in developping it as it got too complicated, not fun, and I found other things to do. If you want to contribute code or even take over the project, feel free !

## Noxel implementation details

A vehicle is called a craft. It's made of object, called NObject (for Noxel Objects)

A noxel is made of nodes and panels. Think of nodes as vertices, and panels as faces.

A node container contains nodes, and a panel container contains panels. A part is a special object that contains both a node container and a panel container, all other object can only  contain nodes for attachment.

A panel can have multiple nodes, and thus connect multiple objects with different nodes containers. A NObject can have more than one node container, for example a thrust bearing would have two nodes container, one for the top and one for the bottom.

A node container can only be connected to one panel container.

Noxel works in "atomic" operations for edition, that are :
- Add node
- Remove node
- Add panel
- Remove panel
- Connect node to panel
- Disconnect node from panel
- Set panel property (material, thickness, etc...)

So for example, if you want to move a node, you have to disconnect it from all of the connected panels, delete it, then add another node, and connect it to the same panels.
This allows for undo-redo and networked craft editing.

## Where it's at

Editing crafts works, physics simulations work locally and somewhat in network, but loading crafts when they enter relevance doesn't quite work. This needs to break free from UE's network model, something that I don't have the patience for now.

Collisions and LODs work.

On the craft edition side, it's missing part selection, thickness selection, object rotation and tranform.

There is also a wiring mode, to connect logic between objects.

The game should be fairly convertable to VR.

I haven't ported it to UE5 due to the differences with RMC5 (missing Providers), also I don't believe the direction the UE5 has taken is the correct one.

Also yes, this is old now. I started working on this in 2016, and stopped in 2023.