# Axiom — Creative Direction

Loaded only for story, dialogue, audio and art tasks. Applies to the Doom 3 track (`axiom-doom3`) and to Axiom Arena. Style references (Spielberg, the Halo series, Detroit: Become Human) guide technique only: no names, designs, characters or story elements from them are used.

## 0. North Star: "This seems like real life, not a game"
Realism is judged on five pillars, and every one must hold, because a single fake element breaks the effect:
* **Image:** path-traced lighting, scanned or high-detail materials, film camera (§4).
* **Motion:** motion-captured or physically driven animation; no foot sliding, no snapping between animations, bodies react physically to hits.
* **Sound:** spatial audio with head-related transfer functions, sound that propagates and is blocked by the level's geometry, material-aware footsteps and impacts.
* **Behavior:** NPCs that perceive, hesitate, react and talk like people (§3, §5).
* **Interaction:** the world responds physically: objects move, break and fall according to the physics engine.
**Realism test (blind panel):** short Axiom clips and matched real-world reference footage (Axiom Arena scenes built from photogrammetry scans with reference photos), balanced classes, randomized order. Scenes, participant count and exclusion rules are registered before the panel runs. Results report the confusion matrix and confidence intervals clustered by participant and clip. "Axiom clips labelled real ≥ 40%" is a descriptive target; "indistinguishable" may be claimed only when the 95% interval of balanced accuracy lies entirely within 45–55%. The panel includes preregistered, separately scored positive controls (unmistakably real and unmistakably synthetic clips); before recruitment, 10 real and 10 synthetic controls and 5 comprehension questions are preregistered; a participant is eligible only with ≥ 9/10 correct in each control class and ≥ 4/5 comprehension, and all exclusions are reported with the fixed rule. Constant-response or failed-control panels are invalid. Equivalence is reported only for the tested participants and clips. NPC believability is tested the same way against recorded human-controlled characters.

## 1. The Core Rule: Story Lives in Gameplay
* The player never loses control. No cutscenes. Input is locked only for death and level loads; the engine logs every lock and a test fails on any other.
* Story is delivered in the world while the player moves and fights: radio, NPCs speaking as the player passes, environmental events, aftermath scenes.
* A missed story beat never blocks progress: every beat has a fallback delivery, and all delivered content is kept in the PDA log.

## 2. The AI Host (opening, inspired by Detroit: Become Human's menu host)
* The game opens with an AI character who greets the player in the main menu and introduces the world. In Doom 3 this is a new, original character: the Mars City facility AI. Nothing in the original story is changed.
* The host remembers the player across sessions (stored locally in the save profile) and reacts to how they played and the choices they made. Over the game it changes: it becomes aware of what is happening on the base, and its arc ends with a choice the player makes about it that permanently changes the menu.
* Limits: the first-launch introduction is skippable and under 30 seconds; later launches get a short greeting reacting to the last session. The host never delays gameplay.
* It runs on the dialogue system (§5) and is the first showcase of Axiom's AI.

## 3. Spielberg-Style Technique
* **Suggest before showing:** each major threat has a reveal budget. It is first suggested through sound, shadow, flickering light and brief glimpses; the full reveal happens only at its authored beat.
* **Reaction before revelation:** instead of taking the camera, an NPC stops, stares and reacts in voice and face; the player turns to see what they see.
* **Ordinary people, personal stakes:** NPCs have personal goals the player can help or ignore, with consequences later.
* **Rhythm:** alternate dread and wonder, with short moments of humor as relief. Every setup has a payoff.
* **Music:** a theme per major character; dynamic score driven by the director's tension level.

## 4. Series-Style Presentation (live-action military sci-fi)
* **Look:** photorealistic path-traced rendering with a film camera: per-scene color grading, anamorphic-style bokeh and flares, film grain, HDR. Always first-person, never taking control.
* **Squad drama:** marines fight alongside the player in parts of the game, each with a personality and personal arc; they talk over the radio during play and react to losses. Their combat behavior is learned.
* **Voiced lead:** the player character speaks (radio dialogue, reactions, relationships).
* **Writing:** original, and consistent with established canon.

## 5. Dialogue System
* A language model writes lines from a canon document (who each character is, what they know) plus current game state, and may request only actions from an allowed list.
* **Deterministic enforcement first:** authoritative facts (who is alive, what the player has done, what each character knows) and every action's world-state preconditions are checked in code; a request that fails is rejected regardless of any model's opinion.
* **Model checker second:** a second model checks each line against the canon document. The checker itself is evaluated against an independently labelled set of contradictions and valid lines; its recall and false-rejection thresholds are frozen before any tuning. Checker approval alone never satisfies canon acceptance.
* Rejected or timed-out requests use authored fallback dialogue. Replays use recorded lines.
* Runs on the Threadripper CPU or a spare machine, never on the GPU during play. Up to ~2 s latency is acceptable for conversation; combat decisions never go through it.
* Voices: licensed or consented only (CONVENTIONS §7). Original Doom 3 characters use only their original recorded lines; new lines are voiced only for new characters and the player character.

## 6. Nostalgia (Doom 3)
* Everything original stays: level layouts, story events, characters (Mars City 2145, the UAC base, Betruger's portal, the Soul Cube). New story layers on top and never replaces.
* **Classic/Remastered toggle:** one key switches instantly between the original look and path-traced photorealism, with identical game state.
* **Classic/Modern settings** for changed mechanics, starting with the flashlight: Classic = choose flashlight or weapon; Modern = armor-mounted light (as in the BFG Edition).
* Familiar events are retold with §3 technique, never removed or rewritten. Original PDAs and audio logs remain; new characters refer to them.

## 7. Director & Story Graph
* Story beats are authored in a graph with conditions (location, player actions, prior beats). The AI director schedules them: exposition only in quiet stretches, never above a set combat-intensity level unless the beat concerns the fight itself.
* The director also controls spawns, items, lighting failures and radio events from player state (health, ammo, recent damage, time since last fight).

## 8. Story Acceptance Tests
Each test states its track and the earliest stage it applies from. A stage must pass only the tests that apply to it.
| Test | Track | From |
| --- | --- | --- |
| Input locked only for death and loads | Arena / Doom | Stage 8 / D2 |
| No exposition above the intensity threshold | Arena / Doom | Stage 8 / D2 |
| Every authored beat reached in seeded bot playthroughs; every fallback delivery works | Arena / Doom | Stage 8 / D2 |
| Every setup has a reachable payoff; full reveals only at authored beats | Arena / Doom | Stage 8 / D2 |
| Tension curve within tolerance of the authored target | Arena / Doom | Stage 8 / D2 |
| Canon: deterministic fact/precondition checks pass; checker meets its frozen recall and false-rejection thresholds on the labelled set | Arena / Doom | Stage 8 dialogue / D3 |
| Every original Doom 3 story beat present | Doom | D2 |
| Classic/Remastered toggle within one frame, identical state hash | Doom | D4 |
| AI host: first-launch intro skippable and under 30 s; reacts to the last session | Doom | D3 |
