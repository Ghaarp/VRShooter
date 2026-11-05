# Interaction UI MVP

Updated: 2026-11-04.

This directory is self-contained within the existing VRShooter module. No existing source, asset, module dependency, or project configuration is changed. Hand selection is deliberately not wired yet. The root context file is not updated because this request restricts edits to the new component directory.

## Integration (future edits)

Create one `UInteractionUIComponent` on the local player's actor per hand. Initialize it with that hand and the player's camera, then pass the existing selection result to it:

```cpp
// Once, after the player components are available:
InteractionUI->Initialize(HandSlot, Camera);

// After the existing FindPointForInteraction call:
InteractionUI->SetTarget(Result.Point, ItemDisplayName);
// Passing nullptr, or calling ClearTarget(), hides the callout immediately.
```

`Initialize` installs tick prerequisites; it does not call `FindPointForInteraction`. `SetTarget` retains a weak interface reference. Geometry continues following the camera and target every tick without resubmitting the same target. Call `SetTarget` again to change the text. Blueprint callers can use `SetTargetObject` and override `ResolveTargetText`.

The existing point interface has no display-name contract. Until integration adds one, pass an `FText` explicitly, or override `ResolveTargetText`. The default is the owning actor's runtime object name, a diagnostic fallback rather than a localized item name. Inventory/master points are supported through the same base interface.

## Layout and rendering

- `BuildAnchors` starts at camera-local Right and rotates this offset around Forward by `Index * (360 / Count)`, then adds the center at `(Distance, 0, 0)`. The component's camera transform supplies the world position and orientation. Equal effective radii produce a circle; independent right/up radii retain ellipse support. Count defaults to eight; an optional array supplies arbitrary normalized directions.
- `FindClosestAnchor` handles projection and switch hysteresis separately. `FitToSafeArea` clamps the camera distance, uniformly scales the padded label to the angular bounds, and limits its gap and anchor radii, returning `FLayoutDimensions`. `PlaceLabel` computes the underline end and label center from the selected anchor. `Build` resets/validates the result and orchestrates these helpers. Rotation always starts from the original vector to avoid accumulated rotation error.
- The chosen anchor minimizes distance in a common central-eye tangent-plane projection. This is independent of the desktop mirror viewport. `AnchorSwitchThreshold` supplies relative-distance hysteresis; zero selects the exact nearest anchor.
- The second segment and label extend left of the anchor for the calling left hand and right for the right hand, in camera space, regardless of the anchor's side. The component reads `GetInteractablePointHand()` from the hand passed to `Initialize` each tick. A missing hand or hand `None` preserves the outward-from-view-center fallback. The label sits above the segment. The configurable safe angular cone reserves room for the whole label in either direction, reducing excessive radii or label sizes as needed.
- Underline length is measured text width plus two space margins. Slate measures the actual shaped text; `LabelFont` supplies both the text and the U+0020 space metrics. An `SBox` adds `LabelPaddingSpaces` spaces at each horizontal end (default one), so the text begins one space from the anchor and the line continues one space past its far end, for either hand. The label's layout footprint includes these transparent margins. There is no fixed-width stretch or character-count approximation.
- Dimensions are physical centimeters converted using `WorldToMeters`, independent of actor scale. `LabelHeight` is the desired physical text-block height. `LabelWidth` now limits the maximum combined text-and-padding width; short names use their natural width. Long names and constrained safe areas uniformly shrink text and margins, preserving proportions. `LabelResolution` controls the longest render-target dimension; explicit uniform Slate/world scaling accounts for pixel rounding without distorting the font. Text, font, and padding changes update measurement/redraw during play.
- Two reusable cylinder meshes and a Slate label in a world-space `UWidgetComponent` use real world coordinates, so the connector starts at the target in each eye. Rendering components have no collision, shadows, navigation influence, replication, scene-capture visibility, or reflection-capture visibility. Only the owning player's view shows them.
- The material is translucent/unlit with depth testing disabled and the After Motion Blur pass. High translucency sort priority places it above ordinary scene translucency. PixelDepth/PixelDepthOffset is not used to defeat occlusion. This controls scene rendering, not compositor/system overlays or unrelated later overlay passes.
- No geometry is attached to the HMD late-update hierarchy: all three pieces use the same game-thread camera pose, keeping the connector junction coherent. Strict render-thread head locking is future work.
- No target, an invalid/destroyed target, a remote player, deactivation, or a point at/behind the camera hides the callout. A target outside the view can have a clipped connector while the label stays in the configured safe cone. “Always visible” here means scene objects do not occlude the active callout, not that offscreen world positions become visible.

## Source-only material default

In editor builds the component duplicates `/Engine/EngineMaterials/Widget3DPassThrough_Translucent` into a shared transient material, enables depth-test bypass and After Motion Blur, and recompiles that isolated copy. Neither the engine asset nor any project asset is edited or saved. The first use may incur shader compilation.

For a cooked build, assign `OverlayMaterial` before the first target is shown. It must be a precompiled, two-sided, unlit, translucent Widget3DPassThrough-compatible material with Disable Depth Test and After Motion Blur. Preserve its `SlateUI`, `TintColorAndOpacity`, `BackColor`, and `OpacityFromTexture` parameters. The same parent serves the text render target and solid line instances. Without this asset a cooked build logs an error and hides the callout instead of silently drawing occluded geometry. Authoring this asset is deferred to the later project-integration step.

