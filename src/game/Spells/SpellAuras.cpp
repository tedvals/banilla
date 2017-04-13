/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos/zero>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "Group.h"
#include "UpdateData.h"
#include "ObjectAccessor.h"
#include "Policies/SingletonImp.h"
#include "Totem.h"
#include "Creature.h"
#include "Formulas.h"
#include "BattleGround.h"
#include "CreatureAI.h"
#include "ScriptMgr.h"
#include "Util.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "MapManager.h"
#include "MoveSpline.h"

#include "ZoneScript.h"
#include "PlayerAI.h"
#include "Anticheat.h"

#define NULL_AURA_SLOT 0xFF

pAuraHandler AuraHandler[TOTAL_AURAS] =
{
    &Aura::HandleNULL,                                      //  0 SPELL_AURA_NONE
    &Aura::HandleBindSight,                                 //  1 SPELL_AURA_BIND_SIGHT
    &Aura::HandleModPossess,                                //  2 SPELL_AURA_MOD_POSSESS
    &Aura::HandlePeriodicDamage,                            //  3 SPELL_AURA_PERIODIC_DAMAGE
    &Aura::HandleAuraDummy,                                 //  4 SPELL_AURA_DUMMY
    &Aura::HandleModConfuse,                                //  5 SPELL_AURA_MOD_CONFUSE
    &Aura::HandleModCharm,                                  //  6 SPELL_AURA_MOD_CHARM
    &Aura::HandleModFear,                                   //  7 SPELL_AURA_MOD_FEAR
    &Aura::HandlePeriodicHeal,                              //  8 SPELL_AURA_PERIODIC_HEAL
    &Aura::HandleModAttackSpeed,                            //  9 SPELL_AURA_MOD_ATTACKSPEED
    &Aura::HandleModThreat,                                 // 10 SPELL_AURA_MOD_THREAT
    &Aura::HandleModTaunt,                                  // 11 SPELL_AURA_MOD_TAUNT
    &Aura::HandleAuraModStun,                               // 12 SPELL_AURA_MOD_STUN
    &Aura::HandleModDamageDone,                             // 13 SPELL_AURA_MOD_DAMAGE_DONE
    &Aura::HandleNoImmediateEffect,                         // 14 SPELL_AURA_MOD_DAMAGE_TAKEN   implemented in Unit::MeleeDamageBonusTaken and Unit::SpellBaseDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         // 15 SPELL_AURA_DAMAGE_SHIELD      implemented in Unit::DealMeleeDamage
    &Aura::HandleModStealth,                                // 16 SPELL_AURA_MOD_STEALTH
    &Aura::HandleNoImmediateEffect,                         // 17 SPELL_AURA_MOD_STEALTH_DETECT
    &Aura::HandleInvisibility,                              // 18 SPELL_AURA_MOD_INVISIBILITY
    &Aura::HandleInvisibilityDetect,                        // 19 SPELL_AURA_MOD_INVISIBILITY_DETECTION
    &Aura::HandleAuraModTotalHealthPercentRegen,            // 20 SPELL_AURA_OBS_MOD_HEALTH
    &Aura::HandleAuraModTotalManaPercentRegen,              // 21 SPELL_AURA_OBS_MOD_MANA
    &Aura::HandleAuraModResistance,                         // 22 SPELL_AURA_MOD_RESISTANCE
    &Aura::HandlePeriodicTriggerSpell,                      // 23 SPELL_AURA_PERIODIC_TRIGGER_SPELL
    &Aura::HandlePeriodicEnergize,                          // 24 SPELL_AURA_PERIODIC_ENERGIZE
    &Aura::HandleAuraModPacify,                             // 25 SPELL_AURA_MOD_PACIFY
    &Aura::HandleAuraModRoot,                               // 26 SPELL_AURA_MOD_ROOT
    &Aura::HandleAuraModSilence,                            // 27 SPELL_AURA_MOD_SILENCE
    &Aura::HandleNoImmediateEffect,                         // 28 SPELL_AURA_REFLECT_SPELLS        implement in Unit::SpellHitResult
    &Aura::HandleAuraModStat,                               // 29 SPELL_AURA_MOD_STAT
    &Aura::HandleAuraModSkill,                              // 30 SPELL_AURA_MOD_SKILL
    &Aura::HandleAuraModIncreaseSpeed,                      // 31 SPELL_AURA_MOD_INCREASE_SPEED
    &Aura::HandleAuraModIncreaseMountedSpeed,               // 32 SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED
    &Aura::HandleAuraModDecreaseSpeed,                      // 33 SPELL_AURA_MOD_DECREASE_SPEED
    &Aura::HandleAuraModIncreaseHealth,                     // 34 SPELL_AURA_MOD_INCREASE_HEALTH
    &Aura::HandleAuraModIncreaseEnergy,                     // 35 SPELL_AURA_MOD_INCREASE_ENERGY
    &Aura::HandleAuraModShapeshift,                         // 36 SPELL_AURA_MOD_SHAPESHIFT
    &Aura::HandleAuraModEffectImmunity,                     // 37 SPELL_AURA_EFFECT_IMMUNITY
    &Aura::HandleAuraModStateImmunity,                      // 38 SPELL_AURA_STATE_IMMUNITY
    &Aura::HandleAuraModSchoolImmunity,                     // 39 SPELL_AURA_SCHOOL_IMMUNITY
    &Aura::HandleAuraModDmgImmunity,                        // 40 SPELL_AURA_DAMAGE_IMMUNITY
    &Aura::HandleAuraModDispelImmunity,                     // 41 SPELL_AURA_DISPEL_IMMUNITY
    &Aura::HandleAuraProcTriggerSpell,                      // 42 SPELL_AURA_PROC_TRIGGER_SPELL  implemented in Unit::ProcDamageAndSpellFor and Unit::HandleProcTriggerSpell
    &Aura::HandleNoImmediateEffect,                         // 43 SPELL_AURA_PROC_TRIGGER_DAMAGE implemented in Unit::ProcDamageAndSpellFor
    &Aura::HandleAuraTrackCreatures,                        // 44 SPELL_AURA_TRACK_CREATURES
    &Aura::HandleAuraTrackResources,                        // 45 SPELL_AURA_TRACK_RESOURCES
    &Aura::HandleUnused,                                    // 46 SPELL_AURA_46
    &Aura::HandleAuraModParryPercent,                       // 47 SPELL_AURA_MOD_PARRY_PERCENT
    &Aura::HandleUnused,                                    // 48 SPELL_AURA_48
    &Aura::HandleAuraModDodgePercent,                       // 49 SPELL_AURA_MOD_DODGE_PERCENT
    &Aura::HandleUnused,                                    // 50 SPELL_AURA_MOD_BLOCK_SKILL    obsolete?
    &Aura::HandleAuraModBlockPercent,                       // 51 SPELL_AURA_MOD_BLOCK_PERCENT
    &Aura::HandleAuraModCritPercent,                        // 52 SPELL_AURA_MOD_CRIT_PERCENT
    &Aura::HandlePeriodicLeech,                             // 53 SPELL_AURA_PERIODIC_LEECH
    &Aura::HandleModHitChance,                              // 54 SPELL_AURA_MOD_HIT_CHANCE
    &Aura::HandleModSpellHitChance,                         // 55 SPELL_AURA_MOD_SPELL_HIT_CHANCE
    &Aura::HandleAuraTransform,                             // 56 SPELL_AURA_TRANSFORM
    &Aura::HandleModSpellCritChance,                        // 57 SPELL_AURA_MOD_SPELL_CRIT_CHANCE
    &Aura::HandleAuraModIncreaseSwimSpeed,                  // 58 SPELL_AURA_MOD_INCREASE_SWIM_SPEED
    &Aura::HandleNoImmediateEffect,                         // 59 SPELL_AURA_MOD_DAMAGE_DONE_CREATURE implemented in Unit::MeleeDamageBonusDone and Unit::SpellDamageBonusDone
    &Aura::HandleAuraModPacifyAndSilence,                   // 60 SPELL_AURA_MOD_PACIFY_SILENCE
    &Aura::HandleAuraModScale,                              // 61 SPELL_AURA_MOD_SCALE
    &Aura::HandlePeriodicHealthFunnel,                      // 62 SPELL_AURA_PERIODIC_HEALTH_FUNNEL
    &Aura::HandleUnused,                                    // 63 SPELL_AURA_PERIODIC_MANA_FUNNEL obsolete?
    &Aura::HandlePeriodicManaLeech,                         // 64 SPELL_AURA_PERIODIC_MANA_LEECH
    &Aura::HandleModCastingSpeed,                           // 65 SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK
    &Aura::HandleFeignDeath,                                // 66 SPELL_AURA_FEIGN_DEATH
    &Aura::HandleAuraModDisarm,                             // 67 SPELL_AURA_MOD_DISARM
    &Aura::HandleAuraModStalked,                            // 68 SPELL_AURA_MOD_STALKED
    &Aura::HandleSchoolAbsorb,                              // 69 SPELL_AURA_SCHOOL_ABSORB implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleUnused,                                    // 70 SPELL_AURA_EXTRA_ATTACKS      Useless, used by only one spell that has only visual effect
    &Aura::HandleModSpellCritChanceShool,                   // 71 SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL
    &Aura::HandleModPowerCostPCT,                           // 72 SPELL_AURA_MOD_POWER_COST_SCHOOL_PCT
    &Aura::HandleModPowerCost,                              // 73 SPELL_AURA_MOD_POWER_COST_SCHOOL
    &Aura::HandleNoImmediateEffect,                         // 74 SPELL_AURA_REFLECT_SPELLS_SCHOOL  implemented in Unit::SpellHitResult
    &Aura::HandleNoImmediateEffect,                         // 75 SPELL_AURA_MOD_LANGUAGE
    &Aura::HandleFarSight,                                  // 76 SPELL_AURA_FAR_SIGHT
    &Aura::HandleModMechanicImmunity,                       // 77 SPELL_AURA_MECHANIC_IMMUNITY
    &Aura::HandleAuraMounted,                               // 78 SPELL_AURA_MOUNTED
    &Aura::HandleModDamagePercentDone,                      // 79 SPELL_AURA_MOD_DAMAGE_PERCENT_DONE
    &Aura::HandleModPercentStat,                            // 80 SPELL_AURA_MOD_PERCENT_STAT
    &Aura::HandleNoImmediateEffect,                         // 81 SPELL_AURA_SPLIT_DAMAGE_PCT       implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleWaterBreathing,                            // 82 SPELL_AURA_WATER_BREATHING
    &Aura::HandleModBaseResistance,                         // 83 SPELL_AURA_MOD_BASE_RESISTANCE
    &Aura::HandleModRegen,                                  // 84 SPELL_AURA_MOD_REGEN
    &Aura::HandleModPowerRegen,                             // 85 SPELL_AURA_MOD_POWER_REGEN
    &Aura::HandleChannelDeathItem,                          // 86 SPELL_AURA_CHANNEL_DEATH_ITEM
    &Aura::HandleNoImmediateEffect,                         // 87 SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN implemented in Unit::MeleeDamageBonusTaken and Unit::SpellDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         // 88 SPELL_AURA_MOD_HEALTH_REGEN_PERCENT implemented in Player::RegenerateHealth
    &Aura::HandlePeriodicDamagePCT,                         // 89 SPELL_AURA_PERIODIC_DAMAGE_PERCENT
    &Aura::HandleUnused,                                    // 90 SPELL_AURA_MOD_RESIST_CHANCE  Useless
    &Aura::HandleNoImmediateEffect,                         // 91 SPELL_AURA_MOD_DETECT_RANGE implemented in Creature::GetAttackDistance
    &Aura::HandlePreventFleeing,                            // 92 SPELL_AURA_PREVENTS_FLEEING
    &Aura::HandleModUnattackable,                           // 93 SPELL_AURA_MOD_UNATTACKABLE
    &Aura::HandleInterruptRegen,                            // 94 SPELL_AURA_INTERRUPT_REGEN implemented in Player::RegenerateAll
    &Aura::HandleAuraGhost,                                 // 95 SPELL_AURA_GHOST
    &Aura::HandleNoImmediateEffect,                         // 96 SPELL_AURA_SPELL_MAGNET implemented in Unit::SelectMagnetTarget
    &Aura::HandleManaShield,                                // 97 SPELL_AURA_MANA_SHIELD implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleAuraModSkill,                              // 98 SPELL_AURA_MOD_SKILL_TALENT
    &Aura::HandleAuraModAttackPower,                        // 99 SPELL_AURA_MOD_ATTACK_POWER
    &Aura::HandleAurasVisible,                              //100 SPELL_AURA_AURAS_VISIBLE
    &Aura::HandleModResistancePercent,                      //101 SPELL_AURA_MOD_RESISTANCE_PCT
    &Aura::HandleNoImmediateEffect,                         //102 SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleAuraModTotalThreat,                        //103 SPELL_AURA_MOD_TOTAL_THREAT
    &Aura::HandleAuraWaterWalk,                             //104 SPELL_AURA_WATER_WALK
    &Aura::HandleAuraFeatherFall,                           //105 SPELL_AURA_FEATHER_FALL
    &Aura::HandleAuraHover,                                 //106 SPELL_AURA_HOVER
    &Aura::HandleAddModifier,                               //107 SPELL_AURA_ADD_FLAT_MODIFIER
    &Aura::HandleAddModifier,                               //108 SPELL_AURA_ADD_PCT_MODIFIER
    &Aura::HandleNoImmediateEffect,                         //109 SPELL_AURA_ADD_TARGET_TRIGGER
    &Aura::HandleModPowerRegenPCT,                          //110 SPELL_AURA_MOD_POWER_REGEN_PERCENT
    &Aura::HandleUnused,                                    //111 SPELL_AURA_ADD_CASTER_HIT_TRIGGER
    &Aura::HandleNoImmediateEffect,                         //112 SPELL_AURA_OVERRIDE_CLASS_SCRIPTS implemented in diff functions.
    &Aura::HandleNoImmediateEffect,                         //113 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //114 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //115 SPELL_AURA_MOD_HEALING                 implemented in Unit::SpellBaseHealingBonusTaken
    &Aura::HandleNoImmediateEffect,                         //116 SPELL_AURA_MOD_REGEN_DURING_COMBAT     imppemented in Player::RegenerateAll and Player::RegenerateHealth
    &Aura::HandleNoImmediateEffect,                         //117 SPELL_AURA_MOD_MECHANIC_RESISTANCE     implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //118 SPELL_AURA_MOD_HEALING_PCT             implemented in Unit::SpellHealingBonusTaken
    &Aura::HandleUnused,                                    //119 SPELL_AURA_SHARE_PET_TRACKING useless
    &Aura::HandleAuraUntrackable,                           //120 SPELL_AURA_UNTRACKABLE
    &Aura::HandleAuraEmpathy,                               //121 SPELL_AURA_EMPATHY
    &Aura::HandleModOffhandDamagePercent,                   //122 SPELL_AURA_MOD_OFFHAND_DAMAGE_PCT
    &Aura::HandleNoImmediateEffect,                         //123 SPELL_AURA_MOD_TARGET_RESISTANCE  implemented in Unit::CalculateAbsorbAndResist and Unit::CalcArmorReducedDamage
    &Aura::HandleAuraModRangedAttackPower,                  //124 SPELL_AURA_MOD_RANGED_ATTACK_POWER
    &Aura::HandleNoImmediateEffect,                         //125 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //126 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonusTaken
    &Aura::HandleNoImmediateEffect,                         //127 SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleModPossessPet,                             //128 SPELL_AURA_MOD_POSSESS_PET
    &Aura::HandleAuraModIncreaseSpeed,                      //129 SPELL_AURA_MOD_SPEED_ALWAYS
    &Aura::HandleAuraModIncreaseMountedSpeed,               //130 SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS
    &Aura::HandleNoImmediateEffect,                         //131 SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleAuraModIncreaseEnergyPercent,              //132 SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT
    &Aura::HandleAuraModIncreaseHealthPercent,              //133 SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT
    &Aura::HandleAuraModRegenInterrupt,                     //134 SPELL_AURA_MOD_MANA_REGEN_INTERRUPT
    &Aura::HandleModHealingDone,                            //135 SPELL_AURA_MOD_HEALING_DONE
    &Aura::HandleNoImmediateEffect,                         //136 SPELL_AURA_MOD_HEALING_DONE_PERCENT   implemented in Unit::SpellHealingBonusDone
    &Aura::HandleModTotalPercentStat,                       //137 SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE
    &Aura::HandleModMeleeSpeedPct,                          //138 SPELL_AURA_MOD_MELEE_HASTE
    &Aura::HandleForceReaction,                             //139 SPELL_AURA_FORCE_REACTION
    &Aura::HandleAuraModRangedHaste,                        //140 SPELL_AURA_MOD_RANGED_HASTE
    &Aura::HandleRangedAmmoHaste,                           //141 SPELL_AURA_MOD_RANGED_AMMO_HASTE
    &Aura::HandleAuraModBaseResistancePCT,                  //142 SPELL_AURA_MOD_BASE_RESISTANCE_PCT
    &Aura::HandleAuraModResistanceExclusive,                //143 SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE
    &Aura::HandleAuraSafeFall,                              //144 SPELL_AURA_SAFE_FALL                         implemented in WorldSession::HandleMovementOpcodes
    &Aura::HandleUnused,                                    //145 SPELL_AURA_CHARISMA obsolete?
    &Aura::HandleUnused,                                    //146 SPELL_AURA_PERSUADED obsolete?
    &Aura::HandleModMechanicImmunityMask,                   //147 SPELL_AURA_MECHANIC_IMMUNITY_MASK            implemented in Unit::IsImmuneToSpell and Unit::IsImmuneToSpellEffect (check part)
    &Aura::HandleAuraRetainComboPoints,                     //148 SPELL_AURA_RETAIN_COMBO_POINTS
    &Aura::HandleNoImmediateEffect,                         //149 SPELL_AURA_RESIST_PUSHBACK
    &Aura::HandleShieldBlockValue,                          //150 SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT
    &Aura::HandleAuraTrackStealthed,                        //151 SPELL_AURA_TRACK_STEALTHED
    &Aura::HandleNoImmediateEffect,                         //152 SPELL_AURA_MOD_DETECTED_RANGE         implemented in Creature::GetAttackDistance
    &Aura::HandleNoImmediateEffect,                         //153 SPELL_AURA_SPLIT_DAMAGE_FLAT          implemented in Unit::CalculateAbsorbAndResist
    &Aura::HandleNoImmediateEffect,                         //154 SPELL_AURA_MOD_STEALTH_LEVEL          implemented in Unit::isVisibleForOrDetect
    &Aura::HandleNoImmediateEffect,                         //155 SPELL_AURA_MOD_WATER_BREATHING        implemented in Player::getMaxTimer
    &Aura::HandleNoImmediateEffect,                         //156 SPELL_AURA_MOD_REPUTATION_GAIN        implemented in Player::CalculateReputationGain
    &Aura::HandleUnused,                                    //157 SPELL_AURA_PET_DAMAGE_MULTI (single test like spell 20782, also single for 214 aura)
    &Aura::HandleShieldBlockValue,                          //158 SPELL_AURA_MOD_SHIELD_BLOCKVALUE
    &Aura::HandleNoImmediateEffect,                         //159 SPELL_AURA_NO_PVP_CREDIT      only for Honorless Target spell
    &Aura::HandleNoImmediateEffect,                         //160 SPELL_AURA_MOD_AOE_AVOIDANCE                 implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //161 SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT
    &Aura::HandleAuraPowerBurn,                             //162 SPELL_AURA_POWER_BURN_MANA
    &Aura::HandleUnused,                                    //163 SPELL_AURA_MOD_CRIT_DAMAGE_BONUS
    &Aura::HandleUnused,                                    //164 useless, only one test spell
    &Aura::HandleNoImmediateEffect,                         //165 SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonusDone
    &Aura::HandleAuraModAttackPowerPercent,                 //166 SPELL_AURA_MOD_ATTACK_POWER_PCT
    &Aura::HandleAuraModRangedAttackPowerPercent,           //167 SPELL_AURA_MOD_RANGED_ATTACK_POWER_PCT
    &Aura::HandleNoImmediateEffect,                         //168 SPELL_AURA_MOD_DAMAGE_DONE_VERSUS            implemented in Unit::SpellDamageBonusDone, Unit::MeleeDamageBonusDone
    &Aura::HandleNoImmediateEffect,                         //169 SPELL_AURA_MOD_CRIT_PERCENT_VERSUS           implemented in Unit::DealDamageBySchool, Unit::DoAttackDamage, Unit::SpellCriticalBonus
    &Aura::HandleNULL,                                      //170 SPELL_AURA_DETECT_AMORE       only for Detect Amore spell
    &Aura::HandleAuraModIncreaseSpeed,                      //171 SPELL_AURA_MOD_SPEED_NOT_STACK
    &Aura::HandleAuraModIncreaseMountedSpeed,               //172 SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK
    &Aura::HandleUnused,                                    //173 SPELL_AURA_ALLOW_CHAMPION_SPELLS  only for Proclaim Champion spell
    &Aura::HandleModSpellDamagePercentFromStat,             //174 SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT  implemented in Unit::SpellBaseDamageBonusDone (in 1.12.* only spirit)
    &Aura::HandleModSpellHealingPercentFromStat,            //175 SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT implemented in Unit::SpellBaseHealingBonusDone (in 1.12.* only spirit)
    &Aura::HandleSpiritOfRedemption,                        //176 SPELL_AURA_SPIRIT_OF_REDEMPTION   only for Spirit of Redemption spell, die at aura end
    &Aura::HandleNULL,                                      //177 SPELL_AURA_AOE_CHARM
    &Aura::HandleNoImmediateEffect,                         //178 SPELL_AURA_MOD_DEBUFF_RESISTANCE          implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //179 SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE implemented in Unit::SpellCriticalBonus
    &Aura::HandleNoImmediateEffect,                         //180 SPELL_AURA_MOD_FLAT_SPELL_DAMAGE_VERSUS   implemented in Unit::SpellDamageBonusDone
    &Aura::HandleUnused,                                    //181 SPELL_AURA_MOD_FLAT_SPELL_CRIT_DAMAGE_VERSUS unused
    &Aura::HandleAuraModResistenceOfStatPercent,            //182 SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT
    &Aura::HandleNoImmediateEffect,                         //183 SPELL_AURA_MOD_CRITICAL_THREAT only used in 28746, implemented in ThreatCalcHelper::CalcThreat
    &Aura::HandleNoImmediateEffect,                         //184 SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE  implemented in Unit::RollMeleeOutcomeAgainst
    &Aura::HandleNoImmediateEffect,                         //185 SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE implemented in Unit::RollMeleeOutcomeAgainst
    &Aura::HandleNoImmediateEffect,                         //186 SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE  implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //187 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_CHANCE  implemented in Unit::GetUnitCriticalChance
    &Aura::HandleNoImmediateEffect,                         //188 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_CHANCE implemented in Unit::GetUnitCriticalChance
    &Aura::HandleUnused,                                    //189 SPELL_AURA_MOD_RATING (not used in 1.12.1)
    &Aura::HandleNoImmediateEffect,                         //190 SPELL_AURA_MOD_FACTION_REPUTATION_GAIN     implemented in Player::CalculateReputationGain
    &Aura::HandleAuraModUseNormalSpeed,                     //191 SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED
		&Aura::HandleModMeleeRangedSpeedPct,                    //192  SPELL_AURA_MOD_MELEE_RANGED_HASTE
		&Aura::HandleModCombatSpeedPct,                         //193 SPELL_AURA_HASTE_ALL (in fact combat (any type attack) speed pct)
		&Aura::HandleUnused,                                    //194 SPELL_AURA_MOD_DEPRICATED_1 not used now (old SPELL_AURA_MOD_SPELL_DAMAGE_OF_INTELLECT)
		&Aura::HandleUnused,                                    //195 SPELL_AURA_MOD_DEPRICATED_2 not used now (old SPELL_AURA_MOD_SPELL_HEALING_OF_INTELLECT)
		&Aura::HandleNULL,                                      //196 SPELL_AURA_MOD_COOLDOWN
		&Aura::HandleNoImmediateEffect,                         //197 SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE implemented in Unit::SpellCriticalBonus Unit::CalculateEffectiveCritChance
		&Aura::HandleUnused,                                    //198 SPELL_AURA_MOD_ALL_WEAPON_SKILLS
		&Aura::HandleNoImmediateEffect,                         //199 SPELL_AURA_MOD_INCREASES_SPELL_PCT_TO_HIT  implemented in Unit::MagicSpellHitResult
		&Aura::HandleNoImmediateEffect,                         //200 SPELL_AURA_MOD_XP_PCT                      implemented in Player::GiveXP
		&Aura::HandleNULL,                           //201 SPELL_AURA_FLY                             this aura enable flight mode...
		&Aura::HandleNoImmediateEffect,                         //202 SPELL_AURA_IGNORE_COMBAT_RESULT            implemented in Unit::MeleeSpellHitResult
		&Aura::HandleNoImmediateEffect,                         //203 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE  implemented in Unit::CalculateMeleeDamage and Unit::SpellCriticalDamageBonus
		&Aura::HandleNoImmediateEffect,                         //204 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE implemented in Unit::CalculateMeleeDamage and Unit::SpellCriticalDamageBonus
		&Aura::HandleNoImmediateEffect,                         //205 SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_DAMAGE  implemented in Unit::SpellCriticalDamageBonus
		&Aura::HandleNULL,                                      //206 SPELL_AURA_MOD_FLIGHT_SPEED
		&Aura::HandleNULL,                                      //207 SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED
		&Aura::HandleNULL,                                      //208 SPELL_AURA_MOD_FLIGHT_SPEED_STACKING
		&Aura::HandleNULL,                                      //209 SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED_STACKING
		&Aura::HandleNULL,                                      //210 SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACKING
		&Aura::HandleNULL,                                      //211 SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED_NOT_STACKING
		&Aura::HandleNULL,                                      //212 SPELL_AURA_MOD_RANGED_ATTACK_POWER_OF_STAT_PERCENT
		&Aura::HandleNoImmediateEffect,                         //213 SPELL_AURA_MOD_RAGE_FROM_DAMAGE_DEALT implemented in Player::RewardRage
		&Aura::HandleNULL,                                      //214 Tamed Pet Passive
		&Aura::HandleNULL,                          	    //215 SPELL_AURA_ARENA_PREPARATION
		&Aura::HandleModCastingSpeed,                           //216 SPELL_AURA_HASTE_SPELLS
		&Aura::HandleUnused,                                    //217                                   unused
		&Aura::HandleAuraModRangedHaste,                        //218 SPELL_AURA_HASTE_RANGED
		&Aura::HandleModManaRegen,                              //219 SPELL_AURA_MOD_MANA_REGEN_FROM_STAT
		&Aura::HandleUnused,                                    //220 SPELL_AURA_MOD_RATING_FROM_STAT
		&Aura::HandleNULL,                                      //221 ignored
		&Aura::HandleUnused,                                    //222 unused
		&Aura::HandleNULL,                                      //223 Cold Stare
		&Aura::HandleUnused,                                    //224 unused
		&Aura::HandlePrayerOfMending,                         //225 SPELL_AURA_PRAYER_OF_MENDING
		&Aura::HandleAuraPeriodicDummy,                         //226 SPELL_AURA_PERIODIC_DUMMY
		&Aura::HandlePeriodicTriggerSpellWithValue,             //227 SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE
		&Aura::HandleNoImmediateEffect,                         //228 SPELL_AURA_DETECT_STEALTH
		&Aura::HandleNoImmediateEffect,                         //229 SPELL_AURA_MOD_AOE_DAMAGE_AVOIDANCE        implemented in Unit::SpellDamageBonusTaken
		&Aura::HandleAuraModIncreaseMaxHealth,                  //230 Commanding Shout
		&Aura::HandleNoImmediateEffect,                         //231 SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE
		&Aura::HandleNoImmediateEffect,                         //232 SPELL_AURA_MECHANIC_DURATION_MOD           implement in Unit::CalculateAuraDuration
		&Aura::HandleNULL,                                      //233 set model id to the one of the creature with id m_modifier.m_miscvalue
		&Aura::HandleNoImmediateEffect,                         //234 SPELL_AURA_MECHANIC_DURATION_MOD_NOT_STACK implement in Unit::CalculateAuraDuration
		&Aura::HandleAuraModDispelResist,                       //235 SPELL_AURA_MOD_DISPEL_RESIST               implement in Unit::MagicSpellHitResult
		&Aura::HandleUnused,                                    //236 unused
		&Aura::HandleModSpellDamagePercentFromAttackPower,      //237 SPELL_AURA_MOD_SPELL_DAMAGE_OF_ATTACK_POWER  implemented in Unit::SpellBaseDamageBonusDone
		&Aura::HandleModSpellHealingPercentFromAttackPower,     //238 SPELL_AURA_MOD_SPELL_HEALING_OF_ATTACK_POWER implemented in Unit::SpellBaseHealingBonusDone
		&Aura::HandleNULL,                                      //239 SPELL_AURA_MOD_SCALE_2 only in Noggenfogger Elixir (16595) before 2.3.0 aura 61
		&Aura::HandleNULL,                          //240 SPELL_AURA_MOD_EXPERTISE
		&Aura::HandleNULL,                          //241 Forces the caster to move forward
		&Aura::HandleUnused,                                    //242 unused
		&Aura::HandleFactionOverride,                           //243 SPELL_AURA_FACTION_OVERRIDE
		&Aura::HandleUnused,                        //244 SPELL_AURA_COMPREHEND_LANGUAGE
		&Aura::HandleUnused,                                    //245 unused
		&Aura::HandleUnused,                                    //246 unused
		&Aura::HandleUnused,                           //247 SPELL_AURA_MIRROR_IMAGE                      target to become a clone of the caster
		&Aura::HandleNoImmediateEffect,                         //248 SPELL_AURA_MOD_COMBAT_RESULT_CHANCE         implemented in Unit::RollMeleeOutcomeAgainst
		&Aura::HandleNULL,                                      //249
		&Aura::HandleAuraModIncreaseHealth,                     //250 SPELL_AURA_MOD_INCREASE_HEALTH_2
		&Aura::HandleNULL,                                      //251 SPELL_AURA_MOD_ENEMY_DODGE
		&Aura::HandleUnused,                                    //252 unused
		&Aura::HandleUnused,                                    //253 unused
		&Aura::HandleUnused,                                    //254 unused
		&Aura::HandleUnused,                                    //255 unused
		&Aura::HandleUnused,                                    //256 unused
		&Aura::HandleUnused,                                    //257 unused
		&Aura::HandleUnused,                                    //258 unused
		&Aura::HandleUnused,                                    //259 unused
		&Aura::HandleUnused,                                    //260 unused
		&Aura::HandleNULL,                                       //261 SPELL_AURA_261 some phased state (44856 spell)
    // Nostalrius - custom
    &Aura::HandleAuraAuraSpell,
};

static AuraType const frozenAuraTypes[] = { SPELL_AURA_MOD_ROOT, SPELL_AURA_MOD_STUN, SPELL_AURA_NONE };

Aura::Aura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target, Unit *caster, Item* castItem) :
    m_spellmod(nullptr), m_periodicTimer(0), m_periodicTick(0), m_removeMode(AURA_REMOVE_BY_DEFAULT),
    m_effIndex(eff), m_positive(false), m_isPeriodic(false), m_isAreaAura(false),
    m_isPersistent(false), m_in_use(0), m_spellAuraHolder(holder),
// NOSTALRIUS: auras exclusifs
    m_applied(false)
{
    MANGOS_ASSERT(target);
    MANGOS_ASSERT(spellproto && spellproto == sSpellMgr.GetSpellEntry(spellproto->Id) && "`info` must be pointer to sSpellStore element");
    ASSERT(spellproto->EffectApplyAuraName[eff]);

    m_currentBasePoints = currentBasePoints ? *currentBasePoints : spellproto->CalculateSimpleValue(eff);

    m_positive = IsPositiveEffect(spellproto, m_effIndex);
    m_applyTime = time(nullptr);

    int32 damage;
    if (!caster)
        damage = m_currentBasePoints;
    else
        damage = caster->CalculateSpellDamage(target, spellproto, m_effIndex, &m_currentBasePoints);

    damage *= GetStackAmount();

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Aura: construct Spellid : %u, Aura : %u Target : %d Damage : %d", spellproto->Id, spellproto->EffectApplyAuraName[eff], spellproto->EffectImplicitTargetA[eff], damage);

    //SetModifier(AuraType(spellproto->EffectApplyAuraName[eff]), damage, spellproto->EffectAmplitude[eff], spellproto->EffectMiscValue[eff]);

	if (spellproto->EffectRealPointsPerLevel[eff])
		SetModifier(AuraType(spellproto->EffectApplyAuraName[eff]), damage, spellproto->EffectAmplitude[eff], spellproto->EffectMiscValue[eff], spellproto->EffectRealPointsPerLevel[eff]);
	else
		SetModifier(AuraType(spellproto->EffectApplyAuraName[eff]), damage, spellproto->EffectAmplitude[eff], spellproto->EffectMiscValue[eff]);


    CalculatePeriodic(caster ? caster->GetSpellModOwner() : NULL, true);
    ComputeExclusive();
    if (IsLastAuraOnHolder() && !m_positive)
    {
        // Exclude passive spells.
        if (holder->IsNeedVisibleSlot(caster))
            holder->CalculateForDebuffLimit();

        holder->CalculateHeartBeat(caster, target);
    }
}

