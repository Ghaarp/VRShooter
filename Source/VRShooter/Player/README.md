# Desktop observer and player selection

Updated: 2026-11-12.

## Current desktop player

`AFlatScreenPlayer` derives directly from `ABasicPlayer`. Its configured asset is
`/Game/Player/BP_FlatScreenPlayer`. It uses the same Manny mesh, ABP_Locomotion,
locomotion actions, landing curve and lower-body IK settings as BP_VRPlayer.
These are copied defaults, not inheritance from BP_VRPlayer; future edits to the
VR Blueprint do not automatically update the desktop Blueprint.

The existing GM_VR retains BP_FlatScreenPlayer as its Default Pawn Class fallback
and selects a configured VR/Flat Blueprint per connection. It retains
BP_VRController, which still installs IMC_VR for local players.
No input mapping asset was changed. Assign desktop movement/jump keys to the
existing IA_Walk / IA_Jump in IMC_VR. MouseX / MouseY are bound directly in the
desktop pawn for yaw / inverted pitch; MouseSensitivity is editable. IA_Turn's
VR snap-turn handler and grab actions are not bound by the desktop pawn.

A collision-testing SpringArm holds the camera 300 cm behind an anchor 60 cm
above the capsule center. Control rotation drives the camera; capsule yaw follows
the controller, while capsule pitch/roll stay upright. Standard ACharacter/CMC
replication handles desktop movement and rotation. No XR tracking, room-scale
movement, controller simulation or VR pose replicator is installed on this pawn.

The native upper IK helper is inactive. IsUpperBodyIKEnabled also gates native
upper IK calculation and the rig's hand/clavicle/spine/head branches during
evaluation, restoring authored flags afterward. Lower IK and animation remain
enabled; the existing VR player and editor previews retain their prior defaults.

`Scripts/ConfigureFlatScreenPlayer.py` reproduces the two Blueprint changes through
the editor API. It saves only BP_FlatScreenPlayer and GM_VR. The current
default_map has no world GameMode override and therefore uses the configured
global GM_VR. Other maps, explicit game URL overrides, placed auto-possessed
pawns and editor spawn options can override the ordinary spawn path.

## Automatic VR / desktop selection

UPlayerModeLocalPlayer is configured as the engine's LocalPlayer class. Auto is
the default in Config/DefaultGame.ini. Restart an already running editor after
changing the configured LocalPlayer class. At login, Auto selects VR only if the
window's viewport has active stereo rendering and the XR system allows tracking
for that world. Otherwise it selects Flat. A connected headset alone does not
select VR, and transient tracking-pose validity does not change the chosen mode.

The world-specific UE check restricts shared-runtime VR Preview to its primary
PIE instance; other windows stay Flat. No headset means Auto stays Flat. Starting
the application in VR still requires the normal VR launch mode/runtime; forcing
a pawn mode does not enable or disable stereo or the headset itself.

Debug controls:

| Control | Effect |
| --- | --- |
| `-PlayerMode=Auto` / `-PlayerMode=VR` / `-PlayerMode=Flat` | Initial override for this process, applied before the first login. Useful in standalone/additional launch parameters. |
| `Player.Mode` | Console query: prints this window's override and resolved mode. |
| `Player.Mode Auto` / `Player.Mode VR` / `Player.Mode Flat` | Updates only this LocalPlayer's in-memory override for its next login. |
| `SetPlayerModeOverride` / `GetResolvedPlayerMode` | Blueprint API on UPlayerModeLocalPlayer, for future debug UI. |

Command-line names are case-insensitive. The initial command-line value overrides
the configured default; a later console/Blueprint change overrides that initial
choice. Console changes are not saved globally and do not leak to another PIE
window. A process launch parameter applies to all windows in that process.

Changing the override does not replace the current pawn. Reconnect or use
non-seamless map travel to apply it. Respawns and seamless travel retain the
mode accepted at login, including transfer to a replacement AVRController.
Forced VR is allowed without hardware for debugging, but supplies no fake poses.

UPlayerModeLocalPlayer::GetGameLoginOptions emits only a concrete
`PlayerMode=VR` or `PlayerMode=Flat`. UE uses it for local SpawnPlayActor, remote
login and split joins. This includes Steam's existing ClientTravel flow without
changes to the Steam plugin. Each connection reports its own choice; the server
never probes its own XR state to classify another player.

