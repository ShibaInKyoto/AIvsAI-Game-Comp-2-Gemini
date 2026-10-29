export type GameMode = "campaign" | "endless" | "bossrush";

export type ShipClassId = "vanguard" | "chrono" | "titan" | "specter";

export interface ShipClass {
  id: ShipClassId;
  name: string;
  tagline: string;
  hull: number;
  shield: number;
  speed: number;
  turnSpeed: number;
  dashCooldown: number;
  specialAbility: string;
  primaryWeapon: string;
  accentColor: string;
  glowColor: string;
}

export interface Vector2D {
  x: number;
  y: number;
}

export interface PlayerState {
  x: number;
  y: number;
  vx: number;
  vy: number;
  angle: number;
  hull: number;
  maxHull: number;
  shield: number;
  maxShield: number;
  shieldRegenTimer: number;
  chronoEnergy: number;
  maxChronoEnergy: number;
  isDashing: boolean;
  dashTimer: number;
  dashCooldownTimer: number;
  shipClass: ShipClassId;
  level: number;
  exp: number;
  expNext: number;
  credits: number;
  upgrades: string[];
  weaponLevel: number;
  activeModules: string[];
  killCount: number;
  combo: number;
  comboTimer: number;
  invulnerableTimer: number;
}

export type EnemyType =
  | "drone_scout"
  | "dart_striker"
  | "heavy_shielder"
  | "void_sniper"
  | "plasma_weaver"
  | "guardian_boss";

export interface Enemy {
  id: number;
  type: EnemyType;
  x: number;
  y: number;
  vx: number;
  vy: number;
  angle: number;
  hp: number;
  maxHp: number;
  radius: number;
  scoreValue: number;
  expValue: number;
  color: string;
  fireTimer: number;
  behaviorTimer: number;
  shieldAngle?: number;
  telegraphTimer?: number;
  phase?: number;
}

export interface Projectile {
  id: number;
  x: number;
  y: number;
  vx: number;
  vy: number;
  radius: number;
  damage: number;
  isEnemy: boolean;
  color: string;
  life: number;
  maxLife: number;
  piercing?: boolean;
  homingTarget?: number;
  isMissile?: boolean;
}

export interface Particle {
  x: number;
  y: number;
  vx: number;
  vy: number;
  life: number;
  maxLife: number;
  color: string;
  size: number;
}

export interface DataShard {
  id: number;
  x: number;
  y: number;
  vx: number;
  vy: number;
  value: number;
  type: "exp" | "credit" | "repair";
}

export interface UpgradeCard {
  id: string;
  name: string;
  category: "weapon" | "defense" | "mobility" | "utility" | "evolution";
  rarity: "common" | "rare" | "epic" | "legendary";
  description: string;
  statModifier: string;
  iconName: string;
  catalystRequired?: string;
}

export interface GameSettings {
  soundVolume: number;
  musicVolume: number;
  screenShake: boolean;
  bloom: boolean;
  crtScanlines: boolean;
  showFps: boolean;
  aiPilotEnabled: boolean;
  reducedFlashing: boolean;
}
