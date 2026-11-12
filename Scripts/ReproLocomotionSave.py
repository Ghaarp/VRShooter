import unreal
asset = unreal.load_asset('/Game/PlayerAnimation/ABP_Locomotion')
unreal.get_editor_subsystem(unreal.AssetEditorSubsystem).open_editor_for_assets([asset])
frames = 0
handle = None
def tick(delta):
    global frames
    frames += 1
    if frames in (120, 240, 360):
        unreal.log('LOCOMOTION_REPRO compile/save ' + str(frames))
        unreal.BlueprintEditorLibrary.compile_blueprint(asset)
        unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
        unreal.SystemLibrary.collect_garbage()
    if frames == 480:
        unreal.get_editor_subsystem(unreal.AssetEditorSubsystem).close_all_editors_for_asset(asset)
    if frames >= 540:
        unreal.log('LOCOMOTION_REPRO completed')
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle = unreal.register_slate_post_tick_callback(tick)

