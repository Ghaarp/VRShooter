# Interaction UI

Updated: 2026-11-11. Native selection uses the smoothed arc Leash and per-point Price Tags.

## Integration

`AVRPlayer` creates `LeftInteractionUI` and `RightInteractionUI` as native `UArcInteractionUIComponent` instances inherited by existing player Blueprints. Their reflected properties retain the `UInteractionUIComponent` base type and the original subobject names, preserving existing settings and Blueprint references. BeginPlay initializes each with its hand slot and player camera. No Blueprint graph changes are required.

Both enable `bFollowHandSelection`: after the hand and camera ticks, the UI reads `UHandSlot::GetCachedInteractionResult()`. It never performs another selection query or changes interaction requests. Free hands show the selected item; occupied hands show the compatible inventory destination selected by the existing system. Missing selection, inactive/unticked hands, disabled/destroyed targets and nonlocal views hide the callout. The left hand always uses the left arc; the right hand always uses the right arc.

Stored items expose their owner's occupied inventory slot to a free hand, rather than exposing item points. Selection uses the slot's position and InteractionRange; the callout starts at the slot and resolves its text from the stored actor's explicitly marked MainPoint. Grab extracts that MainPoint with instant attachment. Missing, inactive, zero-range or hand-incompatible MainPoints disable extraction. Other players cannot select the slot, and direct point reservations on stored items are rejected for all players. Server extraction validates hand/source revisions, the stored attachment ID and current MainPoint before releasing inventory. As with other Grab requests, server admission does not validate distance.

Configure localized `InteractionDisplayName` on the interactable actor (default **Item**), optionally override it on an interaction point, and configure individual inventory slots (default **Inventory**). The `IInteractablePoint::GetInteractionDisplayName()` contract supplies these names. Values are authored locally, not replicated; dynamic names need their own existing gameplay synchronization. Automatic mode refreshes text each tick, including for an unchanged target. Custom unnamed points retain the diagnostic runtime object-name fallback; `ResolveTargetText` remains overridable in Blueprint.

Standalone/custom UI components default to manual mode:

```cpp
InteractionUI->Initialize(HandSlot, Camera);
InteractionUI->SetTarget(Result.Point, ItemDisplayName);
```

Initialize installs tick prerequisites. Manual SetTarget keeps its weak target until replaced or cleared; SetTargetObject is available in Blueprint. In automatic mode the hand cache replaces manual submissions on the next tick. Appearance and OverlayMaterial can be configured separately on each player UI component.

## Per-point highlights

Attach one `UInteractionPointWidgetComponent` directly beneath every `UInteractionPoint` in the interactable actor Blueprint. Each component creates its own `UUI_ItemHighlightBase` instance; animation, opacity and timeout state belong to that point, rather than to a hand or a shared player widget. `BP_Cube` has one such component; `BP_TwoHandStick` has three (GripLower, GripUpper and InventoryOuter). They use `/Game/UI/WBP_ItemHighlight`. `Scripts/ConfigurePointWidgets.py` configures these assets idempotently and saves only the named packages.

For the selected target, `UInteractionUIComponent` checks the item's mesh bounds against the camera frustum before updating visuals. Non-colliding meshes count; widget bounds do not. XR uses the stereo device's per-eye projections and view offsets: intersection with either eye is sufficient. Desktop uses the camera FOV and viewport aspect ratio. A 2 cm exit margin reduces switching at the boundary. This is a game-thread geometric check, with no obstacle traces, occlusion queries or render-time late-update guarantee.

- In view: request that point's highlight, face it toward the local camera, and hide the player's line UI.
- Outside view: release the highlight and show the existing hand-fixed line callout. Behind-camera targets use a finite side-directed connector on the UI plane instead of projecting through negative depth.
- Points without a configured highlight, custom interface targets and inventory-slot targets keep the existing line UI. A stored item's selected target is an InventorySlot, not its item's InteractionPoint.

Highlight requests are tracked per UI source. Releasing one hand does not hide a point still selected by another. Clearing/replacing a target, deactivation, reinitialization and teardown release that source; destroyed sources are pruned. Released highlights animate out while the line UI appears.

