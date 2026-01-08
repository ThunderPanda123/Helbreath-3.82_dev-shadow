import os
import math

BASE_DIR = os.path.dirname(os.path.dirname(__file__))
OUTPUT_DIR = os.path.join(BASE_DIR, "Binaries", "Server", "GameConfigs")
os.makedirs(OUTPUT_DIR, exist_ok=True)
OUTPUT_PATH = os.path.join(OUTPUT_DIR, "DropTables.sql")

# Table id base for NPC body-part drops.
TABLE_ID_BASE = 20000

# npc_type -> npc_name used in npc_configs
NPC_NAMES = {
    10: "Slime",
    11: "Skeleton",
    12: "Stone-Golem",
    13: "Cyclops",
    14: "Orc",
    16: "Giant-Ant",
    17: "Scorpion",
    18: "Zombie",
    22: "Amphis",
    23: "Clay-Golem",
    27: "Hellbound",
    28: "Troll",
    29: "Orge",
    30: "Liche",
    31: "Demon",
    32: "Unicorn",
    33: "WereWolf",
}

# Body-part drops from DeleteNpcInternal.
# Each entry: (item_id, switch_sides, switch_pick, chance_dice, chance_value)
# Probability = (1 / switch_sides) * (1 / chance_dice)
# For entries without a chance_dice, use chance_dice=None -> always when selected.
BODY_DROPS = {
    10: [
        (220, 1, 1, 25, 1),  # SlimeJelly
    ],
    11: [
        (219, 2, 1, 20, 1),  # SkeletonBones
    ],
    12: [
        (221, 2, 1, 30, 1),  # StoneGolemPiece
    ],
    13: [
        (194, 6, 1, 36, 1),  # CyclopsEye
        (195, 6, 2, 40, 1),  # CyclopsHandEdge
        (196, 6, 3, 30, 1),  # CyclopsHeart
        (197, 6, 4, 22, 1),  # CyclopsMeat
        (198, 6, 5, 40, 1),  # CyclopsLeather
    ],
    14: [
        (206, 4, 1, 11, 1),  # OrcMeat
        (207, 4, 2, 20, 1),  # OrcLeather
        (208, 4, 3, 21, 1),  # OrcTeeth
    ],
    16: [
        (192, 3, 1, 9, 1),   # AntLeg
        (193, 3, 2, 10, 1),  # AntFeeler
    ],
    17: [
        (215, 5, 1, 50, 1),  # ScorpionPincers
        (216, 5, 2, 20, 1),  # ScorpionMeat
        (217, 5, 3, 50, 1),  # ScorpionSting
        (218, 5, 4, 40, 1),  # ScorpionSkin
    ],
    18: [
        # Zombie uses bGetItemNameWhenDeleteNpc; no body-part items.
    ],
    22: [
        (188, 5, 1, 15, 1),  # SnakeMeat
        (189, 5, 2, 16, 1),  # SnakeSkin
        (190, 5, 3, 16, 1),  # SnakeTeeth
        (191, 5, 4, 17, 1),  # SnakeTongue
    ],
    23: [
        (205, 2, 1, 30, 1),  # LumpofClay
    ],
    27: [
        (199, 7, 1, 40, 1),  # HelboundHeart
        (200, 7, 2, 38, 1),  # HelboundLeather
        (201, 7, 3, 38, 1),  # HelboundTail
        (202, 7, 4, 36, 1),  # HelboundTeeth
        (203, 7, 5, 36, 1),  # HelboundClaw
        (204, 7, 6, 50, 1),  # HelboundTongue
    ],
    28: [
        (222, 5, 1, 35, 1),  # TrollHeart
        (223, 5, 2, 23, 1),  # TrollMeat
        (224, 5, 3, 25, 1),  # TrollLeather
        (225, 5, 4, 27, 1),  # TrollClaw
    ],
    29: [
        (209, 7, 1, 20, 1),  # OgreHair
        (210, 7, 2, 22, 1),  # OgreHeart
        (211, 7, 3, 25, 1),  # OgreMeat
        (212, 7, 4, 25, 1),  # OgreLeather
        (213, 7, 5, 28, 1),  # OgreTeeth
        (214, 7, 6, 28, 1),  # OgreClaw
    ],
    30: [
        # Liche uses bGetItemNameWhenDeleteNpc; no body-part items.
    ],
    31: [
        (541, 5, 1, 400, 123),  # DemonHeart
        (542, 5, 2, 1000, 123), # DemonMeat
        (543, 5, 3, 200, 123),  # DemonLeather
        (540, 5, 4, 300, 123),  # DemonEye
    ],
    32: [
        (544, 5, 1, 3000, 396), # UnicornHeart
        (545, 5, 2, 500, 3),    # UnicornHorn
        (546, 5, 3, 100, 3),    # UnicornMeat
        (547, 5, 4, 200, 3),    # UnicornLeather
    ],
    33: [
        (551, 8, 1, 30, 3),   # WerewolfTail
        (548, 8, 2, 28, 3),   # WerewolfHeart
        (550, 8, 3, 25, 3),   # WerewolfMeat
        (553, 8, 4, 35, 3),   # WerewolfLeather
        (552, 8, 5, 28, 3),   # WerewolfTeeth
        (554, 8, 6, 28, 3),   # WerewolfClaw
        (549, 8, 7, 38, 3),   # WerewolfNail
    ],
}

