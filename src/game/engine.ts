import {
  PlayerState,
  Enemy,
  EnemyType,
  Projectile,
  Particle,
  DataShard,
  ShipClassId,
  GameMode,
  ShipClass,
  GameSettings,
} from "./types.ts";
import { sound } from "./audio.ts";

export const SHIP_CLASSES: Record<ShipClassId, ShipClass> = {
  vanguard: {
    id: "vanguard",
    name: "Vanguard Interceptor",
    tagline: "Balanced assault fighter with rapid railguns and reinforced deflectors.",
    hull: 100,
    shield: 60,
    speed: 480,
    turnSpeed: 6.2,
    dashCooldown: 2.2,
    specialAbility: "Burst Overdrive: Rapid dual railgun fire.",
    primaryWeapon: "Dual Kinetic Railguns",
    accentColor: "#06b6d4", // Cyan
    glowColor: "rgba(6, 182, 212, 0.6)",
  },
  chrono: {
    id: "chrono",
    name: "Chrono-Stalker",
    tagline: "Temporal skirmisher with extended bullet-time capacitors and phase blinks.",
    hull: 75,
    shield: 75,
    speed: 540,
    turnSpeed: 7.5,
    dashCooldown: 1.5,
    specialAbility: "Phase Blink: Warp through hostiles while dealing slice damage.",
    primaryWeapon: "Tachyon Pulse Cannons",
    accentColor: "#a855f7", // Purple
    glowColor: "rgba(168, 85, 247, 0.6)",
  },
  titan: {
    id: "titan",
    name: "Titan Dreadnought",
    tagline: "Heavy armored fortress with broadside flak and massive hull integrity.",
    hull: 180,
    shield: 100,
    speed: 360,
    turnSpeed: 4.8,
    dashCooldown: 3.5,
    specialAbility: "Defensive Nova: Discharge radial flak clearing incoming projectiles.",
    primaryWeapon: "Heavy Flak Broadside",
    accentColor: "#f59e0b", // Amber
    glowColor: "rgba(245, 158, 11, 0.6)",
  },
  specter: {
    id: "specter",
    name: "Phantom Specter",
    tagline: "Covert long-range infiltrator with high critical strike and seeker pods.",
    hull: 60,
    shield: 90,
    speed: 510,
    turnSpeed: 6.8,
    dashCooldown: 2.0,
    specialAbility: "Target Lock: Homing micro-drones seek weakest hostiles.",
    primaryWeapon: "Precision Beam Cannon",
    accentColor: "#10b981", // Emerald
    glowColor: "rgba(16, 185, 129, 0.6)",
  },
};

export class GameEngine {
  public arenaWidth = 2400;
  public arenaHeight = 1600;

  public player: PlayerState;
  public enemies: Enemy[] = [];
  public projectiles: Projectile[] = [];
  public particles: Particle[] = [];
  public shards: DataShard[] = [];

  public score = 0;
  public sector = 1;
  public sectorTimer = 0;
  public bossActive = false;
  public isGameOver = false;
  public isVictory = false;
  public isPaused = false;
  public isLevelUpPending = false;
  public gameMode: GameMode = "campaign";

  public timeDilationActive = false;
  public timeScale = 1.0;

  public screenShakeAmount = 0;
  public camera = { x: 0, y: 0 };

  private nextEntityId = 1;
  private primaryFireTimer = 0;
  private missileTimer = 0;
  private sawAngle = 0;
  private enemySpawnTimer = 0;

  public settings: GameSettings = {
    soundVolume: 0.7,
    musicVolume: 0.5,
    screenShake: true,
    bloom: true,
    crtScanlines: false,
    showFps: true,
    aiPilotEnabled: false,
    reducedFlashing: false,
  };

  // Keyboard and Mouse input tracking
  public inputs = {
    up: false,
    down: false,
    left: false,
    right: false,
    shooting: false,
    dash: false,
    slomo: false,
    mouseX: 0,
    mouseY: 0,
  };

  constructor(shipClass: ShipClassId = "vanguard", mode: GameMode = "campaign") {
    this.gameMode = mode;
    const baseShip = SHIP_CLASSES[shipClass];

    this.player = {
      x: this.arenaWidth / 2,
      y: this.arenaHeight / 2,
      vx: 0,
      vy: 0,
      angle: -Math.PI / 2,
      hull: baseShip.hull,
      maxHull: baseShip.hull,
      shield: baseShip.shield,
      maxShield: baseShip.shield,
      shieldRegenTimer: 0,
      chronoEnergy: 100,
      maxChronoEnergy: 100,
      isDashing: false,
      dashTimer: 0,
      dashCooldownTimer: 0,
      shipClass,
      level: 1,
      exp: 0,
      expNext: 100,
      credits: 0,
      upgrades: [],
      weaponLevel: 1,
      activeModules: [],
      killCount: 0,
      combo: 1,
      comboTimer: 0,
      invulnerableTimer: 0,
    };
  }