The highlight reads `UInteractionPoint::GetInteractionDisplayName()` on refresh, preserving localized `FText`, point overrides and actor-name fallback. The old `UpdateItemName(FName)` entry point remains available. `ItemName` and `ShowAnimation` bindings are optional: authored designer widgets can supply them; the default supplies a compact native text panel and an opacity fade at `AnimSpeedMultiplier`. Show/hide can reverse an authored animation during transitions. Component offset, scale, draw size and widget class are editable per point in the actor Blueprint. Presentation is local and does not change selection or replication.

## Arc Leash

**Leash** (Russian: «Поводок») means the label with its connector. Its **joint** («сустав») is where the connector meets the horizontal underline. **Price Tag** («Ценник») means the separate animated widget near the selected InteractionPoint.

`UArcInteractionUIComponent` is the current Leash variant. The original `UInteractionUIComponent` remains a usable fixed-joint version for comparison. Only a virtual layout hook was extracted from its tick; its fixed geometry and `InteractionUILayout.h` remain intact. The new layout copies the original safe fitting and horizontal label placement and substitutes an arc joint. Both variants share selection, rendering, materials and Price Tag switching.

- Each hand owns a 90-degree side sector, from -45 to +45 degrees around that side's horizontal direction. The arcs are invisible placement paths. The existing two radii describe an ellipse; equal radii give a circle. A polar intersection keeps the sectors at 90 screen degrees even for unequal radii.
- Item direction is transformed into the camera frame, including camera yaw, pitch and roll. Vertical bearing determines the joint angle; horizontal bearing is mirrored into the assigned hand's half. Angles clamp to the sector endpoints. No division by depth means rear targets retain their elevation. Inside a one-degree cone around the forward/rear axis, the last stable target bearing is kept.
- `ArcSmoothingHalfLife` in **Interaction UI / Arc** controls exponential angular smoothing: the default **0.08 s** halves the remaining angular error every 80 ms; zero disables smoothing. The joint is reconstructed on the ellipse after interpolating the angle, so it follows the arc rather than cutting through its interior. Equivalent elapsed time gives the same response across frame partitions.
- A new presentation starts from the middle of its side arc. Replacing a valid target retains the current angle and glides toward the next direction. Initialize, ClearTarget and Deactivate reset angular state. Showing a Price Tag pauses the Leash angle updates, retaining the last position until the Leash returns.
- Only the joint moves. The underline continues horizontally outward, and the text keeps the original camera-relative rotation. It does not rotate to follow the arc tangent. Maximum label dimensions reserve safe space across the whole path, including the upper endpoint; changing names does not reshape the arc.
- Left/right anchor indices stay 1/0; manual components without a hand use the right arc. `GetJointAngleDegrees()` exposes the current smoothed angle for inspection. Radius/thickness percentages, physical centimeters, text fitting and inherited rendering settings work as before.

## Layout and rendering

- Original fixed-joint variant: only two camera-relative anchors remain: horizontally left and right of view center at the same height. The calling hand fixes both the anchor and label direction; moving a target across the center never changes sides. Manual callers without a hand use the right anchor. Anchor indices remain 0 = right and 1 = left.
- `AnchorRadiusPercent` scales the original horizontal radius (`AnchorRadii.X`, default 25 cm): 100 preserves it, 50 halves it. The safe cone still caps the effective radius. Reserving the maximum configured label footprint keeps anchor positions independent of item-name width. Legacy AnchorCount, AnchorDirections and AnchorSwitchThreshold remain for serialized/Blueprint compatibility but are ignored and hidden from component settings.
- `LineThicknessPercent` scales both line diameters together: 100 preserves the original `LineThickness` (default 0.15 cm), 200 doubles it. Base centimeter values remain available under advanced settings; the percentage controls are editable and Blueprint-writable.
- Slate measures shaped text and font-space padding. LabelPaddingSpaces defaults to one per end; underline length matches the padded footprint. LabelHeight sets physical height, LabelWidth caps padded width, and LabelResolution controls render-target resolution. Long labels shrink proportionally.
- Dimensions use physical centimeters via WorldToMeters, independent of actor scale. The original Build uses fixed hand anchors, FitToSafeArea and PlaceLabel; the arc variant reuses fitting and label placement with a smoothed joint. Legacy generic anchor/projection helpers remain available but do not select runtime anchors.
- Two reusable cylinders and a world-space WidgetComponent render only for the owning player, without collision, shadows, navigation, replication, or capture visibility. Every cylinder material slot, including caps, receives the same white-texture/tint material; no section retains the default mesh material. Solid lines use the tint for BackColor too, rather than transparent black; text keeps its separate widget material.
- Translucent/unlit rendering disables depth testing and uses After Motion Blur with high sort priority. Scene geometry cannot occlude it; compositor or later overlays still can.
- All pieces share the game-thread camera pose, outside HMD late-update hierarchy. Strict render-thread head locking remains unimplemented.
- Invalid targets, remote players and deactivation hide the callout. Offscreen targets retain a callout; forward offscreen connectors may clip while labels remain in the safe cone.