AVRGameMode::InitNewPlayer stores the validated mode on AVRController before
Super's player-start selection. GetDefaultPawnClassForController then selects
VRPawnClass or FlatPawnClass, defaulting to the configured BP_VRPlayer and
BP_FlatScreenPlayer assets. The native classes alone do not contain all authored
mesh/input/physics defaults. Missing, invalid and unresolved Auto login values
fall back to Flat; arbitrary client-supplied class paths are never loaded. If a
server-configured pawn asset is unavailable, an error is logged and the existing
DefaultPawnClass is used. The accepted mode replicates to the owning controller.

The default spawn/possess path creates the selected pawn directly. Placed
auto-possessed pawns or Blueprint overrides that bypass this path remain outside
the selection mechanism. Changing stereo after login does not switch pawn class.

## Possible BasicPlayer extraction: analysis only

Most common functionality already lives in BasicPlayer: locomotion, mesh,
camera reference, lower/upper IK providers, GrabManager and inventory slots.
Mapping-context installation already lives in the shared PlayerController.
There is little justification for moving the rest of VRPlayer wholesale.

| Candidate | Assessment |
| --- | --- |
| Mesh, ABP, locomotion actions/curves and shared body settings in BP_VRPlayer | Good candidate for a common BasicPlayer Blueprint or a shared configuration asset. A desktop C++ sibling cannot inherit defaults from BP_VRPlayer. Preserve CharacterMesh0 and existing serialized component names during any migration. |
| GatherCurrentMovement and rotation quantization | Potential base behavior: GatherCurrentMovement uses the common virtual GetLocomotionVelocity, whose default already returns GetVelocity. The room-scale velocity override must remain in VRPlayer. Quantization is a general network quality setting, currently configured only by VRPlayer. |
| GetIKOffset | A getter for a base-owned value can move to BasicPlayer when a common consumer needs it. No current desktop feature requires that move. |
| HandSlot creation, grab/release routing, interaction UI initialization | Reusable interaction concepts, but current wiring depends on physics hands and per-hand XR UI. Prefer a hand/interaction component with mode-specific transform and selection providers before moving these into BasicPlayer. Desktop currently needs no simulated hands. |
| Physics constraint setup | Linear/angular drives and capsule collision exclusion are generic, but FPhysicsBasedHand mixes those settings with MotionController components. Extract a helper/component with explicit primitive endpoints if another mode needs physical hands; do not add motion controllers to BasicPlayer. |
| Hand-ray debug drawing and configurable ray offsets | Can share a provider-based debug helper. Current physical-hand -Z ray convention and AVRPlayer coupling should remain VR-specific until desktop interaction is designed. |

VROrigin, HMD sampling/cache, recenter handling, pose transport/interpolation,
motion-controller polling, VRViewPoint camera, calibrated height and tracking
stream initialization remain VR-specific. They should stay outside BasicPlayer.

## Validation scope

Focused tests: VRShooter.Player.FlatScreen.Runtime and
VRShooter.Player.FlatScreen.MixedPIE. The latter checks default desktop spawning,
shared mapping context, standard desktop movement/yaw replication and production
VR pose reception by a desktop listen-host observer, using injected tracking.
VRShooter.Player.Mode.Selection exercises production Auto resolution with supplied
XR/stereo state and primary/secondary PIE worlds, launch/console overrides,
login-option generation and controller mode transfer. Mode.FlatHostPIE,
Mode.VRHostPIE and Mode.DedicatedPIE exercise actual pre-spawn login selection,
owning-client mode replication and respawn retention with both configured
Blueprints. Mode.LegacyPIE covers missing/invalid login options falling back to Flat.
UE 5.7.4 Editor Win64 Development build and all five Mode tests passed under
NullRHI/no HMD. The network tests retain asset/socket/transient-network fixture
warnings and the legacy test's intentional invalid-option warning. Actual
seamless map travel was not run; controller mode transfer was checked directly.
Headset/compositor behavior, rendered appearance, physical mouse feel, Steam
transport, separate machines and packaged builds require separate checks.