void Aura::Refresh(Unit* caster, Unit* target, SpellAuraHolder* pRefreshWithHolder)
{
    Aura* pHolderAura = pRefreshWithHolder->GetAuraByEffectIndex(m_effIndex);
    if (!pHolderAura)
        return;
    m_periodicTick = 0;
    Player* modOwner = caster ? caster->GetSpellModOwner() : NULL;
    m_applyTime = time(nullptr);
    // Refresh periodic period, but keep current timer.
    // If we chain refresh a DoT, it should not prevent first damage tick!
    int32 oldPeriodicTimer = m_periodicTimer;
    CalculatePeriodic(modOwner, true);
    if (oldPeriodicTimer < m_periodicTimer)
        m_periodicTimer = oldPeriodicTimer;

    // Re-calculation du montant de degats
    if (IsApplied() || !IsExclusive())
    {
        bool lockStats = false;
        switch (GetSpellProto()->EffectApplyAuraName[m_effIndex])
        {
            case SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE:
                if (GetSpellProto()->Attributes & 0x10)
                    break;
            // no break
            case SPELL_AURA_MOD_STAT:
            case SPELL_AURA_MOD_PERCENT_STAT:
            case SPELL_AURA_MOD_INCREASE_HEALTH:
            case SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT: // Exemple : 27038
                lockStats = true;
                break;
        }
        if (lockStats)
            target->SetCanModifyStats(false);
        //unapply with the old modifiers and reapply with the new.
        ApplyModifier(false, true, false);
        // Refresh de quelques variables du modifier
        m_modifier.m_auraname = pHolderAura->GetModifier()->m_auraname;
        m_modifier.m_amount = pHolderAura->GetModifier()->m_amount;
        m_modifier.m_miscvalue = pHolderAura->GetModifier()->m_miscvalue;
        ApplyModifier(true, true, false);
        if (lockStats)
        {
            target->SetCanModifyStats(true);
            target->UpdateAllStats();
        }
    }
}

void SpellAuraHolder::Refresh(Unit* caster, Unit* target, SpellAuraHolder* pRefreshWithHolder)
{
    m_casterGuid = caster ? caster->GetObjectGuid() : target->GetObjectGuid();
    m_applyTime = time(nullptr);
    m_duration = pRefreshWithHolder->GetAuraDuration();
    m_maxDuration = pRefreshWithHolder->GetAuraMaxDuration();
    for (int i = 0 ; i < MAX_EFFECT_INDEX; ++i)
    {
        if (Aura* pAura = GetAuraByEffectIndex(SpellEffectIndex(i)))
            pAura->Refresh(caster, target, pRefreshWithHolder);
    }
    UpdateAuraDuration();
    UpdateAuraApplication();
}

bool SpellAuraHolder::CanBeRefreshedBy(SpellAuraHolder* other) const
{
    if (!other)
        return false;
    if (other->GetCasterGuid() != GetCasterGuid())
        return false;
    // Meme ID/Effet de sort
    if (other->GetId() != GetId())
        return false;
    if (m_spellProto->StackAmount) // Se stack
        return false;
    if (m_spellProto->procCharges) // Ou a des charges (fix bug visuel)
        return false;
    return true;
}

bool SpellAuraHolder::IsMoreImportantDebuffThan(SpellAuraHolder* other) const
{
    // Same category : last aura applies
    if (m_debuffLimitScore == other->m_debuffLimitScore)
        return m_applyTime > other->m_applyTime;
    // Else, compare categories
    return m_debuffLimitScore > other->m_debuffLimitScore;
}

Aura::~Aura()
{
}

AreaAura::AreaAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target,
                   Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
    m_isAreaAura = true;

    // caster==NULL in constructor args if target==caster in fact
    Unit* caster_ptr = caster ? caster : target;

    m_radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(spellproto->EffectRadiusIndex[m_effIndex]));
    if (Player* modOwner = caster_ptr->GetSpellModOwner())
        modOwner->ApplySpellMod(spellproto->Id, SPELLMOD_RADIUS, m_radius);

    switch (spellproto->Effect[eff])
    {
        case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
            m_areaAuraType = AREA_AURA_PARTY;
            if (target->GetTypeId() == TYPEID_UNIT && ((Creature*)target)->IsTotem())
                m_modifier.m_auraname = SPELL_AURA_NONE;
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_RAID:
            m_areaAuraType = AREA_AURA_RAID;
            if (target->GetTypeId() == TYPEID_UNIT && ((Creature*)target)->IsTotem())
                m_modifier.m_auraname = SPELL_AURA_NONE;
            // Light's Beacon not applied to caster itself (TODO: more generic check for another similar spell if any?)
            else if (target == caster_ptr && spellproto->Id == 53651)
                m_modifier.m_auraname = SPELL_AURA_NONE;
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
            m_areaAuraType = AREA_AURA_FRIEND;
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
            m_areaAuraType = AREA_AURA_ENEMY;
            if (target == caster_ptr)
                m_modifier.m_auraname = SPELL_AURA_NONE;    // Do not do any effect on self
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_PET:
            m_areaAuraType = AREA_AURA_PET;
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_OWNER:
            m_areaAuraType = AREA_AURA_OWNER;
            if (target == caster_ptr)
                m_modifier.m_auraname = SPELL_AURA_NONE;
            break;
        default:
            sLog.outError("Wrong spell effect in AreaAura constructor");
            MANGOS_ASSERT(false);
            break;
    }
}

AreaAura::~AreaAura()
{
}

PersistentAreaAura::PersistentAreaAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target,
                                       Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
    m_isPersistent = true;
}

PersistentAreaAura::~PersistentAreaAura()
{
}

SingleEnemyTargetAura::SingleEnemyTargetAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target,
        Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem)
{
    if (caster)
        m_castersTargetGuid = caster->GetTypeId() == TYPEID_PLAYER ? ((Player*)caster)->GetSelectionGuid() : caster->GetTargetGuid();
}

SingleEnemyTargetAura::~SingleEnemyTargetAura()
{
}

Unit* SingleEnemyTargetAura::GetTriggerTarget() const
{
    return ObjectAccessor::GetUnit(*(m_spellAuraHolder->GetTarget()), m_castersTargetGuid);
}

Aura* CreateAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, SpellAuraHolder *holder, Unit *target, Unit *caster, Item* castItem)
{
    if (IsAreaAuraEffect(spellproto->Effect[eff]))
        return new AreaAura(spellproto, eff, currentBasePoints, holder, target, caster, castItem);

    return new Aura(spellproto, eff, currentBasePoints, holder, target, caster, castItem);
}

SpellAuraHolder* CreateSpellAuraHolder(SpellEntry const* spellproto, Unit *target, WorldObject *caster, Item *castItem)
{
    return new SpellAuraHolder(spellproto, target, caster, castItem);
}

void Aura::SetModifier(AuraType t, int32 a, uint32 pt, int32 miscValue, float scale)
{
    m_modifier.m_auraname = t;
    m_modifier.m_amount = a;
    m_modifier.m_miscvalue = miscValue;
    m_modifier.periodictime = pt;
	m_modifier.m_scale = scale;
}

void Aura::Update(uint32 diff)
{
    if (m_isPeriodic)
    {
        m_periodicTimer -= diff;
        if (m_periodicTimer <= 0) // tick also at m_periodicTimer==0 to prevent lost last tick in case max m_duration == (max m_periodicTimer)*N
        {
            // update before applying (aura can be removed in TriggerSpell or PeriodicTick calls)
            m_periodicTimer += m_modifier.periodictime;
            ++m_periodicTick;                               // for some infinity auras in some cases can overflow and reset
            PeriodicTick();
        }
    }
}

void AreaAura::Update(uint32 diff)
{
    // update for the caster of the aura
    if (GetCasterGuid() == GetTarget()->GetObjectGuid())
    {
        Unit* caster = GetTarget();

        if (!caster->hasUnitState(UNIT_STAT_ISOLATED))
        {
            Unit* owner = caster->GetCharmerOrOwner();
            if (!owner)
                owner = caster;
            Spell::UnitList targets;

            switch (m_areaAuraType)
            {
                case AREA_AURA_PARTY:
                {
                    Group *pGroup = nullptr;

                    if (owner->GetTypeId() == TYPEID_PLAYER)
                        pGroup = ((Player*)owner)->GetGroup();

                    if (pGroup)
                    {
                        uint8 subgroup = ((Player*)owner)->GetSubGroup();
                        for (GroupReference *itr = pGroup->GetFirstMember(); itr != nullptr; itr = itr->next())
                        {
                            Player* Target = itr->getSource();
                            if (Target && Target->isAlive() && Target->GetSubGroup() == subgroup && (!Target->duel || owner == Target) && caster->IsFriendlyTo(Target))
                            {
                                if (caster->IsWithinDistInMap(Target, m_radius))
                                    targets.push_back(Target);
                                Pet *pet = Target->GetPet();
                                if (pet && pet->isAlive() && caster->IsWithinDistInMap(pet, m_radius))
                                    targets.push_back(pet);
                            }
                        }
                    }
                    else
                    {
                        // add owner
                        if (owner != caster && caster->IsWithinDistInMap(owner, m_radius))
                            targets.push_back(owner);
                        // add caster's pet
                        Unit* pet = caster->GetPet();
                        if (pet && caster->IsWithinDistInMap(pet, m_radius))
                            targets.push_back(pet);
                    }
                    break;
                }
                case AREA_AURA_RAID:
                {
                    Group *pGroup = nullptr;

                    if (owner->GetTypeId() == TYPEID_PLAYER)
                        pGroup = ((Player*)owner)->GetGroup();

                    if (pGroup)
                    {
                        for (GroupReference *itr = pGroup->GetFirstMember(); itr != nullptr; itr = itr->next())
                        {
                            Player* Target = itr->getSource();
                            if (Target && Target->isAlive() && caster->IsFriendlyTo(Target))
                            {
                                if (caster->IsWithinDistInMap(Target, m_radius))
                                    targets.push_back(Target);
                                Pet *pet = Target->GetPet();
                                if (pet && pet->isAlive() && caster->IsWithinDistInMap(pet, m_radius))
                                    targets.push_back(pet);
                            }
                        }
                    }
                    else
                    {
                        // add owner
                        if (owner != caster && caster->IsWithinDistInMap(owner, m_radius))
                            targets.push_back(owner);
                        // add caster's pet
                        Unit* pet = caster->GetPet();
                        if (pet && caster->IsWithinDistInMap(pet, m_radius))
                            targets.push_back(pet);
                    }
                    break;
                }
                case AREA_AURA_FRIEND:
                {
                    MaNGOS::AnyFriendlyUnitInObjectRangeCheck u_check(caster, m_radius);
                    MaNGOS::UnitListSearcher<MaNGOS::AnyFriendlyUnitInObjectRangeCheck> searcher(targets, u_check);
                    Cell::VisitAllObjects(caster, searcher, m_radius);
                    break;
                }
                case AREA_AURA_ENEMY:
                {
                    MaNGOS::AnyAoETargetUnitInObjectRangeCheck u_check(caster, m_radius); // No GetCharmer in searcher
                    MaNGOS::UnitListSearcher<MaNGOS::AnyAoETargetUnitInObjectRangeCheck> searcher(targets, u_check);
                    Cell::VisitAllObjects(caster, searcher, m_radius);
                    break;
                }
                case AREA_AURA_OWNER:
                {
                    if (owner != caster && caster->IsWithinDistInMap(owner, m_radius))
                        targets.push_back(owner);
                    break;
                }
                case AREA_AURA_PET:
                {
                    if (owner != caster && caster->IsWithinDistInMap(owner, m_radius))
                        targets.push_back(owner);
                    break;
                }
            }

            for (Spell::UnitList::iterator tIter = targets.begin(); tIter != targets.end(); tIter++)
            {
                // flag for seelction is need apply aura to current iteration target
                bool apply = true;

                // we need ignore present caster self applied are auras sometime
                // in cases if this only auras applied for spell effect
                Unit::SpellAuraHolderBounds spair = (*tIter)->GetSpellAuraHolderBounds(GetId());
                for (Unit::SpellAuraHolderMap::const_iterator i = spair.first; i != spair.second; ++i)
                {
                    if (i->second->IsDeleted())
                        continue;

                    Aura *aur = i->second->GetAuraByEffectIndex(m_effIndex);

                    if (!aur)
                        continue;

                    // in generic case not allow stacking area auras
                    apply = false;
                    break;
                }

                if (!apply)
                    continue;

                // Skip some targets (TODO: Might require better checks, also unclear how the actual caster must/can be handled)
                if (GetSpellProto()->AttributesEx3 & SPELL_ATTR_EX3_TARGET_ONLY_PLAYER && (*tIter)->GetTypeId() != TYPEID_PLAYER)
                    continue;

                if (SpellEntry const *actualSpellInfo = sSpellMgr.SelectAuraRankForLevel(GetSpellProto(), (*tIter)->getLevel()))
                {
                    int32 actualBasePoints = m_currentBasePoints;
                    // recalculate basepoints for lower rank (all AreaAura spell not use custom basepoints?)
                    if (actualSpellInfo != GetSpellProto())
                        actualBasePoints = actualSpellInfo->CalculateSimpleValue(m_effIndex);

                    SpellAuraHolder *holder = (*tIter)->GetSpellAuraHolder(actualSpellInfo->Id, GetCasterGuid());

                    bool addedToExisting = true;
                    if (!holder)
                    {
                        holder = CreateSpellAuraHolder(actualSpellInfo, (*tIter), caster);
                        addedToExisting = false;
                    }

                    holder->SetAuraDuration(GetAuraDuration());

                    AreaAura *aur = new AreaAura(actualSpellInfo, m_effIndex, &actualBasePoints, holder, (*tIter), caster, nullptr);
                    holder->AddAura(aur, m_effIndex);

                    if (addedToExisting)
                    {
                        (*tIter)->AddAuraToModList(aur);
                        holder->SetInUse(true);
                        aur->ApplyModifier(true, true);
                        holder->SetInUse(false);
                    }
                    else
                        (*tIter)->AddSpellAuraHolder(holder);
                }
            }
        }
        Aura::Update(diff);
    }
    else                                                    // aura at non-caster
    {
        Unit* caster = GetCaster();
        Unit* target = GetTarget();

        Aura::Update(diff);

        // remove aura if out-of-range from caster (after teleport for example)
        // or caster is isolated or caster no longer has the aura
        // or caster is (no longer) friendly
        bool needFriendly = (m_areaAuraType == AREA_AURA_ENEMY ? false : true);
        if (!caster || caster->hasUnitState(UNIT_STAT_ISOLATED) ||
                !caster->IsWithinDistInMap(target, m_radius)        ||
                !caster->HasAura(GetId(), GetEffIndex())            ||
                caster->IsFriendlyTo(target) != needFriendly
           )
            target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGuid());
        else if (m_areaAuraType == AREA_AURA_PARTY)         // check if in same sub group
        {
            // not check group if target == owner or target == pet
            if (caster->GetCharmerOrOwnerGuid() != target->GetObjectGuid() && caster->GetObjectGuid() != target->GetCharmerOrOwnerGuid())
            {
                Player* check = caster->GetCharmerOrOwnerPlayerOrPlayerItself();

                Group *pGroup = check ? check->GetGroup() : NULL;
                if (pGroup)
                {
                    Player* checkTarget = target->GetCharmerOrOwnerPlayerOrPlayerItself();
                    if (!checkTarget || !pGroup->SameSubGroup(check, checkTarget))
                        target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGuid());
                }
                else
                    target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGuid());
            }
        }
        else if (m_areaAuraType == AREA_AURA_PET)
        {
            if (target->GetObjectGuid() != caster->GetCharmerOrOwnerGuid())
                target->RemoveSingleAuraFromSpellAuraHolder(GetId(), GetEffIndex(), GetCasterGuid());
        }
    }
}

void PersistentAreaAura::Update(uint32 diff)
{
    bool remove = false;

    // remove the aura if its caster or the dynamic object causing it was removed
    // or if the target moves too far from the dynamic object

    // Nostalrius: piege explosif. Ne doit pas etre retire lorsqu'on sort de la zone.
    if (GetId() != 13812 && GetId() != 14314 && GetId() != 14315)
    {
        if (Unit *caster = GetCaster())
        {
            DynamicObject *dynObj = caster->GetDynObject(GetId(), GetEffIndex());
            if (dynObj)
            {
                if (!GetTarget()->IsWithinDistInMap(dynObj, dynObj->GetRadius()))
                {
                    remove = true;
                    dynObj->RemoveAffected(GetTarget());        // let later reapply if target return to range
                }
            }
            else
                remove = true;
        }
        else
            remove = true;
    }

    Aura::Update(diff);

    if (remove)
        GetTarget()->RemoveAura(GetId(), GetEffIndex());
}

void Aura::ApplyModifier(bool apply, bool Real, bool skipCheckExclusive)
{
    // Dans Unit::RemoveAura, ApplyModifier est toujours appelle.
    if (IsApplied() == apply)
        return;
    AuraType aura = m_modifier.m_auraname;

    GetHolder()->SetInUse(true);
    SetInUse(true);
    // NOSTALRIUS: Auras exclusifs.
    if (apply && !skipCheckExclusive && IsExclusive() && !ExclusiveAuraCanApply())
    {
        GetHolder()->SetInUse(false);
        SetInUse(false);
        return;
    }
    m_applied = apply;
    if (aura < TOTAL_AURAS)
        (*this.*AuraHandler [aura])(apply, Real);

    if (!apply && !skipCheckExclusive && IsExclusive())
        ExclusiveAuraUnapply();

    SetInUse(false);
    GetHolder()->SetInUse(false);
}

bool Aura::isAffectedOnSpell(SpellEntry const *spell) const
{
	if (!spell)
		return  false;
	
    if (m_spellmod)
        return m_spellmod->isAffectedOnSpell(spell);

    // Check family name
    if (spell->SpellFamilyName != GetSpellProto()->SpellFamilyName)
        return false;

    ClassFamilyMask mask = sSpellMgr.GetSpellAffectMask(GetId(), GetEffIndex());
    return spell->IsFitToFamilyMask(mask);
}

bool Aura::CanProcFrom(SpellEntry const *spell, uint32 EventProcEx, uint32 procEx, bool active, bool useClassMask) const
{
    // Check EffectClassMask (in pre-3.x stored in spell_affect in fact)
    ClassFamilyMask mask = sSpellMgr.GetSpellAffectMask(GetId(), GetEffIndex());

    // Nostalrius: c'est la moindre des choses d'utiliser un peu 'spell_proc_event' non ?
    if (!mask)
        if (SpellProcEventEntry const* entry = sSpellMgr.GetSpellProcEvent(GetId()))
            mask = entry->spellFamilyMask[GetEffIndex()];

    // if no class mask defined, or spell_proc_event has SpellFamilyName=0 - allow proc
    if (!useClassMask || !mask)
    {
        if (!(EventProcEx & PROC_EX_EX_TRIGGER_ALWAYS))
        {
            // Check for extra req (if none) and hit/crit
            if (EventProcEx == PROC_EX_NONE)
            {
                // No extra req, so can trigger only for active (damage/healing present) and hit/crit
                if ((procEx & (PROC_EX_NORMAL_HIT | PROC_EX_CRITICAL_HIT)) && active)
                    return true;
                else
                    return false;
            }
            else // Passive spells hits here only if resist/reflect/immune/evade
            {
                // Passive spells can`t trigger if need hit (exclude cases when procExtra include non-active flags)
                if ((EventProcEx & (PROC_EX_NORMAL_HIT | PROC_EX_CRITICAL_HIT) & procEx) && !active)
                    return false;
            }
        }
        return true;
    }
    else
    {
        // SpellFamilyName check is performed in SpellMgr::IsSpellProcEventCanTriggeredBy and it is done once for whole holder
        // note: SpellFamilyName is not checked if no spell_proc_event is defined
        return mask.IsFitToFamilyMask(spell->SpellFamilyFlags);
    }
}

void Aura::ReapplyAffectedPassiveAuras(Unit* target)
{
    // we need store cast item guids for self casted spells
    // expected that not exist permanent auras from stackable auras from different items
    std::map<uint32, ObjectGuid> affectedSelf;

    for (Unit::SpellAuraHolderMap::const_iterator itr = target->GetSpellAuraHolderMap().begin(); itr != target->GetSpellAuraHolderMap().end(); ++itr)
    {
        // permanent passive
        // passive spells can be affected only by own or owner spell mods)
        if (itr->second->IsPassive() && itr->second->IsPermanent() &&
                // non deleted and not same aura (any with same spell id)
                !itr->second->IsDeleted() && itr->second->GetId() != GetId() &&
                // and affected by aura
                itr->second->GetCasterGuid() == target->GetObjectGuid() &&
                // and affected by spellmod
                isAffectedOnSpell(itr->second->GetSpellProto()))
            affectedSelf[itr->second->GetId()] = itr->second->GetCastItemGuid();
    }

    if (!affectedSelf.empty())
    {
        Player* pTarget = target->GetTypeId() == TYPEID_PLAYER ? (Player*)target : NULL;

        for (std::map<uint32, ObjectGuid>::const_iterator map_itr = affectedSelf.begin(); map_itr != affectedSelf.end(); ++map_itr)
        {
            Item* item = pTarget && map_itr->second ? pTarget->GetItemByGuid(map_itr->second) : NULL;
            target->RemoveAurasDueToSpell(map_itr->first);
            target->CastSpell(target, map_itr->first, true, item);
        }
    }
}

struct ReapplyAffectedPassiveAurasHelper
{
    explicit ReapplyAffectedPassiveAurasHelper(Aura* _aura) : aura(_aura) {}
    void operator()(Unit* unit) const
    {
        aura->ReapplyAffectedPassiveAuras(unit);
    }
    Aura* aura;
};

void Aura::ReapplyAffectedPassiveAuras()
{
    // not reapply spell mods with charges (use original value because processed and at remove)
    if (GetSpellProto()->procCharges)
        return;

    // not reapply some spell mods ops (mostly speedup case)
    switch (m_modifier.m_miscvalue)
    {
        case SPELLMOD_DURATION:
        case SPELLMOD_CHARGES:
        case SPELLMOD_NOT_LOSE_CASTING_TIME:
        case SPELLMOD_CASTING_TIME:
        case SPELLMOD_COOLDOWN:
        case SPELLMOD_COST:
        case SPELLMOD_ACTIVATION_TIME:
        case SPELLMOD_CASTING_TIME_OLD:
            return;
    }

    // reapply talents to own passive persistent auras
    ReapplyAffectedPassiveAuras(GetTarget());

    // re-apply talents/passives/area auras applied to pet/totems (it affected by player spellmods)
    GetTarget()->CallForAllControlledUnits(ReapplyAffectedPassiveAurasHelper(this), CONTROLLED_PET | CONTROLLED_TOTEMS);
}

/*********************************************************/
/***               BASIC AURA FUNCTION                 ***/
/*********************************************************/
void Aura::HandleAddModifier(bool apply, bool Real)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER || !Real)
        return;

    if (m_modifier.m_miscvalue >= MAX_SPELLMOD)
        return;

    if (apply)
    {
        // Add custom charges for some mod aura
        switch (GetSpellProto()->Id)
        {
            case 17941:                                     // Shadow Trance
            case 22008:                                     // Netherwind Focus
			case 34936:										// Backlash
                GetHolder()->SetAuraCharges(1);
                break;
        }

        // In pre-TBC wrong spellmods in DBC
        // Nostalrius: fix dans spell_effect_mod (EffectMiscValue)
        /*
        switch (GetSpellProto()->SpellIconID)
        {
            case 143:       // Permafrost Speed Decrease
                if (GetEffIndex() == EFFECT_INDEX_1)
                    m_modifier.m_miscvalue = SPELLMOD_EFFECT1;
                break;
            case 228:       // Improved Curse of Exhaustion Speed Decrease
                if (GetEffIndex() == EFFECT_INDEX_0)
                    m_modifier.m_miscvalue = SPELLMOD_EFFECT1;
                break;
            case 250:       // Camouflage Speed Decrease (13975, 14062, 14063, 14064, 14065)
                if (GetEffIndex() == EFFECT_INDEX_0)
                    m_modifier.m_miscvalue = SPELLMOD_EFFECT3;
                break;
            case 1181:       // Pathfinding Speed Increase (ID = 19560)
                if (GetEffIndex() == EFFECT_INDEX_0)
                    m_modifier.m_miscvalue = SPELLMOD_EFFECT1;
                break;
            case 1494:       // Amplify Curse Speed Decrease (18310, 18311, 18312, 18313)
                if (GetEffIndex() == EFFECT_INDEX_0)
                    m_modifier.m_miscvalue = SPELLMOD_EFFECT1;
                break;
            case 1563:       // Cheetah Sprint (ID = 24348)
                if (GetEffIndex() == EFFECT_INDEX_0)
                    m_modifier.m_miscvalue = SPELLMOD_EFFECT1;
                break;
        }*/

        m_spellmod = new SpellModifier(
            SpellModOp(m_modifier.m_miscvalue),
            SpellModType(m_modifier.m_auraname),            // SpellModType value == spell aura types
            m_modifier.m_amount,
            this,
            // prevent expire spell mods with (charges > 0 && m_stackAmount > 1)
            // all this spell expected expire not at use but at spell proc event check
            GetSpellProto()->StackAmount > 1 ? 0 : GetHolder()->GetAuraCharges());
    }

    ASSERT(m_spellmod);
    ((Player*)GetTarget())->AddSpellMod(m_spellmod, apply);
    if (!apply)
        m_spellmod = nullptr; // Deja supprime par Player::AddSpellMod.

    ReapplyAffectedPassiveAuras();
}