## Material requirements

Editor builds create a shared transient copy of `/Engine/EngineMaterials/Widget3DPassThrough_Translucent`, enable depth bypass/After Motion Blur, and compile it. No asset is saved; first use may compile shaders.

Cooked builds require an assigned, precompiled OverlayMaterial: two-sided, unlit, translucent, Disable Depth Test, After Motion Blur, and Widget3DPassThrough-compatible. Preserve SlateUI, TintColorAndOpacity, BackColor, and OpacityFromTexture parameters. Text and solid lines share this parent. Without it, the cooked component logs an error and hides the callout.

## Validation and remaining work

- Arc validation (2026-11-11, UE 5.7.4): Editor Win64 Development build passed. The three new individually selected tests Arc.Layout, Arc.Component and Arc.PlayerBinding passed under NullRHI/no HMD, with zero errors (PlayerBinding has 17 existing asset/socket warnings). The four existing UI tests also passed. Coverage includes ellipse membership, true 90-degree sectors, half-life/frame independence, rear/ambiguous directions, horizontal label/underline, camera transforms, physical scale, target transitions, Price Tag switching and original/native/Blueprint component compatibility. Initial test assumptions were corrected; the final arc results are in `Saved/Automation/ArcLeashFocused/index.json`; the initial run and existing-test results are in `Saved/Automation/ArcLeash/index.json`. Logs: `Saved/Logs/ArcLeashBuild.log`, `Saved/Logs/ArcLeashFocusedTests.log`. Full suite, rendered pixels, packaged builds and headset comfort remain unverified.
- Validation (2026-11-11, verified UE 5.7.4): Editor Win64 Development build and the four individually selected UI tests (PointHighlight, Component, Layout, HandSelection) passed under NullRHI/no HMD. All had zero errors; HandSelection had 19 existing fixture warnings. Report: `Saved/Automation/PointHighlightFinal/index.json`; logs: `Saved/Logs/PointHighlightBuild.log`, `Saved/Logs/PointHighlightFinalTests.log`. The new PointHighlight test covers view switching, behind-camera fallback, per-point Blueprint configuration and instance uniqueness, point text updates, default fade opacity, partial non-colliding mesh visibility, deactivation and two-hand release. Full suite, authored animation playback, rendered pixels, headset stereo and packaged builds were not checked.
- `Source/VRShooter/Tests/HandInteractionSelectionTest.cpp` adds `UI.HandSelection`: cache-only consumption, empty/destroyed selections, inactive hands, inventory destinations, localized name fallback/override and existing player Blueprint inheritance. Tests in `UI/Tests/InteractionUITest.cpp` cover fixed hand anchors across the view center, legacy-setting independence, safe margins, actual text/space measurement, radius/thickness percentages, lifecycle, physical scale, component reuse, all-section materials, white tint/backing and white texture. They do not establish rendered-pixel color, occlusion or stereo comfort.
- Selection and localized names are integrated; each hand has its own smoothed side arc in the current variant. The original fixed-joint component remains available.
- Supply/cook the overlay material. Validate font quality, line aliasing, safe FOV, and fast head motion in a headset. Hardware VR and packaged rendering remain unverified.
