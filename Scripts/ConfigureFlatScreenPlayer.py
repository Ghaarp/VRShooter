"""Create desktop player defaults and select them in the existing GM_VR.

Run with UnrealEditor-Cmd -run=pythonscript -script=<this file> after building.
Only BP_FlatScreenPlayer and GM_VR are saved. Shared input mappings are untouched.
"""
import unreal

path = '/Game/Player/BP_FlatScreenPlayer'
parent = unreal.load_class(None, '/Script/VRShooter.FlatScreenPlayer')
assert parent is not None
if unreal.EditorAssetLibrary.does_asset_exist(path):
    blueprint = unreal.load_asset(path)
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', parent)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'BP_FlatScreenPlayer', '/Game/Player', unreal.Blueprint, factory)
assert blueprint is not None
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
flat_class = unreal.load_class(None, path + '.BP_FlatScreenPlayer_C')
flat = unreal.get_default_object(flat_class)
assert isinstance(flat, unreal.FlatScreenPlayer)
vr = unreal.get_default_object(unreal.load_class(None, '/Game/Player/BP_VRPlayer.BP_VRPlayer_C'))

mesh = flat.get_editor_property('full_body_mesh')
vr_mesh = vr.get_editor_property('full_body_mesh')
mesh.set_skeletal_mesh_asset(vr_mesh.get_editor_property('skeletal_mesh_asset'))
mesh.set_anim_instance_class(vr_mesh.get_editor_property('anim_class'))
for name in ['relative_location', 'relative_rotation', 'relative_scale3d']:
    mesh.set_editor_property(name, vr_mesh.get_editor_property(name))

for component_name, names in [
    ('locomotion_component', [
        'walk_action', 'jump_action', 'land_prediction_curve', 'default_camera_pitch',
        'interpolate_speed_components', 'prediction_vertical_speed_max_value',
        'prediction_vertical_speed_min_value']),
    ('lower_body_ik_helper', [
        'default_pelvis_offset', 'max_foot_offset', 'minimal_foot_height', 'x_size',
        'y_size', 'z_size', 'trace_offset', 'use_foot_pitch_angle', 'use_foot_yaw_angle',
        'use_foot_roll_angle', 'draw_debug', 'interpolate', 'foot_interpolation_speed',
        'pelvis_interpolation_speed']),
]:
    source = vr.get_editor_property(component_name)
    destination = flat.get_editor_property(component_name)
    for name in names:
        destination.set_editor_property(name, source.get_editor_property(name))

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
assert unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)

game_mode = unreal.load_asset('/Game/Common/GM_VR')
defaults = unreal.get_default_object(unreal.load_class(None, '/Game/Common/GM_VR.GM_VR_C'))
defaults.set_editor_property('default_pawn_class', flat_class)
unreal.BlueprintEditorLibrary.compile_blueprint(game_mode)
assert unreal.EditorAssetLibrary.save_loaded_asset(game_mode, False)
unreal.log('FLAT_CONFIGURED BP_FlatScreenPlayer; GM_VR default pawn updated; shared input unchanged')