void Aura::TriggerSpell()
{
    ObjectGuid casterGUID = GetCasterGuid();
    Unit* triggerTarget = GetTriggerTarget();

    if (!casterGUID || !triggerTarget)
        return;

    // generic casting code with custom spells and target/caster customs
    uint32 trigger_spell_id = GetSpellProto()->EffectTriggerSpell[m_effIndex];

    SpellEntry const *triggeredSpellInfo = sSpellMgr.GetSpellEntry(trigger_spell_id);
    SpellEntry const *auraSpellInfo = GetSpellProto();
    uint32 auraId = auraSpellInfo->Id;
    Unit* target = GetTarget();

    uint32 spellRandom;

    // specific code for cases with no trigger spell provided in field
    if (triggeredSpellInfo == nullptr)
    {
        switch (auraSpellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                switch (auraId)
                {
                    // Firestone Passive (1-5 ranks)
                    case 758:
                    case 17945:
                    case 17947:
                    case 17949:
                    {
                        if (triggerTarget->GetTypeId() != TYPEID_PLAYER)
                            return;
                        Item* item = ((Player*)triggerTarget)->GetWeaponForAttack(BASE_ATTACK);
                        if (!item)
                            return;
                        uint32 enchant_id = 0;
                        switch (GetId())
                        {
                            case   758:
                                enchant_id = 1803;
                                break;   // Rank 1
                            case 17945:
                                enchant_id = 1823;
                                break;   // Rank 2
                            case 17947:
                                enchant_id = 1824;
                                break;   // Rank 3
                            case 17949:
                                enchant_id = 1825;
                                break;   // Rank 4
                            default:
                                return;
                        }
                        // remove old enchanting before applying new
                        ((Player*)triggerTarget)->ApplyEnchantment(item, TEMP_ENCHANTMENT_SLOT, false);
                        item->SetEnchantment(TEMP_ENCHANTMENT_SLOT, enchant_id, m_modifier.periodictime + 1000, 0);
                        // add new enchanting
                        ((Player*)triggerTarget)->ApplyEnchantment(item, TEMP_ENCHANTMENT_SLOT, true);
                        return;
                    }
//                    // Periodic Mana Burn
//                    case 812: break;
//                    // Polymorphic Ray
//                    case 6965: break;
//                    // Fire Nova (1-7 ranks)
//                    case 8350:
//                    case 8508:
//                    case 8509:
//                    case 11312:
//                    case 11313:
//                    case 25540:
//                    case 25544:
//                        break;
                    // Thaumaturgy Channel
                    case 9712:
                        trigger_spell_id = 21029;
                        break;
//                    // Egan's Blaster
//                    case 17368: break;
//                    // Haunted
//                    case 18347: break;
//                    // Ranshalla Waiting
//                    case 18953: break;
//                    // Inferno
//                    case 19695: break;
//                    // Frostwolf Muzzle DND
//                    case 21794: break;
//                    // Alterac Ram Collar DND
//                    case 21866: break;
//                    // Celebras Waiting
//                    case 21916: break;
                    // Brood Affliction: Bronze
					case 19695:                             // Inferno
					{
						int32 damageForTick[8] = { 500, 500, 1000, 1000, 2000, 2000, 3000, 5000 };
						triggerTarget->CastCustomSpell(triggerTarget, 19698, &damageForTick[GetAuraTicks() - 1], nullptr, nullptr, true, nullptr);
						return;
					}
                    case 23170:
                    {
                        int rand = urand(0, 9);
                        if (rand < 4)   // Ustaag <Nostalrius> : 40% chance
                            target->CastSpell(target, 23171, true, nullptr, this);
                        return;
                    }
                    case 23493:                             // Restoration
                    {
                        int32 heal = triggerTarget->GetMaxHealth() / 10;
                        triggerTarget->DealHeal(triggerTarget, heal, auraSpellInfo);

                        if (int32 mana = triggerTarget->GetMaxPower(POWER_MANA))
                        {
                            mana /= 10;
                            triggerTarget->EnergizeBySpell(triggerTarget, 23493, mana, POWER_MANA);
                        }
                        return;
                    }
//                    // Stoneclaw Totem Passive TEST
//                    case 23792: break;
//                    // Axe Flurry
//                    case 24018: break;
//                    // Mark of Arlokk
//                    case 24210: break;
//                    // Restoration
//                    case 24379: break;
//                    // Happy Pet
//                    case 24716: break;
//                    // Dream Fog
//                    case 24780: break;
//                    // Cannon Prep
//                    case 24832: break;
                    case 24834:                             // Shadow Bolt Whirl
                    {
                        uint32 spellForTick[8] = { 24820, 24821, 24822, 24823, 24835, 24836, 24837, 24838 };
                        trigger_spell_id = spellForTick[GetAuraTicks() % 8];
                        break;
                    }
//                    // Stink Trap
//                    case 24918: break;
//                    // Agro Drones
//                    case 25152: break;
                    case 25371:                             // Consume
                    {
                        int32 bpDamage = triggerTarget->GetMaxHealth() * 10 / 100;
                        triggerTarget->CastCustomSpell(triggerTarget, 25373, &bpDamage, nullptr, nullptr, true, nullptr, this, casterGUID);
                        return;
                    }
//                    // Pain Spike
//                    case 25572: break;
                    case 26009:                             // Rotate 360
                    case 26136:                             // Rotate -360
                    {
                        float newAngle = target->GetOrientation();

                        if (auraId == 26009)
                            newAngle += M_PI_F / 40;
                        else
                            newAngle -= M_PI_F / 40;

                        newAngle = MapManager::NormalizeOrientation(newAngle);
                        target->SetFacingTo(newAngle);
                        target->CastSpell(target, 26029, true);
                        return;
                    }
//                    // Consume
//                    case 26196: break;
//                    // Berserk
//                    case 26615: break;
//                    // Defile
//                    case 27177: break;
//                    // Teleport: IF/UC
//                    case 27601: break;
//                    // Five Fat Finger Exploding Heart Technique
//                    case 27673: break;
//                    // Nitrous Boost
//                    case 27746: break;
//                    // Steam Tank Passive
//                    case 27747: break;
                    case 27808:                             // Frost Blast
                    {
                        int32 bpDamage = triggerTarget->GetMaxHealth() * 26 / 100;
                        triggerTarget->CastCustomSpell(triggerTarget, 29879, &bpDamage, nullptr, nullptr, true, nullptr, this, casterGUID);
                        return;
                    }
                    // Detonate Mana
                    case 27819:
                    {
                        // 50% Mana Burn
                        int32 bpDamage = (int32)triggerTarget->GetPower(POWER_MANA) / 2;
                        triggerTarget->ModifyPower(POWER_MANA, -bpDamage);
                        triggerTarget->CastCustomSpell(triggerTarget, 27820, &bpDamage, nullptr, nullptr, true, nullptr, this, triggerTarget->GetObjectGuid());
                        return;
                    }
                    // Stalagg Chain and Feugen Chain
                    case 28096:
                    case 28111:
                    {
                        // X-Chain is casted by Tesla to X, so: caster == Tesla, target = X
                        Unit* pCaster = GetCaster();
                        if (pCaster && pCaster->GetTypeId() == TYPEID_UNIT && !pCaster->IsWithinDistInMap(target, 60.0f))
                        {
                            pCaster->InterruptNonMeleeSpells(true);
                            ((Creature*)pCaster)->SetInCombatWithZone();
                            // Stalagg Tesla Passive or Feugen Tesla Passive
                            pCaster->CastSpell(pCaster, auraId == 28096 ? 28097 : 28109, true, nullptr, nullptr, target->GetObjectGuid());
                        }
                        return;
                    }
//                    // Controller Timer
//                    case 28095: break;
//                    // Stalagg Tesla Passive
//                    case 28097: break;
//                    // Feugen Tesla Passive
//                    case 28109: break;
//                    // Mark of Didier
//                    case 28114: break;
//                    // Communique Timer, camp
//                    case 28346: break;
//                    // Icebolt
//                    case 28522: break;
//                    // Silithyst
//                    case 29519: break;
					case 29528:                             // Inoculate Nestlewood Owlkin
															// prevent error reports in case ignored player target
						if (triggerTarget->GetTypeId() != TYPEID_UNIT)
							return;
						break;
						//                    // Icebolt
						//                    case 28522: break;
						//                    // Guardian of Icecrown Passive
						//                    case 29897: break;
						//                    // Mind Exhaustion Passive
						//                    case 30025: break;
					case 29917:                             // Feed Captured Animal
						trigger_spell_id = 29916;
						break;
					case 30576:                             // Quake
						trigger_spell_id = 30571;
						break;
					case 31347:                             // Doom
					{
						target->CastSpell(target, 31350, true);
						target->DealDamage(target, target->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
						return;
					}
					case 31373:                             // Spellcloth
					{
						// Summon Elemental after create item
						triggerTarget->SummonCreature(17870, 0.0f, 0.0f, 0.0f, triggerTarget->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
						return;
					}
					case 31611:                             // Bloodmyst Tesla
															// no custom effect required; return to avoid spamming with errors
						return;
					case 31944:                             // Doomfire
					{
						int32 damage = m_modifier.m_amount * ((GetAuraDuration() + m_modifier.periodictime) / GetAuraMaxDuration());
						triggerTarget->CastCustomSpell(triggerTarget, 31969, &damage, nullptr, nullptr, true, nullptr, this, casterGUID);
						return;
					}
					case 34229:                             // Flame Quills
					{
						// cast 24 spells 34269-34289, 34314-34316
						for (uint32 spell_id = 34269; spell_id != 34290; ++spell_id)
							triggerTarget->CastSpell(triggerTarget, spell_id, true, nullptr, this, casterGUID);
						for (uint32 spell_id = 34314; spell_id != 34317; ++spell_id)
							triggerTarget->CastSpell(triggerTarget, spell_id, true, nullptr, this, casterGUID);
						return;
					}
					case 36573:                             // Vision Guide
					{
						if (GetAuraTicks() == 10 && target->GetTypeId() == TYPEID_PLAYER)
						{
							((Player*)target)->AreaExploredOrEventHappens(10525);
							target->RemoveAurasDueToSpell(36573);
						}

						return;
					}
					case 37429:                             // Spout (left)
					case 37430:                             // Spout (right)
					{
						float newAngle = target->GetOrientation();

						if (auraId == 37429)
							newAngle += 2 * M_PI_F / 100;
						else
							newAngle -= 2 * M_PI_F / 100;

						newAngle = MapManager::NormalizeOrientation(newAngle);

						target->SetFacingTo(newAngle);

						target->CastSpell(target, 37433, true);
						return;
					}
					case 38495:                             // Eye of Grillok
					{
						target->CastSpell(target, 38530, true);
						return;
					}
					case 38554:                             // Absorb Eye of Grillok (Zezzak's Shard)
					{
						if (target->GetTypeId() != TYPEID_UNIT)
							return;

						if (Unit* caster = GetCaster())
							caster->CastSpell(caster, 38495, true, nullptr, this);
						else
							return;

						Creature* creatureTarget = (Creature*)target;

						creatureTarget->ForcedDespawn();
						return;
					}
					case 39105:                             // Activate Nether-wraith Beacon (31742 Nether-wraith Beacon item)
					{
						float fX, fY, fZ;
						triggerTarget->GetClosePoint(fX, fY, fZ, triggerTarget->GetObjectBoundingRadius(), 20.0f);
						triggerTarget->SummonCreature(22408, fX, fY, fZ, triggerTarget->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
						return;
					}
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_MAGE:
            {
                switch (auraId)
                {
                    case 66:                                // Invisibility
                        // Here need periodic trigger reducing threat spell (or do it manually)
                        return;
                    default:
                        break;
                }
                break;
            }
//            case SPELLFAMILY_WARRIOR:
//            {
//                switch(auraId)
//                {
//                    // Wild Magic
//                    case 23410: break;
//                    // Corrupted Totems
//                    case 23425: break;
//                    default:
//                        break;
//                }
//                break;
//            }
//            case SPELLFAMILY_PRIEST:
//            {
//                switch(auraId)
//                {
//                    // Blue Beam
//                    case 32930: break;
//                    default:
//                        break;
//                }
//                break;
//            }
            case SPELLFAMILY_DRUID:
            {
                switch (auraId)
                {
                    case 768:                               // Cat Form
                        // trigger_spell_id not set and unknown effect triggered in this case, ignoring for while
                        return;
                    case 22842:                             // Frenzied Regeneration
                    case 22895:
                    case 22896:
                    {
                        int32 LifePerRage = GetModifier()->m_amount;

                        int32 lRage = target->GetPower(POWER_RAGE);
                        if (lRage > 100)                    // rage stored as rage*10
                            lRage = 100;
                        target->ModifyPower(POWER_RAGE, -lRage);
                        int32 FRTriggerBasePoints = int32(lRage * LifePerRage / 10);
                        target->CastCustomSpell(target, 22845, &FRTriggerBasePoints, nullptr, nullptr, true, nullptr, this);
                        return;
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_SHAMAN:
            {
                switch (auraId)
                {
                    case 28820:                             // Lightning Shield (The Earthshatterer set trigger after cast Lighting Shield)
                    {
                        // Need remove self if Lightning Shield not active
                        Unit::SpellAuraHolderMap const& auras = triggerTarget->GetSpellAuraHolderMap();
                        for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                        {
                            SpellEntry const* spell = itr->second->GetSpellProto();
                            if (spell->IsFitToFamily<SPELLFAMILY_SHAMAN, CF_SHAMAN_LIGHTNING_SHIELD>())
                                return;
                        }
                        triggerTarget->RemoveAurasDueToSpell(28820);
                        return;
                    }
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }

        // Reget trigger spell proto
        triggeredSpellInfo = sSpellMgr.GetSpellEntry(trigger_spell_id);
    }
    else
    {
        // Spell exist but require custom code
        switch (auraId)
        {
            case 7054:
                spellRandom = urand(0, 14) + 7038;
                target->CastSpell(target, spellRandom, true, nullptr, this);
                return;
                break;
            case 8892:  // Goblin Rocket Boots
            case 13141: // Gnomish Rocket Boots
                // 20 ticks, et une chance sur 5 d'exploser.
                if (urand(0, 20 * 5 - 1) != 0)
                    return;
                break;
            case 9347:                                      // Mortal Strike
            {
                // expected selection current fight target
                triggerTarget = GetTarget()->getVictim();
                if (!triggerTarget)
                    return;

                // avoid triggering for far target
                SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(triggeredSpellInfo->rangeIndex);
                float max_range = GetSpellMaxRange(srange);
                if (!triggerTarget->IsWithinDist(GetTarget(), max_range))
                    return;

                break;
            }
            case 1010:                                      // Curse of Idiocy
            {
                // TODO: spell casted by result in correct way mostly
                // BUT:
                // 1) target show casting at each triggered cast: target don't must show casting animation for any triggered spell
                //      but must show affect apply like item casting
                // 2) maybe aura must be replace by new with accumulative stat mods instead stacking

                // prevent cast by triggered auras
                if (casterGUID == triggerTarget->GetObjectGuid())
                    return;

                // stop triggering after each affected stats lost > 90
                int32 intelectLoss = 0;
                int32 spiritLoss = 0;

                Unit::AuraList const& mModStat = triggerTarget->GetAurasByType(SPELL_AURA_MOD_STAT);
                for (Unit::AuraList::const_iterator i = mModStat.begin(); i != mModStat.end(); ++i)
                {
                    if ((*i)->GetId() == 1010)
                    {
                        switch ((*i)->GetModifier()->m_miscvalue)
                        {
                            case STAT_INTELLECT:
                                intelectLoss += (*i)->GetModifier()->m_amount;
                                break;
                            case STAT_SPIRIT:
                                spiritLoss   += (*i)->GetModifier()->m_amount;
                                break;
                            default:
                                break;
                        }
                    }
                }

                if (intelectLoss <= -90 && spiritLoss <= -90)
                    return;

                break;
            }
            case 16191:                                     // Mana Tide
            {
                triggerTarget->CastCustomSpell(triggerTarget, trigger_spell_id, &m_modifier.m_amount, nullptr, nullptr, true, nullptr, this);
                return;
            }
            //Frost Trap Aura
            case 13810:
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;
                // Pour le talent hunt 'Piege' par exemple (chances de stun)
                caster->ProcDamageAndSpell(target, (PROC_FLAG_ON_TRAP_ACTIVATION | PROC_FLAG_SUCCESSFUL_POSITIVE_AOE_HIT | PROC_FLAG_SUCCESSFUL_AOE_SPELL_HIT), PROC_FLAG_NONE, PROC_EX_NORMAL_HIT, 1, BASE_ATTACK, GetSpellProto());
                return;
            }
        }
    }

    Unit* triggerCaster = triggerTarget;
    WorldObject* triggerTargetObject = nullptr;

    // for channeled spell cast applied from aura owner to channel target (persistent aura affects already applied to true target)
    // come periodic casts applied to targets, so need seelct proper caster (ex. 15790)
    if (IsChanneledSpell(GetSpellProto()) && GetSpellProto()->Effect[GetEffIndex()] != SPELL_EFFECT_PERSISTENT_AREA_AURA)
    {
        // interesting 2 cases: periodic aura at caster of channeled spell
        if (target->GetObjectGuid() == casterGUID)
        {
            triggerCaster = target;

            if (WorldObject* channelTarget = target->GetMap()->GetWorldObject(target->GetChannelObjectGuid()))
            {
                if (channelTarget->isType(TYPEMASK_UNIT))
                    triggerTarget = (Unit*)channelTarget;
                else
                    triggerTargetObject = channelTarget;
            }
        }
        // or periodic aura at caster channel target
        else if (Unit* caster = GetCaster())
        {
            if (target->GetObjectGuid() == caster->GetChannelObjectGuid())
            {
                triggerCaster = caster;
                triggerTarget = target;
            }
        }
    }

    // All ok cast by default case
    if (triggeredSpellInfo)
    {
        if (triggerTargetObject)
            triggerCaster->CastSpell(triggerTargetObject->GetPositionX(), triggerTargetObject->GetPositionY(), triggerTargetObject->GetPositionZ(),
                                     triggeredSpellInfo, true, nullptr, this, casterGUID);
        else
            triggerCaster->CastSpell(triggerTarget, triggeredSpellInfo, true, nullptr, this, casterGUID);
    }
    else
    {
        if (Unit* caster = GetCaster())
        {
            if (triggerTarget->GetTypeId() != TYPEID_UNIT || !sScriptMgr.OnEffectDummy(caster, GetId(), GetEffIndex(), (Creature*)triggerTarget))
                sLog.outError("Aura::TriggerSpell: Spell %u have 0 in EffectTriggered[%d], not handled custom case?", GetId(), GetEffIndex());
        }
    }
}

void Aura::TriggerSpellWithValue()
{
	ObjectGuid casterGuid = GetCasterGuid();
	Unit* target = GetTriggerTarget();

	if (!casterGuid || !target)
		return;

	// generic casting code with custom spells and target/caster customs
	uint32 trigger_spell_id = GetSpellProto()->EffectTriggerSpell[m_effIndex];
	int32  basepoints0 = GetModifier()->m_amount;

	target->CastCustomSpell(target, trigger_spell_id, &basepoints0, nullptr, nullptr, true, nullptr, this, casterGuid);
}

/*********************************************************/
/***                  AURA EFFECTS                     ***/
/*********************************************************/

void Aura::HandleAuraDummy(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    Unit *target = GetTarget();

    // AT APPLY
    if (apply)
    {
        switch (GetSpellProto()->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                switch (GetId())
                {
                    case 2584:                              // Waiting to Resurrect
                    {
                        // for cases where aura would re-apply and player is no longer in BG
                        if (Unit* caster = GetCaster())
                        {
                            if (Player* player = caster->ToPlayer())
                                if (!player->InBattleGround())
                                    player->RemoveAurasDueToSpell(2584);
                        }
                        return;                            
                    }
                    case 10255:                             // Stoned
                    {
                        if (Unit* caster = GetCaster())
                        {
                            if (caster->GetTypeId() != TYPEID_UNIT)
                                return;

                            caster->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            caster->addUnitState(UNIT_STAT_ROOT);
                        }
                        return;
                    }
                    case 13139:                             // net-o-matic
                    {
                        // root to self part of (root_target->charge->root_self sequence
                        if (Unit* caster = GetCaster())
                            caster->CastSpell(caster, 13138, true, nullptr, this);
                        GetHolder()->SetAuraDuration(0); // Remove aura (else stays for ever, and casts at login)
                        return;
                    }
                    case 26234:                             // Ragnaros Submerge Visual
                    {
                        if (Unit* caster = GetCaster())
                        {
                            caster->CastSpell(caster, 21107, true); // Ragnaros Submerge Fade
                            caster->CastSpell(caster, 21859, true); // Ragnaros Submerge Effect
                            caster->RemoveAurasDueToSpell(26234);   // Need remove Submerge Visual after apply
                        }
                        return;
                    }
                    case 22646:                             // Goblin Rocket Helmet
                    {
                        if (Unit* caster = GetCaster())
                            caster->CastSpell(caster, 13360, true);
                        return;
                    }
                    case 24596: // Intoxicating Venom
                    {
                        if (target)
                        {
                            m_isPeriodic            = true;
                            m_modifier.periodictime = 1000;
                        }
                        return;
                    }
					case 31606:                             // Stormcrow Amulet
					{
						CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(17970);

						// we must assume db or script set display id to native at ending flight (if not, target is stuck with this model)
						if (cInfo)
							target->SetDisplayId(Creature::ChooseDisplayId(cInfo));

						return;
					}
					case 32045:                             // Soul Charge
					case 32051:
					case 32052:
					{
						// max duration is 2 minutes, but expected to be random duration
						// real time randomness is unclear, using max 30 seconds here
						// see further down for expire of this aura
						GetHolder()->SetAuraDuration(urand(1, 30)*IN_MILLISECONDS);
						return;
					}
					case 32441:                             // Brittle Bones
					{
						m_isPeriodic = true;
						m_modifier.periodictime = 10 * IN_MILLISECONDS; // randomly applies Rattled 32437
						m_periodicTimer = 0;
						return;
					}
					case 33326:                             // Stolen Soul Dispel
					{
						target->RemoveAurasDueToSpell(32346);
						return;
					}
					case 36587:                             // Vision Guide
					{
						target->CastSpell(target, 36573, true, nullptr, this);
						return;
					}
                }
                break;
            }
            case SPELLFAMILY_SHAMAN:
                break;
        }
    }
    // AT REMOVE
    else
    {
        if (IsQuestTameSpell(GetId()) && target->isAlive())
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->isAlive())
                return;

            uint32 finalSpellId = 0;
            switch (GetId())
            {
                case 19548:
                    finalSpellId = 19597;
                    break;
                case 19674:
                    finalSpellId = 19677;
                    break;
                case 19687:
                    finalSpellId = 19676;
                    break;
                case 19688:
                    finalSpellId = 19678;
                    break;
                case 19689:
                    finalSpellId = 19679;
                    break;
                case 19692:
                    finalSpellId = 19680;
                    break;
                case 19693:
                    finalSpellId = 19684;
                    break;
                case 19694:
                    finalSpellId = 19681;
                    break;
                case 19696:
                    finalSpellId = 19682;
                    break;
                case 19697:
                    finalSpellId = 19683;
                    break;
                case 19699:
                    finalSpellId = 19685;
                    break;
                case 19700:
                    finalSpellId = 19686;
                    break;
				case 30646: 
					finalSpellId = 30647; 
					break;
				case 30653: 
					finalSpellId = 30648; 
					break;
				case 30654: 
					finalSpellId = 30652; 
					break;
				case 30099: 
					finalSpellId = 30100; 
					break;
				case 30102: 
					finalSpellId = 30103; 
					break;
				case 30105: 
					finalSpellId = 30104; 
					break;
            }

            if (finalSpellId)
                caster->CastSpell(target, finalSpellId, true, nullptr, this);

            return;
        }

        switch (GetId())
        {
            case 126: // Kilrogg eye
                if (Unit* caster = GetCaster())
                    if (Player* casterPlayer = caster->ToPlayer())
                        if (Pet* guardian = caster->FindGuardianWithEntry(4277))
                        {
                            casterPlayer->ModPossessPet(guardian, false, AURA_REMOVE_BY_DEFAULT);
                            guardian->DisappearAndDie();
                        }
                return;
            case 10255:                                     // Stoned
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->GetTypeId() != TYPEID_UNIT)
                        return;
                    caster->clearUnitState(UNIT_STAT_ROOT | UNIT_STAT_PENDING_ROOT);
                    caster->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                }
                return;
            }
            case 11826:
                if (m_removeMode != AURA_REMOVE_BY_EXPIRE)
                    return;

                if (Unit* caster = GetCaster())
                    if (caster->GetTypeId() == TYPEID_PLAYER)
                        caster->CastSpell(target, 11828, true, caster->ToPlayer()->GetItemByGuid(GetCastItemGuid()), this);
                break;
            case 12479:                                     // Hex of Jammal'an
                target->CastSpell(target, 12480, true, nullptr, this);
                return;
            case 12774:                                     // (DND) Belnistrasz Idol Shutdown Visual
            {
                if (m_removeMode == AURA_REMOVE_BY_DEATH)
                    return;

                // Idom Rool Camera Shake <- wtf, don't drink while making spellnames?
                if (Unit* caster = GetCaster())
                    caster->CastSpell(caster, 12816, true);

                return;
            }
			case 17189:                                     // Frostwhisper's Lifeblood
															// Ras Frostwhisper gets back to full health when turned to his human form
				if (Unit* caster = GetCaster())
					caster->ModifyHealth(caster->GetMaxHealth() - caster->GetHealth());
				return;
            case 24906:                                     // Emeriss Aura
            {
                if (m_removeMode == AURA_REMOVE_BY_DEATH)
                    target->CastSpell(target, 24904, true, nullptr, this);

                return;
            }
            case 25042:                                     // Mark of Nature
            {
                if (m_removeMode == AURA_REMOVE_BY_DEATH)
                    target->CastSpell(target, 25040, true, nullptr, this);
                return;
            }
            case 23183:                                     // Mark of Frost
            {
                if (m_removeMode == AURA_REMOVE_BY_DEATH)
                    target->CastSpell(target, 23182, true, nullptr, this);
                    return;
            }
            case 28169:                                     // Mutating Injection
            {
                // Mutagen Explosion
                target->CastSpell(target, 28206, true, nullptr, this);
                // Poison Cloud
                target->CastSpell(target, 28240, true, nullptr, this);
                return;
            }
            case 24324: // Ivina < Nostalrius > : Hakkar
            {
                target->RemoveAurasDueToSpell(24321);
                return;
            }
			case 24596:
			{
				((Player*)target)->SetDrunkValue(0);
				return;
			}
			case 25185:  //itch
			{
				target->CastSpell(target, 25187, true, nullptr, this);
				return;
			}
			case 32045:                                     // Soul Charge
			{
				if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
					target->CastSpell(target, 32054, true, nullptr, this);

				return;
			}
			case 32051:                                     // Soul Charge
			{
				if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
					target->CastSpell(target, 32057, true, nullptr, this);

				return;
			}
			case 32052:                                     // Soul Charge
			{
				if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
					target->CastSpell(target, 32053, true, nullptr, this);

				return;
			}
			case 32286:                                     // Focus Target Visual
			{
				if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
					target->CastSpell(target, 32301, true, nullptr, this);

				return;
			}
			case 35079:                                     // Misdirection, triggered buff
			{
				if (Unit* pCaster = GetCaster())
					pCaster->RemoveAurasDueToSpell(34477);
				return;
			}
			case 36730:                                     // Flame Strike
			{
				target->CastSpell(target, 36731, true, nullptr, this);
				return;
			}
        }

        if (m_removeMode == AURA_REMOVE_BY_DEATH)
        {
            // Stop caster Arcane Missle chanelling on death
            if (GetSpellProto()->IsFitToFamily<SPELLFAMILY_MAGE, CF_MAGE_ARCANE_MISSILES_CHANNEL>())
            {
                if (Unit* caster = GetCaster())
                    caster->InterruptSpell(CURRENT_CHANNELED_SPELL);

                return;
            }
        }
    }

    // AT APPLY & REMOVE

    switch (GetSpellProto()->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (GetId())
            {
                case 6606:                                  // Self Visual - Sleep Until Cancelled (DND)
                {
                    if (apply)
                    {
                        target->SetStandState(UNIT_STAND_STATE_SLEEP);
                        target->addUnitState(UNIT_STAT_ROOT);
                    }
                    else
                    {
                        target->clearUnitState(UNIT_STAT_ROOT);
                        target->SetStandState(UNIT_STAND_STATE_STAND);
                    }

                    return;
                }
                case 24658:                                 // Unstable Power
                {
                    if (apply)
                    {
                        Unit* caster = GetCaster();
                        if (!caster)
                            return;

                        caster->CastSpell(target, 24659, true, nullptr, nullptr, GetCasterGuid());
                    }
                    else
                        target->RemoveAurasDueToSpell(24659);
                    return;
                }
                case 24661:                                 // Restless Strength
                {
                    if (apply)
                    {
                        Unit* caster = GetCaster();
                        if (!caster)
                            return;

                        caster->CastSpell(target, 24662, true, nullptr, nullptr, GetCasterGuid());
                    }
                    else
                        target->RemoveAurasDueToSpell(24662);
                    return;
                }
                case 29266:                                 // Permanent Feign Death
				case 35356:                                 // Spawn Feign Death
				case 35357:                                 // Spawn Feign Death
                {
                    // Unclear what the difference really is between them.
                    // Some has effect1 that makes the difference, however not all.
                    // Some appear to be used depending on creature location, in water, at solid ground, in air/suspended, etc
                    // For now, just handle all the same way
                    if (target->GetTypeId() == TYPEID_UNIT)
                        target->SetFeignDeath(apply);

                    return;
                }
                case 10848: // Shroud of Death
                case 22650: // Ghost Visual
                case 27978: // Shroud of Death
                    if (apply)
                        target->m_AuraFlags |= UNIT_AURAFLAG_ALIVE_INVISIBLE;
                    else
                        target->m_AuraFlags |= ~UNIT_AURAFLAG_ALIVE_INVISIBLE;
                    return;
				case 32216:                                 // Victorious
					if (target->getClass() == CLASS_WARRIOR)
						target->ModifyAuraState(AURA_STATE_WARRIOR_VICTORY_RUSH, apply);
					return;
				case 40133:                                 // Summon Fire Elemental
				{
					Unit* caster = GetCaster();
					if (!caster)
						return;

					Unit* owner = caster->GetOwner();
					if (owner && owner->GetTypeId() == TYPEID_PLAYER)
					{
						if (apply)
							owner->CastSpell(owner, 8985, true);
						else
							((Player*)owner)->RemovePet(PET_SAVE_REAGENTS);
					}
					return;
				}
				case 40132:                                 // Summon Earth Elemental
				{
					Unit* caster = GetCaster();
					if (!caster)
						return;

					Unit* owner = caster->GetOwner();
					if (owner && owner->GetTypeId() == TYPEID_PLAYER)
					{
						if (apply)
							owner->CastSpell(owner, 19704, true);
						else
							((Player*)owner)->RemovePet(PET_SAVE_REAGENTS);
					}
					return;
				}
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
			switch (GetId())
			{
			case 34246:                                 // Idol of the Emerald Queen
			{
				if (target->GetTypeId() != TYPEID_PLAYER)
					return;

				if (apply)
					// dummy not have proper effectclassmask
					m_spellmod = new SpellModifier(SPELLMOD_DOT, SPELLMOD_FLAT, m_modifier.m_amount / 7, GetId(), uint64(0x001000000000));

				((Player*)target)->AddSpellMod(m_spellmod, apply);
				return;
			}
			}

			// Lifebloom
			if (GetSpellProto()->SpellFamilyFlags & uint64(0x1000000000))
			{
				if (apply)
				{
					if (Unit* caster = GetCaster())
					{
						// prevent double apply bonuses
						if (target->GetTypeId() != TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
						{
							// Lifebloom ignore stack amount
							m_modifier.m_amount /= GetStackAmount();
							m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
							m_modifier.m_amount = target->SpellHealingBonusTaken(caster, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
						}
					}
				}
				else
				{
					// Final heal on duration end
					if (m_removeMode != AURA_REMOVE_BY_EXPIRE)
						return;

					// final heal
					if (target->IsInWorld() && GetStackAmount() > 0)
					{
						// Lifebloom dummy store single stack amount always
						int32 amount = m_modifier.m_amount;
						target->CastCustomSpell(target, 54724, &amount, nullptr, nullptr, true, nullptr, this, GetCasterGuid());
					}
				}
				return;
			}
            // Predatory Strikes
            if (target->GetTypeId() == TYPEID_PLAYER && GetSpellProto()->SpellIconID == 1563)
            {
                ((Player*)target)->UpdateAttackPowerAndDamage();
                return;
            }
            // Enrage
            if ((target->GetTypeId() == TYPEID_PLAYER) && (GetId() == 5229))
            {
                ((Player*)target)->UpdateArmor(); // Spell managed in UpdateArmor()
                return;
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
            break;
        case SPELLFAMILY_HUNTER:
            break;
        case SPELLFAMILY_SHAMAN:
        {
            switch (GetId())
            {
                case 6495:                                  // Sentry Totem
                {
                    if (target->GetTypeId() != TYPEID_PLAYER)
                        return;

                    Totem* totem = target->GetTotem(TOTEM_SLOT_AIR);

                    if (totem && apply)
                        ((Player*)target)->GetCamera().SetView(totem);
                    else
                        ((Player*)target)->GetCamera().ResetView();

                    return;
                }
				case 974:                                  // Earth Shield				
				case 32593:
				case 32594:
				{
					// prevent double apply bonuses
					if (target->GetTypeId() != TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
					{
						if (Unit* caster = GetCaster())
						{
							m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
							m_modifier.m_amount = target->SpellHealingBonusTaken(caster, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
						}
					}
					return;
				}
            }
            // Improved Weapon Totems
            if (GetSpellProto()->SpellIconID == 57 && target->GetTypeId() == TYPEID_PLAYER)
            {
                if (apply)
                {
                    switch (m_effIndex)
                    {
                        case 0:
                            // Windfury Totem
                            m_spellmod = new SpellModifier(SPELLMOD_EFFECT1, SPELLMOD_PCT, m_modifier.m_amount, GetId(), UI64LIT(0x00200000000));
                            break;
                        case 1:
                            // Flametongue Totem
                            m_spellmod = new SpellModifier(SPELLMOD_EFFECT1, SPELLMOD_PCT, m_modifier.m_amount, GetId(), UI64LIT(0x00400000000));
                            break;
                        default:
                            return;
                    }
                }

                ((Player*)target)->AddSpellMod(m_spellmod, apply);
                return;
            }
            break;
        }
    }

    // pet auras
    if (PetAura const* petSpell = sSpellMgr.GetPetAura(GetId()))
    {
        if (apply)
            target->AddPetAura(petSpell);
        else
            target->RemovePetAura(petSpell);
        return;
    }

    if (GetEffIndex() == EFFECT_INDEX_0 && target->GetTypeId() == TYPEID_PLAYER)
    {
        SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAuraMapBounds(GetId());
        if (saBounds.first != saBounds.second)
        {
            uint32 zone, area;
            target->GetZoneAndAreaId(zone, area);

            for (SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
            {
                // some auras remove at aura remove
                if (!itr->second->IsFitToRequirements((Player*)target, zone, area))
                    target->RemoveAurasDueToSpell(itr->second->spellId);
                // some auras applied at aura apply
                else if (itr->second->autocast)
                {
                    if (!target->HasAura(itr->second->spellId, EFFECT_INDEX_0))
                        target->CastSpell(target, itr->second->spellId, true);
                }
            }
        }
    }

    // script has to "handle with care", only use where data are not ok to use in the above code.
    if (target->GetTypeId() == TYPEID_UNIT)
        sScriptMgr.OnAuraDummy(this, apply);
}

void Aura::HandleAuraMounted(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (apply)
    {
        CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(m_modifier.m_miscvalue);
        if (!ci)
        {
            sLog.outErrorDb("AuraMounted: `creature_template`='%u' not found in database (only need it modelid)", m_modifier.m_miscvalue);
            return;
        }

        uint32 display_id = Creature::ChooseDisplayId(ci);
        CreatureModelInfo const *minfo = sObjectMgr.GetCreatureModelRandomGender(display_id);
        if (minfo)
            display_id = minfo->modelid;

        target->Mount(display_id, GetId());
    }
    else
        target->Unmount(true);
}

void Aura::HandleAuraWaterWalk(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    WorldPacket data;
    if (apply)
        data.Initialize(SMSG_MOVE_WATER_WALK, 8 + 4);
    else
        data.Initialize(SMSG_MOVE_LAND_WALK, 8 + 4);
    data << GetTarget()->GetPackGUID();
    data << uint32(0);

    if (Player* t = GetTarget()->ToPlayer())
    {
        t->GetSession()->SendPacket(&data);
        t->GetCheatData()->OrderSent(&data);
    }
    else
        GetTarget()->SendMovementMessageToSet(std::move(data), true);
}

void Aura::HandleAuraFeatherFall(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;
    WorldPacket data;
    if (apply)
        data.Initialize(SMSG_MOVE_FEATHER_FALL, 8 + 4);
    else
        data.Initialize(SMSG_MOVE_NORMAL_FALL, 8 + 4);
    data << GetTarget()->GetPackGUID();
    data << uint32(0);

    if (Player* t = GetTarget()->ToPlayer())
    {
        t->GetSession()->SendPacket(&data);
        t->GetCheatData()->OrderSent(&data);
        // start fall from current height
        if (!apply)
            t->SetFallInformation(0, t->GetPositionZ());
    }
    else
        GetTarget()->SendMovementMessageToSet(std::move(data), true);
}

void Aura::HandleAuraHover(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    WorldPacket data;
    if (apply)
        data.Initialize(SMSG_MOVE_SET_HOVER, 8 + 4);
    else
        data.Initialize(SMSG_MOVE_UNSET_HOVER, 8 + 4);
    data << GetTarget()->GetPackGUID();
    data << uint32(0);
    GetTarget()->SendMovementMessageToSet(std::move(data), true);
    if (Player* t = GetTarget()->ToPlayer())
        t->GetCheatData()->OrderSent(&data);
}

void Aura::HandleWaterBreathing(bool /*apply*/, bool /*Real*/)
{
    // update timers in client
    if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
        ((Player*)GetTarget())->UpdateMirrorTimers();
}

void Aura::HandleAuraModShapeshift(bool apply, bool Real)
{
    uint32 modelid = 0;
    float mod_x = 0.0f;
    Powers PowerType = POWER_MANA;
    ShapeshiftForm form = ShapeshiftForm(m_modifier.m_miscvalue);

    Unit *target = GetTarget();

    SpellShapeshiftFormEntry const* ssEntry = sSpellShapeshiftFormStore.LookupEntry(form);
    if (!ssEntry)
    {
        sLog.outError("Unknown shapeshift form %u in spell %u", form, GetId());
        return;
    }

    switch (form)
    {
        case FORM_CAT:
        {
            if (Player::TeamForRace(target->getRace()) == ALLIANCE)
                modelid = 892;
            else
                modelid = 8571;
            PowerType = POWER_ENERGY;
            break;
        }
        case FORM_TRAVEL:
            modelid = 632;
            break;
        case FORM_AQUA:
            modelid = 2428;
            break;
        case FORM_BEAR:
        {
            if (Player::TeamForRace(target->getRace()) == ALLIANCE)
                modelid = 2281;
            else
                modelid = 2289;
            PowerType = POWER_RAGE;
            break;
        }
        case FORM_GHOUL:
            if (Player::TeamForRace(target->getRace()) == ALLIANCE)
                modelid = 10045;
            break;
        case FORM_DIREBEAR:
        {
            if (Player::TeamForRace(target->getRace()) == ALLIANCE)
                modelid = 2281;
            else
                modelid = 2289;
            PowerType = POWER_RAGE;
            break;
        }
        case FORM_CREATUREBEAR:
            modelid = 902;
            break;
        case FORM_GHOSTWOLF:
            modelid = 4613;
            break;
        case FORM_MOONKIN:
        {
            if (Player::TeamForRace(target->getRace()) == ALLIANCE)
                modelid = 15374;
            else
                modelid = 15375;
            break;
        }
        case FORM_AMBIENT:
        case FORM_SHADOW:
        case FORM_STEALTH:
            break;
        case FORM_TREE:
            modelid = 864;
            break;
        case FORM_BATTLESTANCE:
        case FORM_BERSERKERSTANCE:
        case FORM_DEFENSIVESTANCE:
            PowerType = POWER_RAGE;
            break;
        case FORM_SPIRITOFREDEMPTION:
            modelid = 16031;
            break;
        default:
            break;
    }

    // remove polymorph before changing display id to keep new display id
    if (Real)
    {
        switch (form)
        {
            case FORM_CAT:
            case FORM_TREE:
            case FORM_TRAVEL:
            case FORM_AQUA:
            case FORM_BEAR:
            case FORM_DIREBEAR:
            case FORM_MOONKIN:
            {
                // remove movement affects
                target->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT, GetHolder());
                Unit::AuraList const& slowingAuras = target->GetAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);
                for (Unit::AuraList::const_iterator iter = slowingAuras.begin(); iter != slowingAuras.end();)
                {
                    SpellEntry const* aurSpellInfo = (*iter)->GetSpellProto();

                    uint32 aurMechMask = aurSpellInfo->GetAllSpellMechanicMask();

                    // If spell that caused this aura has Croud Control or Daze effect
                    if ((aurMechMask & MECHANIC_NOT_REMOVED_BY_SHAPESHIFT) ||
                            // some Daze spells have these parameters instead of MECHANIC_DAZE (skip snare spells)
                            (aurSpellInfo->SpellIconID == 15 && aurSpellInfo->Dispel == 0 &&
                            (aurMechMask & (1 << (MECHANIC_SNARE - 1))) == 0))
                    {
                        ++iter;
                        continue;
                    }

                    // All OK, remove aura now
                    target->RemoveAurasDueToSpellByCancel(aurSpellInfo->Id);
                    iter = slowingAuras.begin();
                }

                break;
            }
            default:
                break;
        }
    }

    if (modelid > 0 && !target->getTransForm())
    {
        // fix Tauren shapeshift scaling
        if (target->getRace() == RACE_TAUREN)
        {
            if (target->getGender() == GENDER_MALE)
                mod_x = -25.9f; // 0.741 * 1.35 ~= 1.0
            else
                mod_x = -20.0f; // 0.8 * 1.25    = 1.0
        }
        
        if (apply)
            target->SetDisplayId(modelid);
        else
            target->SetDisplayId(target->GetNativeDisplayId());
        target->ApplyPercentModFloatValue(OBJECT_FIELD_SCALE_X, mod_x, apply);
    }
    
    if (!Real)
        return;
    
    if (apply)
    {
        // remove other shapeshift before applying a new one
        target->RemoveSpellsCausingAura(SPELL_AURA_MOD_SHAPESHIFT, GetHolder());

        if (PowerType != POWER_MANA)
        {
            // reset power to default values only at power change
            uint32 powaa = target->GetPower(PowerType);
            if (target->getPowerType() != PowerType)
                target->setPowerType(PowerType);

            switch (form)
            {
                case FORM_CAT:
                case FORM_BEAR:
                case FORM_DIREBEAR:
                {
                    // get furor proc chance
                    int32 furorChance = 0;
                    Unit::AuraList const& mDummy = target->GetAurasByType(SPELL_AURA_DUMMY);
                    for (Unit::AuraList::const_iterator i = mDummy.begin(); i != mDummy.end(); ++i)
                    {
                        if ((*i)->GetSpellProto()->SpellIconID == 238)
                        {
                            furorChance = (*i)->GetModifier()->m_amount;
                            break;
                        }
                    }

                    if (m_modifier.m_miscvalue == FORM_CAT)
                    {
                        target->SetPower(POWER_ENERGY, 0);
                        if (irand(1, 100) <= furorChance)
                            target->CastSpell(target, 17099, true, nullptr, this);
                    }
                    else
                    {
                        target->SetPower(POWER_RAGE, powaa);
                        if (irand(1, 100) <= furorChance)
                            target->CastSpell(target, 17057, true, nullptr, this);
                    }
                    break;
                }
                case FORM_BATTLESTANCE:
                case FORM_DEFENSIVESTANCE:
                case FORM_BERSERKERSTANCE:
                {
                    uint32 Rage_val = 0;
                    //Tactical mastery
                    if (target->GetTypeId() == TYPEID_PLAYER)
                    {
                        Unit::AuraList const& aurasOverrideClassScripts = target->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                        for (Unit::AuraList::const_iterator iter = aurasOverrideClassScripts.begin(); iter != aurasOverrideClassScripts.end(); ++iter)
                        {
                            // select by script id
                            switch ((*iter)->GetModifier()->m_miscvalue)
                            {
                                case 831:
                                    Rage_val =  50;
                                    break;
                                case 832:
                                    Rage_val = 100;
                                    break;
                                case 833:
                                    Rage_val = 150;
                                    break;
                                case 834:
                                    Rage_val = 200;
                                    break;
                                case 835:
                                    Rage_val = 250;
                                    break;
                            }
                            if (Rage_val != 0)
                                break;
                        }
                    }
                    if (target->GetPower(POWER_RAGE) > Rage_val)
                        target->SetPower(POWER_RAGE, Rage_val);
                    break;
                }
                default:
                    break;
            }
        }

        target->SetShapeshiftForm(form);
    }
    else
    {
        if (target->getClass() == CLASS_DRUID)
        {
            target->setPowerType(POWER_MANA);
            target->SetPower(POWER_RAGE, 0);
        }

        target->SetShapeshiftForm(FORM_NONE);
    }

    // adding/removing linked auras
    // add/remove the shapeshift aura's boosts
    HandleShapeshiftBoosts(apply);
    target->UpdateModelData();

    if (target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)target)->InitDataForForm();
}

void Aura::HandleAuraTransform(bool apply, bool Real)
{
    Unit *target = GetTarget();
    if (apply)
    {
        uint32 model_id;
        
        // Discombobulate removes mount auras.
        if (GetId() == 4060 && Real)
            target->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
        
        // update active transform spell only not set or not overwriting negative by positive case
        if (!target->getTransForm() || !IsPositiveSpell(GetId()) || IsPositiveSpell(target->getTransForm()))
        {
            if (GetId() == 23603)   // Ustaag <Nostalrius> : Nefarian Class Call Mage
            {
                int rand = 0;
                rand = urand(0, 2);
                switch (rand)
                {
                    case 0:
                        model_id = 1060;
                        break;
                    case 1:
                        model_id = 4473;
                        break;
                    case 2:
                        model_id = 7898;
                        break;
                }
            }
            else if (m_modifier.m_miscvalue == 0)         // special case (spell specific functionality)
            {
                switch (GetId())
                {
                    case 16739:                                 // Orb of Deception
                    {
                        uint32 orb_model = target->GetNativeDisplayId();
                        switch (orb_model)
                        {
                            // Troll Female
                            case 1479:
                                model_id = 10134;
                                break;
                            // Troll Male
                            case 1478:
                                model_id = 10135;
                                break;
                            // Tauren Male
                            case 59:
                                model_id = 10136;
                                break;
                            // Human Male
                            case 49:
                                model_id = 10137;
                                break;
                            // Human Female
                            case 50:
                                model_id = 10138;
                                break;
                            // Orc Male
                            case 51:
                                model_id = 10139;
                                break;
                            // Orc Female
                            case 52:
                                model_id = 10140;
                                break;
                            // Dwarf Male
                            case 53:
                                model_id = 10141;
                                break;
                            // Dwarf Female
                            case 54:
                                model_id = 10142;
                                break;
                            // NightElf Male
                            case 55:
                                model_id = 10143;
                                break;
                            // NightElf Female
                            case 56:
                                model_id = 10144;
                                break;
                            // Undead Female
                            case 58:
                                model_id = 10145;
                                break;
                            // Undead Male
                            case 57:
                                model_id = 10146;
                                break;
                            // Tauren Female
                            case 60:
                                model_id = 10147;
                                break;
                            // Gnome Male
                            case 1563:
                                model_id = 10148;
                                break;
                            // Gnome Female
                            case 1564:
                                model_id = 10149;
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    default:
                        sLog.outError("Aura::HandleAuraTransform, spell %u does not have creature entry defined, need custom defined model.", GetId());
                        break;
                }
            }
            else
            {
                CreatureInfo const * ci = ObjectMgr::GetCreatureTemplate(m_modifier.m_miscvalue);
                if (!ci)
                {
                    model_id = 16358;                           // pig pink ^_^
                    sLog.outError("Auras: unknown creature id = %d (only need its modelid) Form Spell Aura Transform in Spell ID = %d", m_modifier.m_miscvalue, GetId());
                }
                else
                    model_id = Creature::ChooseDisplayId(ci);   // Will use the default model here

                // creature case, need to update equipment
                if (ci && target->GetTypeId() == TYPEID_UNIT)
                    ((Creature*)target)->LoadEquipment(ci->equipmentId, true);
            }

            //fix tauren scaling
            if (!target->getTransForm() && target->GetShapeshiftForm() == FORM_NONE && target->getRace() == RACE_TAUREN)
            {
                float mod_x = 0;
                if (target->getGender() == GENDER_MALE)
                    mod_x = -25.9f; // 0.741 * 1.35 ~= 1.0
                else
                    mod_x = -20.0f; // 0.8 * 1.25    = 1.0
                target->ApplyPercentModFloatValue(OBJECT_FIELD_SCALE_X, mod_x, apply);
            }
            
            target->SetDisplayId(model_id);
            target->setTransForm(GetId());
        }
    }
    else
    {
        //reset cosmetics only if it's the current transform
        if (target->getTransForm() == GetId())
        {
            target->setTransForm(0);
            target->SetDisplayId(target->GetNativeDisplayId());

            // apply default equipment for creature case
            if (target->GetTypeId() == TYPEID_UNIT)
                ((Creature*)target)->LoadEquipment(((Creature*)target)->GetCreatureInfo()->equipmentId, true);

            // re-apply some from still active with preference negative cases
            Unit::AuraList const& otherTransforms = target->GetAurasByType(SPELL_AURA_TRANSFORM);
            if (!otherTransforms.empty())
            {
                //fix tauren scaling
                if (target->getRace() == RACE_TAUREN && target->GetShapeshiftForm() == FORM_NONE)
                {
                    float mod_x = 0;
                    if (target->getGender() == GENDER_MALE)
                        mod_x = -25.9f; // 0.741 * 1.35 ~= 1.0
                    else
                        mod_x = -20.0f; // 0.8 * 1.25    = 1.0
                    target->ApplyPercentModFloatValue(OBJECT_FIELD_SCALE_X, mod_x, apply);
                }
            
                // look for other transform auras
                Aura* handledAura = *otherTransforms.rbegin();
                for (Unit::AuraList::const_reverse_iterator i = otherTransforms.rbegin(); i != otherTransforms.rend(); ++i)
                {
                    // negative auras are preferred
                    if (!IsPositiveSpell((*i)->GetSpellProto()->Id))
                    {
                        handledAura = *i;
                        break;
                    }
                }
                handledAura->HandleAuraTransform(true,false);
            }
            else //reapply shapeshifting, there should be only one.
            {
                //fix tauren scaling
                if (target->getRace() == RACE_TAUREN)
                {
                    float mod_x = 0;
                    if (target->getGender() == GENDER_MALE)
                        mod_x = -25.9f; // 0.741 * 1.35 ~= 1.0
                    else
                        mod_x = -20.0f; // 0.8 * 1.25    = 1.0
                    target->ApplyPercentModFloatValue(OBJECT_FIELD_SCALE_X, mod_x, apply);
                }
                
                Unit::AuraList const& shapeshift = target->GetAurasByType(SPELL_AURA_MOD_SHAPESHIFT);
                if (!shapeshift.empty() && !shapeshift.front()->IsInUse())
                    shapeshift.front()->HandleAuraModShapeshift(true,false);
            }
        }
    }
}

void Aura::HandleForceReaction(bool apply, bool Real)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if (!Real)
        return;

    Player* player = (Player*)GetTarget();

    uint32 faction_id = m_modifier.m_miscvalue;
    ReputationRank faction_rank = ReputationRank(m_modifier.m_amount);

    player->GetReputationMgr().ApplyForceReaction(faction_id, faction_rank, apply);
    player->GetReputationMgr().SendForceReactions();

    // stop fighting if at apply forced rank friendly or at remove real rank friendly
    if ((apply && faction_rank >= REP_FRIENDLY) || (!apply && player->GetReputationRank(faction_id) >= REP_FRIENDLY))
        player->StopAttackFaction(faction_id);

    if (!apply && player->GetZoneId() == 1377 && GetId() == 29519 && m_removeMode == AURA_REMOVE_BY_DEATH)
    {
        // OutdoorPVP Silithus : Perte du buff silithyste
        if (ZoneScript* pScript = player->GetZoneScript())
            pScript->HandleDropFlag(player, GetId());
    }
}

void Aura::HandleAuraModSkill(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    uint32 prot = GetSpellProto()->EffectMiscValue[m_effIndex];
	int32 points = GetModifierAmount(((Player*)GetTarget())->getLevel());

	//custom for Anticipation and similar spells;
	//float basePointsPerLevel = GetSpellProto()->EffectRealPointsPerLevel[m_effIndex];
	//points += int32(((Player*)GetTarget())->getLevel() * basePointsPerLevel);

    ((Player*)GetTarget())->ModifySkillBonus(prot, (apply ? points : -points), m_modifier.m_auraname == SPELL_AURA_MOD_SKILL_TALENT);
    if (prot == SKILL_DEFENSE)
        ((Player*)GetTarget())->UpdateDefenseBonusesMod();
}
void Aura::HandleChannelDeathItem(bool apply, bool Real)
{
    if (Real && !apply)
    {
        if (m_removeMode != AURA_REMOVE_BY_DEATH)
            return;
        // Item amount
        if (m_modifier.m_amount <= 0)
            return;

        SpellEntry const *spellInfo = GetSpellProto();
        if (spellInfo->EffectItemType[m_effIndex] == 0)
            return;

        Unit* victim = GetTarget();
        Unit* caster = GetCaster();
        if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
            return;
        // Demonistes : un seul fragment d'ame si on caste "Brulure de l'ombre" et "Siphon d'ame" sur une cible.
        if (spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK)
            if (victim->HasAuraTypeByCaster(SPELL_AURA_CHANNEL_DEATH_ITEM, caster->GetObjectGuid()))
                return;

        // Soul Shard (target req.)
        if (spellInfo->EffectItemType[m_effIndex] == 6265)
        {
            // Only from non-grey units
            if (!((Player*)caster)->isHonorOrXPTarget(victim) ||
                    (victim->GetTypeId() == TYPEID_UNIT && !victim->ToCreature()->IsTappedBy(caster->ToPlayer())))
                return;
        }

        //Adding items
        uint32 noSpaceForCount = 0;
        uint32 count = m_modifier.m_amount;

        ItemPosCountVec dest;
        InventoryResult msg = ((Player*)caster)->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, spellInfo->EffectItemType[m_effIndex], count, &noSpaceForCount);
        if (msg != EQUIP_ERR_OK)
        {
            count -= noSpaceForCount;
            ((Player*)caster)->SendEquipError(msg, nullptr, nullptr, spellInfo->EffectItemType[m_effIndex]);
            if (count == 0)
                return;
        }

        Item* newitem = ((Player*)caster)->StoreNewItem(dest, spellInfo->EffectItemType[m_effIndex], true);
        ((Player*)caster)->SendNewItem(newitem, count, true, true);
    }
}

void Aura::HandleBindSight(bool apply, bool /*Real*/)
{
    Unit* caster = GetCaster();
    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Camera& camera = ((Player*)caster)->GetCamera();
    if (apply)
        camera.SetView(GetTarget());
    else
        camera.ResetView();
}

void Aura::HandleFarSight(bool apply, bool /*Real*/)
{
    Unit* caster = GetCaster();
    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Camera& camera = ((Player*)caster)->GetCamera();
    if (apply)
        camera.SetView(GetTarget());
    else
        camera.ResetView();
}

void Aura::HandleAuraTrackCreatures(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if (apply)
        GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

    if (apply)
        GetTarget()->SetFlag(PLAYER_TRACK_CREATURES, uint32(1) << (m_modifier.m_miscvalue - 1));
    else
        GetTarget()->RemoveFlag(PLAYER_TRACK_CREATURES, uint32(1) << (m_modifier.m_miscvalue - 1));
}

void Aura::HandleAuraTrackResources(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if (apply)
        GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

    if (apply)
        GetTarget()->SetFlag(PLAYER_TRACK_RESOURCES, uint32(1) << (m_modifier.m_miscvalue - 1));
    else
        GetTarget()->RemoveFlag(PLAYER_TRACK_RESOURCES, uint32(1) << (m_modifier.m_miscvalue - 1));
}

void Aura::HandleAuraTrackStealthed(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if (apply)
        GetTarget()->RemoveNoStackAurasDueToAuraHolder(GetHolder());

    GetTarget()->ApplyModByteFlag(PLAYER_FIELD_BYTES, 0, PLAYER_FIELD_BYTE_TRACK_STEALTHED, apply);
}

void Aura::HandleAuraModScale(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyPercentModFloatValue(OBJECT_FIELD_SCALE_X, float(m_modifier.m_amount), apply);
    GetTarget()->UpdateModelData();
}

void Aura::HandleModPossess(bool apply, bool Real)
{
    if (!Real)
        return;

    Unit* target = GetTarget();
    Unit* caster = GetCaster();
    if (!caster || !target)
        return;
    caster->ModPossess(target, apply, m_removeMode);
}

void Unit::ModPossess(Unit* target, bool apply, AuraRemoveMode m_removeMode)
{
    // not possess yourself
    if (target == this)
        return;

    Unit* caster = this;

    Player* p_caster = nullptr;
    if (caster->IsCreature() && target->IsPlayer())
    {
        // Creature cast CM sur Joueur
    }
    else if (caster->IsPlayer())
        p_caster = (Player*)caster;
    else
        return;

    Camera& camera = p_caster->GetCamera();

    if (apply)
    {
        target->addUnitState(UNIT_STAT_CONTROLLED);

        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED | UNIT_FLAG_PVP_ATTACKABLE);
        target->SetCharmerGuid(caster->GetObjectGuid());
        target->setFaction(caster->getFaction());

        // target should became visible at SetView call(if not visible before):
        // otherwise client\p_caster will ignore packets from the target(SetClientControl for example)
        p_caster->GetCamera().SetView(target);

        caster->SetCharm(target);
        p_caster->SetMover(target);

        target->CombatStop(true);
        target->DeleteThreatList();

        if (CharmInfo *charmInfo = target->InitCharmInfo(target))
        {
            charmInfo->InitPossessCreateSpells();
            charmInfo->SetReactState(REACT_PASSIVE);
            charmInfo->SetCommandState(COMMAND_STAY);
        }

        p_caster->PossessSpellInitialize();

        if (Creature* pTargetCrea = target->ToCreature())
            if (pTargetCrea->AI()->SwitchAiAtControl())
                pTargetCrea->AIM_Initialize();

        if (Player* p_target = target->ToPlayer())
                p_target->SetControlledBy(caster);
        // Les mobs doivent attaquer celui qui est CM.
        // On appelle donc 'MoveInLineOfSight' pour les mobs a cote.
        target->ScheduleAINotify(0);
        target->UpdateControl();
        if (target->hasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_PENDING_STUNNED | UNIT_STAT_ROOT | UNIT_STAT_PENDING_ROOT))
            target->SetMovement(MOVE_ROOT);
        target->StopMoving();
    }
    else
    {
        // On transfert la menace vers celui qui a CM
        target->TransferAttackersThreatTo(caster);

        caster->InterruptSpell(CURRENT_CHANNELED_SPELL);  // the spell is not automatically canceled when interrupted, do it now

        p_caster->SetMover(nullptr);

        caster->SetCharm(nullptr);
        caster->UpdateControl();

        // there is a possibility that target became invisible for client\p_caster at ResetView call:
        // it must be called after movement control unapplying, not before! the reason is same as at aura applying
        p_caster->GetCamera().ResetView();
        p_caster->RemovePetActionBar();

        // on delete only do caster related effects
        if (m_removeMode == AURA_REMOVE_BY_DELETE)
            return;

        target->clearUnitState(UNIT_STAT_CONTROLLED);

        target->DeleteThreatList();

        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
        if (!target->GetAffectingPlayer())
            target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);

        target->SetCharmerGuid(ObjectGuid());

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            Player* p_target = ((Player*)target);
            p_target->setFactionForRace(target->getRace());
            p_target->RemoveAI();
            p_target->RelocateToLastClientPosition(); // Movement interpolation - prevent undermap.
        }
        else if (target->GetTypeId() == TYPEID_UNIT)
        {
            CreatureInfo const *cinfo = ((Creature*)target)->GetCreatureInfo();
            target->setFaction(cinfo->faction_A);
        }
        target->StopMoving();
        target->UpdateControl();

        if (Creature* pCreature = target->ToCreature())
        {
            if (pCreature->AI() && pCreature->AI()->SwitchAiAtControl())
                pCreature->AIM_Initialize();

            pCreature->AttackedBy(caster);
        }
    }
    target->SetUnitMovementFlags(MOVEFLAG_NONE);
}

void Aura::HandleModPossessPet(bool apply, bool Real)
{
    if (!Real)
        return;

    Unit* caster = GetCaster();
    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Unit* target = GetTarget();
    if (target->GetTypeId() != TYPEID_UNIT || !((Creature*)target)->IsPet())
        return;

    Pet* pet = (Pet*)target;

    Player* p_caster = (Player*)caster;
    p_caster->ModPossessPet(pet, apply, m_removeMode);
}

void Player::ModPossessPet(Pet* pet, bool apply, AuraRemoveMode m_removeMode)
{
    Player* p_caster = this;
    Camera& camera = p_caster->GetCamera();

    if (apply)
    {
        pet->addUnitState(UNIT_STAT_CONTROLLED);

        // target should became visible at SetView call(if not visible before):
        // otherwise client\p_caster will ignore packets from the target(SetClientControl for example)
        camera.SetView(pet);

        p_caster->SetCharm(pet);
        p_caster->SetMover(pet);

        pet->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
        pet->SetCharmerGuid(p_caster->GetObjectGuid());

        pet->StopMoving();
        pet->GetMotionMaster()->Clear(false);
        pet->GetMotionMaster()->MoveIdle();
        pet->UpdateControl();
    }
    else
    {
        p_caster->SetCharm(nullptr);
        p_caster->SetMover(nullptr);

        // there is a possibility that target became invisible for client\p_caster at ResetView call:
        // it must be called after movement control unapplying, not before! the reason is same as at aura applying
        camera.ResetView();
        pet->UpdateControl();
        pet->SetCharmerGuid(ObjectGuid());

        // on delete only do caster related effects
        if (m_removeMode == AURA_REMOVE_BY_DELETE)
            return;

        pet->clearUnitState(UNIT_STAT_CONTROLLED);

        pet->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

        //pet->AttackStop();

        // out of range pet dismissed
        if (!pet->IsWithinDistInMap(p_caster, pet->GetMap()->GetGridActivationDistance()))
            p_caster->RemovePet(PET_SAVE_REAGENTS);
        else if (!pet->isInCombat())
            pet->GetMotionMaster()->MoveFollow(p_caster, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
    }
}

void Aura::HandleModCharm(bool apply, bool Real)
{
    if (!Real)
        return;

    Unit *target = GetTarget();

    // not charm yourself
    if (GetCasterGuid() == target->GetObjectGuid())
        return;

    Unit* caster = GetCaster();
    if (!caster)
        return;

    if (apply)
    {
        // is it really need after spell check checks?
        target->RemoveSpellsCausingAura(SPELL_AURA_MOD_CHARM, GetHolder());
        target->RemoveSpellsCausingAura(SPELL_AURA_MOD_POSSESS, GetHolder());

        target->SetCharmerGuid(GetCasterGuid());
        target->setFaction(caster->getFaction());
        target->CastStop(target == caster ? GetId() : 0);
        caster->SetCharm(target);

        target->CombatStop(true);
        target->DeleteThreatList();
        target->getHostileRefManager().deleteReferences();

        CharmInfo *charmInfo = target->InitCharmInfo(target);
        charmInfo->InitCharmCreateSpells();
        charmInfo->SetReactState(REACT_DEFENSIVE);
        // Default movement is follow
        charmInfo->SetCommandState(COMMAND_FOLLOW);

        charmInfo->SetIsCommandAttack(false);
        charmInfo->SetIsAtStay(false);
        charmInfo->SetIsReturning(true);
        charmInfo->SetIsCommandFollow(true);
        charmInfo->SetIsFollowing(false);

        target->AttackStop();
        target->InterruptNonMeleeSpells(false);
        if (caster->GetTypeId() == TYPEID_PLAYER) // Units will make the controlled player attack (MoveChase)
            target->GetMotionMaster()->MoveFollow(caster, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);

        if (Creature* pCreaTarget = target->ToCreature())
        {
            if (pCreaTarget->AI() && pCreaTarget->AI()->SwitchAiAtControl())
                pCreaTarget->AIM_Initialize();
            if (caster->GetTypeId() == TYPEID_PLAYER && caster->getClass() == CLASS_WARLOCK)
            {
                CreatureInfo const *cinfo = ((Creature*)target)->GetCreatureInfo();
                if (cinfo && cinfo->type == CREATURE_TYPE_DEMON)
                {
                    // creature with pet number expected have class set
                    if (target->GetByteValue(UNIT_FIELD_BYTES_0, 1) == 0)
                    {
                        if (cinfo->unit_class == 0)
                            sLog.outErrorDb("Creature (Entry: %u) have unit_class = 0 but used in charmed spell, that will be result client crash.", cinfo->Entry);
                        else
                            sLog.outError("Creature (Entry: %u) have unit_class = %u but at charming have class 0!!! that will be result client crash.", cinfo->Entry, cinfo->unit_class);

                        target->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MAGE);
                    }

                    //just to enable stat window
                    charmInfo->SetPetNumber(sObjectMgr.GeneratePetNumber(), true);
                    //if charmed two demons the same session, the 2nd gets the 1st one's name
                    target->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(nullptr)));
                }
            }
        }
        else if (Player* pPlayer = target->ToPlayer())
        {
            PlayerAI *oldAi = pPlayer->i_AI;
            delete oldAi;
            pPlayer->i_AI = new PlayerControlledAI(pPlayer, caster);

            if (caster->GetTypeId() == TYPEID_UNIT)
                pPlayer->SetControlledBy(caster);
        }
        target->UpdateControl();
        if (caster->GetTypeId() == TYPEID_PLAYER)
            ((Player*)caster)->CharmSpellInitialize();
    }
    else
    {
        target->SetCharmerGuid(ObjectGuid());

        if (target->GetTypeId() == TYPEID_PLAYER)
            ((Player*)target)->setFactionForRace(target->getRace());
        else
        {
            CreatureInfo const *cinfo = ((Creature*)target)->GetCreatureInfo();

            // restore faction
            if (((Creature*)target)->IsPet())
            {
                if (Unit* owner = target->GetOwner())
                    target->setFaction(owner->getFaction());
                else if (cinfo)
                    target->setFaction(cinfo->faction_A);
            }
            else if (cinfo)                             // normal creature
                target->setFaction(cinfo->faction_A);

            // restore UNIT_FIELD_BYTES_0
            if (cinfo && caster->GetTypeId() == TYPEID_PLAYER && caster->getClass() == CLASS_WARLOCK && cinfo->type == CREATURE_TYPE_DEMON)
            {
                // DB must have proper class set in field at loading, not req. restore, including workaround case at apply
                // target->SetByteValue(UNIT_FIELD_BYTES_0, 1, cinfo->unit_class);

                if (target->GetCharmInfo())
                    target->GetCharmInfo()->SetPetNumber(0, true);
                else
                    sLog.outError("Aura::HandleModCharm: target (GUID: %u TypeId: %u) has a charm aura but no charm info!", target->GetGUIDLow(), target->GetTypeId());
            }
        }

        caster->SetCharm(nullptr);

        if (caster->GetTypeId() == TYPEID_PLAYER)
            ((Player*)caster)->RemovePetActionBar();

        target->UpdateControl();
        target->CombatStop(true);
        target->DeleteThreatList();
        target->getHostileRefManager().deleteReferences();

        target->SetUnitMovementFlags(MOVEFLAG_NONE);
        target->StopMoving();
        target->GetMotionMaster()->Clear(false);
        target->GetMotionMaster()->MoveIdle();

        if (Creature* pTargetCrea = target->ToCreature())
        {
            if (pTargetCrea->AI() && pTargetCrea->AI()->SwitchAiAtControl())
                pTargetCrea->AIM_Initialize();
            pTargetCrea->AttackedBy(caster);
        }
        else if (Player* pPlayer = target->ToPlayer())
            pPlayer->RemoveAI();
    }

	if (GetId() == 30019)
		target->SetTurningOff(apply);
}