  public reset(shipClass: ShipClassId = "vanguard", mode: GameMode = "campaign") {
    this.gameMode = mode;
    const baseShip = SHIP_CLASSES[shipClass];
    this.player = {
      x: this.arenaWidth / 2,
      y: this.arenaHeight / 2,
      vx: 0,
      vy: 0,
      angle: -Math.PI / 2,
      hull: baseShip.hull,
      maxHull: baseShip.hull,
      shield: baseShip.shield,
      maxShield: baseShip.shield,
      shieldRegenTimer: 0,
      chronoEnergy: 100,
      maxChronoEnergy: 100,
      isDashing: false,
      dashTimer: 0,
      dashCooldownTimer: 0,
      shipClass,
      level: 1,
      exp: 0,
      expNext: 100,
      credits: 0,
      upgrades: [],
      weaponLevel: 1,
      activeModules: [],
      killCount: 0,
      combo: 1,
      comboTimer: 0,
      invulnerableTimer: 0,
    };

    this.enemies = [];
    this.projectiles = [];
    this.particles = [];
    this.shards = [];
    this.score = 0;
    this.sector = 1;
    this.sectorTimer = 0;
    this.bossActive = false;
    this.isGameOver = false;
    this.isVictory = false;
    this.isPaused = false;
    this.isLevelUpPending = false;
    this.timeDilationActive = false;
    this.timeScale = 1.0;
    sound.startMusic(1);
  }

  public update(dt: number) {
    if (this.isPaused || this.isLevelUpPending || this.isGameOver) return;

    // Time Dilation logic (Chrono-Drive)
    if (this.inputs.slomo && this.player.chronoEnergy > 5) {
      if (!this.timeDilationActive) {
        this.timeDilationActive = true;
        sound.setTimeDilation(true);
      }
      this.timeScale = 0.22;
      this.player.chronoEnergy = Math.max(0, this.player.chronoEnergy - dt * 28);
    } else {
      if (this.timeDilationActive) {
        this.timeDilationActive = false;
        sound.setTimeDilation(false);
      }
      this.timeScale = 1.0;
      // Chrono energy recharge
      const rechargeRate = this.player.upgrades.includes("chronoflux_capacitor") ? 32 : 18;
      this.player.chronoEnergy = Math.min(this.player.maxChronoEnergy, this.player.chronoEnergy + dt * rechargeRate);
    }

    const simDt = dt * this.timeScale;

    // Update screen shake decay
    if (this.screenShakeAmount > 0) {
      this.screenShakeAmount = Math.max(0, this.screenShakeAmount - dt * 25);
    }

    // Update Player
    this.updatePlayer(dt, simDt);

    // Update Weapons & Fire
    this.updateWeapons(dt, simDt);

    // Update Projectiles
    this.updateProjectiles(simDt);

    // Update Enemies & Spawns
    this.updateEnemies(simDt);

    // Update Data Shards
    this.updateShards(simDt);

    // Update Particles
    this.updateParticles(simDt);

    // Check Collisions
    this.checkCollisions();

    // Update Sector Timer & Boss spawning
    this.updateSectorProgress(simDt);

    // Update Camera position centering on player
    this.camera.x = this.player.x;
    this.camera.y = this.player.y;
  }