## Remaining work and validation

- Integrate selection and the item-name source into the existing player/hand code.
- Decide whether two hands share/reserve anchors or allow overlapping callouts; the MVP keeps them independent.
- Validate font quality, thin-line aliasing, safe FOV, and fast head movement in the actual headset. The safe cone is configurable and is not queried from OpenXR per-eye FOV.
- Supply/cook the overlay material before shipping. Packaged builds and hardware VR rendering have not been validated.
- Tests in `Tests/InteractionUITest.cpp` exercise projection/depth invariance, custom anchors, safe margins, hysteresis, lifecycle, camera motion, physical scale, component reuse, and no-depth material settings.

Latest measured-text underline validation on UE 5.7.4:

- Added font-space padding and content measurement; the line matches the padded text footprint. `LabelFont` and `LabelPaddingSpaces` are configurable. Existing `LabelWidth` is retained as a maximum. Font shadow was removed so its offset does not enlarge the measured text footprint.
- Editor build succeeded with `-StressTestUnity`; both `VRShooter.Interaction.UI` tests passed with `-NullRHI -nohmd`, process exit codes 0. Logs: `Saved/Logs/InteractionUITextBuild.log` and `Saved/Logs/InteractionUITextTests.log`. Tests verify actual Slate space margins, short/long text widths, different glyph widths at equal character counts, proportional safe-area fitting, and both hand directions.
- `git diff --check` passed. Changes were limited to the UI component header/implementation, layout helper, existing UI tests, and this README. Existing detection/GrabManager/serialization changes and deleted files found at session start were preserved. Ordinary build and hardware rendering were not rerun; the previously established limitations below still apply. The tests validate measured layout and component geometry, not rendered pixels in a headset.

Previous anchor-generation refactor validation on UE 5.7.4:

- Editor build succeeded using the same `-StressTestUnity` workaround described below. Log: `Saved/Logs/InteractionUIAnchorBuild.log`. The ordinary build was not rerun; its previously established locomotion header issue remains outside scope.
- Both `VRShooter.Interaction.UI` tests passed with `-NullRHI -nohmd`, exit code 0. Log: `Saved/Logs/InteractionUIAnchorTests.log`. Coverage now includes cardinal circle positions, camera rotation/translation, separate ellipse radii, zero-count clamping, and empty selection, alongside existing layout and lifecycle checks.
- `git diff --check` passed. All 15 pre-existing modified/untracked files outside this directory retained their checksums. Only the layout helper, its existing test, and this README changed in this refactor. No rendering or headset tests were repeated for the geometry-only change.

Original MVP validation on UE 5.7.4 (before the anchor-generation refactor):

- Both new translation units compiled independently in the ordinary Editor build. That full build fails in the unchanged `Player/Component/Locomotion/Interface/LocomotionCompatible.h:43`, where `UCapsuleComponent` lacks a declaration. Adding the new reflected class changed unity grouping and exposed this existing include-order dependency. It was not patched because existing files are outside this request's scope. Log: `Saved/Logs/InteractionUIBuild.log`.
- Full Editor build succeeded with `-StressTestUnity`, which changes compilation grouping without editing project files. Command: `F:/UE/UE_5.7/Engine/Build/BatchFiles/Build.bat VRShooterEditor Win64 Development -Project=F:/proj/VRShooter/VRShooter.uproject -WaitMutex -NoHotReloadFromIDE -NoUBA -StressTestUnity -Log=F:/proj/VRShooter/Saved/Logs/InteractionUIBuildUnity.log`.
- Final run: all 12 `VRShooter.Interaction` tests passed, including both new UI tests. D3D12/SM6 was enabled with `-RenderOffscreen -nohmd`; process exit code was 0. Log: `Saved/Logs/InteractionUITestsVerified.log`. Command: `UnrealEditor-Cmd.exe F:/proj/VRShooter/VRShooter.uproject -unattended -nop4 -nosplash -RenderOffscreen -nosound -nohmd -NoLiveCoding -DisablePlugins=FunplayMCP -ExecCmds="Automation RunTests VRShooter.Interaction" -TestExit="Automation Test Queue Empty" -abslog=F:/proj/VRShooter/Saved/Logs/InteractionUITestsVerified.log`.
- Earlier UI test runs failed because the fixture counted the camera's editor mesh, then assumed generated component names had no numeric suffix. The final fixture compares component identities and the final run passed. Rendering code was unchanged by those fixture fixes.
- These tests verify geometry/state/material configuration, not rendered-pixel occlusion or stereo comfort. No headset or packaged-build test was performed. Startup also reported an OpenXR runtime API 1.0/1.1 mismatch on stderr despite `-nohmd`; no hardware VR compatibility is claimed.
- `git diff --check` passed. Checksums of all 15 pre-existing modified/untracked source files were unchanged; the only new project files are in this directory. Existing XRBase, deprecated skeletal-mesh access, animation pose, and missing `Create_Curves` warnings remain.