void Aura::HandleModConfuse(bool apply, bool Real)
{
    if (!Real)
        return;

    GetTarget()->SetConfused(apply, GetCasterGuid(), GetId());
}

void Aura::HandleModFear(bool apply, bool Real)
{
    if (!Real)
        return;

    GetTarget()->SetFeared(apply, GetCasterGuid(), GetId());
}

void Aura::HandleFeignDeath(bool apply, bool Real)
{
    if (!Real)
        return;

    Unit* pTarget = GetTarget();
    // Toutes les personnes qui castent sur le casteur de FD doivent etre interrompues.
    if (apply)
        pTarget->InterruptSpellsCastedOnMe();

    pTarget->SetFeignDeath(apply, GetCasterGuid(), GetId());
}

void Aura::HandleAuraModDisarm(bool apply, bool Real)
{
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (!apply && target->HasAuraType(SPELL_AURA_MOD_DISARM))
        return;

    target->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED, apply);

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // main-hand attack speed already set to special value for feral form already and don't must change and reset at remove.
    if (target->IsInFeralForm())
        return;

    if (apply)
        target->SetAttackTime(BASE_ATTACK, BASE_ATTACK_TIME);
    else
        ((Player *)target)->SetRegularAttackTime();

    target->UpdateDamagePhysical(BASE_ATTACK);
}

