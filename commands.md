# CS 1.6 - RBOT [![Version]( https://img.shields.io/badge/version-v1.0-blue )]( https://github.com/KennySusak/rbot/releases )

## **Command Settings**

|       Command        |  Default Value  |                                      Range                                      |  Support  |
|:--------------------:|:--------------:|:--------------------------------------------------------------------------------:|:---------:|
|  rbot_debug         |      0          |  0 / 1                                                                           |   v1.0    |
|  rbot_debuggoal     |     -1          |  N / A                                                                           |   v1.0    |
|  rbot_gamemod       |      0          |  0 : classic , 1 : deathmatch , 2 : Zombie Plague , 3 : deathmatch , 4 : Zombie Hell , 5 : Team DeathMatch  |   v1.0    |
|  rbot_follow_user_max  |   1          |  Integer                                                                         |   v1.0    |
|  rbot_onlyknifemode   |   0          |  0 / 1                                                                           |   v1.0    |
|  rbot_walk_allow     |      1          |  0 / 1                                                                           |   v1.0    |
|  rbot_stop_bots      |      0          |  0 / 1                                                                           |   v1.0    |
|  rbot_spray_paints   |      1          |  0 / 1                                                                           |   v1.0    |
|  rbot_restrict_weapons |   ""         |  String ( e.g., `"weapon_ak47"` )                                                 |   v1.0    |
|  rbot_camp_time_min  |     15          |  Integer                                                                         |   v1.0    |
|  rbot_camp_time_max  |     30          |  Integer                                                                         |   v1.0    |
|  rbot_use_radio      |      0          |  0 / 1                                                                           |   v1.0    |
|  rbot_force_flashlight |   0          |  0 / 1                                                                           |   v1.0    |
|  rbot_zm_use_flares  |      1          |  0 / 1                                                                           |   v1.0    |
|  rbot_eco_rounds     |      1          |  0 / 1                                                                           |   v1.0    |
|  rbot_avoid_grenades |      1          |  0 / 1                                                                           |   v1.0    |
|  rbot_breakable_health_limit |  3000.0  |  Float                                                                          |   v1.0    |
|  rbot_chatter_path   |  radio/bot      |  String                                                                          |   v1.0    |
|  rbot_zombie_escape_mode |   0         |  0 / 1                                                                           |   v1.0    |
|  rbot_zm_use_grenade_percent |  25     |  0 – 100 ( percentage )                                                          |   v1.0    |
|  rbot_zm_escape_distance |  400        |  Integer                                                                         |   v1.0    |
|  rbot_zombie_speed_factor |  0.54      |  Float                                                                          |   v1.0    |
|  rbot_sb_mode        |      0          |  0 / 1                                                                           |   v1.0    |
|  rbot_zombie_knife_mode |   1          |  0 : Off , 1 : Normal ( Tactical ) , 2 : Only Left Click , 3 : Only Right Click |   v1.0    |

## **Waypoint Commands**

|          Command          |                    Description                     |
|:-------------------------:|:--------------------------------------------------:|
|  `rbot autowp`           |  Toggle autowaypointing                            |
|  `rbot wp`               |  Toggle waypoint showing                           |
|  `rbot wp on noclip`     |  Enable noclip cheat                               |
|  `rbot wp save nocheck`  |  Save waypoints without checking                   |
|  `rbot wp add`           |  Open menu for waypoint creation                   |
|  `rbot wp menu`          |  Open main waypoint menu                           |
|  `rbot wp addbasic`      |  Create basic waypoints on map                     |
|  `rbot wp find`          |  Show direction to specified waypoint              |
|  `rbot wp load`          |  Load the waypoint file from hard disk             |
|  `rbot wp check`         |  Check if all waypoint connections are valid       |
|  `rbot wp cache`         |  Cache nearest waypoint                            |
|  `rbot wp teleport`      |  Teleport hostile to specified waypoint            |
|  `rbot wp setradius`     |  Manually set the wayzone radius for this waypoint |
|  `rbot path autodistance`|  Open menu for setting autopath max distance       |
|  `rbot path cache`       |  Remember the nearest-to-player waypoint          |
|  `rbot path create`      |  Open menu for path creation                       |
|  `rbot path delete`      |  Delete path from cached to nearest waypoint      |
|  `rbot path create_in`   |  Create incoming path connection                   |
|  `rbot path create_out`  |  Create outgoing path connection                   |
|  `rbot path create_both` |  Create both-ways path connection                  |

## **Waypoint Settings**

|        Command        |           Description            |   Range    |
|:--------------------:|:--------------------------------:|:----------:|
|  `rbot wp setmesh`   |  Set mesh for waypoint           |  Any number |
|  `rbot wp setgravity`|  Set gravity for waypoint        |  200 - 800  |
|  `rbot wp erase`     |  Remove waypoint from disk       |  N / A      |
|  `rbot wp save`      |  Save waypoint data              |  N / A      |
|  `rbot wp check`     |  Check all nodes for validation  |  N / A      |
|  `rbot wp find`      |  Show direction to waypoint      |  N / A      |
|  `rbot wp delete`    |  Delete nearest waypoint         |  N / A      |
