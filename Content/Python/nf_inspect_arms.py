"""Neuron Forge - inspecte le mesh des bras (SKM_Manny_Simple) : slots de material et
position des os dans la pose de référence (pour séparer peau et manches dans un material).

Rapport : Saved/nf_inspect_arms.txt
"""
import os

import unreal

lines = []
eal = unreal.EditorAssetLibrary
mesh = eal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")

for i, slot in enumerate(mesh.get_editor_property("materials")):
    mat = slot.get_editor_property("material_interface")
    lines.append("slot {} : {} -> {}".format(i, slot.get_editor_property("material_slot_name"), mat.get_name() if mat else None))

# Position des os en pose de référence (espace du mesh) via un acteur temporaire
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actor = eas.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
component = actor.skeletal_mesh_component
component.set_skeletal_mesh_asset(mesh)
for bone in ["clavicle_l", "upperarm_l", "lowerarm_l", "hand_l", "middle_01_l", "middle_03_l",
             "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r", "middle_03_r", "spine_05", "head"]:
    try:
        location = component.get_socket_location(bone)
        lines.append("os {:12s} : x={:7.1f} y={:7.1f} z={:7.1f}".format(bone, location.x, location.y, location.z))
    except Exception as exc:
        lines.append("os {} : <{}>".format(bone, exc))
eas.destroy_actor(actor)

with open(os.path.join(unreal.Paths.project_saved_dir(), "nf_inspect_arms.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")