# For each NPC, compute weights (scaled to 10000) for tier 1.
# Include a no-drop entry (item_id = 0) to keep individual item probability <= 30%.

def compute_weight(switch_sides, chance_dice):
    prob = 1.0 / switch_sides
    if chance_dice is not None:
        prob *= 1.0 / chance_dice
    weight = int(round(prob * 10000))
    return max(1, weight)

# Build tables and entries
entries = []
for npc_type, items in BODY_DROPS.items():
    if not items:
        continue
    table_id = TABLE_ID_BASE + npc_type
    total_weight = 0
    for item_id, switch_sides, _pick, chance_dice, _val in items:
        w = compute_weight(switch_sides, chance_dice)
        entries.append((table_id, 1, item_id, w, 1, 1))
        total_weight += w
    # Add no-drop filler to keep single item <= 30% and preserve low probabilities
    filler = max(0, 10000 - total_weight)
    if filler > 0:
        entries.append((table_id, 1, 0, filler, 0, 0))

# Write SQL
with open(OUTPUT_PATH, "w", newline="") as f:
    f.write("-- NPC body-part drops (tier 1 only). item_id=0 represents no-drop.\n")
    f.write("CREATE TABLE IF NOT EXISTS drop_tables (\n")
    f.write("  drop_table_id INTEGER PRIMARY KEY,\n")
    f.write("  name TEXT NOT NULL,\n")
    f.write("  description TEXT NOT NULL\n")
    f.write(");\n\n")

    f.write("CREATE TABLE IF NOT EXISTS drop_entries (\n")
    f.write("  drop_table_id INTEGER NOT NULL,\n")
    f.write("  tier INTEGER NOT NULL,\n")
    f.write("  item_id INTEGER NOT NULL,\n")
    f.write("  weight INTEGER NOT NULL,\n")
    f.write("  min_count INTEGER NOT NULL,\n")
    f.write("  max_count INTEGER NOT NULL,\n")
    f.write("  PRIMARY KEY (drop_table_id, tier, item_id)\n")
    f.write(");\n\n")

    f.write("ALTER TABLE npc_configs ADD COLUMN drop_table_id INTEGER NOT NULL DEFAULT 0;\n\n")
    f.write("DELETE FROM drop_entries;\n")
    f.write("DELETE FROM drop_tables;\n\n")

    for npc_type, name in NPC_NAMES.items():
        table_id = TABLE_ID_BASE + npc_type
        f.write("INSERT INTO drop_tables(drop_table_id, name, description) VALUES(%d, %r, %r);\n" % (
            table_id, f"body_{name}", f"Body-part drops for {name}"))
    f.write("\n")

    for table_id, tier, item_id, weight, min_c, max_c in entries:
        f.write("INSERT INTO drop_entries(drop_table_id, tier, item_id, weight, min_count, max_count) VALUES(%d, %d, %d, %d, %d, %d);\n" % (
            table_id, tier, item_id, weight, min_c, max_c))
    f.write("\n")

    for npc_type, name in NPC_NAMES.items():
        table_id = TABLE_ID_BASE + npc_type
        f.write("UPDATE npc_configs SET drop_table_id=%d WHERE name=%r;\n" % (table_id, name))

print("Wrote", OUTPUT_PATH)
