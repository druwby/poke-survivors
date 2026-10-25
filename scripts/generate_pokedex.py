import xml.etree.ElementTree as ET
import os

# Complete 151 Gen 1 Pokemon definitions
# (id, name, type1, type2, stage, attack_archetype)
pokemon_defs = [
    (1, 'Bulbasaur', 'POKE_TYPE_GRASS', 'POKE_TYPE_POISON', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (2, 'Ivysaur', 'POKE_TYPE_GRASS', 'POKE_TYPE_POISON', 2, 'ENEMY_ATTACK_RANGED_SPORE'),
    (3, 'Venusaur', 'POKE_TYPE_GRASS', 'POKE_TYPE_POISON', 3, 'ENEMY_ATTACK_RANGED_SPORE'),
    (4, 'Charmander', 'POKE_TYPE_FIRE', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_EMBER'),
    (5, 'Charmeleon', 'POKE_TYPE_FIRE', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_EMBER'),
    (6, 'Charizard', 'POKE_TYPE_FIRE', 'POKE_TYPE_FLYING', 3, 'ENEMY_ATTACK_RANGED_EMBER'),
    (7, 'Squirtle', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_WATER'),
    (8, 'Wartortle', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_WATER'),
    (9, 'Blastoise', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 3, 'ENEMY_ATTACK_RANGED_WATER'),
    (10, 'Caterpie', 'POKE_TYPE_BUG', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (11, 'Metapod', 'POKE_TYPE_BUG', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (12, 'Butterfree', 'POKE_TYPE_BUG', 'POKE_TYPE_FLYING', 3, 'ENEMY_ATTACK_RANGED_SPORE'),
    (13, 'Weedle', 'POKE_TYPE_BUG', 'POKE_TYPE_POISON', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (14, 'Kakuna', 'POKE_TYPE_BUG', 'POKE_TYPE_POISON', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (15, 'Beedrill', 'POKE_TYPE_BUG', 'POKE_TYPE_POISON', 3, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (16, 'Pidgey', 'POKE_TYPE_NORMAL', 'POKE_TYPE_FLYING', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (17, 'Pidgeotto', 'POKE_TYPE_NORMAL', 'POKE_TYPE_FLYING', 2, 'ENEMY_ATTACK_RANGED_GUST'),
    (18, 'Pidgeot', 'POKE_TYPE_NORMAL', 'POKE_TYPE_FLYING', 3, 'ENEMY_ATTACK_RANGED_GUST'),
    (19, 'Rattata', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (20, 'Raticate', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (21, 'Spearow', 'POKE_TYPE_NORMAL', 'POKE_TYPE_FLYING', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (22, 'Fearow', 'POKE_TYPE_NORMAL', 'POKE_TYPE_FLYING', 2, 'ENEMY_ATTACK_RANGED_GUST'),
    (23, 'Ekans', 'POKE_TYPE_POISON', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_SPORE'),
    (24, 'Arbok', 'POKE_TYPE_POISON', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_SPORE'),
    (25, 'Pikachu', 'POKE_TYPE_ELECTRIC', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_SPARK'),
    (26, 'Raichu', 'POKE_TYPE_ELECTRIC', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_SPARK'),
    (27, 'Sandshrew', 'POKE_TYPE_GROUND', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (28, 'Sandslash', 'POKE_TYPE_GROUND', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (29, 'NidoranF', 'POKE_TYPE_POISON', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_SPORE'),
    (30, 'Nidorina', 'POKE_TYPE_POISON', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_SPORE'),
    (31, 'Nidoqueen', 'POKE_TYPE_POISON', 'POKE_TYPE_GROUND', 3, 'ENEMY_ATTACK_SLAM_AOE'),
    (32, 'NidoranM', 'POKE_TYPE_POISON', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_SPORE'),
    (33, 'Nidorino', 'POKE_TYPE_POISON', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_SPORE'),
    (34, 'Nidoking', 'POKE_TYPE_POISON', 'POKE_TYPE_GROUND', 3, 'ENEMY_ATTACK_SLAM_AOE'),
    (35, 'Clefairy', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (36, 'Clefable', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (37, 'Vulpix', 'POKE_TYPE_FIRE', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_EMBER'),
    (38, 'Ninetales', 'POKE_TYPE_FIRE', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_EMBER'),
    (39, 'Jigglypuff', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (40, 'Wigglytuff', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (41, 'Zubat', 'POKE_TYPE_POISON', 'POKE_TYPE_FLYING', 1, 'ENEMY_ATTACK_RANGED_GUST'),
    (42, 'Golbat', 'POKE_TYPE_POISON', 'POKE_TYPE_FLYING', 2, 'ENEMY_ATTACK_RANGED_GUST'),
    (43, 'Oddish', 'POKE_TYPE_GRASS', 'POKE_TYPE_POISON', 1, 'ENEMY_ATTACK_RANGED_SPORE'),
    (44, 'Gloom', 'POKE_TYPE_GRASS', 'POKE_TYPE_POISON', 2, 'ENEMY_ATTACK_RANGED_SPORE'),
    (45, 'Vileplume', 'POKE_TYPE_GRASS', 'POKE_TYPE_POISON', 3, 'ENEMY_ATTACK_RANGED_SPORE'),
    (46, 'Paras', 'POKE_TYPE_BUG', 'POKE_TYPE_GRASS', 1, 'ENEMY_ATTACK_RANGED_SPORE'),
    (47, 'Parasect', 'POKE_TYPE_BUG', 'POKE_TYPE_GRASS', 2, 'ENEMY_ATTACK_RANGED_SPORE'),
    (48, 'Venonat', 'POKE_TYPE_BUG', 'POKE_TYPE_POISON', 1, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (49, 'Venomoth', 'POKE_TYPE_BUG', 'POKE_TYPE_POISON', 2, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (50, 'Diglett', 'POKE_TYPE_GROUND', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_BURROW_ASSASSIN'),
    (51, 'Dugtrio', 'POKE_TYPE_GROUND', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_BURROW_ASSASSIN'),
    (52, 'Meowth', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (53, 'Persian', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (54, 'Psyduck', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_WATER'),
    (55, 'Golduck', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_WATER'),
    (56, 'Mankey', 'POKE_TYPE_FIGHTING', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (57, 'Primeape', 'POKE_TYPE_FIGHTING', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (58, 'Growlithe', 'POKE_TYPE_FIRE', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_EMBER'),
    (59, 'Arcanine', 'POKE_TYPE_FIRE', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_EMBER'),
    (60, 'Poliwag', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_WATER'),
    (61, 'Poliwhirl', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_WATER'),
    (62, 'Poliwrath', 'POKE_TYPE_WATER', 'POKE_TYPE_FIGHTING', 3, 'ENEMY_ATTACK_RANGED_WATER'),
    (63, 'Abra', 'POKE_TYPE_PSYCHIC', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (64, 'Kadabra', 'POKE_TYPE_PSYCHIC', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (65, 'Alakazam', 'POKE_TYPE_PSYCHIC', 'POKE_TYPE_NONE', 3, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (66, 'Machop', 'POKE_TYPE_FIGHTING', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (67, 'Machoke', 'POKE_TYPE_FIGHTING', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (68, 'Machamp', 'POKE_TYPE_FIGHTING', 'POKE_TYPE_NONE', 3, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (69, 'Bellsprout', 'POKE_TYPE_GRASS', 'POKE_TYPE_POISON', 1, 'ENEMY_ATTACK_RANGED_SPORE'),
    (70, 'Weepinbell', 'POKE_TYPE_GRASS', 'POKE_TYPE_POISON', 2, 'ENEMY_ATTACK_RANGED_SPORE'),
    (71, 'Victreebel', 'POKE_TYPE_GRASS', 'POKE_TYPE_POISON', 3, 'ENEMY_ATTACK_RANGED_SPORE'),
    (72, 'Tentacool', 'POKE_TYPE_WATER', 'POKE_TYPE_POISON', 1, 'ENEMY_ATTACK_RANGED_WATER'),
    (73, 'Tentacruel', 'POKE_TYPE_WATER', 'POKE_TYPE_POISON', 2, 'ENEMY_ATTACK_RANGED_WATER'),
    (74, 'Geodude', 'POKE_TYPE_ROCK', 'POKE_TYPE_GROUND', 1, 'ENEMY_ATTACK_RANGED_ROCK'),
    (75, 'Graveler', 'POKE_TYPE_ROCK', 'POKE_TYPE_GROUND', 2, 'ENEMY_ATTACK_RANGED_ROCK'),
    (76, 'Golem', 'POKE_TYPE_ROCK', 'POKE_TYPE_GROUND', 3, 'ENEMY_ATTACK_SLAM_AOE'),
    (77, 'Ponyta', 'POKE_TYPE_FIRE', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_EMBER'),
    (78, 'Rapidash', 'POKE_TYPE_FIRE', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_EMBER'),
    (79, 'Slowpoke', 'POKE_TYPE_WATER', 'POKE_TYPE_PSYCHIC', 1, 'ENEMY_ATTACK_RANGED_WATER'),
    (80, 'Slowbro', 'POKE_TYPE_WATER', 'POKE_TYPE_PSYCHIC', 2, 'ENEMY_ATTACK_RANGED_WATER'),
    (81, 'Magnemite', 'POKE_TYPE_ELECTRIC', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_SPARK'),
    (82, 'Magneton', 'POKE_TYPE_ELECTRIC', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_SPARK'),
    (83, 'Farfetchd', 'POKE_TYPE_NORMAL', 'POKE_TYPE_FLYING', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (84, 'Doduo', 'POKE_TYPE_NORMAL', 'POKE_TYPE_FLYING', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (85, 'Dodrio', 'POKE_TYPE_NORMAL', 'POKE_TYPE_FLYING', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (86, 'Seel', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_WATER'),
    (87, 'Dewgong', 'POKE_TYPE_WATER', 'POKE_TYPE_ICE', 2, 'ENEMY_ATTACK_RANGED_WATER'),
    (88, 'Grimer', 'POKE_TYPE_POISON', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_SPORE'),
    (89, 'Muk', 'POKE_TYPE_POISON', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_SPORE'),
    (90, 'Shellder', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_WATER'),
    (91, 'Cloyster', 'POKE_TYPE_WATER', 'POKE_TYPE_ICE', 2, 'ENEMY_ATTACK_RANGED_WATER'),
    (92, 'Gastly', 'POKE_TYPE_GHOST', 'POKE_TYPE_POISON', 1, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (93, 'Haunter', 'POKE_TYPE_GHOST', 'POKE_TYPE_POISON', 2, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (94, 'Gengar', 'POKE_TYPE_GHOST', 'POKE_TYPE_POISON', 3, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (95, 'Onix', 'POKE_TYPE_ROCK', 'POKE_TYPE_GROUND', 2, 'ENEMY_ATTACK_SLAM_AOE'),
    (96, 'Drowzee', 'POKE_TYPE_PSYCHIC', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (97, 'Hypno', 'POKE_TYPE_PSYCHIC', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (98, 'Krabby', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_WATER'),
    (99, 'Kingler', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_WATER'),
    (100, 'Voltorb', 'POKE_TYPE_ELECTRIC', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_SPARK'),
    (101, 'Electrode', 'POKE_TYPE_ELECTRIC', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_SPARK'),
    (102, 'Exeggcute', 'POKE_TYPE_GRASS', 'POKE_TYPE_PSYCHIC', 1, 'ENEMY_ATTACK_RANGED_SPORE'),
    (103, 'Exeggutor', 'POKE_TYPE_GRASS', 'POKE_TYPE_PSYCHIC', 2, 'ENEMY_ATTACK_SLAM_AOE'),
    (104, 'Cubone', 'POKE_TYPE_GROUND', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_ROCK'),
    (105, 'Marowak', 'POKE_TYPE_GROUND', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_ROCK'),
    (106, 'Hitmonlee', 'POKE_TYPE_FIGHTING', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (107, 'Hitmonchan', 'POKE_TYPE_FIGHTING', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (108, 'Lickitung', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (109, 'Koffing', 'POKE_TYPE_POISON', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_SPORE'),
    (110, 'Weezing', 'POKE_TYPE_POISON', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_SPORE'),
    (111, 'Rhyhorn', 'POKE_TYPE_GROUND', 'POKE_TYPE_ROCK', 1, 'ENEMY_ATTACK_SLAM_AOE'),
    (112, 'Rhydon', 'POKE_TYPE_GROUND', 'POKE_TYPE_ROCK', 2, 'ENEMY_ATTACK_SLAM_AOE'),
    (113, 'Chansey', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (114, 'Tangela', 'POKE_TYPE_GRASS', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_SPORE'),
    (115, 'Kangaskhan', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (116, 'Horsea', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_WATER'),
    (117, 'Seadra', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_WATER'),
    (118, 'Goldeen', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_WATER'),
    (119, 'Seaking', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_WATER'),
    (120, 'Staryu', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_WATER'),
    (121, 'Starmie', 'POKE_TYPE_WATER', 'POKE_TYPE_PSYCHIC', 2, 'ENEMY_ATTACK_RANGED_WATER'),
    (122, 'MrMime', 'POKE_TYPE_PSYCHIC', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (123, 'Scyther', 'POKE_TYPE_BUG', 'POKE_TYPE_FLYING', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (124, 'Jynx', 'POKE_TYPE_ICE', 'POKE_TYPE_PSYCHIC', 2, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (125, 'Electabuzz', 'POKE_TYPE_ELECTRIC', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_SPARK'),
    (126, 'Magmar', 'POKE_TYPE_FIRE', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_EMBER'),
    (127, 'Pinsir', 'POKE_TYPE_BUG', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (128, 'Tauros', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (129, 'Magikarp', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (130, 'Gyarados', 'POKE_TYPE_WATER', 'POKE_TYPE_FLYING', 3, 'ENEMY_ATTACK_RANGED_WATER'),
    (131, 'Lapras', 'POKE_TYPE_WATER', 'POKE_TYPE_ICE', 3, 'ENEMY_ATTACK_RANGED_WATER'),
    (132, 'Ditto', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (133, 'Eevee', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_TOUCH_ONLY'),
    (134, 'Vaporeon', 'POKE_TYPE_WATER', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_WATER'),
    (135, 'Jolteon', 'POKE_TYPE_ELECTRIC', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_SPARK'),
    (136, 'Flareon', 'POKE_TYPE_FIRE', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_EMBER'),
    (137, 'Porygon', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_SPARK'),
    (138, 'Omanyte', 'POKE_TYPE_ROCK', 'POKE_TYPE_WATER', 1, 'ENEMY_ATTACK_RANGED_ROCK'),
    (139, 'Omastar', 'POKE_TYPE_ROCK', 'POKE_TYPE_WATER', 2, 'ENEMY_ATTACK_RANGED_ROCK'),
    (140, 'Kabuto', 'POKE_TYPE_ROCK', 'POKE_TYPE_WATER', 1, 'ENEMY_ATTACK_RANGED_ROCK'),
    (141, 'Kabutops', 'POKE_TYPE_ROCK', 'POKE_TYPE_WATER', 2, 'ENEMY_ATTACK_RANGED_ROCK'),
    (142, 'Aerodactyl', 'POKE_TYPE_ROCK', 'POKE_TYPE_FLYING', 3, 'ENEMY_ATTACK_RANGED_GUST'),
    (143, 'Snorlax', 'POKE_TYPE_NORMAL', 'POKE_TYPE_NONE', 3, 'ENEMY_ATTACK_SLAM_AOE'),
    (144, 'Articuno', 'POKE_TYPE_ICE', 'POKE_TYPE_FLYING', 3, 'ENEMY_ATTACK_RANGED_GUST'),
    (145, 'Zapdos', 'POKE_TYPE_ELECTRIC', 'POKE_TYPE_FLYING', 3, 'ENEMY_ATTACK_RANGED_SPARK'),
    (146, 'Moltres', 'POKE_TYPE_FIRE', 'POKE_TYPE_FLYING', 3, 'ENEMY_ATTACK_RANGED_EMBER'),
    (147, 'Dratini', 'POKE_TYPE_DRAGON', 'POKE_TYPE_NONE', 1, 'ENEMY_ATTACK_RANGED_WATER'),
    (148, 'Dragonair', 'POKE_TYPE_DRAGON', 'POKE_TYPE_NONE', 2, 'ENEMY_ATTACK_RANGED_WATER'),
    (149, 'Dragonite', 'POKE_TYPE_DRAGON', 'POKE_TYPE_FLYING', 3, 'ENEMY_ATTACK_RANGED_EMBER'),
    (150, 'Mewtwo', 'POKE_TYPE_PSYCHIC', 'POKE_TYPE_NONE', 3, 'ENEMY_ATTACK_RANGED_PSYCHIC'),
    (151, 'Mew', 'POKE_TYPE_PSYCHIC', 'POKE_TYPE_NONE', 3, 'ENEMY_ATTACK_RANGED_PSYCHIC')
]

lines = []
lines.append("/* Auto-generated Pokedex Data Header for all 151 Gen 1 Pokemon */")
lines.append("#ifndef POKEDEX_DATA_H")
lines.append("#define POKEDEX_DATA_H")
lines.append("")
lines.append('#include "types.h"')
lines.append("")
lines.append("static const PokedexEntry g_pokedex[152] = {")
lines.append("    { 0 }, // 0 is unused")

for item in pokemon_defs:
    pid, name, t1, t2, stage, atk = item
    folder = f"/Users/drewby/Downloads/testsprites/assets/sprites/sprite/{pid:04d}"
    xml_path = os.path.join(folder, "AnimData.xml")
    
    fw, fh, fc = 32, 32, 4
    try:
        tree = ET.parse(xml_path)
        root = tree.getroot()
        for a in root.findall('.//Anim'):
            n = a.find('Name')
            if n is not None and n.text == 'Walk':
                fw = int(a.find('FrameWidth').text)
                fh = int(a.find('FrameHeight').text)
                dur = a.find('Durations')
                if dur is not None:
                    fc = len(dur.findall('Duration'))
                break
    except Exception as e:
        print(f"Error parsing {xml_path}: {e}")

    # Scale according to stage and special sizes
    if pid in (50, 51): # Diglett / Dugtrio
        scale = 1.6 if pid == 50 else 2.0
    elif pid in (95, 130, 143): # Onix, Gyarados, Snorlax
        scale = 3.4
    elif stage == 1:
        scale = 1.6
    elif stage == 2:
        scale = 2.2
    else:
        scale = 3.0

    # Base attributes according to stage
    if stage == 1:
        hp = 65.0
        spd = 190.0
        tdmg = 10.0
        rad = 14.0
        xp = 1
        kbr = 0.15
    elif stage == 2:
        hp = 160.0
        spd = 160.0
        tdmg = 18.0
        rad = 20.0
        xp = 4
        kbr = 0.40
    else:
        hp = 380.0
        spd = 125.0
        tdmg = 28.0
        rad = 30.0
        xp = 14
        kbr = 0.70

    # Attack configuration
    if atk == 'ENEMY_ATTACK_BURROW_ASSASSIN':
        range_px = 75.0
        cd = 2.8
        windup = 0.40
        dmg = 28.0
        atk_rad = 55.0
    elif atk == 'ENEMY_ATTACK_SLAM_AOE':
        range_px = 100.0
        cd = 3.8
        windup = 0.45
        dmg = 32.0 if stage < 3 else 45.0
        atk_rad = 110.0 if stage < 3 else 140.0
    elif atk == 'ENEMY_ATTACK_RANGED_EMBER':
        range_px = 300.0
        cd = 2.4 if stage > 1 else 3.2
        windup = 0.30
        dmg = 16.0 if stage == 1 else (24.0 if stage == 2 else 36.0)
        atk_rad = 20.0
    elif atk == 'ENEMY_ATTACK_RANGED_WATER':
        range_px = 280.0
        cd = 2.5 if stage > 1 else 3.4
        windup = 0.32
        dmg = 15.0 if stage == 1 else (22.0 if stage == 2 else 34.0)
        atk_rad = 22.0
    elif atk == 'ENEMY_ATTACK_RANGED_SPARK':
        range_px = 320.0
        cd = 2.6 if stage > 1 else 3.5
        windup = 0.30
        dmg = 14.0 if stage == 1 else (20.0 if stage == 2 else 30.0)
        atk_rad = 25.0
    elif atk == 'ENEMY_ATTACK_RANGED_SPORE':
        range_px = 260.0
        cd = 3.0 if stage > 1 else 4.0
        windup = 0.40
        dmg = 12.0 if stage == 1 else (18.0 if stage == 2 else 26.0)
        atk_rad = 50.0
    elif atk == 'ENEMY_ATTACK_RANGED_ROCK':
        range_px = 320.0
        cd = 3.2 if stage > 1 else 4.2
        windup = 0.45
        dmg = 22.0 if stage == 1 else (32.0 if stage == 2 else 45.0)
        atk_rad = 30.0
    elif atk == 'ENEMY_ATTACK_RANGED_GUST':
        range_px = 300.0
        cd = 2.5 if stage > 1 else 3.2
        windup = 0.28
        dmg = 16.0 if stage == 1 else (22.0 if stage == 2 else 32.0)
        atk_rad = 24.0
    elif atk == 'ENEMY_ATTACK_RANGED_PSYCHIC':
        range_px = 340.0
        cd = 2.8 if stage > 1 else 3.6
        windup = 0.35
        dmg = 18.0 if stage == 1 else (25.0 if stage == 2 else 40.0)
        atk_rad = 28.0
    else: # MELEE / TOUCH
        range_px = 50.0
        cd = 1.8
        windup = 0.20
        dmg = 12.0 if stage == 1 else (20.0 if stage == 2 else 30.0)
        atk_rad = 20.0

    entry = f'    [{pid}] = {{ {pid}, "{name}", {t1}, {t2}, {stage}, {scale:.2f}f, {hp:.1f}f, {spd:.1f}f, {tdmg:.1f}f, {rad:.1f}f, {xp}, {kbr:.2f}f, {fw}, {fh}, {fc}, {atk}, {range_px:.1f}f, {cd:.2f}f, {windup:.2f}f, {dmg:.1f}f, {atk_rad:.1f}f }},'
    lines.append(entry)

lines.append("};")
lines.append("")
lines.append("#endif // POKEDEX_DATA_H")

out_path = "/Users/drewby/Downloads/testsprites/src/pokedex_data.h"
with open(out_path, "w") as f:
    f.write("\n".join(lines) + "\n")

print(f"Generated {out_path} with {len(pokemon_defs)} entries successfully!")
