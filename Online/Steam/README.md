# Minimal Steam sessions

Runtime plugin loaded from the project's `AdditionalPluginDirectories`. Existing gameplay classes are unchanged. Steam OSS provides lobbies/invites; SteamSockets provides the network transport. Development App ID: 480 (shared Spacewar test application).

## Try with two players

1. Start Steam on two Windows PCs using different accounts that are Steam friends. Enable Steam Overlay. Use the same project revision on both PCs.
2. Launch each game outside PIE. With an installed editor, run `UnrealEditor.exe VRShooter.uproject /Game/Maps/default_map -game -log` using absolute executable/project paths. Keep both games running for the first test.
3. On the host, enter `Steam.Host` in the game console. Wait for the map to reload as a listen server.
4. Enter `Steam.Invite`. Select the friend in Steam's overlay.
5. Accept the invitation on the second PC. The subsystem joins the lobby, resolves its connection address and travels to the host automatically.
6. `Steam.Leave` destroys the local session and reloads the map offline. The host can subsequently create a new session.

Blueprints can use **Get Game Instance Subsystem (SteamSessionSubsystem)** and call **Create Game**, **Invite Friend**, or **Leave Game**. Bind **On Status** to display progress/errors. No menu assets are added; console commands are the minimal host controls. Diagnostics use `LogVRShooterSteam`.

## Scope and limits

- Lobby capacity is two, including the host. No additional gameplay admission checks or GameMode changes are introduced.
- Session operations are asynchronous; simultaneous operations are rejected. Joining another invitation first destroys the existing local session.
- The host's map is `/Game/Maps/default_map`. Existing GameMode, pawn and replication remain responsible for spawning and gameplay.
- Steam OSS handles `+connect_lobby` startup invitations, but launching this specific development build from Steam with shared App ID 480 is not configured. Test invitations with both games running.
- No host migration, automatic reconnect, or custom VR overlay is provided. Steam overlay access depends on the local Steam/VR setup.
- Existing editor module dependencies in the main game module may block packaged builds. They are outside this integration; editor `-game` is the initial development launch path.
- A successful compile or offline smoke test does not verify Steam authentication, relay connectivity, two-PC invitations, or VR gameplay.

## Validation (2026-11-12)

- UE 5.7.4 Editor Win64 Development build passed, including UHT and the new runtime module.
- Editor `-game -nosteam -nullrhi -nohmd` smoke run passed: all three console commands reached the subsystem, reported the expected unavailable/not-hosting states, and exited normally. Log: `Saved/Logs/SteamOfflineSmoke.log`. No Error/Fatal entries were found.
- Game Win64 Development build failed before compilation on the existing `VRShooter -> BlueprintGraph -> KismetCompiler -> UnrealEd` dependency. Existing source classes and build rules were left unchanged.
- Automation suite, Steam login, live invites, two-PC connectivity and headset behavior were not run.