  private updatePlayer(rawDt: number, simDt: number) {
    const baseShip = SHIP_CLASSES[this.player.shipClass];
    let topSpeed = baseShip.speed;
    let turnSpeed = baseShip.turnSpeed;

    if (this.player.upgrades.includes("vector_thrusters")) {
      topSpeed *= 1.25;
      turnSpeed *= 1.35;
    }

    // Aim angle toward mouse
    const worldMouseX = this.inputs.mouseX + this.camera.x;
    const worldMouseY = this.inputs.mouseY + this.camera.y;
    const targetAngle = Math.atan2(worldMouseY - this.player.y, worldMouseX - this.player.x);

    // Smooth turn towards target angle
    let diff = targetAngle - this.player.angle;
    while (diff < -Math.PI) diff += Math.PI * 2;
    while (diff > Math.PI) diff -= Math.PI * 2;
    this.player.angle += diff * Math.min(1.0, turnSpeed * rawDt);

    // Directional movement vector (Newtonian thrust)
    let moveX = 0;
    let moveY = 0;
    if (this.inputs.up) moveY -= 1;
    if (this.inputs.down) moveY += 1;
    if (this.inputs.left) moveX -= 1;
    if (this.inputs.right) moveX += 1;

    const len = Math.hypot(moveX, moveY);
    if (len > 0) {
      moveX /= len;
      moveY /= len;
      const accel = topSpeed * 4.5;
      this.player.vx += moveX * accel * simDt;
      this.player.vy += moveY * accel * simDt;

      // Spawn thruster particles
      if (Math.random() < 0.6) {
        const backAngle = this.player.angle + Math.PI + (Math.random() - 0.5) * 0.4;
        this.particles.push({
          x: this.player.x - Math.cos(this.player.angle) * 16,
          y: this.player.y - Math.sin(this.player.angle) * 16,
          vx: Math.cos(backAngle) * (140 + Math.random() * 80) + this.player.vx * 0.2,
          vy: Math.sin(backAngle) * (140 + Math.random() * 80) + this.player.vy * 0.2,
          life: 0.25,
          maxLife: 0.25,
          color: baseShip.accentColor,
          size: 3 + Math.random() * 3,
        });
      }
    }

    // Clamp speed and apply space drag
    const currentSpeed = Math.hypot(this.player.vx, this.player.vy);
    const maxAllowedSpeed = this.player.isDashing ? topSpeed * 2.8 : topSpeed;
    if (currentSpeed > maxAllowedSpeed) {
      this.player.vx = (this.player.vx / currentSpeed) * maxAllowedSpeed;
      this.player.vy = (this.player.vy / currentSpeed) * maxAllowedSpeed;
    }
    // Inertial friction
    this.player.vx *= Math.pow(0.88, simDt * 60);
    this.player.vy *= Math.pow(0.88, simDt * 60);

    // Apply movement
    this.player.x += this.player.vx * simDt;
    this.player.y += this.player.vy * simDt;

    // Bounds check
    const margin = 30;
    this.player.x = Math.max(margin, Math.min(this.arenaWidth - margin, this.player.x));
    this.player.y = Math.max(margin, Math.min(this.arenaHeight - margin, this.player.y));

    // Dash / Warp handling
    if (this.player.dashCooldownTimer > 0) {
      this.player.dashCooldownTimer -= rawDt;
    }

    if (this.inputs.dash && this.player.dashCooldownTimer <= 0 && !this.player.isDashing) {
      this.player.isDashing = true;
      this.player.dashTimer = 0.22;
      this.player.dashCooldownTimer = baseShip.dashCooldown;
      this.player.invulnerableTimer = 0.35;
      sound.playDash();

      // Dash impulse
      const dashAngle = len > 0 ? Math.atan2(moveY, moveX) : this.player.angle;
      const dashForce = topSpeed * 2.6;
      this.player.vx = Math.cos(dashAngle) * dashForce;
      this.player.vy = Math.sin(dashAngle) * dashForce;

      this.triggerScreenShake(8);

      // Dash warp clone particles
      for (let i = 0; i < 18; i++) {
        const spread = (Math.random() - 0.5) * Math.PI;
        this.particles.push({
          x: this.player.x,
          y: this.player.y,
          vx: Math.cos(dashAngle + Math.PI + spread) * (180 + Math.random() * 120),
          vy: Math.sin(dashAngle + Math.PI + spread) * (180 + Math.random() * 120),
          life: 0.35,
          maxLife: 0.35,
          color: "#38bdf8",
          size: 4 + Math.random() * 4,
        });
      }
    }

    if (this.player.isDashing) {
      this.player.dashTimer -= rawDt;
      if (this.player.dashTimer <= 0) {
        this.player.isDashing = false;
      }
    }

    if (this.player.invulnerableTimer > 0) {
      this.player.invulnerableTimer -= rawDt;
    }

    // Shield Regen
    if (this.player.shield < this.player.maxShield) {
      this.player.shieldRegenTimer += rawDt;
      if (this.player.shieldRegenTimer >= 2.5) {
        const regenRate = this.player.upgrades.includes("reactive_barrier") ? 25 : 16;
        this.player.shield = Math.min(this.player.maxShield, this.player.shield + rawDt * regenRate);
      }
    }

    // Nanite hull repair
    if (this.player.upgrades.includes("nanite_repair") && this.player.hull < this.player.maxHull) {
      this.player.hull = Math.min(this.player.maxHull, this.player.hull + rawDt * 1.5);
    }

    // Combo timer decay
    if (this.player.comboTimer > 0) {
      this.player.comboTimer -= rawDt;
      if (this.player.comboTimer <= 0) {
        this.player.combo = 1;
      }
    }
  }

