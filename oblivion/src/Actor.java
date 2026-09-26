/**
 * Renamed from decompiled/j.java (see docs/CLASS_MAP.md, docs/rename.map).
 * The universal actor record: the player and every monster/NPC. Field names
 * come from a descriptor-aware decompile (tools/decompile_renamed.py), so the
 * many same-lettered fields of the original are all told apart. Meanings were
 * derived from ActorSystem's use of them and the character sheet in Game;
 * "..." marks best-effort guesses.
 *
 * Positions (pos/footB/footC/prevPos) are sub-tile world units, 128 per grid
 * cell; screenPos is the isometric projection of pos.
 */
public final class Actor {
   // draw-order cell: whichever of cell/footBCell/footCCell is nearest the camera (ActorSystem.updateSortCell)
   public byte[] sortCell = new byte[2];
   // grid cell of pos (pos >> 7); the three cells are the footprint tested against the collision layer
   public byte[] cell = new byte[2];
   // grid cell of footB
   public byte[] footBCell = new byte[2];
   // grid cell of footC
   public byte[] footCCell = new byte[2];
   // pixel width of the sprite's group 1 frame (used to lay out the footprint)
   public byte spriteWidth = 0;
   // spriteWidth / 2
   public byte spriteHalfWidth = 0;
   // 1-based index into Game.actors (1 = the player); also packed into projectile ownership
   public byte slot = 0;
   // 1=+y 2=-y 3=+x 4=-x; added to animStateOffset[animState] to get the sprite group id
   public byte facing = 2;
   // 0 idle, 1 walk, 4 attack/aggro, 6 dead, 7 ranged cast (2/3 = other special states); see ActorSystem.animStateOffset
   public byte animState = 0;
   // character class / monster kind 1-8; the player sheet maps 1 Monk, 2 Nightblade, 3 Barbarian, 4 Archer, 5 Knight, 6 Spellsword, 7 Sorcerer, 8 Battlemage
   public byte classId = -1;
   // HUD status icon frame id above the actor (-1 none); see ActorSystem.setStatusIcon
   public byte statusIcon = -1;
   // ProjectileManager slot of the looping buff effect, or -1
   public byte buffFxSlot = -1;
   // damage of the equipped weapon (row[3] of the weapon table)
   public byte weaponPower = 0;
   // equipped weapon id (weapon table row [0]); 0 = none
   public byte weapon = 0;
   // script id run by ScriptInterpreter when this actor dies, or -1
   public byte deathScript = -1;
   // value of the trigger layer under the actor (0/-1 = none); Game runs that script when it changes
   public byte enterScript = -1;
   // the previous tile's trigger value, run by Game when the actor steps off it (see Game paint/update loop)
   public byte leaveScript = -1;
   // value of the overlay layer under the actor (0-254), or -1
   public byte zoneId = -1;
   // character level 1-25 (also scales monster stats)
   public byte level = 0;
   // 0 = ignore the collision layer / bounds (isBlocked returns false)
   public byte collides = 1;
   // 1 once hp <= 0
   public byte dead = 0;
   // faction; actors on different teams are enemies (player = 1)
   public byte team = 1;
   // 1 = roll ScriptInterpreter.rollLoot() and spawn an item on death
   public byte dropsLoot = 1;
   // 1 = attacks with projectiles (class 4 / bow weapon type 4)
   public byte ranged = 0;
   // 1 = ignores applyDamage
   public byte invulnerable = 0;
   // HUD icon frame id of the current special attack type (see updateSpecialIcon)
   public byte attackIcon = -45;
   // HUD icon frame id of the active status effect (-47 poison, -48/-50 buffs), -1 none
   public byte effectIcon = -1;
   // damage per tick of the damage-over-time effect
   public byte dotDamage = 0;
   // monster AI type from spawn row [18]: 2 = blinker (teleports), 3 = ..., 4 = ranged
   public byte aiType = -1;
   // 1 = monster AI runs (chase/attack)
   public byte aiActive = 1;
   // 1 while a teleporting monster is off the map (teleportStep)
   public byte vanished = 0;
   // ms left before animState returns to 0 after a move/attack
   public short idleTimer = 0;
   // ms since the last animation frame advance
   public short animTimer = 0;
   // ms accumulator for hp regeneration
   public short hpRegenTimer = 0;
   // 40000 / maxHp ms per hp point
   public short hpRegenInterval = 0;
   // ms accumulator for mp regeneration
   public short mpRegenTimer = 0;
   // 40000 / maxMp ms per mp point
   public short mpRegenInterval = 0;
   // ms accumulator for movement steps
   public short moveTimer = 0;
   // ms accumulator for the floating damage text
   public short floatTextTimer = 0;
   // ms since death; at 250 the actor is removed
   public short deathTimer = 0;
   // ms elapsed of an item buff (vs itemBuffDuration)
   public short itemBuffElapsed = 0;
   // ms left on the damage-over-time effect
   public short dotRemaining = 0;
   // ms until the next dot tick (1000 ms period)
   public short dotTick = 0;
   // ms between monster attacks (spawn row [20] * 1000)
   public short attackInterval = 1000;
   // ms until a blinking monster may teleport again
   public short teleportTimer = 0;
   // level*4 + (strength+buffStrength)*2 + endurance*2 + bonusMaxHp
   public short maxHp = 100;
   // level*4 + intelligence*2 + bonusMaxMp
   public short maxMp = 100;
   // current hit points
   public short hp = 1;
   // current magic points
   public short mp = 1;
   // attribute (lang 415)
   public short strength = 0;
   // attribute (lang 416)
   public short intelligence = 0;
   // attribute (lang 417)
   public short willpower = 0;
   // attribute (lang 418); contributes to damage reduction
   public short agility = 0;
   // movement speed in world units/s (the character sheet shows a fixed "42")
   public short speed = 0;
   // attribute (lang 419); contributes to maxHp
   public short endurance = 0;
   // attribute (lang 420)
   public short personality = 0;
   // sum of worn armor defence (armor table column 4)
   public short armor = 0;
   // class/level bonus, % (lang 471 "Dodge")
   public short dodgeChance = 0;
   // class/level bonus, % (lang 470 "Block")
   public short blockChance = 0;
   // class/level percentage shown as "Defense Rating" (x3)
   public short defenseRating = 100;
   // weapon-skill damage multiplier %, shown as "Attack Rating" (x3)
   public short attackRating = 100;
   // aggro range; monster chases targets within it (default 300)
   public short sightRange = 0;
   // attack/keep-away range (default 200)
   public short attackRange = 0;
   // ms left on a spell buff; clears buff fields at 0
   public short buffTimer = 0;
   // multiplier % applied to dodgeChance (default 100)
   public short dodgeScale = 100;
   // extra max hp from equipment
   public short bonusMaxHp = 0;
   // extra max mp from equipment
   public short bonusMaxMp = 0;
   // attack bonus from a buff (item effect column 6)
   public short buffAttack = 0;
   // armor bonus from a buff (item effect column 7 / spell case 0)
   public short buffArmor = 0;
   // flat damage reduction from a buff (column 8)
   public short buffDefense = 0;
   // attack bonus from a buff (column 10 / spell case 1)
   public short buffAttack2 = 0;
   // strength bonus from a buff (column 11)
   public short buffStrength = 0;
   // duration of an item buff, ms (column 5)
   public short itemBuffDuration = 0;
   // current y offset of the floating text
   public short floatTextY = 0;
   // y offset the floating text started at
   public short floatTextStartY = 0;
   // row of the class table (ScriptInterpreter.getRow(5, classId))
   public int[] classRow = null;
   // world position (x, y), sub-tile units; >> 7 = grid cell
   public int[] pos = new int[2];
   // second footprint point (pos + half sprite width, isometric)
   public int[] footB = new int[2];
   // third footprint point (pos + full sprite width, isometric)
   public int[] footC = new int[2];
   // position before the last step, restored by undoMove
   public int[] prevPos = new int[2];
   // quick-use health potion row, or null
   public int[] hpPotion = null;
   // quick-use mana potion row, or null
   public int[] mpPotion = null;
   // ids the class may equip, -1 terminated (ScriptInterpreter.classLists)
   public int[] classList = null;
   // isometric screen position: x=(px-py)>>3, y=(px+py)>>4
   public int[] screenPos = new int[2];
   // destination (x,y) for auto-walking; [0] == -1 = none
   public int[] moveTarget = new int[]{-1, -1};
   // 255 packed slots: (category<<8)|id; category 0 weapon, 1 armor, 2 consumable; 0 = empty
   public int[] inventory = new int[255];
   // active special-attack row (ScriptInterpreter.specials)
   public int[] special = null;
   // alternative special-attack row toggled by handleAction(2)
   public int[] altSpecial = null;
   // worn armor id per slot (8 slots), -1 = empty
   public int[] wornArmor = new int[8];
   // the monster-table row this actor was spawned from
   public int[] spawnRow = null;
   // ms since this actor last killed something (reset on kill)
   public int killTimer = 0;
   // experience points (player: compared to ActorSystem.xpForLevel)
   public int xp = 0;
   // colour of the floating text (0xFF0000 red default)
   public int floatTextColor = 16711680;
   // shadow colour of the floating text
   public int floatTextShadow = 0;
   // ms since the last attack
   public int attackTimer = 0;
   // current attack target
   public Actor target = null;
   // actor that applied the damage-over-time effect
   public Actor dotSource = null;
   // minion this actor summoned (e.g. /oh_scamp.cml)
   public Actor summon = null;
   // the summoner, if this actor is a minion
   public Actor owner = null;
   // floating text above the actor (damage number / Dodge / Block)
   public String floatText = null;
   // .cml sprite resource path
   public String cmlPath = null;
   // display name ("Champion" for the player)
   public String name = null;
   // parsed .cml sprite tree
   public SpriteFrame sprite = null;
}
