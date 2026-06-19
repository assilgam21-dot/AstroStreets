# AstroStreets — C++/OpenGL engine, Lua gameplay

A 2D ASCII game where **C++ owns the engine** and **Lua owns the gameplay**.
The engine opens an OpenGL window, renders a grid of ASCII glyphs, and polls
input. Everything that makes it a *game* — the player, NPC AI, damage, spells,
area triggers — lives in Lua scripts you can edit without recompiling.

## Threading model

The engine runs on **two threads** that communicate through small, lock-guarded
hand-off points — no shared mutable game state:

```
 main / render thread                 simulation thread
 ────────────────────                 ─────────────────
 owns GL context + window             owns the Lua VM
 glfwPollEvents()                     fixed-timestep update (60 Hz)
   └─ feeds ──▶ Input (mutex) ──read─▶ on_update(dt) / on_key(name)
 draw latest frame  ◀── SnapshotBuffer (mutex) ◀── publish on_draw() result
 swap @ vsync
```

- **Why split this way:** OpenGL contexts and GLFW window/event calls must stay
  on one thread, and Lua is single-threaded. So the render thread keeps the GL
  context + event pump, and the sim thread keeps the Lua VM. Neither reaches
  into the other's state directly.
- **`Input`** (`Core/Input.h`) — the render thread writes key events; the sim
  thread reads held keys and drains discrete presses. One mutex.
- **`SnapshotBuffer`** (`Core/SnapshotBuffer.h`) — the sim thread publishes a
  finished `CellGrid`; the render thread copies the latest. If the renderer is
  faster it redraws the last frame; if the sim is faster, old frames are
  dropped. One mutex.

The two rates are independent: rendering runs at vsync, simulation at a fixed
60 Hz step (with catch-up clamped to avoid a spiral of death).

## Project layout

```
src/
  main.cpp                 wires the two threads together
  Core/                    pure data + threading primitives (no GL, no Lua)
    CellGrid.h               a cols x rows grid of glyph+color cells
    Sprite.h                 one glyph + color (a visual)
    Animation.h / .cpp       timed sequence of sprites (frame timers)
    Input.h / .cpp           thread-safe key state (string-named keys)
    SnapshotBuffer.h         frame hand-off between sim and render threads
  Engine/                  windowing + OpenGL (render thread only)
    Window.h / .cpp          GLFW window + GL context + event polling
    Renderer.h / .cpp        batched glyph renderer for a CellGrid
    Font.h / .cpp            builds the 8x8 bitmap-font atlas texture
    font8x8_basic.h          public-domain font data
  Game/                    gameplay (sim thread only)
    World.h / .cpp           owns entities + map + queries; drives the sim
    GameLoop.h / .cpp        the fixed-timestep simulation loop
    ScriptEngine.h / .cpp    Lua VM (reserved for the spell layer, not yet wired)
    Entities/
      Entity.h / .cpp        base: id, position, Sprite + Animation, lifecycle
      Unit.h / .cpp          Entity + Health/Speed/AttackRange + combat hooks
      Player.h / .cpp        Unit: input-driven movement + melee
      Npc.h / .cpp           Unit: wander / chase / bite AI
      GameObject.h / .cpp    non-combat world object (e.g. chest)
      AreaTrigger.h / .cpp   rectangular trigger volume (enter/exit/stay)
      FireRune.h / .cpp      AreaTrigger: flickering damage-over-time rune
      DamageInfo.h           a mutable damage packet (amount, school, attacker)
scripts/
  spells.lua               reserved for the Lua spell layer (placeholder)
vcpkg.json                 dependency manifest
```

Dependencies (vcpkg manifest mode): `lua`, `sol2`, `glfw3`, `glad`.

## Entity model

Gameplay lives in a C++ class hierarchy owned by the `World` (Lua is reserved
for spells, added later):

```
Entity                       id, (x,y), Sprite, Animation, lifecycle hooks
 ├─ Unit                     Health, Speed, AttackRange, AttackPower
 │   │                       combat pipeline + virtual hooks:
 │   │                         OnDamageTaken(Unit* attacker, DamageInfo* dmg)
 │   │                         OnDeath / OnHeal / OnKill
 │   ├─ Player               held-key movement, melee attack
 │   └─ Npc                  wander / chase / bite AI
 ├─ GameObject               non-combat object (OnUse)
 └─ AreaTrigger              rect volume, OnUnitEnter/Exit/Stay
     └─ FireRune             damage-over-time, animated
```

Combat flows `attacker.DealDamage(target, dmg)` → `target.TakeDamage(dmg)` →
`OnDamageTaken` (may absorb/modify) → apply → `Kill` → `OnDeath` / `OnKill`.
Every entity owns a `Sprite` (and optional `Animation` whose frame timers the
engine advances each tick), so visuals and animation are per-entity attributes.

To add an entity type: subclass the right base under `Game/Entities/`, override
the hooks, and `World::Spawn<YourType>(...)` it. To expose a new engine
capability, extend `World`'s query/spawn API.

## Build & run

From Visual Studio: open `AstroStreets.slnx`, pick **x64 / Debug**, press F5.

From the command line:

```sh
msbuild AstroStreets.vcxproj /p:Configuration=Debug /p:Platform=x64
```

The first build downloads and compiles the vcpkg dependencies automatically.

## Controls

- **WASD / arrows** — move the player (`@`)
- **Space** — melee attack the nearest unit in range
- **Esc** — quit

`g` glyphs are hostile NPCs; the yellow `$` is a GameObject; the orange `*`
block is a fire-rune area trigger.

## Lua (spell layer)

Gameplay is C++ now. Lua remains in the build (`Game/ScriptEngine.*`, sol2 +
Lua 5.5) but is **reserved for spells, spell visuals and spell visual effects** —
the binding is intentionally not wired up yet. `scripts/spells.lua` is a
placeholder sketching where that's heading; `World`'s `scriptsDir_` marks the
seam where the spell host will plug in.