  private updateWeapons(rawDt: number, simDt: number) {
    this.primaryFireTimer -= rawDt;
    let fireInterval = 0.16;
    if (this.player.upgrades.includes("high_frequency_plasma")) fireInterval *= 0.7;
    if (this.player.upgrades.includes("evo_plasma_god")) fireInterval = 0.04;

    if (this.inputs.shooting && this.primaryFireTimer <= 0) {
      this.primaryFireTimer = fireInterval;
      this.firePrimaryWeapon();
    }

    // Orbiting saw blades angle update
    if (this.player.upgrades.includes("orbital_saw_blades") || this.player.upgrades.includes("evo_saw_storm")) {
      const rotSpeed = this.player.upgrades.includes("evo_saw_storm") ? 6.5 : 4.0;
      this.sawAngle += rotSpeed * simDt;

      const sawCount = this.player.upgrades.includes("evo_saw_storm") ? 4 : 2;
      const sawRadius = 85;

      for (let i = 0; i < sawCount; i++) {
        const angle = this.sawAngle + (i * Math.PI * 2) / sawCount;
        const sawX = this.player.x + Math.cos(angle) * sawRadius;
        const sawY = this.player.y + Math.sin(angle) * sawRadius;

        // Damage enemies near saws
        for (const enemy of this.enemies) {
          const dist = Math.hypot(enemy.x - sawX, enemy.y - sawY);
          if (dist < enemy.radius + 18) {
            enemy.hp -= 45 * simDt;
            this.particles.push({
              x: sawX,
              y: sawY,
              vx: (Math.random() - 0.5) * 150,
              vy: (Math.random() - 0.5) * 150,
              life: 0.15,
              maxLife: 0.15,
              color: "#38bdf8",
              size: 3,
            });
          }
        }
      }
    }

    // Micro missiles auto-launcher
    if (this.player.upgrades.includes("seeking_micro_missiles") || this.player.upgrades.includes("evo_singularity_missiles")) {
      this.missileTimer += simDt;
      if (this.missileTimer >= 1.2 && this.enemies.length > 0) {
        this.missileTimer = 0;
        this.fireMicroMissiles();
      }
    }
  }

  private firePrimaryWeapon() {
    const angle = this.player.angle;
    const baseDamage = 35 + this.player.weaponLevel * 10;
    const isEvoBeam = this.player.upgrades.includes("evo_plasma_god");

    sound.playLaser(isEvoBeam ? 1.6 : 1.0);

    const projectileSpeed = 1100;
    const spawnX = this.player.x + Math.cos(angle) * 22;
    const spawnY = this.player.y + Math.sin(angle) * 22;

    // Base projectile
    this.projectiles.push({
      id: this.nextEntityId++,
      x: spawnX,
      y: spawnY,
      vx: Math.cos(angle) * projectileSpeed + this.player.vx * 0.25,
      vy: Math.sin(angle) * projectileSpeed + this.player.vy * 0.25,
      radius: isEvoBeam ? 9 : 5,
      damage: isEvoBeam ? baseDamage * 1.5 : baseDamage,
      isEnemy: false,
      color: isEvoBeam ? "#f43f5e" : "#38bdf8",
      life: 1.4,
      maxLife: 1.4,
      piercing: this.player.upgrades.includes("heavy_slugs") || isEvoBeam,
    });

    // Split Shot matrix
    if (this.player.upgrades.includes("split_shot")) {
      const spreadAngles = [-0.18, 0.18];
      spreadAngles.forEach((spread) => {
        this.projectiles.push({
          id: this.nextEntityId++,
          x: spawnX,
          y: spawnY,
          vx: Math.cos(angle + spread) * (projectileSpeed * 0.95),
          vy: Math.sin(angle + spread) * (projectileSpeed * 0.95),
          radius: 4,
          damage: baseDamage * 0.65,
          isEnemy: false,
          color: "#06b6d4",
          life: 1.2,
          maxLife: 1.2,
        });
      });
    }

    this.triggerScreenShake(1.5);
  }

  private fireMicroMissiles() {
    // Target closest enemy
    let closestEnemy: Enemy | null = null;
    let minDist = 99999;
    for (const e of this.enemies) {
      const d = Math.hypot(e.x - this.player.x, e.y - this.player.y);
      if (d < minDist) {
        minDist = d;
        closestEnemy = e;
      }
    }

    if (!closestEnemy) return;

    sound.playRailgun();
    const count = this.player.upgrades.includes("evo_singularity_missiles") ? 5 : 3;

    for (let i = 0; i < count; i++) {
      const spreadAngle = this.player.angle + (i - (count - 1) / 2) * 0.35;
      this.projectiles.push({
        id: this.nextEntityId++,
        x: this.player.x,
        y: this.player.y,
        vx: Math.cos(spreadAngle) * 450,
        vy: Math.sin(spreadAngle) * 450,
        radius: 6,
        damage: 65,
        isEnemy: false,
        color: "#f59e0b",
        life: 2.5,
        maxLife: 2.5,
        isMissile: true,
        homingTarget: closestEnemy.id,
      });
    }
  }