void Aura::HandleAuraModStun(bool apply, bool Real)
{
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (apply)
    {
        // Stun/roots effects apply at charge end
        bool inCharge = target->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHARGE_MOTION_TYPE;
        // Frost stun aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
            target->ModifyAuraState(AURA_STATE_FROZEN, apply);

        target->addUnitState(inCharge ? UNIT_STAT_PENDING_STUNNED : UNIT_STAT_STUNNED);
        target->SetTargetGuid(ObjectGuid());

        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        target->CastStop(target->GetObjectGuid() == GetCasterGuid() ? GetId() : 0);
        if (target->GetObjectGuid() != GetCasterGuid())
            target->InterruptNonMeleeSpells(false);

        // Player specific
        if (target->GetTypeId() == TYPEID_PLAYER && !target->IsMounted())
            target->SetStandState(UNIT_STAND_STATE_STAND);// in 1.5 client

        if (!target->movespline->Finalized() || target->GetTypeId() == TYPEID_UNIT)
            if (!inCharge)
                target->StopMoving();

        target->SetMovement(MOVE_ROOT);
    }
    else
    {
        // Frost stun aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
        {
            bool found_another = false;
            for (AuraType const* itr = &frozenAuraTypes[0]; *itr != SPELL_AURA_NONE; ++itr)
            {
                Unit::AuraList const& auras = target->GetAurasByType(*itr);
                for (Unit::AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
                {
                    if (GetSpellSchoolMask((*i)->GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
                    {
                        found_another = true;
                        break;
                    }
                }
                if (found_another)
                    break;
            }

            if (!found_another)
                target->ModifyAuraState(AURA_STATE_FROZEN, apply);
        }

        // Real remove called after current aura remove from lists, check if other similar auras active
        if (target->HasAuraType(SPELL_AURA_MOD_STUN))
            return;

        target->clearUnitState(UNIT_STAT_STUNNED | UNIT_STAT_PENDING_STUNNED);
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);


        if (target->getVictim() && target->isAlive())
            target->SetTargetGuid(target->getVictim()->GetObjectGuid());

        if (!target->hasUnitState(UNIT_STAT_ROOT | UNIT_STAT_PENDING_ROOT))         // prevent allow move if have also root effect
            target->SetMovement(MOVE_UNROOT);
		//other Wyvern Sting
		if (GetId() == 24335)
		{
			target->CastSpell(target, 24336, true, nullptr, this);
			return;
		}

        // Wyvern Sting
        if (GetSpellProto()->IsFitToFamily<SPELLFAMILY_HUNTER, CF_HUNTER_MISC>())
        {
            Unit* caster = GetCaster();
            if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                return;

            uint32 spell_id = 0;

            switch (GetId())
            {
                case 19386:
                    spell_id = 24131;
                    break;
                case 24132:
                    spell_id = 24134;
                    break;
                case 24133:
                    spell_id = 24135;
                    break;
                default:
                    sLog.outError("Spell selection called for unexpected original spell %u, new spell for this spell family?", GetId());
                    return;
            }

            SpellEntry const* spellInfo = sSpellMgr.GetSpellEntry(spell_id);

            if (!spellInfo)
                return;

            caster->CastSpell(target, spellInfo, true, nullptr, this);
            return;
        }
    }
}

void Aura::HandleModStealth(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if (apply)
    {
        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            if (SpellAuraHolder *holder = ((Player*)target)->GetSpellAuraHolder(29519))
            {
                if (target->GetZoneId() == 1377)
                {
                    // OutdoorPVP Silithus : Perte du buff silithyste
                    if (ZoneScript* pScript = ((Player*)target)->GetZoneScript())
                        pScript->HandleDropFlag((Player*)target, 29519);
                }
            }
        }

        // drop flag at stealth in bg
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        // only at real aura add
        if (Real)
        {
            target->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAGS_CREEP);

            if (target->GetTypeId() == TYPEID_PLAYER)
                target->SetByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_STEALTH);

            // apply only if not in GM invisibility (and overwrite invisibility state)
            if (target->GetVisibility() != VISIBILITY_OFF)
            {
                target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
                target->SetVisibility(VISIBILITY_GROUP_STEALTH);
            }

            // for RACE_NIGHTELF stealth
            if (target->GetTypeId() == TYPEID_PLAYER && GetId() == 20580)
                target->CastSpell(target, 21009, true, nullptr, this);

            target->InterruptSpellsCastedOnMe();
        }
    }
    else
    {
        // for RACE_NIGHTELF stealth
        if (Real && target->GetTypeId() == TYPEID_PLAYER && GetId() == 20580)
            target->RemoveAurasDueToSpell(21009);

        // only at real aura remove of _last_ SPELL_AURA_MOD_STEALTH
        if (Real && !target->HasAuraType(SPELL_AURA_MOD_STEALTH))
        {
            // if no GM invisibility
            if (target->GetVisibility() != VISIBILITY_OFF)
            {
                target->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAGS_CREEP);

                if (target->GetTypeId() == TYPEID_PLAYER)
                    target->RemoveByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_STEALTH);

                // restore invisibility if any
                if (target->HasAuraType(SPELL_AURA_MOD_INVISIBILITY))
                {
                    target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
                    target->SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
                }
                else
                    target->SetVisibility(VISIBILITY_ON);
            }
			// apply full stealth period bonuses only at first stealth aura in stack
			if (target->GetAurasByType(SPELL_AURA_MOD_STEALTH).size() <= 1)
			{
				Unit::AuraList const& mDummyAuras = target->GetAurasByType(SPELL_AURA_DUMMY);
				for (Unit::AuraList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
				{
					// Master of Subtlety
					if ((*i)->GetSpellProto()->SpellIconID == 2114)
					{
						target->RemoveAurasDueToSpell(31666);
						int32 bp = (*i)->GetModifier()->m_amount;
						target->CastCustomSpell(target, 31665, &bp, nullptr, nullptr, true);
						break;
					}
				}
			}
        }
    }
}

void Aura::HandleInvisibility(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if (apply)
    {
        target->m_invisibilityMask |= (1 << m_modifier.m_miscvalue);
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        if (Real && target->GetTypeId() == TYPEID_PLAYER)
        {
            // apply glow vision
            target->SetByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);

        }

        // apply only if not in GM invisibility and not stealth
        if (target->GetVisibility() == VISIBILITY_ON)
        {
            // Aura not added yet but visibility code expect temporary add aura
            target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
            target->SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
        }
    }
    else
    {
        // recalculate value at modifier remove (current aura already removed)
        target->m_invisibilityMask = 0;
        Unit::AuraList const& auras = target->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY);
        for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            target->m_invisibilityMask |= (1 << (*itr)->GetModifier()->m_miscvalue);

        // only at real aura remove and if not have different invisibility auras.
        if (Real && target->m_invisibilityMask == 0)
        {
            // remove glow vision
            if (target->GetTypeId() == TYPEID_PLAYER)
                target->RemoveByteFlag(PLAYER_FIELD_BYTES2, 1, PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);

            // apply only if not in GM invisibility & not stealthed while invisible
            if (target->GetVisibility() != VISIBILITY_OFF)
            {
                // if have stealth aura then already have stealth visibility
                if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH))
                    target->SetVisibility(VISIBILITY_ON);
            }
        }
    }
}

void Aura::HandleInvisibilityDetect(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if (apply)
        target->m_detectInvisibilityMask |= (1 << m_modifier.m_miscvalue);
    else
    {
        // recalculate value at modifier remove (current aura already removed)
        target->m_detectInvisibilityMask = 0;
        Unit::AuraList const& auras = target->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY_DETECTION);
        for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            target->m_detectInvisibilityMask |= (1 << (*itr)->GetModifier()->m_miscvalue);
    }
    if (Real && target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)target)->GetCamera().UpdateVisibilityForOwner();
}

void Aura::HandleAuraModRoot(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (apply)
    {
        // Stun/roots effects apply at charge end
        bool inCharge = target->GetMotionMaster()->GetCurrentMovementGeneratorType() == CHARGE_MOTION_TYPE;
        // Frost root aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
            target->ModifyAuraState(AURA_STATE_FROZEN, apply);

        target->addUnitState(inCharge ? UNIT_STAT_PENDING_ROOT : UNIT_STAT_ROOT);

        //Save last orientation
        if (target->getVictim())
            target->SetOrientation(target->GetAngle(target->getVictim()));

        if (!target->movespline->Finalized() && !inCharge)
            target->StopMoving();

        target->SetMovement(MOVE_ROOT);
    }
    else
    {
        // Frost root aura -> freeze/unfreeze target
        if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
        {
            bool found_another = false;
            for (AuraType const* itr = &frozenAuraTypes[0]; *itr != SPELL_AURA_NONE; ++itr)
            {
                Unit::AuraList const& auras = target->GetAurasByType(*itr);
                for (Unit::AuraList::const_iterator i = auras.begin(); i != auras.end(); ++i)
                {
                    if (GetSpellSchoolMask((*i)->GetSpellProto()) & SPELL_SCHOOL_MASK_FROST)
                    {
                        found_another = true;
                        break;
                    }
                }
                if (found_another)
                    break;
            }

            if (!found_another)
                target->ModifyAuraState(AURA_STATE_FROZEN, apply);
        }

        // Real remove called after current aura remove from lists, check if other similar auras active
        if (target->HasAuraType(SPELL_AURA_MOD_ROOT))
            return;

        target->clearUnitState(UNIT_STAT_ROOT | UNIT_STAT_PENDING_ROOT);

        if (!target->hasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_PENDING_STUNNED))      // prevent allow move if have also stun effect
        {
            target->SetMovement(MOVE_UNROOT);
            target->SetUnitMovementFlags(MOVEFLAG_NONE);
        }
    }
}

void Aura::HandleAuraModSilence(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (apply)
    {
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
        // Stop cast only spells vs PreventionType == SPELL_PREVENTION_TYPE_SILENCE
        for (uint32 i = CURRENT_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
            if (Spell* spell = target->GetCurrentSpell(CurrentSpellTypes(i)))
                if (spell->m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
                    // Stop spells on prepare or casting state
                    target->InterruptSpell(CurrentSpellTypes(i), false);

        switch (GetId())
        {
            // Arcane Torrent (Energy)
            case 25046:
			{
				Unit * caster = GetCaster();
				if (!caster)
					return;

				// Search Mana Tap auras on caster
				Aura * dummy = caster->GetDummyAura(28734);
				if (dummy)
				{
					int32 bp = dummy->GetStackAmount() * 10;
					caster->CastCustomSpell(caster, 25048, &bp, nullptr, nullptr, true);
					caster->RemoveAurasDueToSpell(28734);
				}
				break;
			}		
			// Arcane Torrent (Rage)
			case 54636:
            {
                Unit * caster = GetCaster();
                if (!caster)
                    return;

                // Search Mana Tap auras on caster
                Aura * dummy = caster->GetDummyAura(54632);
                if (dummy)
                {
                    int32 bp = dummy->GetStackAmount() * 10;
                    caster->CastCustomSpell(caster, 25048, &bp, nullptr, nullptr, true);
					caster->RemoveAurasDueToSpell(54632);
                }
				break;
            }
        }
    }
    else
    {
        // Real remove called after current aura remove from lists, check if other similar auras active
        if (target->HasAuraType(SPELL_AURA_MOD_SILENCE))
            return;

        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
    }
}

void Aura::HandleModThreat(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    int level_diff = 0;
    int multiplier = 0;
    switch (GetId())
    {
        // Arcane Shroud
        case 26400:
            level_diff = target->getLevel() - 60;
            multiplier = 2;
            break;
        // The Eye of Diminution
        case 28862:
            level_diff = target->getLevel() - 60;
            multiplier = 1;
            break;
    }

    if (level_diff > 0)
        m_modifier.m_amount += multiplier * level_diff;

    if (target->GetTypeId() == TYPEID_PLAYER)
        for (int8 x = 0; x < MAX_SPELL_SCHOOL; x++)
            if (m_modifier.m_miscvalue & int32(1 << x))
                ApplyPercentModFloatVar(target->m_threatModifier[x], float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModTotalThreat(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (!target->isAlive() || target->GetTypeId() != TYPEID_PLAYER)
        return;

    Unit* caster = GetCaster();

    if (!caster || !caster->isAlive())
        return;

    target->getHostileRefManager().addTempThreat(m_modifier.m_amount, apply);
}

void Aura::HandleModTaunt(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    Unit *target = GetTarget();

    if (!target->isAlive() || !target->CanHaveThreatList())
        return;

    Unit* caster = GetCaster();

    if (!caster || !caster->isAlive())
        return;

    if (apply)
        target->TauntApply(caster);
    else
    {
        // When taunt aura fades out, mob will switch to previous target if current has less than 1.1 * secondthreat
        target->TauntFadeOut(caster);
    }
}

/*********************************************************/
/***                  MODIFY SPEED                     ***/
/*********************************************************/
void Aura::HandleAuraModIncreaseSpeed(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if (!Real)
        return;

    GetTarget()->UpdateSpeed(MOVE_RUN, true);
}

void Aura::HandleAuraModIncreaseMountedSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if (!Real)
        return;

    GetTarget()->UpdateSpeed(MOVE_RUN, true);
}

void Aura::HandleAuraModIncreaseSwimSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if (!Real)
        return;

    GetTarget()->UpdateSpeed(MOVE_SWIM, true);
}

void Aura::HandleAuraModDecreaseSpeed(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if (!Real)
        return;

    Unit* target = GetTarget();

    target->UpdateSpeed(MOVE_RUN, true);
    target->UpdateSpeed(MOVE_SWIM, true);
}

void Aura::HandleAuraModUseNormalSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if (!Real)
        return;

    Unit *target = GetTarget();

    target->UpdateSpeed(MOVE_RUN, true);
    target->UpdateSpeed(MOVE_SWIM, true);
}

/*********************************************************/
/***                     IMMUNITY                      ***/
/*********************************************************/

void Aura::HandleModMechanicImmunity(bool apply, bool /*Real*/)
{
    uint32 misc  = m_modifier.m_miscvalue;
    Unit *target = GetTarget();

    if (apply && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
        target->RemoveAurasAtMechanicImmunity(1 << (misc - 1), GetId());

    // Transfert ne doit pas appliquer d'immunite pendant 1sec, mais simplement dispel
    if (GetSpellProto()->DurationIndex == 36)
        return;

    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, misc, apply);

    // re-apply Fear Ward if it was not wasted during Bersereker Rage
    if (!apply && GetSpellProto()->IsFitToFamily<SPELLFAMILY_WARRIOR, CF_WARRIOR_BERSERKER_RAGE>())
    {
        if (target->HasAura(6346))
        {
            auto aura = target->GetAura(6346, EFFECT_INDEX_0);
            aura->HandleModMechanicImmunity(true, true);
        }
    }

	// Bestial Wrath
	if (GetSpellProto()->SpellFamilyName == SPELLFAMILY_HUNTER && GetSpellProto()->SpellIconID == 1680)
	{
		// The Beast Within cast on owner if talent present
		if (Unit* owner = target->GetOwner())
		{
			// Search talent The Beast Within
			Unit::AuraList const& dummyAuras = owner->GetAurasByType(SPELL_AURA_DUMMY);
			for (Unit::AuraList::const_iterator i = dummyAuras.begin(); i != dummyAuras.end(); ++i)
			{
				if ((*i)->GetSpellProto()->SpellIconID == 2229)
				{
					if (apply)
						owner->CastSpell(owner, 34471, true, nullptr, this);
					else
						owner->RemoveAurasDueToSpell(34471);
					break;
				}
			}
		}
	}
}

void Aura::HandleModMechanicImmunityMask(bool apply, bool /*Real*/)
{
    uint32 mechanic  = m_modifier.m_miscvalue;

    if (apply && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
        GetTarget()->RemoveAurasAtMechanicImmunity(mechanic, GetId());

    // check implemented in Unit::IsImmuneToSpell and Unit::IsImmuneToSpellEffect
}

//this method is called whenever we add / remove aura which gives m_target some imunity to some spell effect
void Aura::HandleAuraModEffectImmunity(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();

    // when removing flag aura, handle flag drop
    if (!apply && target->GetTypeId() == TYPEID_PLAYER
            && (GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION))
    {
        // En CM, si un ennemi nous met un boubou, on ne doit pas perdre le flag.
        if (!target->HasAuraType(SPELL_AURA_MOD_POSSESS))
            if (BattleGround *bg = ((Player*)target)->GetBattleGround())
                bg->EventPlayerDroppedFlag(((Player*)target));
    }

    target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModStateImmunity(bool apply, bool Real)
{
    if (apply && Real && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
    {
        Unit::AuraList const& auraList = GetTarget()->GetAurasByType(AuraType(m_modifier.m_miscvalue));
        for (Unit::AuraList::const_iterator itr = auraList.begin(); itr != auraList.end();)
        {
            if (auraList.front() != this)                   // skip itself aura (it already added)
            {
                GetTarget()->RemoveAurasDueToSpell(auraList.front()->GetId());
                itr = auraList.begin();
            }
            else
                ++itr;
        }
    }

    GetTarget()->ApplySpellImmune(GetId(), IMMUNITY_STATE, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModSchoolImmunity(bool apply, bool Real)
{
    Unit* target = GetTarget();
    target->ApplySpellImmune(GetId(), IMMUNITY_SCHOOL, m_modifier.m_miscvalue, apply);

    // remove all flag auras (they are positive, but they must be removed when you are immune)
    if (GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY
            && GetSpellProto()->AttributesEx2 & SPELL_ATTR_EX2_DAMAGE_REDUCED_SHIELD)
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

    // TODO: optimalize this cycle - use RemoveAurasWithInterruptFlags call or something else
    if (Real && apply
            && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY
            && IsPositiveSpell(GetId()))                        //Only positive immunity removes auras
    {
        uint32 school_mask = m_modifier.m_miscvalue;
        Unit::SpellAuraHolderMap& Auras = target->GetSpellAuraHolderMap();
        for (Unit::SpellAuraHolderMap::iterator iter = Auras.begin(), next; iter != Auras.end(); iter = next)
        {
            next = iter;
            ++next;
            SpellEntry const *spell = iter->second->GetSpellProto();
            if ((GetSpellSchoolMask(spell) & school_mask) //Check for school mask
                    && !(spell->Attributes & SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY)    //Spells unaffected by invulnerability
                    && !iter->second->IsPositive()          //Don't remove positive spells
                    && spell->Id != GetId())                //Don't remove self
            {
                target->RemoveAurasDueToSpell(spell->Id);
                if (Auras.empty())
                    break;
                else
                    next = Auras.begin();
            }
        }
    }
    if (Real && GetSpellProto()->Mechanic == MECHANIC_BANISH)
    {
        if (apply)
            target->addUnitState(UNIT_STAT_ISOLATED);
        else
            target->clearUnitState(UNIT_STAT_ISOLATED);
    }
}

void Aura::HandleAuraModDmgImmunity(bool apply, bool /*Real*/)
{
    GetTarget()->ApplySpellImmune(GetId(), IMMUNITY_DAMAGE, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModDispelImmunity(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if (!Real)
        return;

    if (GetId() == 20594) // Forme de pierre
    {
        GetTarget()->ApplySpellDispelImmunity(GetSpellProto(), DISPEL_DISEASE, apply);
        GetTarget()->ApplySpellDispelImmunity(GetSpellProto(), DISPEL_POISON, apply);
        return;
    }
    GetTarget()->ApplySpellDispelImmunity(GetSpellProto(), DispelType(m_modifier.m_miscvalue), apply);
}

void Aura::HandleAuraProcTriggerSpell(bool apply, bool Real)
{
    if (!Real)
        return;

    switch (GetId())
    {
        // some spell have charges by functionality not have its in spell data
        case 28200:                                         // Ascendance (Talisman of Ascendance trinket)
            if (apply)
                GetHolder()->SetAuraCharges(6);
            break;
        default:
            break;
    }
}

void Aura::HandleAuraModStalked(bool apply, bool /*Real*/)
{
    // used by spells: Hunter's Mark, Mind Vision, Syndicate Tracker (MURP) DND
    if (apply)
        GetTarget()->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
    else
        GetTarget()->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
}

/*********************************************************/
/***                   PERIODIC                        ***/
/*********************************************************/

void Aura::HandlePeriodicTriggerSpell(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    Unit *target = GetTarget();

    if (!apply)
    {
        switch (GetId())
        {
			case 66:                                        // Invisibility
			if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
				target->CastSpell(target, 32612, true, nullptr, this);

			return;
            case 23620:                                     // Burning Adrenaline
                if (m_removeMode == AURA_REMOVE_BY_DEATH)
                    target->CastSpell(target, 23478, true);
                return;
            case 29213:                                     // Curse of the Plaguebringer
                if (m_removeMode != AURA_REMOVE_BY_DISPEL)
                    // Cast Wrath of the Plaguebringer if not dispelled
                    target->CastSpell(target, 29214, true, nullptr, this);
                return;
			case 42783:                                     // Wrath of the Astrom...
				if (m_removeMode == AURA_REMOVE_BY_EXPIRE && GetEffIndex() + 1 < MAX_EFFECT_INDEX)
					target->CastSpell(target, GetSpellProto()->CalculateSimpleValue(SpellEffectIndex(GetEffIndex() + 1)), true);
            default:
                break;
        }
    }

	if (GetId() == 30616) // Magtheridon - Blast Nova
		target->SetTurningOff(apply);
}

void Aura::HandlePeriodicTriggerSpellWithValue(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicEnergize(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandleAuraPowerBurn(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePrayerOfMending(bool apply, bool /*Real*/)
{
	if (apply) // only on initial cast apply SP
	{
		if (const SpellEntry* entry = GetSpellProto())
		{
			if (GetHolder()->GetAuraCharges() == entry->procCharges)
			{
				m_modifier.m_amount = GetCaster()->SpellHealingBonusDone(GetTarget(), GetSpellProto(), m_modifier.m_amount, HEAL);
			}
		}
	}
}

void Aura::HandleAuraPeriodicDummy(bool apply, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	Unit* target = GetTarget();

	// For prevent double apply bonuses
	bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

	SpellEntry const* spell = GetSpellProto();
	switch (spell->SpellFamilyName)
	{
	case SPELLFAMILY_ROGUE:
	{
		// Master of Subtlety
		if (spell->Id == 31666 && !apply)
		{
			target->RemoveAurasDueToSpell(31665);
			break;
		}
		break;
	}
	case SPELLFAMILY_HUNTER:
	{
		// Aspect of the Viper
		if (spell->SpellFamilyFlags & uint64(0x0004000000000000))
		{
			// Update regen on remove
			if (!apply && target->GetTypeId() == TYPEID_PLAYER)
				((Player*)target)->UpdateManaRegen();
			break;
		}
		break;
	}
	}

	m_isPeriodic = apply;
}

void Aura::HandlePeriodicHeal(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    Unit *target = GetTarget();

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if (loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;

		// Renewal
		if (target && target->HasAura(54884))
			m_modifier.m_amount = int32(m_modifier.m_amount * 105 / 100);

		// Contagion
		if (target && target->HasAura(54804))
			m_modifier.m_amount *= int32(m_modifier.m_amount * 90 / 100);

        m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
    }
}

void Aura::HandlePeriodicDamage(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    m_isPeriodic = apply;

    Unit *target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();

    // For prevent double apply bonuses
    bool loading = (target->GetTypeId() == TYPEID_PLAYER && ((Player*)target)->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if (loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;

        switch (spellProto->SpellFamilyName)
        {
			case SPELLFAMILY_WARRIOR:
			{
				//Taste for Blood
				if (spellProto->Mechanic == MECHANIC_BLEED)
				{
					Aura* tfb_aura = caster->GetAuraWithMiscValue(SPELL_AURA_PROC_TRIGGER_SPELL, 8507);
					if (tfb_aura && roll_chance_i(tfb_aura->GetSpellProto()->procChance))
						m_modifier.m_amount *= 2;
				}

				// Rend
				if (spellProto->SpellFamilyFlags & uint64(0x0000000000000020))
				{
					// 0.00743*(($MWB+$mwb)/2+$AP/14*$MWS) bonus per tick
					float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);
					int32 mws = caster->GetAttackTime(BASE_ATTACK);
					float mwb_min = caster->GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE);
					float mwb_max = caster->GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE);
					m_modifier.m_amount += int32(((mwb_min + mwb_max) / 2 + ap * mws / 14000) * 0.00743f);
				}
				break;
			}
            case SPELLFAMILY_DRUID:
            {
                // Rip
                if (spellProto->IsFitToFamilyMask<CF_DRUID_RIP_BITE>())
                {
                    // $AP * min(0.06*$cp, 0.24)/6 [Yes, there is no difference, whether 4 or 5 CPs are being used]
                    if (caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        uint8 cp = ((Player*)caster)->GetComboPoints();

						// Idol of Feral Shadows. Cant be handled as SpellMod in SpellAura:Dummy due its dependency from CPs
						Unit::AuraList const& dummyAuras = caster->GetAurasByType(SPELL_AURA_DUMMY);
						for (Unit::AuraList::const_iterator itr = dummyAuras.begin(); itr != dummyAuras.end(); ++itr)
						{
							if ((*itr)->GetId() == 34241)
							{
								m_modifier.m_amount += cp * (*itr)->GetModifier()->m_amount;
								break;
							}
						}

                        if (cp > 4) cp = 4;
                        m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * cp / 100);
                    }
                }
                break;
            }
			case SPELLFAMILY_WARLOCK:
			{
				// Amplify Curse
				if (caster->HasAura(18288))  //Amplify Curse
				{
					if ((spellProto->School == SPELL_SCHOOL_SHADOW) && (roll_chance_f(caster->ToCreature()->GetUnitCriticalChance(SPELL_SCHOOL_MASK_SHADOW))))
						m_modifier.m_amount *= 2;  //double  damage;
				}
				break;
			}
			case SPELLFAMILY_MAGE:
			{
				// Amplify Curse
				if ((spellProto->School == SPELL_SCHOOL_FIRE) && caster->HasAura(11129))  //Combustion
				{
					if (roll_chance_f(caster->ToCreature()->GetUnitCriticalChance(SPELL_SCHOOL_MASK_FIRE)))
						m_modifier.m_amount *= 2;  //double  damage;
				}
				break;
			}
            case SPELLFAMILY_ROGUE:
            {
                // Rupture
                if (spellProto->IsFitToFamilyMask<CF_ROGUE_RUPTURE>())
                {
                    // Dmg/tick = $AP*min(0.01*$cp, 0.03) [Like Rip: only the first three CP increase the contribution from AP]
                    if (caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        uint8 cp = ((Player*)caster)->GetComboPoints();
                        if (cp > 3) cp = 3;
                        m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * cp / 100);
                    }
                }
                break;
            }
            default:
                break;
        }

        if (m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
        {
            // SpellDamageBonusDone for magic spells
            if (spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE || spellProto->DmgClass == SPELL_DAMAGE_CLASS_MAGIC)
                m_modifier.m_amount = caster->SpellDamageBonusDone(target, GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
            // MeleeDamagebonusDone for weapon based spells
            else
            {
                WeaponAttackType attackType = GetWeaponAttackType(GetSpellProto());
                m_modifier.m_amount = caster->MeleeDamageBonusDone(target, m_modifier.m_amount, attackType, GetSpellProto(), DOT, GetStackAmount());
            }

			// Renewal
			if (target && target->HasAura(54884))
				if (spellProto->DmgClass == SPELL_DAMAGE_CLASS_MAGIC)
					m_modifier.m_amount = int32(m_modifier.m_amount * 95 / 100);

			// Misery
			if (spellProto->DmgClass == SPELL_DAMAGE_CLASS_MAGIC)
			{
				if (target && target->HasAura(33200))
					m_modifier.m_amount = int32(m_modifier.m_amount * 110 / 100);
				else if (target && target->HasAura(33199))
					m_modifier.m_amount = int32(m_modifier.m_amount * 108 / 100);
				else if (target && target->HasAura(33198))
					m_modifier.m_amount = int32(m_modifier.m_amount * 106 / 100);
				else  if (target && target->HasAura(33197))
					m_modifier.m_amount = int32(m_modifier.m_amount * 104 / 100);
				else if (target && target->HasAura(33196))
					m_modifier.m_amount = int32(m_modifier.m_amount * 102 / 100);
			}
		}
		// remove time effects
		else
		{
			switch (spellProto->Id)
			{
			case 35201: // Paralytic Poison
				if (m_removeMode == AURA_REMOVE_BY_DEFAULT)
					target->CastSpell(target, 35202, true); // Paralysis
				break;
			case 41917: // Parasitic Shadowfiend - handle summoning of two Shadowfiends on DoT expire
				target->CastSpell(target, 41915, true);
				break;
			}
        }
    }
}

void Aura::HandlePeriodicDamagePCT(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicLeech(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    // For prevent double apply bonuses
    bool loading = (GetTarget()->GetTypeId() == TYPEID_PLAYER && ((Player*)GetTarget())->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if (loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;

        m_modifier.m_amount = caster->SpellDamageBonusDone(GetTarget(), GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
		Unit* target = GetTarget();
		SpellEntry const* spellProto = GetSpellProto();
		// Contagion
		if (target && target->HasAuraWithMiscValue(SPELL_AURA_DUMMY, 8506))
			m_modifier.m_amount = (m_modifier.m_amount * 110 / 100);

		//Harvest Life 
		if (caster && caster->HasAuraWithMiscValue(SPELL_AURA_DUMMY, 8505))
			if (caster->GetHealthPercent() >= 90.f)	 // Fel Transfusion			
				caster->CastCustomSpell(caster, 54937, &m_modifier.m_amount, nullptr, nullptr, true, nullptr, this);
	}
}

void Aura::HandlePeriodicManaLeech(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandlePeriodicHealthFunnel(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;

    // For prevent double apply bonuses
    bool loading = (GetTarget()->GetTypeId() == TYPEID_PLAYER && ((Player*)GetTarget())->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if (loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;

        m_modifier.m_amount = caster->SpellDamageBonusDone(GetTarget(), GetSpellProto(), m_modifier.m_amount, DOT, GetStackAmount());
    }
}

/*********************************************************/
/***                  MODIFY STATS                     ***/
/*********************************************************/

/********************************/
/***        RESISTANCE        ***/
/********************************/

void Aura::HandleAuraModResistanceExclusive(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();
    // Calcul du montant des autres buffs similaires.
    int32 maxModifiersOthers[MAX_SPELL_SCHOOL] = {0};
    Unit::AuraList const& mModResistanceExcl = target->GetAurasByType(SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE);
    for (Unit::AuraList::const_iterator i = mModResistanceExcl.begin(); i != mModResistanceExcl.end(); ++i)
        for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; ++x)
            if ((*i)->GetSpellProto()->Id != spellProto->Id && (*i)->GetModifier()->m_miscvalue & int32(1 << x))
                if (maxModifiersOthers[x] < (*i)->GetModifierAmount(GetTarget()->getLevel()))
                    maxModifiersOthers[x] = (*i)->GetModifierAmount(GetTarget()->getLevel());
    // Application des effets.
    for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; x++)
    {
        GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_VALUE, float(maxModifiersOthers[x]), !apply);
        if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
            ((Player*)GetTarget())->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(maxModifiersOthers[x]), !apply);
        GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_VALUE, float(std::max(maxModifiersOthers[x], m_modifier.m_miscvalue & int32(1 << x) ? m_modifier.m_amount : 0)), apply);
        if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
            ((Player*)GetTarget())->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(std::max(maxModifiersOthers[x], m_modifier.m_miscvalue & int32(1 << x) ? m_modifier.m_amount : 0)), apply);
    }
}

void Aura::HandleAuraModResistance(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();

    for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; x++)
    {
        if (m_modifier.m_miscvalue & int32(1 << x))
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            if (target->GetTypeId() == TYPEID_PLAYER)
                ((Player*)target)->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(m_modifier.m_amount), apply);
        }
    }

    // Faerie Fire (druid versions)
    if (spellProto->SpellIconID == 109 && spellProto->IsFitToFamily<SPELLFAMILY_DRUID, CF_DRUID_FAERIE_FIRE>())
    {
        target->ApplySpellDispelImmunity(spellProto, DISPEL_STEALTH, apply);
        target->ApplySpellDispelImmunity(spellProto, DISPEL_INVISIBILITY, apply);
    }
}

void Aura::HandleAuraModBaseResistancePCT(bool apply, bool /*Real*/)
{
    // only players have base stats
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        //pets only have base armor
        if (((Creature*)GetTarget())->IsPet() && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
            GetTarget()->HandleStatModifier(UNIT_MOD_ARMOR, BASE_PCT, float(m_modifier.m_amount), apply);
    }
    else
    {
        for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; x++)
        {
            if (m_modifier.m_miscvalue & int32(1 << x))
                GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_PCT, float(m_modifier.m_amount), apply);
        }
    }
}

void Aura::HandleAurasVisible(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_AURAS_VISIBLE, apply);
}

void Aura::HandleModResistancePercent(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();

    for (int8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
    {
        if (m_modifier.m_miscvalue & int32(1 << i))
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
            if (target->GetTypeId() == TYPEID_PLAYER)
            {
                ((Player*)target)->ApplyResistanceBuffModsPercentMod(SpellSchools(i), true, float(m_modifier.m_amount), apply);
                ((Player*)target)->ApplyResistanceBuffModsPercentMod(SpellSchools(i), false, float(m_modifier.m_amount), apply);
            }
        }
    }
}

void Aura::HandleModBaseResistance(bool apply, bool /*Real*/)
{
    // only players have base stats
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
    {
        //only pets have base stats
        if (((Creature*)GetTarget())->IsPet() && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
            GetTarget()->HandleStatModifier(UNIT_MOD_ARMOR, TOTAL_VALUE, float(m_modifier.m_amount), apply);
    }
    else
    {
        for (int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
            if (m_modifier.m_miscvalue & (1 << i))
                GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
    }
}

/********************************/
/***           STAT           ***/
/********************************/

void Aura::HandleAuraModStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -2 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Spell %u effect %u have unsupported misc value (%i) for SPELL_AURA_MOD_STAT ", GetId(), GetEffIndex(), m_modifier.m_miscvalue);
        return;
    }

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        // -1 or -2 is all stats ( misc < -2 checked in function beginning )
        if (m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue == i)
        {
            //m_target->ApplyStatMod(Stats(i), m_modifier.m_amount,apply);
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
                ((Player*)GetTarget())->ApplyStatBuffMod(Stats(i), float(m_modifier.m_amount), apply);
        }
    }
}

void Aura::HandleModPercentStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    // only players have base stats
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        if (m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
            GetTarget()->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), BASE_PCT, float(m_modifier.m_amount), apply);
    }
}

