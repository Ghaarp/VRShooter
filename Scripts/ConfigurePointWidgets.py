"""Add a separate local highlight component to every point of the two test items.
Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.
Only the named widget and item Blueprint packages are saved.
"""
import unreal

library = unreal.SubobjectDataBlueprintFunctionLibrary
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
widget_path = '/Game/UI/WBP_ItemHighlight'
widget = unreal.load_asset(widget_path) if unreal.EditorAssetLibrary.does_asset_exist(widget_path) else None
if widget is None:
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class', unreal.load_class(None, '/Script/VRShooter.UI_ItemHighlightBase'))
    widget = unreal.AssetToolsHelpers.get_asset_tools().create_asset('WBP_ItemHighlight', '/Game/UI', unreal.WidgetBlueprint, factory)
    assert widget is not None
    unreal.BlueprintEditorLibrary.compile_blueprint(widget)
    assert unreal.EditorAssetLibrary.save_loaded_asset(widget, False)
widget_class = unreal.load_class(None, widget_path + '.WBP_ItemHighlight_C')
assert widget_class is not None

for path in ['/Game/Items/BP_Cube', '/Game/Items/BP_TwoHandStick']:
    blueprint = unreal.load_asset(path)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(blueprint)
    points = {}
    existing = {}
    for handle in handles:
        obj = library.get_associated_object(library.get_data(handle))
        if isinstance(obj, unreal.InteractionPoint):
            points[obj.get_path_name()] = (handle, obj)
        elif isinstance(obj, unreal.InteractionPointWidgetComponent):
            existing[obj.get_name()] = obj
    for handle, point in points.values():
        name = point.get_name().removesuffix('_GEN_VARIABLE') + '_Highlight'
        component = existing.get(name + '_GEN_VARIABLE')
        if component is None:
            params = unreal.AddNewSubobjectParams(parent_handle=handle, new_class=unreal.InteractionPointWidgetComponent, blueprint_context=blueprint)
            new_handle, reason = subsystem.add_new_subobject(params)
            assert library.is_handle_valid(new_handle), str(reason)
            assert subsystem.rename_subobject(new_handle, unreal.Text(name))
            component = library.get_associated_object(library.get_data(new_handle))
        component.set_editor_property('widget_class', widget_class)
        component.set_editor_property('relative_location', unreal.Vector(0, 0, 12))
        component.set_editor_property('relative_scale3d', unreal.Vector(0.05, 0.05, 0.05))
        unreal.log('POINT_WIDGET_CONFIGURED ' + path + ' ' + point.get_name() + ' ' + component.get_name())
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    assert unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
    unreal.log('POINT_WIDGET_SAVED ' + path + ' count=' + str(len(points)))
