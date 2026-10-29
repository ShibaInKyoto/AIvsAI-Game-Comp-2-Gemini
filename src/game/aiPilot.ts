import { GameEngine } from "./engine.ts";

export class AIPilot {
  private engine: GameEngine;
  private thinkTimer = 0;

  constructor(engine: GameEngine) {
    this.engine = engine;
  }

  public update(dt: number) {
    this.thinkTimer += dt;
    if (this.thinkTimer < 0.05) return;
    this.thinkTimer = 0;

    const p = this.engine.player;

    // 1. Evade incoming enemy projectiles
    let dodgeX = 0;
    let dodgeY = 0;
    let immediateDanger = false;

    for (const proj of this.engine.projectiles) {
      if (!proj.isEnemy) continue;
      const dist = Math.hypot(proj.x - p.x, proj.y - p.y);
      if (dist < 220) {
        // Bullet is close! Dodge perpendicular
        immediateDanger = true;
        const awayAngle = Math.atan2(p.y - proj.y, p.x - proj.x);
        dodgeX += Math.cos(awayAngle);
        dodgeY += Math.sin(awayAngle);
      }
    }

    // 2. Identify nearest high-priority enemy target
    let targetEnemy = null;
    let minEnemyDist = 99999;

    for (const enemy of this.engine.enemies) {
      const dist = Math.hypot(enemy.x - p.x, enemy.y - p.y);
      if (dist < minEnemyDist) {
        minEnemyDist = dist;
        targetEnemy = enemy;
      }
    }

    // 3. Movement logic
    if (immediateDanger) {
      // Evade danger
      this.engine.inputs.up = dodgeY < -0.3;
      this.engine.inputs.down = dodgeY > 0.3;
      this.engine.inputs.left = dodgeX < -0.3;
      this.engine.inputs.right = dodgeX > 0.3;

      // Use bullet time if in dense danger
      if (p.chronoEnergy > 25) {
        this.engine.inputs.slomo = true;
      }

      // Tactical dash if about to get hit
      if (p.dashCooldownTimer <= 0 && Math.random() < 0.4) {
        this.engine.inputs.dash = true;
      }
    } else if (targetEnemy) {
      this.engine.inputs.slomo = false;
      this.engine.inputs.dash = false;

      // Orbit target at ideal distance (~350px)
      const idealDist = 340;
      const angle = Math.atan2(targetEnemy.y - p.y, targetEnemy.x - p.x);

      if (minEnemyDist > idealDist + 60) {
        // Move closer
        this.engine.inputs.up = Math.sin(angle) < -0.2;
        this.engine.inputs.down = Math.sin(angle) > 0.2;
        this.engine.inputs.left = Math.cos(angle) < -0.2;
        this.engine.inputs.right = Math.cos(angle) > 0.2;
      } else if (minEnemyDist < idealDist - 60) {
        // Back off
        this.engine.inputs.up = Math.sin(angle) > 0.2;
        this.engine.inputs.down = Math.sin(angle) < -0.2;
        this.engine.inputs.left = Math.cos(angle) > 0.2;
        this.engine.inputs.right = Math.cos(angle) < -0.2;
      } else {
        // Strafe orbit
        const strafeAngle = angle + Math.PI / 2;
        this.engine.inputs.up = Math.sin(strafeAngle) < -0.2;
        this.engine.inputs.down = Math.sin(strafeAngle) > 0.2;
        this.engine.inputs.left = Math.cos(strafeAngle) < -0.2;
        this.engine.inputs.right = Math.cos(strafeAngle) > 0.2;
      }

      // Aim and fire at target (with lead prediction)
      const leadTime = minEnemyDist / 1100;
      const aimX = targetEnemy.x + targetEnemy.vx * leadTime;
      const aimY = targetEnemy.y + targetEnemy.vy * leadTime;

      // Convert world coord to screen relative for mouse inputs
      this.engine.inputs.mouseX = aimX - this.engine.camera.x;
      this.engine.inputs.mouseY = aimY - this.engine.camera.y;
      this.engine.inputs.shooting = true;
    } else {
      // Seek center of arena or wander
      this.engine.inputs.slomo = false;
      this.engine.inputs.shooting = false;
      const centerX = this.engine.arenaWidth / 2;
      const centerY = this.engine.arenaHeight / 2;
      const centerAngle = Math.atan2(centerY - p.y, centerX - p.x);
      this.engine.inputs.up = Math.sin(centerAngle) < -0.3;
      this.engine.inputs.down = Math.sin(centerAngle) > 0.3;
      this.engine.inputs.left = Math.cos(centerAngle) < -0.3;
      this.engine.inputs.right = Math.cos(centerAngle) > 0.3;
    }
  }
}