void Aura::HandleModSpellDamagePercentFromStat(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Magic damage modifiers implemented in Unit::SpellDamageBonusDone
    // This information for client side use only
    // Recalculate bonus
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModSpellHealingPercentFromStat(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Recalculate bonus
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleAuraModDispelResist(bool apply, bool Real)
{
	if (!Real || !apply)
		return;

	if (GetId() == 33206)
		GetTarget()->CastSpell(GetTarget(), 44416, true, nullptr, this, GetCasterGuid());
}

void Aura::HandleModSpellDamagePercentFromAttackPower(bool /*apply*/, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	// Magic damage modifiers implemented in Unit::SpellDamageBonusDone
	// This information for client side use only
	// Recalculate bonus
	((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModSpellHealingPercentFromAttackPower(bool /*apply*/, bool /*Real*/)
{
	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	// Recalculate bonus
	((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModHealingDone(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;
    // implemented in Unit::SpellHealingBonusDone
    // this information is for client side only
    ((Player*)GetTarget())->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModRating(bool apply, bool /*Real*/)
{
	if (m_modifier.m_miscvalue < ITEM_MOD_DEFENSE_SKILL_RATING || m_modifier.m_miscvalue > MAX_COMBAT_RATING)
	{
		sLog.outError("WARNING: Spell %u effect %u have unsupported misc value (%i) for SPELL_AURA_MOD_STAT ", GetId(), GetEffIndex(), m_modifier.m_miscvalue);
		return;
	}

	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
		if (m_modifier.m_miscvalue & (1 << rating))
			((Player*)GetTarget())->ApplyRatingMod(CombatRating(rating), m_modifier.m_amount, apply);
}

void Aura::HandleModTotalPercentStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    Unit *target = GetTarget();

    //save current and max HP before applying aura
    uint32 curHPValue = target->GetHealth();
    uint32 maxHPValue = target->GetMaxHealth();

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        if (m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
            if (target->GetTypeId() == TYPEID_PLAYER)
                ((Player*)target)->ApplyStatPercentBuffMod(Stats(i), float(m_modifier.m_amount), apply);
        }
    }

    //recalculate current HP/MP after applying aura modifications (only for spells with 0x10 flag)
    if ((m_modifier.m_miscvalue == STAT_STAMINA) && (maxHPValue > 0) && (GetSpellProto()->Attributes & 0x10) && target->isAlive())
    {
        // newHP = (curHP / maxHP) * newMaxHP = (newMaxHP * curHP) / maxHP -> which is better because no int -> double -> int conversion is needed
        uint32 newHPValue = (target->GetMaxHealth() * curHPValue) / maxHPValue;
        target->SetHealth(newHPValue);
    }
}

void Aura::HandleAuraModResistenceOfStatPercent(bool /*apply*/, bool /*Real*/)
{
  if (m_modifier.m_miscvalue == SPELL_SCHOOL_MASK_NORMAL)
		return;
        // support required adding replace UpdateArmor by loop by UpdateResistence at intellect update
        // and include in UpdateResistence same code as in UpdateArmor for aura mod apply.
    //    sLog.outError("Aura SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT(182) need adding support for non-armor resistances!");
    //    return;

	Stats usedStat;
	float value = 0;

	/*
	if (m_modifier.m_miscvalue & uint64(0x00000100))
		usedStat = STAT_STRENGTH;
	else if (m_modifier.m_miscvalue & uint64(0x00000200))
		usedStat = STAT_AGILITY;
	else if (m_modifier.m_miscvalue & uint64(0x00000400))
		usedStat = STAT_STAMINA;
	else if (m_modifier.m_miscvalue & uint64(0x00000800))
		usedStat = STAT_SPIRIT;
	else usedStat = STAT_INTELLECT;

	value += int32(GetTarget()->GetStat(usedStat) * m_modifier.m_amount / 100.0f);

	
	for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
	{
		float res_value = GetTarget()->GetTotalAuraModValue(UnitMods(UNIT_MOD_RESISTANCE_START + i));

		if (m_modifier.m_miscvalue & (1 << i))
		{
			value += int32(GetTarget()->GetStat(usedStat) * m_modifier.m_amount / 100.0f);
			GetTarget()->SetResistance(SpellSchools(i), int32(res_value + value));
		}
	}
		*/
    // Recalculate Armor
    GetTarget()->UpdateArmor();	
}

/********************************/
/***      HEAL & ENERGIZE     ***/
/********************************/
void Aura::HandleAuraModTotalHealthPercentRegen(bool apply, bool /*Real*/)
{
    m_isPeriodic = apply;
}

void Aura::HandleAuraModTotalManaPercentRegen(bool apply, bool /*Real*/)
{
    if (m_modifier.periodictime == 0)
        m_modifier.periodictime = 1000;

    m_periodicTimer = m_modifier.periodictime;
    m_isPeriodic = apply;
}

void Aura::HandleModRegen(bool apply, bool /*Real*/)        // eating
{
    if (m_modifier.periodictime == 0)
        m_modifier.periodictime = 5000;

    m_periodicTimer = 5000;
    m_isPeriodic = apply;
}

void Aura::HandleModPowerRegen(bool apply, bool Real)       // drinking
{
    if (!Real)
        return;

    Powers pt = GetTarget()->getPowerType();
    if (m_modifier.periodictime == 0)
    {
        // Anger Management (only spell use this aura for rage)
        if (pt == POWER_RAGE)
            m_modifier.periodictime = 3000;
        else
            m_modifier.periodictime = 2000;
    }

    m_periodicTimer = 5000;

    if (GetTarget()->GetTypeId() == TYPEID_PLAYER && m_modifier.m_miscvalue == POWER_MANA)
        ((Player*)GetTarget())->UpdateManaRegen();

    m_isPeriodic = apply;
}

void Aura::HandleModPowerRegenPCT(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    // Update manaregen value
    if (m_modifier.m_miscvalue == POWER_MANA)
        ((Player*)GetTarget())->UpdateManaRegen();
}

void Aura::HandleModManaRegen(bool /*apply*/, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
		return;

	// Note: an increase in regen does NOT cause threat.
	((Player*)GetTarget())->UpdateManaRegen();
}

void Aura::HandleAuraModIncreaseHealth(bool apply, bool Real)
{
    Unit *target = GetTarget();

    // Special case with temporary increase max/current health
    switch (GetId())
    {
        case 12976:                                         // Warrior Last Stand triggered spell
        case 28726:                                         // Nightmare Seed ( Nightmare Seed )
        {
            if (Real)
            {
                if (apply)
                {
                    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
                    target->ModifyHealth(m_modifier.m_amount);
                }
                else
                {
                    if (int32(target->GetHealth()) > m_modifier.m_amount)
                        target->ModifyHealth(-m_modifier.m_amount);
                    else
                        target->SetHealth(1);
                    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
                }
            }
            return;
        }
        // Druid bear form
        case 1178:
        case 9635:
        {
            if (Real)
            {
                float fHealthPercent = float(target->GetHealth()) / target->GetMaxHealth();
                int32 newMaxHealth = target->GetMaxHealth();
                if (apply)
                    newMaxHealth += m_modifier.m_amount;
                else
                    newMaxHealth -= m_modifier.m_amount;

                uint32 newHealth = ceil(newMaxHealth * fHealthPercent);
                target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
                target->SetHealth(newHealth);
            }
            return;
        }
        // Trinket BWL (Don de vie)
        case 23782:
        {
            if (Real)
            {
                target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, 15.0f, apply);
                if (apply)
                {
                    int32 healAmount = target->GetMaxHealth() * 0.15f;
                    target->CastCustomSpell(target, 23783, &healAmount, nullptr, nullptr, true, nullptr, this);
                }
            }
            return;
        }
    }

    // generic case
    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void  Aura::HandleAuraModIncreaseMaxHealth(bool apply, bool /*Real*/)
{
	Unit* target = GetTarget();
	uint32 oldhealth = target->GetHealth();
	double healthPercentage = (double)oldhealth / (double)target->GetMaxHealth();

	target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);

	// refresh percentage
	if (oldhealth > 0)
	{
		uint32 newhealth = uint32(ceil((double)target->GetMaxHealth() * healthPercentage));
		if (newhealth == 0)
			newhealth = 1;

		target->SetHealth(newhealth);
	}
}

void Aura::HandleAuraModIncreaseEnergy(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    Powers powerType = Powers(m_modifier.m_miscvalue);

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    target->HandleStatModifier(unitMod, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseEnergyPercent(bool apply, bool /*Real*/)
{
    Powers powerType = Powers(m_modifier.m_miscvalue);

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    GetTarget()->HandleStatModifier(unitMod, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseHealthPercent(bool apply, bool /*Real*/)
{
    GetTarget()->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, float(m_modifier.m_amount), apply);

    if (GetTarget()->GetMaxHealth() == 1)
        GetTarget()->DoKillUnit(GetTarget());
}

/********************************/
/***          FIGHT           ***/
/********************************/

void Aura::HandleAuraModParryPercent(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateParryPercentage();
}

void Aura::HandleAuraModDodgePercent(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateDodgePercentage();
    //sLog.outError("BONUS DODGE CHANCE: + %f", float(m_modifier.m_amount));
}

void Aura::HandleAuraModBlockPercent(bool /*apply*/, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateBlockPercentage();
    //sLog.outError("BONUS BLOCK CHANCE: + %f", float(m_modifier.m_amount));
}

void Aura::HandleAuraModRegenInterrupt(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)GetTarget())->UpdateManaRegen();
}

void Aura::HandleAuraModCritPercent(bool apply, bool Real)
{
    Unit *target = GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // apply item specific bonuses for already equipped weapon
    if (Real)
    {
        for (int i = 0; i < MAX_ATTACK; ++i)
            if (Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i), true, false))
                ((Player*)target)->_ApplyWeaponDependentAuraCritMod(pItem, WeaponAttackType(i), this, apply);
    }

    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if (GetSpellProto()->EquippedItemClass == -1)
    {
        ((Player*)target)->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float(m_modifier.m_amount), apply);
        ((Player*)target)->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float(m_modifier.m_amount), apply);
        ((Player*)target)->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float(m_modifier.m_amount), apply);
    }
    else
    {
        // done in Player::_ApplyWeaponDependentAuraMods
    }
}

void Aura::HandleModHitChance(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();

    if (GetId() != 22780) // [Ranged Hit Bonus +3] as stated in name ...
        target->m_modMeleeHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
    target->m_modRangedHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
}

void Aura::HandleModSpellHitChance(bool apply, bool /*Real*/)
{
    GetTarget()->m_modSpellHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
}

void Aura::HandleModSpellCritChance(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
        ((Player*)GetTarget())->UpdateAllSpellCritChances();
    else
        GetTarget()->m_baseSpellCritChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
}

void Aura::HandleModSpellCritChanceShool(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    for (int school = SPELL_SCHOOL_NORMAL; school < MAX_SPELL_SCHOOL; ++school)
        if (m_modifier.m_miscvalue & (1 << school))
            ((Player*)GetTarget())->UpdateSpellCritChance(school);
}

/********************************/
/***         ATTACK SPEED     ***/
/********************************/

void Aura::HandleModCastingSpeed(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyCastTimePercentMod(float(m_modifier.m_amount), apply);
}

void Aura::HandleModMeleeRangedSpeedPct(bool apply, bool /*Real*/)
{
	Unit* target = GetTarget();
	target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
	target->ApplyAttackTimePercentMod(OFF_ATTACK, float(m_modifier.m_amount), apply);
	target->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleModCombatSpeedPct(bool apply, bool /*Real*/)
{
	Unit* target = GetTarget();
	target->ApplyCastTimePercentMod(float(m_modifier.m_amount), apply);
	target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
	target->ApplyAttackTimePercentMod(OFF_ATTACK, float(m_modifier.m_amount), apply);
	target->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleModAttackSpeed(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply, true);
    target->UpdateDamagePhysical(BASE_ATTACK);
}

void Aura::HandleModMeleeSpeedPct(bool apply, bool /*Real*/)
{
    Unit *target = GetTarget();
    target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedHaste(bool apply, bool /*Real*/)
{
    GetTarget()->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleRangedAmmoHaste(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;
    GetTarget()->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

/********************************/
/***        ATTACK POWER      ***/
/********************************/

void Aura::HandleAuraModAttackPower(bool apply, bool /*Real*/)
{
    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPower(bool apply, bool /*Real*/)
{
    if ((GetTarget()->getClassMask() & CLASSMASK_WAND_USERS) != 0)
        return;

    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModAttackPowerPercent(bool apply, bool /*Real*/)
{
    //UNIT_FIELD_ATTACK_POWER_MULTIPLIER = multiplier - 1
    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPowerPercent(bool apply, bool /*Real*/)
{
    if ((GetTarget()->getClassMask() & CLASSMASK_WAND_USERS) != 0)
        return;

    //UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER = multiplier - 1
    GetTarget()->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPowerOfStatPercent(bool /*apply*/, bool Real)
{
	// spells required only Real aura add/remove
	if (!Real)
		return;

	// Recalculate bonus
	if (GetTarget()->GetTypeId() == TYPEID_PLAYER && !(GetTarget()->getClassMask() & CLASSMASK_WAND_USERS))
		((Player*)GetTarget())->UpdateAttackPowerAndDamage(true);
}
/********************************/
/***        DAMAGE BONUS      ***/
/********************************/
void Aura::HandleModDamageDone(bool apply, bool Real)
{
    Unit *target = GetTarget();

    // apply item specific bonuses for already equipped weapon
    if (Real && target->GetTypeId() == TYPEID_PLAYER)
    {
        for (int i = 0; i < MAX_ATTACK; ++i)
            if (Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i), true, false))
                ((Player*)target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
    }

    // m_modifier.m_miscvalue is bitmask of spell schools
    // 1 ( 0-bit ) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
    // 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wands
    // 127 - full bitmask any damages
    //
    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) != 0)
    {
        // apply generic physical damage bonuses including wand case
        if (GetSpellProto()->EquippedItemClass == -1 || target->GetTypeId() != TYPEID_PLAYER)
        {
            target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_VALUE, float(m_modifier.m_amount), apply);
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            if (m_positive)
                target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS, m_modifier.m_amount, apply);
            else
                target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG, m_modifier.m_amount, apply);
        }
    }

    // Skip non magic case for speedup
    if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) == 0)
        return;

    if (GetSpellProto()->EquippedItemClass != -1 || GetSpellProto()->EquippedItemInventoryTypeMask != 0)
    {
        // wand magic case (skip generic to all item spell bonuses)
        // done in Player::_ApplyWeaponDependentAuraMods

        // Skip item specific requirements for not wand magic damage
        return;
    }

    // Magic damage modifiers implemented in Unit::SpellDamageBonusDone
    // This information for client side use only
    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        if (m_positive)
        {
            for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if ((m_modifier.m_miscvalue & (1 << i)) != 0)
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i, m_modifier.m_amount, apply);
            }
        }
        else
        {
            for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if ((m_modifier.m_miscvalue & (1 << i)) != 0)
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + i, m_modifier.m_amount, apply);
            }
        }
        Pet* pet = target->GetPet();
        if (pet)
            pet->UpdateAttackPowerAndDamage();
    }
}

void Aura::HandleModDamagePercentDone(bool apply, bool Real)
{
    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "AURA MOD DAMAGE type:%u negative:%u", m_modifier.m_miscvalue, m_positive ? 0 : 1);
    Unit *target = GetTarget();

    // apply item specific bonuses for already equipped weapon
    if (Real && target->GetTypeId() == TYPEID_PLAYER)
    {
        for (int i = 0; i < MAX_ATTACK; ++i)
            if (Item* pItem = ((Player*)target)->GetWeaponForAttack(WeaponAttackType(i), true, false))
                ((Player*)target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
    }

    // m_modifier.m_miscvalue is bitmask of spell schools
    // 1 ( 0-bit ) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
    // 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wand
    // 127 - full bitmask any damages
    //
    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) != 0)
    {
        // apply generic physical damage bonuses including wand case
        if (GetSpellProto()->EquippedItemClass == -1 || target->GetTypeId() != TYPEID_PLAYER)
        {
            target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }
        // For show in client
        if (target->GetTypeId() == TYPEID_PLAYER)
            target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT, m_modifier.m_amount / 100.0f, apply);
    }

    // Skip non magic case for speedup
    if ((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) == 0)
        return;

    if (GetSpellProto()->EquippedItemClass != -1 || GetSpellProto()->EquippedItemInventoryTypeMask != 0)
    {
        // wand magic case (skip generic to all item spell bonuses)
        // done in Player::_ApplyWeaponDependentAuraMods

        // Skip item specific requirements for not wand magic damage
        return;
    }

    // Magic damage percent modifiers implemented in Unit::SpellDamageBonusDone
    // Send info to client
    if (target->GetTypeId() == TYPEID_PLAYER)
        for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT + i, m_modifier.m_amount / 100.0f, apply);
}

void Aura::HandleModOffhandDamagePercent(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "AURA MOD OFFHAND DAMAGE");

    GetTarget()->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

/********************************/
/***        POWER COST        ***/
/********************************/

void Aura::HandleModPowerCostPCT(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    float amount = m_modifier.m_amount / 100.0f;
    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if (m_modifier.m_miscvalue & (1 << i))
            GetTarget()->ApplyModSignedFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + i, amount, apply);
}

void Aura::HandleModPowerCost(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if (m_modifier.m_miscvalue & (1 << i))
            GetTarget()->ApplyModInt32Value(UNIT_FIELD_POWER_COST_MODIFIER + i, m_modifier.m_amount, apply);
}

/*********************************************************/
/***                    OTHERS                         ***/
/*********************************************************/

void Aura::HandleShapeshiftBoosts(bool apply)
{
    uint32 spellId1 = 0;
    uint32 spellId2 = 0;
    uint32 HotWSpellId = 0;

    ShapeshiftForm form = ShapeshiftForm(GetModifier()->m_miscvalue);

    Unit *target = GetTarget();

    switch (form)
    {
        case FORM_CAT:
            spellId1 = 3025;
            HotWSpellId = 24900;
            break;
        case FORM_TREE:
            spellId1 = 5420;
            break;
        case FORM_TRAVEL:
            spellId1 = 5419;
            break;
        case FORM_AQUA:
            spellId1 = 5421;
            break;
        case FORM_BEAR:
            spellId1 = 1178;
            spellId2 = 21178;
            HotWSpellId = 24899;
            break;
        case FORM_DIREBEAR:
            spellId1 = 9635;
            spellId2 = 21178;
            HotWSpellId = 24899;
            break;
        case FORM_BATTLESTANCE:
            spellId1 = 21156;
            break;
        case FORM_DEFENSIVESTANCE:
            spellId1 = 7376;
            break;
        case FORM_BERSERKERSTANCE:
            spellId1 = 7381;
            break;
        case FORM_MOONKIN:
            spellId1 = 24905;
            break;
        case FORM_SPIRITOFREDEMPTION:
            spellId1 = 27792;
            spellId2 = 27795;                               // must be second, this important at aura remove to prevent to early iterator invalidation.
            break;
        case FORM_GHOSTWOLF:
        case FORM_AMBIENT:
        case FORM_GHOUL:
        case FORM_SHADOW:
        case FORM_STEALTH:
        case FORM_CREATURECAT:
        case FORM_CREATUREBEAR:
            break;
    }

    if (apply)
    {
        if (spellId1)
            target->AddAura(spellId1, 0, target);
        if (spellId2)
            target->AddAura(spellId2, 0, target);

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            const PlayerSpellMap& sp_list = ((Player *)target)->GetSpellMap();
            for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
            {
                if (itr->second.state == PLAYERSPELL_REMOVED) continue;
                if (itr->first == spellId1 || itr->first == spellId2) continue;
                SpellEntry const *spellInfo = sSpellMgr.GetSpellEntry(itr->first);
                if (!spellInfo || !IsNeedCastSpellAtFormApply(spellInfo, form))
                    continue;
                target->CastSpell(target, itr->first, true, nullptr, this);
            }

            // Leader of the Pack
            if (((Player*)target)->HasSpell(17007))
            {
                SpellEntry const *spellInfo = sSpellMgr.GetSpellEntry(24932);
                if (spellInfo && spellInfo->Stances & (1 << (form - 1)))
                    target->CastSpell(target, 24932, true, nullptr, this);
            }

            // Heart of the Wild
            if (HotWSpellId)
            {
                Unit::AuraList const& mModTotalStatPct = target->GetAurasByType(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE);
                for (Unit::AuraList::const_iterator i = mModTotalStatPct.begin(); i != mModTotalStatPct.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellIconID == 240 && (*i)->GetModifier()->m_miscvalue == 3)
                    {
                        int32 HotWMod = (*i)->GetModifier()->m_amount;
                        target->CastCustomSpell(target, HotWSpellId, &HotWMod, nullptr, nullptr, true, nullptr, this);
                        break;
                    }
                }
            }
        }
    }
    else
    {
        if (spellId1)
            target->RemoveAurasDueToSpell(spellId1);
        if (spellId2)
            target->RemoveAurasDueToSpell(spellId2);

        Unit::SpellAuraHolderMap& tAuras = target->GetSpellAuraHolderMap();
        for (Unit::SpellAuraHolderMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
        {
            if (itr->second->IsRemovedOnShapeLost())
            {
                target->RemoveAurasDueToSpell(itr->second->GetId());
                itr = tAuras.begin();
            }
            else
                ++itr;
        }
    }
}

void Aura::HandleAuraEmpathy(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_UNIT)
        return;

    CreatureInfo const * ci = ObjectMgr::GetCreatureTemplate(GetTarget()->GetEntry());
    if (ci && ci->type == CREATURE_TYPE_BEAST)
        GetTarget()->ApplyModUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_SPECIALINFO, apply);
}

void Aura::HandleAuraUntrackable(bool apply, bool /*Real*/)
{
    if (apply)
        GetTarget()->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
    else
        GetTarget()->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
}

void Aura::HandleAuraModPacify(bool apply, bool /*Real*/)
{
    if (apply)
        GetTarget()->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
    else
        GetTarget()->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
}

void Aura::HandleAuraModPacifyAndSilence(bool apply, bool Real)
{
    HandleAuraModPacify(apply, Real);
    HandleAuraModSilence(apply, Real);
}

void Aura::HandleAuraGhost(bool apply, bool /*Real*/)
{
    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if (apply)
        GetTarget()->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
    else
        GetTarget()->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
    
    if (((Player*)GetTarget())->GetGroup())
        ((Player*)GetTarget())->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_STATUS);
}

void Aura::HandleShieldBlockValue(bool apply, bool /*Real*/)
{
    BaseModType modType = FLAT_MOD;
    if (m_modifier.m_auraname == SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT)
        modType = PCT_MOD;

    if (GetTarget()->GetTypeId() == TYPEID_PLAYER)
        ((Player*)GetTarget())->HandleBaseModValue(SHIELD_BLOCK_VALUE, modType, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraRetainComboPoints(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *target = (Player*)GetTarget();

    // combo points was added in SPELL_EFFECT_ADD_COMBO_POINTS handler
    // remove only if aura expire by time (in case combo points amount change aura removed without combo points lost)
    if (!apply && m_removeMode == AURA_REMOVE_BY_EXPIRE && target->GetComboTargetGuid())
        if (Unit* unit = ObjectAccessor::GetUnit(*GetTarget(), target->GetComboTargetGuid()))
            target->AddComboPoints(unit, -m_modifier.m_amount);
}

void Aura::HandleModUnattackable(bool Apply, bool Real)
{
    if (Real && Apply)
    {
        GetTarget()->CombatStop();
        GetTarget()->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
    }
    GetTarget()->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE, Apply);
}

void Aura::HandleSpiritOfRedemption(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    Unit *target = GetTarget();

    // prepare spirit state
    if (apply)
    {
        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            // disable breath/etc timers
            ((Player*)target)->StopMirrorTimers();

            // set stand state (expected in this form)
            if (!target->IsStandState())
                target->SetStandState(UNIT_STAND_STATE_STAND);
        }

        target->SetHealth(1);
    }
    // die at aura end
    else
        target->DealDamage(target, target->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, GetSpellProto(), false);
}