  private updateProjectiles(dt: number) {
    for (let i = this.projectiles.length - 1; i >= 0; i--) {
      const p = this.projectiles[i];

      // Homing missile navigation
      if (p.isMissile && p.homingTarget) {
        const target = this.enemies.find((e) => e.id === p.homingTarget);
        if (target) {
          const targetAngle = Math.atan2(target.y - p.y, target.x - p.x);
          const currentAngle = Math.atan2(p.vy, p.vx);
          let diff = targetAngle - currentAngle;
          while (diff < -Math.PI) diff += Math.PI * 2;
          while (diff > Math.PI) diff -= Math.PI * 2;
          const newAngle = currentAngle + diff * Math.min(1.0, 5.0 * dt);
          const speed = Math.hypot(p.vx, p.vy) + 300 * dt;
          p.vx = Math.cos(newAngle) * speed;
          p.vy = Math.sin(newAngle) * speed;

          // Smoke trail
          if (Math.random() < 0.4) {
            this.particles.push({
              x: p.x,
              y: p.y,
              vx: (Math.random() - 0.5) * 40,
              vy: (Math.random() - 0.5) * 40,
              life: 0.25,
              maxLife: 0.25,
              color: "#fb923c",
              size: 2.5,
            });
          }
        }
      }

      p.x += p.vx * dt;
      p.y += p.vy * dt;
      p.life -= dt;

      // Remove out of bounds or dead
      if (p.life <= 0 || p.x < 0 || p.x > this.arenaWidth || p.y < 0 || p.y > this.arenaHeight) {
        this.projectiles.splice(i, 1);
      }
    }
  }

  private updateEnemies(dt: number) {
    const px = this.player.x;
    const py = this.player.y;

    for (let i = this.enemies.length - 1; i >= 0; i--) {
      const enemy = this.enemies[i];
      const distToPlayer = Math.hypot(px - enemy.x, py - enemy.y);
      const angleToPlayer = Math.atan2(py - enemy.y, px - enemy.x);

      enemy.fireTimer -= dt;
      enemy.behaviorTimer -= dt;

      switch (enemy.type) {
        case "drone_scout": {
          // Swarm approach
          enemy.vx += Math.cos(angleToPlayer) * 350 * dt;
          enemy.vy += Math.sin(angleToPlayer) * 350 * dt;
          enemy.vx *= Math.pow(0.92, dt * 60);
          enemy.vy *= Math.pow(0.92, dt * 60);
          enemy.angle = angleToPlayer;

          if (enemy.fireTimer <= 0 && distToPlayer < 650) {
            enemy.fireTimer = 1.8 + Math.random() * 0.8;
            this.fireEnemyBullet(enemy.x, enemy.y, enemy.angle, 400, 15, "#f43f5e");
          }
          break;
        }

        case "dart_striker": {
          // Charges fast when aligned
          if (enemy.telegraphTimer && enemy.telegraphTimer > 0) {
            enemy.telegraphTimer -= dt;
            if (enemy.telegraphTimer <= 0) {
              // Launch charge
              enemy.vx = Math.cos(enemy.angle) * 850;
              enemy.vy = Math.sin(enemy.angle) * 850;
            }
          } else {
            enemy.vx *= Math.pow(0.94, dt * 60);
            enemy.vy *= Math.pow(0.94, dt * 60);
            enemy.angle = angleToPlayer;

            if (enemy.behaviorTimer <= 0 && distToPlayer < 700) {
              enemy.behaviorTimer = 2.4;
              enemy.telegraphTimer = 0.55; // Red laser warning
            }
          }
          break;
        }

        case "heavy_shielder": {
          // Holds directional energy shield front towards player
          enemy.shieldAngle = angleToPlayer;
          enemy.vx += Math.cos(angleToPlayer) * 180 * dt;
          enemy.vy += Math.sin(angleToPlayer) * 180 * dt;
          enemy.vx *= Math.pow(0.9, dt * 60);
          enemy.vy *= Math.pow(0.9, dt * 60);
          enemy.angle = angleToPlayer;

          if (enemy.fireTimer <= 0 && distToPlayer < 500) {
            enemy.fireTimer = 2.2;
            this.fireEnemyBullet(enemy.x, enemy.y, angleToPlayer - 0.15, 380, 20, "#e11d48");
            this.fireEnemyBullet(enemy.x, enemy.y, angleToPlayer + 0.15, 380, 20, "#e11d48");
          }
          break;
        }

        case "void_sniper": {
          // Keeps distance, charges beam
          if (distToPlayer < 550) {
            enemy.vx -= Math.cos(angleToPlayer) * 280 * dt;
            enemy.vy -= Math.sin(angleToPlayer) * 280 * dt;
          } else {
            enemy.vx += Math.cos(angleToPlayer) * 120 * dt;
            enemy.vy += Math.sin(angleToPlayer) * 120 * dt;
          }
          enemy.vx *= Math.pow(0.92, dt * 60);
          enemy.vy *= Math.pow(0.92, dt * 60);
          enemy.angle = angleToPlayer;

          if (enemy.fireTimer <= 0) {
            enemy.fireTimer = 3.2;
            this.fireEnemyBullet(enemy.x, enemy.y, angleToPlayer, 950, 45, "#a855f7", 8);
            sound.playRailgun();
          }
          break;
        }

        case "guardian_boss": {
          // Multi-phase arena boss
          enemy.angle += 0.8 * dt;
          enemy.vx += Math.cos(angleToPlayer) * 140 * dt;
          enemy.vy += Math.sin(angleToPlayer) * 140 * dt;
          enemy.vx *= Math.pow(0.94, dt * 60);
          enemy.vy *= Math.pow(0.94, dt * 60);

          if (enemy.fireTimer <= 0) {
            enemy.fireTimer = 0.8;
            // Radial bullet curtain
            const arms = 8;
            for (let a = 0; a < arms; a++) {
              const bAngle = enemy.angle + (a * Math.PI * 2) / arms;
              this.fireEnemyBullet(enemy.x, enemy.y, bAngle, 320, 25, "#fb7185", 6);
            }
          }
          break;
        }
      }

      enemy.x += enemy.vx * dt;
      enemy.y += enemy.vy * dt;

      // Arena bounds
      enemy.x = Math.max(40, Math.min(this.arenaWidth - 40, enemy.x));
      enemy.y = Math.max(40, Math.min(this.arenaHeight - 40, enemy.y));
    }

    // Spawn new enemies if below density limit
    const maxEnemies = Math.min(45, 12 + this.sector * 6);
    this.enemySpawnTimer += dt;
    if (this.enemies.length < maxEnemies && this.enemySpawnTimer >= 1.0 && !this.bossActive) {
      this.enemySpawnTimer = 0;
      this.spawnRandomEnemy();
    }
  }

