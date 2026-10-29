import { UpgradeCard } from "./types.ts";

export const UPGRADE_CARDS: UpgradeCard[] = [
  // --- WEAPON UPGRADES ---
  {
    id: "split_shot",
    name: "Split Shot Matrix",
    category: "weapon",
    rarity: "common",
    description: "Adds +2 angled secondary barrels firing kinetic spread shards.",
    statModifier: "+2 Projectiles, 15° Spread",
    iconName: "Split",
  },
  {
    id: "high_frequency_plasma",
    name: "HF Plasma Emitter",
    category: "weapon",
    rarity: "common",
    description: "Overclocks firing capacitor for lightning-fast fire rate.",
    statModifier: "+30% Fire Rate, -10% Heat",
    iconName: "Zap",
  },
  {
    id: "heavy_slugs",
    name: "Depleted Uranium Slugs",
    category: "weapon",
    rarity: "rare",
    description: "Massive solid kinetic slugs punch directly through smaller targets.",
    statModifier: "+40% Damage, +1 Piercing",
    iconName: "Crosshair",
  },
  {
    id: "seeking_micro_missiles",
    name: "Swarm Micro-Missiles",
    category: "weapon",
    rarity: "epic",
    description: "Fires autonomous micro-missiles that track the nearest hostile.",
    statModifier: "Launches 3 Tracking Missiles/sec",
    iconName: "Rocket",
  },
  {
    id: "arc_coil",
    name: "Tesla Arc Coil",
    category: "weapon",
    rarity: "rare",
    description: "Primary shots discharge chain lightning across 3 adjacent enemies.",
    statModifier: "Chain Lightning 60% Dmg",
    iconName: "Sparkles",
  },
  {
    id: "orbital_saw_blades",
    name: "Chrono-Saw Orbiters",
    category: "weapon",
    rarity: "rare",
    description: "Deploys 2 spinning saw blades orbiting your ship that shred enemies.",
    statModifier: "2 Orbiting Contact Saws",
    iconName: "Disc",
  },

  // --- DEFENSE & HULL UPGRADES ---
  {
    id: "reactive_barrier",
    name: "Reactive Force Shield",
    category: "defense",
    rarity: "common",
    description: "Strengthens energy matrix to absorb higher impact damage.",
    statModifier: "+40 Max Shield, +20% Shield Regen",
    iconName: "Shield",
  },
  {
    id: "nanite_repair",
    name: "Nanite Fabrication Swarm",
    category: "defense",
    rarity: "rare",
    description: "Hull-repairing nanites automatically reconstruct damaged plating.",
    statModifier: "Regenerates 1.5 Hull/sec",
    iconName: "HeartPulse",
  },
  {
    id: "emp_counter_burst",
    name: "EMP Retaliation Core",
    category: "defense",
    rarity: "epic",
    description: "Taking hull damage discharges a shockwave erasing enemy bullets.",
    statModifier: "Screen Bullet Clear on Hit",
    iconName: "Radio",
  },

  // --- MOBILITY & TIME DILATION ---
  {
    id: "chronoflux_capacitor",
    name: "Chrono-Flux Overdrive",
    category: "mobility",
    rarity: "rare",
    description: "Doubles maximum bullet-time duration and accelerates energy recharge.",
    statModifier: "+100% Slomo Energy, +50% Recharge",
    iconName: "Hourglass",
  },
  {
    id: "vector_thrusters",
    name: "Vector Maneuver Thrusters",
    category: "mobility",
    rarity: "common",
    description: "Upgraded gimbal thrusters for instant turning and high top speed.",
    statModifier: "+25% Top Speed, +35% Turn Rate",
    iconName: "Gauge",
  },
  {
    id: "quantum_dash",
    name: "Phase-Shift Warp Dash",
    category: "mobility",
    rarity: "epic",
    description: "Dash causes your ship to teleport forward, damaging enemies passed through.",
    statModifier: "+50% Dash Dist, 120 Phase Damage",
    iconName: "FastForward",
  },

  // --- UTILITY & SCORING ---
  {
    id: "magnetic_siphon",
    name: "Graviton Shard Magnet",
    category: "utility",
    rarity: "common",
    description: "Extends data shard attraction radius across half the combat arena.",
    statModifier: "+150% Shard Pickup Range",
    iconName: "Magnet",
  },
  {
    id: "overclock_reactor",
    name: "Overclocked Dark Matter Core",
    category: "utility",
    rarity: "rare",
    description: "Increases overall combat experience gains and combo multiplier duration.",
    statModifier: "+30% EXP Gain, +2s Combo Window",
    iconName: "Flame",
  },

  // --- LEGENDARY EVOLUTIONS ---
  {
    id: "evo_saw_storm",
    name: "EVOLUTION: Saw Tempest",
    category: "evolution",
    rarity: "legendary",
    description: "Evolves Chrono-Saws into a razor vortex throwing projectile blades.",
    statModifier: "4 Blades + Continuous Shard Throw",
    iconName: "Tornado",
    catalystRequired: "orbital_saw_blades",
  },
  {
    id: "evo_singularity_missiles",
    name: "EVOLUTION: Void Singularity",
    category: "evolution",
    rarity: "legendary",
    description: "Micro-missiles detonate into miniature black holes pulling enemies.",
    statModifier: "Gravity Well Detonations",
    iconName: "CircleDot",
    catalystRequired: "seeking_micro_missiles",
  },
  {
    id: "evo_plasma_god",
    name: "EVOLUTION: Supernova Lance",
    category: "evolution",
    rarity: "legendary",
    description: "Primary railgun transforms into an unstoppable continuous plasma beam.",
    statModifier: "Piercing 2400 RPM Death Beam",
    iconName: "Sun",
    catalystRequired: "heavy_slugs",
  },
];

export function getRandomUpgradeCards(count = 3, existingUpgrades: string[] = []): UpgradeCard[] {
  const eligible = UPGRADE_CARDS.filter((card) => {
    if (card.category === "evolution") {
      // Must own the catalyst card and not already own this evolution
      return card.catalystRequired && existingUpgrades.includes(card.catalystRequired) && !existingUpgrades.includes(card.id);
    }
    // Don't repeat unique legendary / epics if already maxed
    return !existingUpgrades.includes(card.id);
  });

  const shuffled = [...eligible].sort(() => 0.5 - Math.random());
  return shuffled.slice(0, count);
}