void Aura::HandleSchoolAbsorb(bool apply, bool Real)
{
    if (!Real)
        return;

    Unit* caster = GetCaster();
    if (!caster)
        return;

    Unit *target = GetTarget();
    SpellEntry const* spellProto = GetSpellProto();
    if (apply)
    {
        // prevent double apply bonuses
        if (target->GetTypeId() != TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
        {
            float DoneActualBenefit = 0.0f;
            switch (spellProto->SpellFamilyName)
            {
                case SPELLFAMILY_PRIEST:
                    // Power Word: Shield
                    if (spellProto->IsFitToFamilyMask<CF_PRIEST_POWER_WORD_SHIELD>())
                    {
                        //+10% from +healing bonus
                        DoneActualBenefit = caster->SpellBaseHealingBonusDone(GetSpellSchoolMask(spellProto)) * 0.1f;
                        break;
                    }
                    break;
                case SPELLFAMILY_MAGE:
                    // Frost ward, Fire ward
                    if (spellProto->IsFitToFamilyMask<CF_MAGE_FIRE_WARD, CF_MAGE_FROST_WARD>())
                    {
                        //+10% from +spd bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto)) * 0.1f;
                        break;
                    }
                    break;
                case SPELLFAMILY_WARLOCK:
                    // Shadow Ward
                    if (spellProto->SpellIconID == 207 && spellProto->Category == 56)
                    {
                        //+10% from +spd bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto)) * 0.1f;
                        break;
                    }
                    break;
                default:
                    break;
            }

            DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

            m_modifier.m_amount += (int32)DoneActualBenefit;
        }
    }
}

void Aura::PeriodicTick(SpellEntry const* sProto, AuraType auraType, uint32 data)
{
    Unit *target = GetTarget();
    SpellEntry const* spellProto = sProto ? sProto : GetSpellProto();

    switch (sProto ? auraType : m_modifier.m_auraname)
    {
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
        {
            // don't damage target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            Unit *pCaster = GetCaster();
            if (!pCaster)
                return;

            if (spellProto->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                    pCaster->SpellHitResult(target, spellProto, GetEffIndex(), false) != SPELL_MISS_NONE)
                return;

            // Check for immune (not use charges)
            if (target->IsImmuneToDamage(GetSpellSchoolMask(spellProto)) && !(spellProto->Attributes & SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY))
                return;

            uint32 absorb = 0;
            uint32 resist = 0;
            CleanDamage cleanDamage = CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL, 0, 0);

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 amount = 0;
            if (!sProto)
                amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;
            else
                amount = data;

            uint32 pdamage;

            if (sProto)
            {
                if (auraType == SPELL_AURA_PERIODIC_DAMAGE)
                    pdamage = amount;
                else
                    pdamage = uint32(target->GetMaxHealth() * amount / 100);
            }
            else
            {
                if (m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
                    pdamage = amount;
                else
                    pdamage = uint32(target->GetMaxHealth() * amount / 100);                
            }

			bool isNotBleed = GetEffectMechanic(spellProto, m_effIndex) != MECHANIC_BLEED;

            // SpellDamageBonus for magic spells
            if ((spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE && isNotBleed) || spellProto->DmgClass == SPELL_DAMAGE_CLASS_MAGIC)
                pdamage = target->SpellDamageBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());
            // MeleeDamagebonus for weapon based spells
            else
            {
                WeaponAttackType attackType = GetWeaponAttackType(spellProto);
                pdamage = target->MeleeDamageBonusTaken(pCaster, pdamage, attackType, spellProto, DOT, GetStackAmount());
            }

            // Calculate armor mitigation if it is a physical spell
            // But not for bleed mechanic spells
            if (GetSpellSchoolMask(spellProto) & SPELL_SCHOOL_MASK_NORMAL && GetEffectMechanic(spellProto, m_effIndex) != MECHANIC_BLEED && !(spellProto->Custom & SPELL_CUSTOM_IGNORE_ARMOR))
            {
                uint32 pdamageReductedArmor = pCaster->CalcArmorReducedDamage(target, pdamage);
                cleanDamage.damage += pdamage - pdamageReductedArmor;
                pdamage = pdamageReductedArmor;
            }

            // Curse of Agony damage-per-tick calculation
            if (spellProto->IsFitToFamily<SPELLFAMILY_WARLOCK, CF_WARLOCK_CURSE_OF_AGONY>())
            {
                // 1..4 ticks, 1/2 from normal tick damage
                if (GetAuraTicks() <= 4)
                    pdamage = pdamage / 2;
                // 9..12 ticks, 3/2 from normal tick damage
                else if (GetAuraTicks() >= 9)
                    pdamage += (pdamage + 1) / 2;       // +1 prevent 0.5 damage possible lost at 1..4 ticks
                // 5..8 ticks have normal tick damage
            }

            target->CalculateDamageAbsorbAndResist(pCaster, GetSpellSchoolMask(spellProto), DOT, pdamage, &absorb, &resist, spellProto);

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s attacked %s for %u dmg inflicted by %u",
                              GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

            pCaster->DealDamageMods(target, pdamage, &absorb);

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC | GetHolder()->spellFirstHitAttackerProcFlags;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC | GetHolder()->spellFirstHitTargetProcFlags;
            pdamage = (pdamage <= absorb + resist) ? 0 : (pdamage - absorb - resist);

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, absorb, resist, 0.0f);
            target->SendPeriodicAuraLog(&pInfo, sProto ? auraType : SPELL_AURA_NONE);

            if (pdamage)
                procVictim |= PROC_FLAG_TAKEN_ANY_DAMAGE;

            pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, PROC_EX_NORMAL_HIT, pdamage, BASE_ATTACK, spellProto);

            cleanDamage.absorb = absorb;
            cleanDamage.resist = resist;
            pCaster->DealDamage(target, pdamage, &cleanDamage, DOT, GetSpellSchoolMask(spellProto), spellProto, true);
            // Curse of Doom: If the target dies from this damage, there is a chance that a Doomguard will be summoned.
            if (spellProto->Id == 603 && !target->isAlive() && !urand(0, 9))
                pCaster->CastSpell(pCaster, 18662, true);
            break;
        }
        case SPELL_AURA_PERIODIC_LEECH:
        case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
        {
            // don't damage target if not alive, possible death persistent effects
            if (!target->IsInWorld() || !target->isAlive())
                return;

            Unit *pCaster = GetCaster();
            if (!pCaster)
                return;

            if (!pCaster->IsInWorld() || !pCaster->isAlive())
                return;

            if (spellProto->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                    pCaster->SpellHitResult(target, spellProto, GetEffIndex(), false) != SPELL_MISS_NONE)
                return;

            // Check for immune
            if (target->IsImmuneToDamage(GetSpellSchoolMask(spellProto)))
                return;

            uint32 absorb = 0;
            uint32 resist = 0;
            CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL, 0, 0);

            uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            //Calculate armor mitigation if it is a physical spell
            if (GetSpellSchoolMask(spellProto) & SPELL_SCHOOL_MASK_NORMAL)
            {
                uint32 pdamageReductedArmor = pCaster->CalcArmorReducedDamage(target, pdamage);
                cleanDamage.damage += pdamage - pdamageReductedArmor;
                pdamage = pdamageReductedArmor;
            }

            pdamage = target->SpellDamageBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());

            target->CalculateDamageAbsorbAndResist(pCaster, GetSpellSchoolMask(spellProto), DOT, pdamage, &absorb, &resist, spellProto);

            if (target->GetHealth() < pdamage)
                pdamage = uint32(target->GetHealth());

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s health leech of %s for %u dmg inflicted by %u abs is %u",
                              GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId(), absorb);

            pCaster->DealDamageMods(target, pdamage, &absorb);

            pCaster->SendSpellNonMeleeDamageLog(target, GetId(), pdamage, GetSpellSchoolMask(spellProto), absorb, resist, false, 0);

            float multiplier = spellProto->EffectMultipleValue[GetEffIndex()] > 0 ? spellProto->EffectMultipleValue[GetEffIndex()] : 1;

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC | GetHolder()->spellFirstHitAttackerProcFlags;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC | GetHolder()->spellFirstHitTargetProcFlags;

            pdamage = (pdamage <= absorb + resist) ? 0 : (pdamage - absorb - resist);
            if (pdamage)
                procVictim |= PROC_FLAG_TAKEN_ANY_DAMAGE;

            cleanDamage.absorb = absorb;
            cleanDamage.resist = resist;

            pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, PROC_EX_NORMAL_HIT, pdamage, BASE_ATTACK, spellProto);
            int32 new_damage = pCaster->DealDamage(target, pdamage, &cleanDamage, DOT, GetSpellSchoolMask(spellProto), spellProto, false);

            if (!target->isAlive() && pCaster->IsNonMeleeSpellCasted(false))
                for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
                    if (Spell* spell = pCaster->GetCurrentSpell(CurrentSpellTypes(i)))
                        if (spell->m_spellInfo->Id == GetId())
                            spell->cancel();

            if (Player *modOwner = pCaster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetId(), SPELLMOD_MULTIPLE_VALUE, multiplier);

            uint32 heal = int32(new_damage * multiplier);

            int32 gain = pCaster->DealHeal(pCaster, heal, spellProto);
            pCaster->getHostileRefManager().threatAssist(pCaster, gain * 0.5f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
            break;
        }
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_OBS_MOD_HEALTH:
        {
            // don't heal target if not alive, mostly death persistent effects from items
            if (!target->isAlive())
                return;

            Unit *pCaster = GetCaster();
            if (!pCaster)
                return;

            // Don't heal target if it is already at max health
            if (target->GetHealth() == target->GetMaxHealth())
                return;

            // heal for caster damage (must be alive)
            if (target != pCaster && spellProto->SpellVisual == 163 && !pCaster->isAlive())
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            uint32 pdamage;

            if (m_modifier.m_auraname == SPELL_AURA_OBS_MOD_HEALTH)
                pdamage = uint32(target->GetMaxHealth() * amount / 100);
            else
                pdamage = amount;

            pdamage = target->SpellHealingBonusTaken(pCaster, spellProto, pdamage, DOT, GetStackAmount());

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s heal of %s for %u health inflicted by %u",
                              GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

            int32 gain = target->ModifyHealth(pdamage);
            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
            target->SendPeriodicAuraLog(&pInfo);

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC | GetHolder()->spellFirstHitAttackerProcFlags;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC | GetHolder()->spellFirstHitTargetProcFlags;
            uint32 procEx = PROC_EX_NORMAL_HIT | PROC_EX_PERIODIC_POSITIVE;
            pCaster->ProcDamageAndSpell(target, procAttacker, procVictim, procEx, gain, BASE_ATTACK, spellProto);

            // Grande tenue de marchereve (Druide T3)
            if (spellProto->IsFitToFamily<SPELLFAMILY_DRUID, CF_DRUID_REJUVENATION>())
            {
                Unit::AuraList const& auraClassScripts = pCaster->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                for (Unit::AuraList::const_iterator itr = auraClassScripts.begin(); itr != auraClassScripts.end(); ++itr)
                {
                    uint32 triggered_spell_id = 0;

                    // Aura giving mana / health at recuperation tick
                    if ((*itr)->GetModifier()->m_miscvalue == 4533 && roll_chance_i(50))
                    {
                        switch (target->getPowerType())
                        {
                            case POWER_MANA:
                                triggered_spell_id = 28722;
                                break;
                            case POWER_RAGE:
                                triggered_spell_id = 28723;
                                break;
                            case POWER_ENERGY:
                                triggered_spell_id = 28724;
                                break;
                        }
                    }
                    else if ((*itr)->GetModifier()->m_miscvalue == 4537)
                        triggered_spell_id = 28750;
                    if (triggered_spell_id)
                        pCaster->CastSpell(target, triggered_spell_id, true);
                }
            }
            target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);

            // heal for caster damage
            if (target != pCaster && spellProto->SpellVisual == 163)
            {
                uint32 dmg = spellProto->manaPerSecond;
                if (pCaster->GetHealth() <= dmg && pCaster->GetTypeId() == TYPEID_PLAYER)
                {
                    pCaster->RemoveAurasDueToSpell(GetId());

                    // finish current generic/channeling spells, don't affect autorepeat
                    pCaster->FinishSpell(CURRENT_GENERIC_SPELL);
                    pCaster->FinishSpell(CURRENT_CHANNELED_SPELL);
                }
                else
                {
                    uint32 damage = gain;
                    uint32 absorb = 0;
                    pCaster->DealDamageMods(pCaster, damage, &absorb);
                    pCaster->SendSpellNonMeleeDamageLog(pCaster, GetId(), damage, GetSpellSchoolMask(spellProto), absorb, 0, false, 0, false);

                    CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL, absorb, 0);
                    pCaster->DealDamage(pCaster, damage, &cleanDamage, NODAMAGE, GetSpellSchoolMask(spellProto), spellProto, true);
                }
            }
            break;
        }
        case SPELL_AURA_PERIODIC_MANA_LEECH:
        {
            // don't damage target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            if (m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue >= MAX_POWERS)
                return;

            Powers power = Powers(m_modifier.m_miscvalue);

            // power type might have changed between aura applying and tick (druid's shapeshift)
            if (target->getPowerType() != power)
                return;

            Unit *pCaster = GetCaster();
            if (!pCaster)
                return;

            if (!pCaster->isAlive())
                return;

            if (GetSpellProto()->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                    pCaster->SpellHitResult(target, spellProto, GetEffIndex(), false) != SPELL_MISS_NONE)
                return;

            // Check for immune (not use charges)
            if (target->IsImmuneToDamage(GetSpellSchoolMask(spellProto)))
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s power leech of %s for %u dmg inflicted by %u",
                              GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

            int32 drain_amount = target->GetPower(power) > pdamage ? pdamage : target->GetPower(power);

            target->ModifyPower(power, -drain_amount);

            float gain_multiplier = 0;

            if (pCaster->GetMaxPower(power) > 0)
            {
                gain_multiplier = spellProto->EffectMultipleValue[GetEffIndex()];

                if (Player *modOwner = pCaster->GetSpellModOwner())
                    modOwner->ApplySpellMod(GetId(), SPELLMOD_MULTIPLE_VALUE, gain_multiplier);
            }

            SpellPeriodicAuraLogInfo pInfo(this, drain_amount, 0, 0, gain_multiplier);
            target->SendPeriodicAuraLog(&pInfo);

            int32 gain_amount = int32(drain_amount * gain_multiplier);

            if (gain_amount)
            {
                float threat = pCaster->ModifyPower(power, gain_amount) * sSpellMgr.GetSpellThreatMultiplier(spellProto);
                threat *= 0.5; // Mana Drain
                target->AddThreat(pCaster, threat, false, GetSpellSchoolMask(spellProto), spellProto);
            }
            if (target->GetPower(power) == 0)
            {
                if (spellProto->Id == 21056) //Marque de Kazzak
                {
                    // Explose quand y'a plus de mana a drainer
                    target->CastSpell(target, 21058, true);
                    GetHolder()->SetAuraDuration(0);
                }
            }

            // Improved Drain Mana
            auto improvedManaDrain1 = pCaster->GetAura(17864, EFFECT_INDEX_0);
            auto improvedManaDrain2 = pCaster->GetAura(18393, EFFECT_INDEX_0);

            if (improvedManaDrain2)
                PeriodicTick(improvedManaDrain2->GetHolder()->GetSpellProto(), SPELL_AURA_PERIODIC_DAMAGE, drain_amount * 0.3f);
            else if (improvedManaDrain1)
                PeriodicTick(improvedManaDrain1->GetHolder()->GetSpellProto(), SPELL_AURA_PERIODIC_DAMAGE, drain_amount * 0.15f);

            // Nostalrius: break des controles type 'AURA_INTERRUPT_FLAG_DAMAGE'
            target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_DAMAGE);
            break;
        }
        case SPELL_AURA_PERIODIC_ENERGIZE:
        {
            // don't energize target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s energize %s for %u dmg inflicted by %u",
                              GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

            if (m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue >= MAX_POWERS)
                break;

            Powers power = Powers(m_modifier.m_miscvalue);

            if (target->GetMaxPower(power) == 0)
                break;

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
            target->SendPeriodicAuraLog(&pInfo);

            int32 gain = target->ModifyPower(power, pdamage);

            if (power != POWER_MANA)     // 1.9 - Mana regeneration over time will no longer generate threat.
                if (Unit* pCaster = GetCaster())
                    target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
            break;
        }
        case SPELL_AURA_OBS_MOD_MANA:
        {
            // don't energize target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            uint32 pdamage = uint32(target->GetMaxPower(POWER_MANA) * amount / 100);

            DETAIL_FILTER_LOG(LOG_FILTER_PERIODIC_AFFECTS, "PeriodicTick: %s energize %s for %u mana inflicted by %u",
                              GetCasterGuid().GetString().c_str(), target->GetGuidStr().c_str(), pdamage, GetId());

            if (target->GetMaxPower(POWER_MANA) == 0)
                break;

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
            target->SendPeriodicAuraLog(&pInfo);

            int32 gain = target->ModifyPower(POWER_MANA, pdamage);

            if (Unit* pCaster = GetCaster())
                target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
            break;
        }
        case SPELL_AURA_POWER_BURN_MANA:
        {
            // don't mana burn target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            Unit *pCaster = GetCaster();
            if (!pCaster)
                return;

            // Check for immune (not use charges)
            if (target->IsImmuneToDamage(GetSpellSchoolMask(spellProto)))
                return;

            int32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            Powers powerType = Powers(m_modifier.m_miscvalue);

            if (!target->isAlive() || target->getPowerType() != powerType)
                return;

            uint32 gain = uint32(-target->ModifyPower(powerType, -pdamage));

            gain = uint32(gain * spellProto->EffectMultipleValue[GetEffIndex()]);

            // maybe has to be sent different to client, but not by SMSG_PERIODICAURALOG
            SpellNonMeleeDamage damageInfo(pCaster, target, spellProto->Id, SpellSchools(spellProto->School));
            pCaster->CalculateSpellDamage(&damageInfo, gain, spellProto);

            damageInfo.target->CalculateAbsorbResistBlock(pCaster, &damageInfo, spellProto);

            pCaster->DealDamageMods(damageInfo.target, damageInfo.damage, &damageInfo.absorb);

            pCaster->SendSpellNonMeleeDamageLog(&damageInfo);

            // Set trigger flag
            uint32 procAttacker = PROC_FLAG_ON_DO_PERIODIC | GetHolder()->spellFirstHitAttackerProcFlags;
            uint32 procVictim   = PROC_FLAG_ON_TAKE_PERIODIC | GetHolder()->spellFirstHitTargetProcFlags;
            uint32 procEx       = createProcExtendMask(&damageInfo, SPELL_MISS_NONE);
            if (damageInfo.damage)
                procVictim |= PROC_FLAG_TAKEN_ANY_DAMAGE;

            pCaster->ProcDamageAndSpell(damageInfo.target, procAttacker, procVictim, procEx, damageInfo.damage, BASE_ATTACK, spellProto);

            pCaster->DealSpellDamage(&damageInfo, true);
            break;
        }
        case SPELL_AURA_MOD_REGEN:
        {
            // don't heal target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            int32 gain = target->ModifyHealth(m_modifier.m_amount);
            if (Unit *caster = GetCaster())
                target->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f  * sSpellMgr.GetSpellThreatMultiplier(spellProto), spellProto);
            // Eating anim
            if (spellProto->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED)
                target->HandleEmoteCommand(EMOTE_ONESHOT_EAT);
            break;
        }
        case SPELL_AURA_MOD_POWER_REGEN:
        {
            // don't energize target if not alive, possible death persistent effects
            if (!target->isAlive())
                return;

            Powers pt = target->getPowerType();
            if (int32(pt) != m_modifier.m_miscvalue)
                return;

            if (spellProto->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED)
            {
                // eating anim
                target->HandleEmoteCommand(EMOTE_ONESHOT_EAT);
            }
            else if (GetId() == 20577)
            {
                // cannibalize anim
                target->HandleEmoteCommand(EMOTE_STATE_CANNIBALIZE);
            }

            // Anger Management
            // amount = 1+ 16 = 17 = 3,4*5 = 10,2*5/3
            // so 17 is rounded amount for 5 sec tick grow ~ 1 range grow in 3 sec
            if (pt == POWER_RAGE)
                target->ModifyPower(pt, m_modifier.m_amount * 3 / 5);
            break;
        }
        // Here tick dummy auras
        case SPELL_AURA_DUMMY:                              // some spells have dummy aura
        {
            PeriodicDummyTick();
            break;
        }
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
        {
            TriggerSpell();
            break;
        }
		case SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
		{
				TriggerSpellWithValue();
				break;
		}
        default:
            break;
    }
    // First tick is done now.
    GetHolder()->spellFirstHitAttackerProcFlags = 0;
    GetHolder()->spellFirstHitTargetProcFlags = 0;
}

void Aura::PeriodicDummyTick()
{
    SpellEntry const* spell = GetSpellProto();
    Unit *target = GetTarget();
    switch (spell->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (spell->Id)
            {
                // Forsaken Skills
                case 7054:
                {
                    uint32 spellRandom = urand(0, 14) + 7038;
                    sLog.outInfo("7054 %u", spellRandom);

                    target->CastSpell(target, spellRandom, true, nullptr, this);
                    // Possibly need cast one of them (but
                    // 7038 Forsaken Skill: Swords
                    // 7039 Forsaken Skill: Axes
                    // 7040 Forsaken Skill: Daggers
                    // 7041 Forsaken Skill: Maces
                    // 7042 Forsaken Skill: Staves
                    // 7043 Forsaken Skill: Bows
                    // 7044 Forsaken Skill: Guns
                    // 7045 Forsaken Skill: 2H Axes
                    // 7046 Forsaken Skill: 2H Maces
                    // 7047 Forsaken Skill: 2H Swords
                    // 7048 Forsaken Skill: Defense
                    // 7049 Forsaken Skill: Fire
                    // 7050 Forsaken Skill: Frost
                    // 7051 Forsaken Skill: Holy
                    // 7053 Forsaken Skill: Shadow
                    return;
                }
                case 7057:                                  // Haunting Spirits
                    if (roll_chance_i(33))
                        target->CastSpell(target, m_modifier.m_amount, true, nullptr, this);
                    return;
                case 24596:                                 // Intoxicating Venom
                    if (target->isInCombat() && urand(0, 99) < 7)
                        target->AddAura(8379); // Disarm
                    return;
				case 30019:                                 // Control Piece
				{
					if (target->GetTypeId() != TYPEID_PLAYER)
						return;

					Unit* chessPiece = target->GetCharm();
					if (!chessPiece)
					{
						target->CastSpell(target, 30529, true);
						target->RemoveAurasDueToSpell(30019);
						target->RemoveAurasDueToSpell(30532);
					}
					return;
				}
				// Gossip NPC Periodic - Talk
				case 32441:                                 // Brittle Bones
					if (roll_chance_i(33))
						target->CastSpell(target, 32437, true, nullptr, this);  // Rattled
					return;
				case SPELLFAMILY_HUNTER:
				{
					// Aspect of the Viper
					switch (spell->Id)
					{
					case 34074:
					{
						if (target->GetTypeId() != TYPEID_PLAYER)
							return;
						// Should be manauser
						if (target->getPowerType() != POWER_MANA)
							return;
						Unit* caster = GetCaster();
						if (!caster)
							return;
						// Regen amount is max (100% from spell) on 21% or less mana and min on 92.5% or greater mana (20% from spell)
						int mana = target->GetPower(POWER_MANA);
						int max_mana = target->GetMaxPower(POWER_MANA);
						int32 base_regen = caster->CalculateSpellDamage(target, GetSpellProto(), m_effIndex, &m_currentBasePoints);
						float regen_pct = 1.20f - 1.1f * mana / max_mana;
						if (regen_pct > 1.0f) regen_pct = 1.0f;
						else if (regen_pct < 0.2f) regen_pct = 0.2f;
						m_modifier.m_amount = int32(base_regen * regen_pct);
						((Player*)target)->UpdateManaRegen();
						return;
					}
					}
					break;
				}
				break;
            }
            break;
        }
        default:
            break;
    }
}

void Aura::HandlePreventFleeing(bool apply, bool Real)
{
    if (!Real)
        return;

    Unit::AuraList const& fearAuras = GetTarget()->GetAurasByType(SPELL_AURA_MOD_FEAR);
    if (fearAuras.empty())
    {
        // Units may be feared without spell (Creature::DoFleeToGetAssistance)
        if (GetTarget()->GetTypeId() == TYPEID_UNIT)
            if (GetTarget()->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLEEING_MOTION_TYPE ||
                    GetTarget()->GetMotionMaster()->GetCurrentMovementGeneratorType() == ASSISTANCE_MOTION_TYPE ||
                    GetTarget()->GetMotionMaster()->GetCurrentMovementGeneratorType() == ASSISTANCE_DISTRACT_MOTION_TYPE ||
                    GetTarget()->GetMotionMaster()->GetCurrentMovementGeneratorType() == TIMED_FLEEING_MOTION_TYPE)
                GetTarget()->GetMotionMaster()->MovementExpired(false);
    }
    else
    {
        if (apply)
            GetTarget()->SetFeared(false, fearAuras.front()->GetCasterGuid());
        else
            GetTarget()->SetFeared(true);
    }
}

void Aura::HandleManaShield(bool apply, bool Real)
{
    if (!Real)
        return;

    // prevent double apply bonuses
    if (apply && (GetTarget()->GetTypeId() != TYPEID_PLAYER || !((Player*)GetTarget())->GetSession()->PlayerLoading()))
    {
        if (Unit* caster = GetCaster())
        {
            float DoneActualBenefit = 0.0f;

            // Mana Shield
            // +50% from +spd bonus
            if (GetSpellProto()->IsFitToFamily<SPELLFAMILY_MAGE, CF_MAGE_MANA_SHIELD>())
                DoneActualBenefit = caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(GetSpellProto())) * 0.5f;

            DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

            m_modifier.m_amount += (int32)DoneActualBenefit;
        }
    }
}

bool Aura::IsLastAuraOnHolder()
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (i != GetEffIndex() && GetHolder()->m_auras[i])
            return false;
    return true;
}

SpellAuraHolder::SpellAuraHolder(SpellEntry const* spellproto, Unit *target, WorldObject *caster, Item *castItem) :
    m_spellProto(spellproto), m_target(target), m_castItemGuid(castItem ? castItem->GetObjectGuid() : ObjectGuid()),
    m_auraSlot(MAX_AURAS), m_auraLevel(1), m_procCharges(0),
    m_stackAmount(1), m_removeMode(AURA_REMOVE_BY_DEFAULT), m_AuraDRGroup(DIMINISHING_NONE), m_timeCla(1000),
    m_permanent(false), m_isRemovedOnShapeLost(true), m_deleted(false), m_in_use(0),
    m_debuffLimitAffected(false), m_debuffLimitScore(0), _heartBeatRandValue(0), _pveHeartBeatData(nullptr),
    spellFirstHitAttackerProcFlags(0), spellFirstHitTargetProcFlags(0), m_spellTriggered(false)
{
    MANGOS_ASSERT(target);
    MANGOS_ASSERT(spellproto && spellproto == sSpellMgr.GetSpellEntry(spellproto->Id) && "`info` must be pointer to sSpellStore element");

    if (!caster)
        m_casterGuid = target->GetObjectGuid();
    else
    {
        // remove this assert when not unit casters will be supported
        MANGOS_ASSERT(caster->isType(TYPEMASK_UNIT))
        m_casterGuid = caster->GetObjectGuid();
    }

    m_applyTime      = time(nullptr);
    m_isPassive      = IsPassiveSpell(GetId()) || spellproto->Attributes == 0x80;
    m_isDeathPersist = IsDeathPersistentSpell(spellproto);
    m_isSingleTarget = IsSingleTargetSpell(spellproto);
    m_procCharges    = spellproto->procCharges;

    m_isRemovedOnShapeLost = (m_casterGuid == m_target->GetObjectGuid() &&
                              (m_spellProto->Stances || m_spellProto->Id == 24864) &&
                              !(m_spellProto->AttributesEx2 & SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT) &&
                              !(m_spellProto->Attributes & SPELL_ATTR_NOT_SHAPESHIFT));
    // Exceptions
    // Attaques circulaires
    if (m_spellProto->Id == 12292)
        m_isRemovedOnShapeLost = false;

    Unit* unitCaster = caster && caster->isType(TYPEMASK_UNIT) ? (Unit*)caster : NULL;

    m_duration = m_maxDuration = CalculateSpellDuration(spellproto, unitCaster);

    if (m_maxDuration == -1 || (m_isPassive && spellproto->DurationIndex == 0))
        m_permanent = true;
    // Fix de l'affichage dans le journal de combat des buffs tres cours.
    // Exemple: immunite des trinket PvP.
    else if (m_maxDuration < 200)
    {
        m_duration = 300;
        m_maxDuration = 300;
    }

    if (unitCaster)
    {
        if (Player* modOwner = unitCaster->GetSpellModOwner())
            modOwner->ApplySpellMod(GetId(), SPELLMOD_CHARGES, m_procCharges);
    }

    // some custom stack values at aura holder create
    switch (m_spellProto->Id)
    {
        // some auras applied with max stack
        case 24575:                                         // Brittle Armor
        case 24659:                                         // Unstable Power
        case 24662:                                         // Restless Strength
        case 26464:                                         // Mercurial Shield
            m_stackAmount = m_spellProto->StackAmount;
            break;
    }

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        m_auras[i] = nullptr;
    m_makesTargetSecondaryFocus = !IsPositiveSpell(GetSpellProto()) && (GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_DAMAGE
                                                                    ||  IsSpellHaveAura(m_spellProto, SPELL_AURA_MOD_CONFUSE)
                                                                    ||  IsSpellHaveAura(m_spellProto, SPELL_AURA_MOD_FEAR));
}

void SpellAuraHolder::AddAura(Aura *aura, SpellEffectIndex index)
{
    ASSERT(index == aura->GetEffIndex());
    m_auras[index] = aura;
}

void SpellAuraHolder::RemoveAura(SpellEffectIndex index)
{
    m_auras[index] = nullptr;
}

void SpellAuraHolder::ApplyAuraModifiers(bool apply, bool real)
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX && !IsDeleted(); ++i)
        if (Aura *aur = GetAuraByEffectIndex(SpellEffectIndex(i)))
            aur->ApplyModifier(apply, real);
}

void SpellAuraHolder::_AddSpellAuraHolder()
{
    if (!GetId())
        return;
    if (!m_target)
        return;

    // Try find slot for aura
    uint8 slot = NULL_AURA_SLOT;
    Unit* caster = GetCaster();

    // Lookup free slot
    // will be < MAX_AURAS slot (if find free) with !secondaura
    if (IsNeedVisibleSlot(caster))
    {
        if (IsPositive())                                   // empty positive slot
        {
            for (uint8 i = 0; i < MAX_POSITIVE_AURAS; i++)
            {
                if (m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + i)) == 0)
                {
                    slot = i;
                    break;
                }
            }
        }
        else                                                // empty negative slot
        {
            for (uint8 i = MAX_POSITIVE_AURAS; i < MAX_AURAS; i++)
            {
                if (m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + i)) == 0)
                {
                    slot = i;
                    break;
                }
            }
        }
    }

    // set infinity cooldown state for spells
    if (caster)
    {
        if (m_spellProto->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE)
        {
            Item* castItem = nullptr;
            if (m_castItemGuid && caster->GetTypeId() == TYPEID_PLAYER)
                castItem = ((Player*)caster)->GetItemByGuid(m_castItemGuid);
            caster->AddSpellAndCategoryCooldowns(m_spellProto, castItem ? castItem->GetEntry() : 0, nullptr, true);
        }
    }

    SetAuraSlot(slot);

    // Not update fields for not first spell's aura, all data already in fields
    if (slot < MAX_AURAS)                                   // slot found
    {
        SetAura(slot, false);
        SetAuraFlag(slot, true);
        SetAuraLevel(slot, caster ? caster->getLevel() : sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));
        UpdateAuraApplication();

        // update for out of range group members
        m_target->UpdateAuraForGroup(slot);

        UpdateAuraDuration();
    }

    //*****************************************************
    // Update target aura state flag (at 1 aura apply)
    // TODO: Make it easer
    //*****************************************************
    // Sitdown on apply aura req seated
    if (m_spellProto->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED && !m_target->IsSitState())
        m_target->SetStandState(UNIT_STAND_STATE_SIT);

    // register aura diminishing on apply
    if (getDiminishGroup() != DIMINISHING_NONE)
        m_target->ApplyDiminishingAura(getDiminishGroup(), true);

    // Update Seals information
    if (IsSealSpell(GetSpellProto()))
        m_target->ModifyAuraState(AURA_STATE_JUDGEMENT, true);

	// Conflagrate aura state
	if (GetSpellProto()->IsFitToFamily(SPELLFAMILY_WARLOCK, uint64(0x0000000000000004)))
		m_target->ModifyAuraState(AURA_STATE_CONFLAGRATE, true);

	// Faerie Fire (druid versions)
	if (m_spellProto->IsFitToFamily(SPELLFAMILY_DRUID, uint64(0x0000000000000400)))
		m_target->ModifyAuraState(AURA_STATE_FAERIE_FIRE, true);

	// Swiftmend state on Regrowth & Rejuvenation
	if (m_spellProto->IsFitToFamily(SPELLFAMILY_DRUID, uint64(0x0000000000000050)))
		m_target->ModifyAuraState(AURA_STATE_SWIFTMEND, true);

	// Deadly poison aura state
	if (m_spellProto->IsFitToFamily(SPELLFAMILY_ROGUE, uint64(0x0000000000010000)))
		m_target->ModifyAuraState(AURA_STATE_DEADLY_POISON, true);
}

void SpellAuraHolder::_RemoveSpellAuraHolder()
{
    // Remove all triggered by aura spells vs unlimited duration
    // except same aura replace case
    if (m_removeMode != AURA_REMOVE_BY_STACK)
        CleanupTriggeredSpells();

    Unit* caster = GetCaster();

    if (caster && IsPersistent())
    {
        DynamicObject *dynObj = caster->GetDynObject(GetId());
        if (dynObj)
            dynObj->RemoveAffected(m_target);
    }

    //passive auras do not get put in slots
    // Note: but totem can be not accessible for aura target in time remove (to far for find in grid)
    //if(m_isPassive && !(caster && caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->isTotem()))
    //    return false;

    uint8 slot = GetAuraSlot();

    if (slot < MAX_AURAS)
    {
        if (m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + slot)) == 0)
            return;
        SetAura(slot, true);
        SetAuraFlag(slot, false);
        SetAuraLevel(slot, caster ? caster->getLevel() : sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));
    }

    // unregister aura diminishing (and store last time)
    if (getDiminishGroup() != DIMINISHING_NONE)
        m_target->ApplyDiminishingAura(getDiminishGroup(), false);

    m_procCharges = 0;
    m_stackAmount = 1;
    UpdateAuraApplication();

    if (m_removeMode != AURA_REMOVE_BY_DELETE)
    {
        // update for out of range group members
        if (slot < MAX_AURAS)
            m_target->UpdateAuraForGroup(slot);

        //*****************************************************
        // Update target aura state flag (at last aura remove)
        //*****************************************************
        uint32 removeState = 0;
        ClassFamilyMask removeFamilyFlag = m_spellProto->SpellFamilyFlags;
        switch (m_spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_PALADIN:
                if (IsSealSpell(m_spellProto))
                    removeState = AURA_STATE_JUDGEMENT;     // Update Seals information
                break;

			case SPELLFAMILY_WARLOCK:
				if (m_spellProto->IsFitToFamilyMask(uint64(0x0000000000000004)))
					removeState = AURA_STATE_CONFLAGRATE;   // Conflagrate aura state
				break;
			case SPELLFAMILY_DRUID:
				if (m_spellProto->IsFitToFamilyMask(uint64(0x0000000000000400)))
					removeState = AURA_STATE_FAERIE_FIRE;   // Faerie Fire (druid versions)
				else if (m_spellProto->IsFitToFamilyMask(uint64(0x0000000000000050)))
				{
					removeFamilyFlag = ClassFamilyMask(uint64(0x00000000000050));
					removeState = AURA_STATE_SWIFTMEND;     // Swiftmend aura state
				}
				break;
			case SPELLFAMILY_ROGUE:
				if (m_spellProto->IsFitToFamilyMask(uint64(0x0000000000010000)))
					removeState = AURA_STATE_DEADLY_POISON; // Deadly poison aura state
				break;
        }

        // Remove state (but need check other auras for it)
        if (removeState)
        {
            bool found = false;
            Unit::SpellAuraHolderMap const& holders = m_target->GetSpellAuraHolderMap();
            for (Unit::SpellAuraHolderMap::const_iterator i = holders.begin(); i != holders.end(); ++i)
            {
                SpellEntry const *auraSpellInfo = (*i).second->GetSpellProto();
                if (auraSpellInfo->IsFitToFamily(SpellFamily(m_spellProto->SpellFamilyName), removeFamilyFlag))
                {
                    found = true;
                    break;
                }
            }

            // this has been last aura
            if (!found)
                m_target->ModifyAuraState(AuraState(removeState), false);
        }

        // reset cooldown state for spells
        if (caster)
            if (GetSpellProto()->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE)
                // note: item based cooldowns and cooldown spell mods with charges ignored (unknown existing cases)
                caster->CooldownEvent(GetSpellProto());
    }
}

void SpellAuraHolder::CleanupTriggeredSpells()
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (!m_spellProto->EffectApplyAuraName[i])
            continue;

        uint32 tSpellId = m_spellProto->EffectTriggerSpell[i];
        if (!tSpellId)
            continue;

        SpellEntry const* tProto = sSpellMgr.GetSpellEntry(tSpellId);
        if (!tProto)
            continue;

        if (GetSpellDuration(tProto) != -1)
            continue;

        // needed for spell 43680, maybe others
        // TODO: is there a spell flag, which can solve this in a more sophisticated way?
        if (m_spellProto->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_TRIGGER_SPELL &&
                GetSpellDuration(m_spellProto) == int32(m_spellProto->EffectAmplitude[i]))
            continue;

        m_target->RemoveAurasDueToSpell(tSpellId);
    }
}

bool SpellAuraHolder::ModStackAmount(int32 num)
{
    uint32 protoStackAmount = m_spellProto->StackAmount;

    // Can`t mod
    if (!protoStackAmount)
        return true;

    // Modify stack but limit it
    int32 stackAmount = m_stackAmount + num;
    if (stackAmount > (int32)protoStackAmount)
        stackAmount = protoStackAmount;
    else if (stackAmount <= 0) // Last aura from stack removed
    {
        m_stackAmount = 0;
        return true; // need remove aura
    }

    // Update stack amount
    SetStackAmount(stackAmount);
    return false;
}

void SpellAuraHolder::SetStackAmount(uint32 stackAmount)
{
    Unit *target = GetTarget();
    Unit *caster = GetCaster();
    if (!target || !caster)
        return;

    bool refresh = stackAmount >= m_stackAmount;
    if (stackAmount != m_stackAmount)
    {
        m_stackAmount = stackAmount;
        UpdateAuraApplication();

        for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            if (Aura *aur = m_auras[i])
            {
                int32 bp = aur->GetBasePoints();
                int32 amount = m_stackAmount * caster->CalculateSpellDamage(target, m_spellProto, SpellEffectIndex(i), &bp);
                // Reapply if amount change
                if (amount != aur->GetModifier()->m_amount)
                {
                    aur->ApplyModifier(false, true);
                    aur->GetModifier()->m_amount = amount;
                    aur->ApplyModifier(true, true);
                }
            }
        }
    }

    if (refresh)
        // Stack increased refresh duration
        RefreshHolder();
}

Unit* SpellAuraHolder::GetCaster() const
{
    if (GetCasterGuid() == m_target->GetObjectGuid())
        return m_target;

    return ObjectAccessor::GetUnit(*m_target, m_casterGuid);// player will search at any maps
}

bool SpellAuraHolder::IsWeaponBuffCoexistableWith(SpellAuraHolder const* ref) const
{
    // only item casted spells
    if (!GetCastItemGuid())
        return false;

    // Exclude Debuffs
    if (!IsPositive())
        return false;

    // Exclude Non-generic Buffs and Executioner-Enchant
    if (GetSpellProto()->SpellFamilyName != SPELLFAMILY_GENERIC)
        return false;

    // Exclude Stackable Buffs [ie: Blood Reserve]
    if (GetSpellProto()->StackAmount)
        return false;

    // only self applied player buffs
    if (m_target->GetTypeId() != TYPEID_PLAYER || m_target->GetObjectGuid() != GetCasterGuid())
        return false;

    Item* castItem = ((Player*)m_target)->GetItemByGuid(GetCastItemGuid());
    if (!castItem)
        return false;

    // Limit to Weapon-Slots
    if (!castItem->IsEquipped() ||
            (castItem->GetSlot() != EQUIPMENT_SLOT_MAINHAND && castItem->GetSlot() != EQUIPMENT_SLOT_OFFHAND))
        return false;

    // form different weapons
    return ref->GetCastItemGuid() && ref->GetCastItemGuid() != GetCastItemGuid();
}