  private fireEnemyBullet(x: number, y: number, angle: number, speed: number, damage: number, color: string, radius = 4.5) {
    this.projectiles.push({
      id: this.nextEntityId++,
      x,
      y,
      vx: Math.cos(angle) * speed,
      vy: Math.sin(angle) * speed,
      radius,
      damage,
      isEnemy: true,
      color,
      life: 3.5,
      maxLife: 3.5,
    });
  }

  private spawnRandomEnemy() {
    // Pick spawn position off camera view
    const angle = Math.random() * Math.PI * 2;
    const distance = 850 + Math.random() * 300;
    const spawnX = Math.max(60, Math.min(this.arenaWidth - 60, this.player.x + Math.cos(angle) * distance));
    const spawnY = Math.max(60, Math.min(this.arenaHeight - 60, this.player.y + Math.sin(angle) * distance));

    const roll = Math.random();
    let type: EnemyType = "drone_scout";
    let hp = 40 + this.sector * 15;
    let radius = 14;
    let color = "#38bdf8";
    let scoreVal = 100;
    let expVal = 20;

    if (roll > 0.75 && this.sector >= 2) {
      type = "void_sniper";
      hp = 70 + this.sector * 25;
      radius = 18;
      color = "#c084fc";
      scoreVal = 350;
      expVal = 55;
    } else if (roll > 0.5 && this.sector >= 2) {
      type = "heavy_shielder";
      hp = 120 + this.sector * 35;
      radius = 22;
      color = "#f59e0b";
      scoreVal = 250;
      expVal = 40;
    } else if (roll > 0.3) {
      type = "dart_striker";
      hp = 50 + this.sector * 18;
      radius = 16;
      color = "#fb7185";
      scoreVal = 150;
      expVal = 28;
    }

    this.enemies.push({
      id: this.nextEntityId++,
      type,
      x: spawnX,
      y: spawnY,
      vx: 0,
      vy: 0,
      angle: 0,
      hp,
      maxHp: hp,
      radius,
      scoreValue: scoreVal,
      expValue: expVal,
      color,
      fireTimer: 1.0 + Math.random(),
      behaviorTimer: 2.0,
    });
  }

  public spawnGuardianBoss() {
    this.bossActive = true;
    const bossHp = 2200 + this.sector * 1200;
    this.enemies.push({
      id: this.nextEntityId++,
      type: "guardian_boss",
      x: this.player.x,
      y: this.player.y - 650,
      vx: 0,
      vy: 0,
      angle: 0,
      hp: bossHp,
      maxHp: bossHp,
      radius: 55,
      scoreValue: 5000 * this.sector,
      expValue: 600,
      color: "#f43f5e",
      fireTimer: 2.0,
      behaviorTimer: 5.0,
      phase: 1,
    });

    this.triggerScreenShake(12);
    sound.playExplosion(true);
  }

  private updateShards(dt: number) {
    const px = this.player.x;
    const py = this.player.y;
    let magnetRange = 180;
    if (this.player.upgrades.includes("magnetic_siphon")) magnetRange = 450;

    for (let i = this.shards.length - 1; i >= 0; i--) {
      const shard = this.shards[i];
      const dist = Math.hypot(px - shard.x, py - shard.y);

      if (dist < magnetRange) {
        // Gravitational pull towards player
        const angle = Math.atan2(py - shard.y, px - shard.x);
        const pullSpeed = 750 * (1 - dist / magnetRange) + 250;
        shard.vx = Math.cos(angle) * pullSpeed;
        shard.vy = Math.sin(angle) * pullSpeed;
      }

      shard.x += shard.vx * dt;
      shard.y += shard.vy * dt;
      shard.vx *= Math.pow(0.9, dt * 60);
      shard.vy *= Math.pow(0.9, dt * 60);

      // Pickup collision
      if (dist < 26) {
        this.shards.splice(i, 1);
        sound.playShardPickup();

        if (shard.type === "exp") {
          let expGain = shard.value;
          if (this.player.upgrades.includes("overclock_reactor")) expGain = Math.round(expGain * 1.3);
          this.gainExp(expGain);
        } else if (shard.type === "credit") {
          this.player.credits += shard.value;
        } else if (shard.type === "repair") {
          this.player.hull = Math.min(this.player.maxHull, this.player.hull + 20);
        }
      }
    }
  }

  private gainExp(amount: number) {
    this.player.exp += amount;
    if (this.player.exp >= this.player.expNext) {
      this.player.exp -= this.player.expNext;
      this.player.level++;
      this.player.expNext = Math.round(this.player.expNext * 1.35 + 50);
      this.isLevelUpPending = true;
      sound.playLevelUp();
    }
  }

  private updateParticles(dt: number) {
    for (let i = this.particles.length - 1; i >= 0; i--) {
      const p = this.particles[i];
      p.x += p.vx * dt;
      p.y += p.vy * dt;
      p.life -= dt;
      if (p.life <= 0) {
        this.particles.splice(i, 1);
      }
    }
  }

  private checkCollisions() {
    // 1. Player Projectiles vs Enemies
    for (let pi = this.projectiles.length - 1; pi >= 0; pi--) {
      const p = this.projectiles[pi];
      if (p.isEnemy) continue;

      for (let ei = this.enemies.length - 1; ei >= 0; ei--) {
        const e = this.enemies[ei];
        const dist = Math.hypot(e.x - p.x, e.y - p.y);

        if (dist < e.radius + p.radius) {
          // Check if hitting directional shield of heavy_shielder
          if (e.type === "heavy_shielder" && e.shieldAngle !== undefined) {
            const hitAngle = Math.atan2(p.y - e.y, p.x - e.x);
            let diff = hitAngle - e.shieldAngle;
            while (diff < -Math.PI) diff += Math.PI * 2;
            while (diff > Math.PI) diff -= Math.PI * 2;

            if (Math.abs(diff) < Math.PI * 0.4) {
              // Shield blocked!
              this.particles.push({
                x: p.x,
                y: p.y,
                vx: Math.cos(hitAngle) * 180,
                vy: Math.sin(hitAngle) * 180,
                life: 0.18,
                maxLife: 0.18,
                color: "#38bdf8",
                size: 4,
              });
              if (!p.piercing) this.projectiles.splice(pi, 1);
              break;
            }
          }

          // Damage enemy
          e.hp -= p.damage;
          this.triggerScreenShake(2);

          // Hit spark particles
          for (let s = 0; s < 4; s++) {
            this.particles.push({
              x: p.x,
              y: p.y,
              vx: (Math.random() - 0.5) * 180,
              vy: (Math.random() - 0.5) * 180,
              life: 0.2,
              maxLife: 0.2,
              color: p.color,
              size: 2.5,
            });
          }

          // Enemy destroyed?
          if (e.hp <= 0) {
            this.destroyEnemy(e, ei);
          }

          if (!p.piercing) {
            this.projectiles.splice(pi, 1);
            break;
          }
        }
      }
    }

    // 2. Enemy Projectiles vs Player
    if (this.player.invulnerableTimer <= 0) {
      for (let pi = this.projectiles.length - 1; pi >= 0; pi--) {
        const p = this.projectiles[pi];
        if (!p.isEnemy) continue;

        const dist = Math.hypot(this.player.x - p.x, this.player.y - p.y);
        if (dist < 20 + p.radius) {
          this.projectiles.splice(pi, 1);
          this.damagePlayer(p.damage);
          break;
        }
      }

      // 3. Enemy Body Contact vs Player
      for (const e of this.enemies) {
        const dist = Math.hypot(this.player.x - e.x, this.player.y - e.y);
        if (dist < 22 + e.radius) {
          this.damagePlayer(e.type === "guardian_boss" ? 35 : 20);
          break;
        }
      }
    }
  }