bool SpellAuraHolder::IsNeedVisibleSlot(Unit const* caster) const
{
    bool totemAura = caster && caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->IsTotem();

    // Check for persistent area auras that only do damage. If it has a secondary effect, it takes
    // up a slot
    bool persistent = m_spellProto->Effect[EFFECT_INDEX_0] == SPELL_EFFECT_PERSISTENT_AREA_AURA;
    bool persistentWithSecondaryEffect = false;
    
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        // Check for persistent aura here since the effect aura is applied to the holder 
        // by a dynamic object as the target passes through the object field, meaning 
        // m_auras will be unset when this method is called (initialization)
        if (!m_auras[i] && !persistent)
            continue;
            
        // special area auras cases
        switch (m_spellProto->Effect[i])
        {
			case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
			return m_target != caster;
            case SPELL_EFFECT_APPLY_AREA_AURA_PET:
			case SPELL_EFFECT_APPLY_AREA_AURA_OWNER:
			case SPELL_EFFECT_APPLY_AREA_AURA_RAID:
			case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
            case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
                // passive auras (except totem auras) do not get placed in caster slot
                return (m_target != caster || totemAura || !m_isPassive) && m_auras[i]->GetModifier()->m_auraname != SPELL_AURA_NONE;
                
                break;
            case SPELL_EFFECT_PERSISTENT_AREA_AURA:
                // If spell aura applies something other than plain damage, it takes
                // up a debuff slot.
                if (m_spellProto->EffectApplyAuraName[i] != SPELL_AURA_PERIODIC_DAMAGE)
                    persistentWithSecondaryEffect = true;
                    
                break;
            default:
                break;
        }
    }

    /*  Persistent area auras such as Blizzard/RoF/Volley do not get require debuff slots
        since they just do area damage with no additional effects. However, spells like
        Hurricane do since they have a secondary effect attached to them. There are enough
        persistent area spells in-game that making a switch for all of them is a bit 
        unreasonable. Any spell with a secondary affect should take up a slot. Note
        that most (usable) persistent spells only deal damage.
        
        It was considered whether spells with secondary effects should still deal damage,
        even if there is no room for the other effect, however the debuff tooltip states
        that the spell causes damage AND slows, therefore it must take a debuff slot.
     */
    if (persistent && !persistentWithSecondaryEffect)
    {
        return false;
    }
    
    // necessary for some spells, e.g. Immolate visual passive 28330
    if (m_spellProto->SpellVisual)
        return true;

    // passive auras (except totem auras) do not get placed in the slots
    return !m_isPassive || totemAura;
}

void SpellAuraHolder::HandleSpellSpecificBoosts(bool apply)
{
    uint32 spellId1 = 0;
    uint32 spellId2 = 0;
    uint32 spellId3 = 0;
    uint32 spellId4 = 0;

    switch (GetSpellProto()->SpellFamilyName)
    {
		case SPELLFAMILY_GENERIC:
		{
			switch (GetId())
			{
				// Stoneform (dwarven racial)
			case 20594:
			{
				spellId1 = 20612;
				break;
			}
			default:
				return;
			}
			break;
		}
        case SPELLFAMILY_SHAMAN:
        {
            // Nostalrius : Pas de marche sur l'eau + loup fantome.
            if (apply && GetSpellProto()->Id == 2645)
                if (Unit* pCaster = GetCaster())
                    if (Aura* aura = pCaster->GetAura(546, EFFECT_INDEX_0))
                        pCaster->RemoveAura(aura);
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            switch (GetId())
            {
                case 11189:                                 // Frost Warding
                case 28332:
                {
                    if (m_target->GetTypeId() == TYPEID_PLAYER && !apply)
                    {
                        // reflection chance (effect 1) of Frost Ward, applied in dummy effect
                        if (SpellModifier *mod = ((Player*)m_target)->GetSpellMod(SPELLMOD_EFFECT2, GetId()))
                            ((Player*)m_target)->AddSpellMod(mod, false);
                    }
                    return;
                }
                default:
                    return;
            }
            break;
        }
		case SPELLFAMILY_WARRIOR:
		{
			if (!apply)
			{
				// Remove Blood Frenzy only if target no longer has any Deep Wound or Rend (applying is handled by procs)
				if (GetSpellProto()->Mechanic != MECHANIC_BLEED)
					return;

				// If target still has one of Warrior's bleeds, do nothing
				Unit::AuraList const& PeriodicDamage = m_target->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
				for (Unit::AuraList::const_iterator i = PeriodicDamage.begin(); i != PeriodicDamage.end(); ++i)
					if ((*i)->GetCasterGuid() == GetCasterGuid() &&
						(*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_WARRIOR &&
						(*i)->GetSpellProto()->Mechanic == MECHANIC_BLEED)
						return;

				spellId1 = 30069;                           // Blood Frenzy (Rank 1)
				spellId2 = 30070;                           // Blood Frenzy (Rank 2)
				spellId1 = 30071;                           // Blood Frenzy (Rank 3)
				spellId2 = 30072;                           // Blood Frenzy (Rank 4)		
				spellId2 = 30073;                           // Blood Frenzy (Rank 5)
			}
			else
				return;
			break;
		}
        case SPELLFAMILY_HUNTER:
        {
            switch (GetId())
            {
                // The Beast Within and Bestial Wrath - immunity
                case 19574:
                {
                    spellId1 = 24395;
                    spellId2 = 24396;
                    spellId3 = 24397;
                    spellId4 = 26592;
                    break;
                }
				// Misdirection, main spell
				case 34477:
				{
					if (!apply)
						m_target->getHostileRefManager().ResetThreatRedirection();
					return;
				}
                default:
                    return;
            }
            break;
        }
        default:
            return;
    }

    // prevent aura deletion, specially in multi-boost case
    SetInUse(true);

    if (apply)
    {
        if (spellId1)
            m_target->CastSpell(m_target, spellId1, true, nullptr, nullptr, GetCasterGuid());
        if (spellId2 && !IsDeleted())
            m_target->CastSpell(m_target, spellId2, true, nullptr, nullptr, GetCasterGuid());
        if (spellId3 && !IsDeleted())
            m_target->CastSpell(m_target, spellId3, true, nullptr, nullptr, GetCasterGuid());
        if (spellId4 && !IsDeleted())
            m_target->CastSpell(m_target, spellId4, true, nullptr, nullptr, GetCasterGuid());
    }
    else
    {
        if (spellId1)
            m_target->RemoveAurasByCasterSpell(spellId1, GetCasterGuid());
        if (spellId2)
            m_target->RemoveAurasByCasterSpell(spellId2, GetCasterGuid());
        if (spellId3)
            m_target->RemoveAurasByCasterSpell(spellId3, GetCasterGuid());
        if (spellId4)
            m_target->RemoveAurasByCasterSpell(spellId4, GetCasterGuid());
    }

    SetInUse(false);
}

void Aura::HandleAuraSafeFall(bool Apply, bool Real)
{
    // implemented in WorldSession::HandleMovementOpcodes
}

void Aura::HandleFactionOverride(bool apply, bool Real)
{
	if (!Real)
		return;

	Unit* target = GetTarget();
	if (!target || !sFactionTemplateStore.LookupEntry(GetMiscValue()))
		return;

	if (apply)
		target->setFaction(GetMiscValue());
	else
		target->RestoreOriginalFaction();
}

SpellAuraHolder::~SpellAuraHolder()
{
    // note: auras in delete list won't be affected since they clear themselves from holder when adding to deletedAuraslist
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        delete m_auras[i];

    delete _pveHeartBeatData;
}

void SpellAuraHolder::Update(uint32 diff)
{
    // Battements de coeur : 2 fonctionnements.
    // PvP
    if (_heartBeatRandValue)
    {
        float elapsedTime = (m_maxDuration - m_duration) / 1000.0f;
        static const float averageBreakTime = 12.0f; // 50% chance to break after 12 secs
        static const float chanceBreakAt15 = 1.0f; // 1% break > 15 secs. Patch 1.2: "Players now have an increasing chance to break free of the effect, such that it is unlikely the effect will last more than 15 seconds."
        static const float coeff = (1.0f / (15 - averageBreakTime)) * log((100 - chanceBreakAt15) / chanceBreakAt15);
        float currHeartBeatValue = 100.0f / (1.0f + exp(coeff * (averageBreakTime - elapsedTime)));
        DEBUG_UNIT(GetTarget(), DEBUG_DR, "|HB Duration [Curr%.2f|Max%u]. Value[Curr%.2f|Limit%.2f]",
                           elapsedTime, m_maxDuration / 1000, currHeartBeatValue, _heartBeatRandValue);
        if (_heartBeatRandValue <=  currHeartBeatValue)
        {
            if (Unit* pTarget = GetTarget())
                pTarget->RemoveSpellAuraHolder(this);
            return;
        }
    }
    // PvE
    if (_pveHeartBeatData)
    {
        if (_pveHeartBeatData->timer <= diff)
        {
            _pveHeartBeatData->timer += 5000 - diff;
            if (Unit* pTarget = GetTarget())
            {
                uint32 missChance = 10000 - _pveHeartBeatData->hitChance;
                uint32 rand = urand(0, 10000);

                if (rand < missChance)
                {
                    delete _pveHeartBeatData;
                    _pveHeartBeatData = nullptr;
                    pTarget->RemoveSpellAuraHolder(this);
                    return;
                }
            }
        }
        else
            _pveHeartBeatData->timer -= diff;
    }

    if (m_duration > 0)
    {
        m_duration -= diff;
        if (m_duration < 0)
            m_duration = 0;

        m_timeCla -= diff;

        if (m_timeCla <= 0)
        {
            if (Unit* caster = GetCaster())
            {
                Powers powertype = Powers(GetSpellProto()->powerType);
                int32 manaPerSecond = GetSpellProto()->manaPerSecond + GetSpellProto()->manaPerSecondPerLevel * caster->getLevel();
                m_timeCla = 1 * IN_MILLISECONDS;

                if (manaPerSecond)
                {
                    if (powertype == POWER_HEALTH)
                        caster->ModifyHealth(-manaPerSecond);
                    else
                        caster->ModifyPower(powertype, -manaPerSecond);
                }
            }
        }
    }

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aura = m_auras[i])
            aura->UpdateAura(diff);

    // Channeled aura required check distance from caster
    if (IsChanneledSpell(m_spellProto) && GetCasterGuid() != m_target->GetObjectGuid())
    {
        Unit* caster = GetCaster();
        if (!caster)
        {
            m_target->RemoveAurasByCasterSpell(GetId(), GetCasterGuid());
            return;
        }

        // Nostalrius : mise en combat
        if (!(GetSpellProto()->AttributesEx3 & SPELL_ATTR_EX3_NO_INITIAL_AGGRO) && !IsPositive() &&
                caster->isVisibleForOrDetect(m_target, m_target, false))
        {
            m_target->SetInCombatWith(caster);
            caster->SetInCombatWith(m_target);
        }
        // unlimited range
        if (m_spellProto->Custom & SPELL_CUSTOM_CHAN_NO_DIST_LIMIT)
            return;

        // need check distance for channeled target only
        if (caster->GetChannelObjectGuid() == m_target->GetObjectGuid())
        {
            // spell range + guessed interrupt leeway range
            float max_range = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellProto->rangeIndex));

            if (m_target->IsHostileTo(caster))
                max_range *= 1.33f;

            if (Player* modOwner = caster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetId(), SPELLMOD_RANGE, max_range, nullptr);

            if (caster->GetCombatDistance(m_target) > max_range)
                caster->InterruptSpell(CURRENT_CHANNELED_SPELL);
        }
    }
}

void SpellAuraHolder::RefreshHolder()
{
    SetAuraDuration(GetAuraMaxDuration());
    UpdateAuraDuration();
}

void SpellAuraHolder::SetAuraMaxDuration(int32 duration)
{
    m_maxDuration = duration;

    // possible overwrite persistent state
    if (duration > 0)
    {
        if (!(IsPassive() && GetSpellProto()->DurationIndex == 0))
            SetPermanent(false);
    }
}

bool SpellAuraHolder::HasMechanic(uint32 mechanic) const
{
    if (mechanic == m_spellProto->Mechanic)
        return true;

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (m_auras[i] && m_spellProto->EffectMechanic[i] == mechanic)
            return true;
    return false;
}

bool SpellAuraHolder::HasMechanicMask(uint32 mechanicMask) const
{
    if (m_spellProto->Mechanic && mechanicMask & (1 << (m_spellProto->Mechanic - 1)))
        return true;

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (m_auras[i] && m_spellProto->EffectMechanic[i] && ((1 << (m_spellProto->EffectMechanic[i] - 1)) & mechanicMask))
            return true;
    return false;
}

bool SpellAuraHolder::IsPersistent() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = m_auras[i])
            if (aur->IsPersistent())
                return true;
    return false;
}

bool SpellAuraHolder::IsAreaAura() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = m_auras[i])
            if (aur->IsAreaAura())
                return true;
    return false;
}

bool SpellAuraHolder::IsPositive() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (Aura *aur = m_auras[i])
            if (!aur->IsPositive())
                return false;
    return true;
}

bool SpellAuraHolder::IsEmptyHolder() const
{
    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (m_auras[i])
            return false;
    return true;
}

void SpellAuraHolder::UnregisterSingleCastHolder()
{
    if (IsSingleTarget())
    {
        if (Unit* caster = GetCaster())
            caster->GetSingleCastSpellTargets().erase(GetSpellProto());

        m_isSingleTarget = false;
    }
}

void SpellAuraHolder::SetAuraFlag(uint32 slot, bool add)
{
    uint32 index    = slot >> 3;
    uint32 byte     = (slot & 7) << 2;
    uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURAFLAGS + index);
    if (add)
        val |= ((uint32)AFLAG_MASK << byte);
    else
        val &= ~((uint32)AFLAG_MASK << byte);

    m_target->SetUInt32Value(UNIT_FIELD_AURAFLAGS + index, val);
}

void SpellAuraHolder::SetAuraLevel(uint32 slot, uint32 level)
{
    uint32 index    = slot / 4;
    uint32 byte     = (slot % 4) * 8;
    uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURALEVELS + index);
    val &= ~(0xFF << byte);
    val |= (level << byte);
    m_target->SetUInt32Value(UNIT_FIELD_AURALEVELS + index, val);
}

void SpellAuraHolder::UpdateAuraApplication()
{
    if (m_auraSlot >= MAX_AURAS)
        return;

    uint32 stackCount = m_procCharges > 0 ? m_procCharges * m_stackAmount : m_stackAmount;

    uint32 index    = m_auraSlot / 4;
    uint32 byte     = (m_auraSlot % 4) * 8;
    uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURAAPPLICATIONS + index);
    val &= ~(0xFF << byte);
    // field expect count-1 for proper amount show, also prevent overflow at client side
    val |= ((uint8(stackCount <= 255 ? stackCount - 1 : 255 - 1)) << byte);
    m_target->SetUInt32Value(UNIT_FIELD_AURAAPPLICATIONS + index, val);
}

void SpellAuraHolder::UpdateAuraDuration() const
{
    if (GetAuraSlot() >= MAX_AURAS || m_isPassive)
        return;

    if (m_target->GetTypeId() == TYPEID_PLAYER)
    {
        WorldPacket data(SMSG_UPDATE_AURA_DURATION, 5);
        data << uint8(GetAuraSlot());
        data << uint32(GetAuraDuration());
        ((Player*)m_target)->SendDirectMessage(&data);
    }
}

void SpellAuraHolder::SetAffectedByDebuffLimit(bool isAffectedByDebuffLimit)
{
    m_debuffLimitAffected = isAffectedByDebuffLimit;
}

/** NOSTALRIUS
 Debuff limitation
    Categories:
     * 4 (highest)  Highest Priority Debuffs (Generally, a debuff that alters the ability of a mob to act normally)
     * 3            Generally, a standalone debuff ability intentionally casted that gives benefits group/raid wide
     * 2            Generally, a debuff that is a secondary result of some other ability but still gives raid-wide benefits
     * 1            Generally, debuffs that are intentionally cast, stand-alone damage-over-time abilities
     * 0            Generally, a debuff that is a secondarily applied damage-over-time effect
  */
void SpellAuraHolder::CalculateForDebuffLimit()
{
    m_debuffLimitAffected = true;
    m_debuffLimitScore = 0;


    // First, some exceptions
    switch (m_spellProto->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (m_spellProto->Id)
            {
                // Hakkar's Blood Siphon
                case 24323:
                case 24322:
                    m_debuffLimitScore = 4; 
                    return;
            }
        }
        case SPELLFAMILY_PALADIN:
        {
            switch (m_spellProto->Id)
            {
                // Judgement of Wisdom (3 ranks)
                case 20186:
                case 20354:
                case 20355:
                // Judgement of Light
                case 20185: // r1
                case 20344: // r2
                case 20345: // r3
                case 20346: // r4
                    m_debuffLimitScore = 3;
                    return;
                // Judgement of the Crusader (6 ranks)
                case 21183:
                case 20188:
                case 20300:
                case 20301:
                case 20302:
                case 20303:
                    m_debuffLimitScore = 2;
                    return;
            }
        }
        case SPELLFAMILY_MAGE:
        {
            switch (m_spellProto->Id)
            {
                // Ignite talent trigger
                case 12654:
                    m_debuffLimitScore = 2;
                    return;
            }
        }
    }

    // Fireball
    if (m_spellProto->IsFitToFamily<SPELLFAMILY_MAGE, CF_MAGE_FIREBALL, CF_MAGE_CONJURE>())
    {
        m_debuffLimitScore = 0;
        return;
    }

    // Gestion des priorites par type d'aura
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (!m_spellProto->EffectApplyAuraName[i])
            continue;

        uint32 currEffectScore = 0;
        switch (m_spellProto->EffectApplyAuraName[i])
        {
            case SPELL_AURA_MOD_TAUNT:
            case SPELL_AURA_MOD_THREAT:
            case SPELL_AURA_MOD_CHARM:
            case SPELL_AURA_MOD_FEAR:
            case SPELL_AURA_MOD_CONFUSE:
            case SPELL_AURA_MOD_POSSESS:
            case SPELL_AURA_MOD_STUN:
                currEffectScore = 4;
                break;
            case SPELL_AURA_MOD_RESISTANCE:
            case SPELL_AURA_MOD_LANGUAGE:                   // Language curse
            case SPELL_AURA_MOD_STALKED:                    // Hunter mark
            case SPELL_AURA_MOD_DISARM:
            case SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN:
            case SPELL_AURA_PREVENTS_FLEEING:               // Curse of recklessness
            case SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE: // Winter's Chill
            case SPELL_AURA_MOD_MELEE_HASTE:
            case SPELL_AURA_MOD_ATTACK_POWER:
            case SPELL_AURA_MOD_DAMAGE_DONE:
            case SPELL_AURA_MOD_DAMAGE_TAKEN:
            case SPELL_AURA_MOD_HEALING_PCT:                // Mortal Strike
                currEffectScore = 3;
                break;
            case SPELL_AURA_PERIODIC_DAMAGE:
            case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
            case SPELL_AURA_PERIODIC_LEECH:
                currEffectScore = 1;
                break;
            case SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS: // Expose Weakness
                m_debuffLimitScore = 3;
                return;
            case SPELL_AURA_DUMMY:
                if (m_spellProto->SpellVisual == 3582 && m_spellProto->SpellIconID == 150) // Vampiric Embrace
                {
                    m_debuffLimitScore = 3;
                    return;
                }
                break;
        }
        if (currEffectScore > m_debuffLimitScore)
            m_debuffLimitScore = currEffectScore;
    }

    if (IsTriggered())
    {
        if (m_debuffLimitScore > 2)
            m_debuffLimitScore = 2;
        else
            m_debuffLimitScore = 0;
    }
    else if (IsChanneledSpell(GetSpellProto()) && m_debuffLimitScore < 1)
        m_debuffLimitScore = 1;
}

// Fix premiers tics
void Aura::CalculatePeriodic(Player * modOwner, bool create)
{
    //m_modifier.periodictime = GetSpellProto()->EffectAmplitude[m_effIndex];

    // prepare periodics
    switch (GetSpellProto()->EffectApplyAuraName[m_effIndex])
    {
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_PERIODIC_ENERGIZE:
        case SPELL_AURA_OBS_MOD_HEALTH:
        case SPELL_AURA_PERIODIC_LEECH:
        case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
        case SPELL_AURA_PERIODIC_MANA_LEECH:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
        case SPELL_AURA_POWER_BURN_MANA:
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
            m_isPeriodic = true;
            break;
        default:
            break;
    }

    if (!m_isPeriodic)
        return;

    // Apply casting time mods
    if (m_modifier.periodictime && modOwner)
    {
        // Apply periodic time mod
        modOwner->ApplySpellMod(GetId(), SPELLMOD_ACTIVATION_TIME, m_modifier.periodictime);
    }

    // Totem griffes de pierre
    if (GetSpellProto()->SpellVisual == 0 && GetSpellProto()->SpellIconID == 689)
        return;

    switch (GetId())
    {
        case 8145:  // Tremor Tortem Passive
        case 6474:  // Earthbind Totem Passive
        case 8179:  // Grounding Totem Passive
        case 8172:  // Disease Cleansing Totem Passive
        case 8167:  // Poison Cleansing Totem Passive
        case 8515:  // Windfury Totem Passive (Rank 1)
        case 10609: // Windfury Totem Passive (Rank 2)
        case 10612: // Windfury Totem Passive (Rank 3)
        case 13797: // Immolation Trap Effect (Rank 1)
        case 14298: // Immolation Trap Effect (Rank 2)
        case 14299: // Immolation Trap Effect (Rank 3)
        case 14300: // Immolation Trap Effect (Rank 4)
        case 14301: // Immolation Trap Effect (Rank 5)
        case 23184: // Mark of Frost
        case 25041: // Mark of Nature
            break;
        default:
            m_periodicTimer = m_modifier.periodictime;
    }
}

// Battements de coeur (chance de briser les CC)
void SpellAuraHolder::CalculateHeartBeat(Unit* caster, Unit* target)
{
    _heartBeatRandValue = 0;

    // Ni les sorts permanents, ni les sorts positifs ne sont affectes.
    // Check si positif fait dans Aura::Aura car ici le dernier Aura ajoute n'est pas encore dans 'm_auras'
    if (!m_permanent && m_maxDuration > 10000)
    {
        if (m_spellProto->Attributes & SPELL_ATTR_DIMINISHING_RETURNS
                // Exception pour la coiffe de controle gnome/Heaume-fusee gobelin
                || m_spellProto->Id == 13181 || m_spellProto->Id == 13327)
        {
            // Pour les joueurs
            if (target->GetCharmerOrOwnerPlayerOrPlayerItself())
                _heartBeatRandValue = rand_norm_f() * 100.0f;
        }
        // En PvE. Ne concerne pas certains sorts avec DR (fear geres avec dmg par exemple).
        if (caster && target->GetTypeId() == TYPEID_UNIT && m_spellProto->IsPvEHeartBeat())
        {
            _pveHeartBeatData = new HeartBeatData;
            _pveHeartBeatData->timer = 5000;
            _pveHeartBeatData->hitChance = caster->MagicSpellHitChance(target, m_spellProto);
        }
    }
}

void Aura::HandleInterruptRegen(bool apply, bool real)
{
    if (!real)
        return;
    if (!apply)
        return;
    GetTarget()->SetInCombatState(false);
}

// Un nouvel aura ...
void Aura::HandleAuraAuraSpell(bool apply, bool real)
{
    Unit* target = GetTarget();
    if (!real || !target)
        return;
    uint32 spell = GetSpellProto()->EffectTriggerSpell[m_effIndex];
    if (apply)
        target->AddAura(spell, ADD_AURA_PASSIVE, target);
    else
        target->RemoveAurasDueToSpell(spell);
}


// Auras exclusifs
/*
Sorts d'exemple:
11364 (+50), 21849 (+15)
*/

bool _IsExclusiveSpellAura(SpellEntry const* spellproto, SpellEffectIndex eff, AuraType auraname)
{
    // Flametongue Totem / Totem of Wrath / Strength of Earth Totem / Fel Intelligence / Leader of the Pack
    // Moonkin Aura / Mana Spring Totem / Tree of Life Aura / Improved Devotion Aura / Improved Icy Talons / Trueshot Aura
    // Improved Moonkin Form / Sanctified Retribution Aura / Blood Pact
    if (spellproto->Effect[eff] == SPELL_EFFECT_APPLY_AREA_AURA_RAID)
        return false;

    // Exceptions - se stack avec tout.
    switch (spellproto->Id)
    {
        // Terres foudroyees et Zanza
        case 10693:
        case 10691: // +25 esprit
        case 10668:
        case 10671: // +25 endu
        case 10667:
        case 10670: // +25 force
        case 10669:
        case 10672: // +25 agi
        case 10692:
        case 10690: // +25 intel
        case 24382:             // Buff zanza
        // Alcools
        case 25804:
        case 20875:
        case 25722:
        case 25037:
        case 22789:
        case 22790:
        case 6114:
        case 5020:
        case 5021:
        case 23179: //le proc de l'�p�e de Razorgore (+300 force) devrait se cumuler avec TOUT : ID 23179
        case 20007: //le proc Crois� devrait se cumuler avec TOUT : ID 20007
        case 20572: //Le racial Orc (ID 20572)
        case 17038: //l'Eau des Tombe-Hiver (ID 17038)
        case 16329: //le Juju's Might (ID 16329)
        case 25891: //le buff du Bijou Choc de Terre (ID : 25891)
        case 18264: // Charge du maitre (baton epique scholo)
        case 12022: // Chapeau pirate
        case 22817: // PA HT Nord
        // Aura de precision (hunt)
        case 19506:
        case 20905:
        case 20906:
        case 18262: // Pierre � Aiguiser El�mentaire (+2% crit)
        case 24932: // chef de la Meute
        case 24907: // aura S�l�nien
        case 22888: // Buff Onyxia
        case 15366: // Buff Felwood
        case 22820: // HT +3% crit sorts
        case 17628: // Supreme Power
        case 22730: // Bouffe +10 Intell
        case 18141: // Bouffe +10 Esprit
        case 18125: // Bouffe +10 Force
        case 18192: // Bouffe +10 Agility
        case 18191: // Bouffe +10 Endu
        case 25661: // Bouffe +25 Endu
            return false;

        case 17538: // Le +crit du buff de l'Elixir de la Mangouste 17538, devrait se stack avec TOUT.
            return (eff == EFFECT_INDEX_0);
    }
    switch (spellproto->SpellFamilyName)
    {
        case SPELLFAMILY_WARLOCK:
            // Blood Pact
            if (spellproto->IsFitToFamilyMask<CF_WARLOCK_IMP_BUFFS>())
                return false;
            break;
        case SPELLFAMILY_SHAMAN:
            // Strength of Earth (ID 8076, 8162, 8163, 10441, 25362)
            if (spellproto->IsFitToFamilyMask<CF_SHAMAN_STRENGTH_OF_EARTH>())
                return false;
            break;
        case SPELLFAMILY_WARRIOR:
            // Battle Shout (ID 6673, 5242, 6192, 11549, 11550, 11551, 25289)
            if (spellproto->IsFitToFamilyMask<CF_WARRIOR_BATTLE_SHOUT>())
                return false;
            break;
        case SPELLFAMILY_PALADIN:
            // Blessing of Might (ID 19740, 19834, 19835, 19836, 19837, 19838, 25291, 25782, 25916)
            if (spellproto->IsFitToFamilyMask<CF_PALADIN_BLESSING_OF_MIGHT, CF_PALADIN_BLESSINGS>())
                return false;
            break;
        case SPELLFAMILY_HUNTER:
            // Aspect of the Hawk
            if (spellproto->IsFitToFamilyMask<CF_HUNTER_ASPECT_OF_THE_HAWK>())
                return false;
            break;
    }

    // La bouffe
    if (spellproto->AttributesEx2 & SPELL_ATTR_EX2_FOOD_BUFF)
        return false;

    switch (auraname)
    {
        //case SPELL_AURA_PERIODIC_DAMAGE:
        //case SPELL_AURA_DUMMY:
        //    return false;
        case SPELL_AURA_MOD_HEALING_DONE:                               // Demonic Pact
        case SPELL_AURA_MOD_DAMAGE_DONE:                                // Demonic Pact
        case SPELL_AURA_MOD_ATTACK_POWER_PCT:                           // Abomination's Might / Unleashed Rage
        case SPELL_AURA_MOD_RANGED_ATTACK_POWER_PCT:
        case SPELL_AURA_MOD_ATTACK_POWER:                               // (Greater) Blessing of Might / Battle Shout
        case SPELL_AURA_MOD_RANGED_ATTACK_POWER:
        case SPELL_AURA_MOD_POWER_REGEN:                                // (Greater) Blessing of Wisdom
        case SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN:                       // Glyph of Salvation / Pain Suppression / Safeguard ?
        case SPELL_AURA_MOD_STAT:
            return true;
        case SPELL_AURA_MOD_SPELL_CRIT_CHANCE:
            return true;
        case SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE:                 // Winter's Chill / Improved Scorch
            if (spellproto->SpellFamilyName == SPELLFAMILY_MAGE)
                return false;
            return true;
        case SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE: // A gere autrement (exlusif par rapport a la resistance)
            return false;
        case SPELL_AURA_MOD_HEALING_PCT:                                // Mortal Strike / Wound Poison / Aimed Shot / Furious Attacks
            // Healing taken debuffs
            if (spellproto->EffectBasePoints[eff] < 0)
                return false;
            return true;
        case SPELL_AURA_MOD_RESISTANCE_PCT:
            // Ancestral Healing / Inspiration
            if (spellproto->SpellFamilyName == SPELLFAMILY_SHAMAN ||
                    spellproto->SpellFamilyName == SPELLFAMILY_PRIEST)
                return false;
            return true;
        default:
            return false;
    }
}

void Aura::ComputeExclusive()
{
    m_exclusive = false;
    //return;
    if (GetHolder()->IsPassive() || !GetHolder()->IsPositive())
        return;
    m_exclusive = _IsExclusiveSpellAura(GetSpellProto(), GetEffIndex(), GetModifier()->m_auraname);
}

// Resultat :
// - 0 : pas dans la meme categorie.
// - 1 : je suis plus important. Je m'applique.
// - 2 : il est plus important. Il s'applique.
int Aura::CheckExclusiveWith(Aura const* other) const
{
    ASSERT(IsExclusive());
    ASSERT(other);
    ASSERT(other->IsExclusive());
    if (other->GetModifier()->m_auraname != GetModifier()->m_auraname)
        return 0;
    if (other->GetModifier()->m_miscvalue != GetModifier()->m_miscvalue)
        return 0;
    if (other->GetSpellProto()->EffectItemType[other->GetEffIndex()] != GetSpellProto()->EffectItemType[GetEffIndex()])
        return 0;

    // Lui est mieux
    if (other->GetModifier()->m_amount > GetModifier()->m_amount)
        return 2;
    return 1;
}

bool Aura::ExclusiveAuraCanApply()
{
    Unit* target = GetTarget();
    ASSERT(target);
    if (Aura* mostImportant = target->GetMostImportantAuraAfter(this, this))
    {
        // Il y a un souci dans le sort.
        if (mostImportant->IsInUse())
        {
            sLog.outInfo("[%s:Map%u:Aura%u:AuraImportant%u] Aura::ExclusiveAuraCanApply IsInUse", target->GetName(), target->GetMapId(), GetId(), mostImportant->GetId());
            return false;
        }
        ASSERT(!mostImportant->IsInUse());
        int checkResult = CheckExclusiveWith(mostImportant);
        switch (checkResult)
        {
            case 1: // Je suis plus important.
                // Normalement, 'mostImportant' en etant le plus important de sa categorie
                // doit etre applique.
                if (!mostImportant->IsApplied())
                {
                    sLog.outInfo("[%s:Map%u:Aura%u:AuraImportant%u] Aura::ExclusiveAuraCanApply IsApplied", target->GetName(), target->GetMapId(), GetId(), mostImportant->GetId());
                    return false;
                }
                ASSERT(mostImportant->IsApplied());
                // On le desactive, et je m'active.
                mostImportant->ApplyModifier(false, true, true);
                break;
            case 2: // Il est plus important, je le laisse.
                return false;
            case 0: // Impossible.
                ASSERT(false);
            default: // Impossible aussi
                ASSERT(false);
        }
    }
    // Pas d'autre aura trouve, je m'applique.
    return true;
}

void Aura::ExclusiveAuraUnapply()
{
    Unit* target = GetTarget();
    ASSERT(target);
    // On restaure le precedent plus grand aura.
    if (Aura* mostImportant = target->GetMostImportantAuraAfter(this, this))
    {
        if (mostImportant->IsInUse())
        {
            sLog.outInfo("[%s:Map%u:Aura%u:AuraImportant%u] Aura::ExclusiveAuraUnapply IsInUse", target->GetName(), target->GetMapId(), GetId(), mostImportant->GetId());
            return;
        }
        if (mostImportant->IsApplied())
        {
            sLog.outInfo("[%s:Map%u:Aura%u:AuraImportant%u] Aura::ExclusiveAuraUnapply IsApplied", target->GetName(), target->GetMapId(), GetId(), mostImportant->GetId());
            return;
        }
        ASSERT(!mostImportant->IsInUse());
        ASSERT(!mostImportant->IsApplied());
        mostImportant->ApplyModifier(true, true, true);
    }
}