  private destroyEnemy(e: Enemy, index: number) {
    this.enemies.splice(index, 1);
    this.player.killCount++;
    this.player.combo = Math.min(10, this.player.combo + 1);
    this.player.comboTimer = this.player.upgrades.includes("overclock_reactor") ? 4.5 : 2.8;

    const points = e.scoreValue * this.player.combo;
    this.score += points;

    sound.playExplosion(e.type === "guardian_boss");
    this.triggerScreenShake(e.type === "guardian_boss" ? 15 : 4);

    // Spawn destruction shockwave particles
    const pCount = e.type === "guardian_boss" ? 60 : 16;
    for (let i = 0; i < pCount; i++) {
      const pAngle = Math.random() * Math.PI * 2;
      const pSpeed = 60 + Math.random() * 220;
      this.particles.push({
        x: e.x,
        y: e.y,
        vx: Math.cos(pAngle) * pSpeed,
        vy: Math.sin(pAngle) * pSpeed,
        life: 0.4 + Math.random() * 0.3,
        maxLife: 0.7,
        color: e.color,
        size: 3 + Math.random() * 5,
      });
    }

    // Drop EXP and Credit shards
    const shardCount = e.type === "guardian_boss" ? 8 : 2;
    for (let s = 0; s < shardCount; s++) {
      const sAngle = Math.random() * Math.PI * 2;
      this.shards.push({
        id: this.nextEntityId++,
        x: e.x + (Math.random() - 0.5) * 20,
        y: e.y + (Math.random() - 0.5) * 20,
        vx: Math.cos(sAngle) * (80 + Math.random() * 80),
        vy: Math.sin(sAngle) * (80 + Math.random() * 80),
        value: Math.round(e.expValue / shardCount),
        type: Math.random() < 0.15 ? "credit" : "exp",
      });
    }

    // Boss defeat triggers sector victory
    if (e.type === "guardian_boss") {
      this.bossActive = false;
      this.advanceSector();
    }
  }

  private damagePlayer(amount: number) {
    this.player.shieldRegenTimer = 0;
    this.player.invulnerableTimer = 0.5; // Brief invulnerability window
    this.triggerScreenShake(8);

    // Reactive EMP burst on hit
    if (this.player.upgrades.includes("emp_counter_burst")) {
      // Erase nearby enemy bullets
      for (let i = this.projectiles.length - 1; i >= 0; i--) {
        if (this.projectiles[i].isEnemy) {
          const d = Math.hypot(this.projectiles[i].x - this.player.x, this.projectiles[i].y - this.player.y);
          if (d < 450) {
            this.projectiles.splice(i, 1);
          }
        }
      }
    }

    // Absorb through shield first
    if (this.player.shield > 0) {
      if (this.player.shield >= amount) {
        this.player.shield -= amount;
        return;
      } else {
        const leftover = amount - this.player.shield;
        this.player.shield = 0;
        this.player.hull -= leftover;
      }
    } else {
      this.player.hull -= amount;
    }

    // Reset combo on hit
    this.player.combo = 1;

    // Game over check
    if (this.player.hull <= 0) {
      this.player.hull = 0;
      this.isGameOver = true;
      sound.playExplosion(true);
      sound.stopMusic();
    }
  }

  private advanceSector() {
    this.sector++;
    this.sectorTimer = 0;
    this.score += 10000 * this.sector;
    sound.startMusic(this.sector);

    if (this.sector > 5 && this.gameMode === "campaign") {
      this.isVictory = true;
      sound.stopMusic();
    }
  }

  private updateSectorProgress(dt: number) {
    this.sectorTimer += dt;
    // Guardian boss appears after 55 seconds in campaign mode
    if (this.sectorTimer >= 55 && !this.bossActive && this.gameMode === "campaign" && this.sector <= 5) {
      this.spawnGuardianBoss();
    }
  }

  public triggerScreenShake(magnitude: number) {
    if (this.settings.screenShake) {
      this.screenShakeAmount = Math.min(22, this.screenShakeAmount + magnitude);
    }
  }

  public applyUpgrade(cardId: string) {
    this.player.upgrades.push(cardId);
    this.isLevelUpPending = false;

    // Stat bonuses
    if (cardId === "reactive_barrier") {
      this.player.maxShield += 40;
      this.player.shield = this.player.maxShield;
    } else if (cardId === "chronoflux_capacitor") {
      this.player.maxChronoEnergy = 200;
      this.player.chronoEnergy = 200;
    }
  }
}
