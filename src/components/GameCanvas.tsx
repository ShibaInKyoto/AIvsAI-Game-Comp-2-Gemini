import React, { useRef, useEffect } from "react";
import { GameEngine, SHIP_CLASSES } from "../game/engine.ts";

interface GameCanvasProps {
  engine: GameEngine;
}

export const GameCanvas: React.FC<GameCanvasProps> = ({ engine }) => {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    let animationFrameId: number;
    let lastTime = performance.now();

    const handleResize = () => {
      canvas.width = window.innerWidth;
      canvas.height = window.innerHeight;
    };
    handleResize();
    window.addEventListener("resize", handleResize);

    // Mouse / Touch movement tracking
    const handleMouseMove = (e: MouseEvent) => {
      const rect = canvas.getBoundingClientRect();
      engine.inputs.mouseX = e.clientX - rect.left - canvas.width / 2;
      engine.inputs.mouseY = e.clientY - rect.top - canvas.height / 2;
    };

    const handleMouseDown = (e: MouseEvent) => {
      if (e.button === 0) {
        engine.inputs.shooting = true;
      } else if (e.button === 2) {
        e.preventDefault();
        engine.inputs.slomo = true;
      }
    };

    const handleMouseUp = (e: MouseEvent) => {
      if (e.button === 0) {
        engine.inputs.shooting = false;
      } else if (e.button === 2) {
        engine.inputs.slomo = false;
      }
    };

    const handleContextMenu = (e: MouseEvent) => {
      e.preventDefault();
    };

    // Keyboard controls
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.repeat) return;
      const key = e.key.toLowerCase();
      if (key === "w" || key === "arrowup") engine.inputs.up = true;
      if (key === "s" || key === "arrowdown") engine.inputs.down = true;
      if (key === "a" || key === "arrowleft") engine.inputs.left = true;
      if (key === "d" || key === "arrowright") engine.inputs.right = true;
      if (key === " " || key === "shift") engine.inputs.dash = true;
      if (key === "q" || key === "e") engine.inputs.slomo = true;
    };

    const handleKeyUp = (e: KeyboardEvent) => {
      const key = e.key.toLowerCase();
      if (key === "w" || key === "arrowup") engine.inputs.up = false;
      if (key === "s" || key === "arrowdown") engine.inputs.down = false;
      if (key === "a" || key === "arrowleft") engine.inputs.left = false;
      if (key === "d" || key === "arrowright") engine.inputs.right = false;
      if (key === " " || key === "shift") engine.inputs.dash = false;
      if (key === "q" || key === "e") engine.inputs.slomo = false;
    };

    window.addEventListener("mousemove", handleMouseMove);
    window.addEventListener("mousedown", handleMouseDown);
    window.addEventListener("mouseup", handleMouseUp);
    window.addEventListener("contextmenu", handleContextMenu);
    window.addEventListener("keydown", handleKeyDown);
    window.addEventListener("keyup", handleKeyUp);

    // Render loop
    const render = (time: number) => {
      const dt = Math.min(0.05, (time - lastTime) / 1000);
      lastTime = time;

      // Update engine simulation
      engine.update(dt);

      // Clear Canvas
      ctx.fillStyle = engine.timeDilationActive ? "#050b14" : "#020617";
      ctx.fillRect(0, 0, canvas.width, canvas.height);

      ctx.save();

      // Apply screen shake
      if (engine.screenShakeAmount > 0) {
        const shakeX = (Math.random() - 0.5) * engine.screenShakeAmount;
        const shakeY = (Math.random() - 0.5) * engine.screenShakeAmount;
        ctx.translate(shakeX, shakeY);
      }

      // Camera transformation centered on player
      const cx = canvas.width / 2 - engine.camera.x;
      const cy = canvas.height / 2 - engine.camera.y;
      ctx.translate(cx, cy);

      // 1. Draw Space Grid with Neon Lines
      const gridSize = 100;
      const startX = Math.floor((engine.camera.x - canvas.width / 2) / gridSize) * gridSize;
      const endX = startX + canvas.width + gridSize * 2;
      const startY = Math.floor((engine.camera.y - canvas.height / 2) / gridSize) * gridSize;
      const endY = startY + canvas.height + gridSize * 2;

      ctx.strokeStyle = engine.timeDilationActive
        ? "rgba(168, 85, 247, 0.14)"
        : engine.sector === 1
        ? "rgba(6, 182, 212, 0.08)"
        : engine.sector === 2
        ? "rgba(244, 63, 94, 0.08)"
        : "rgba(245, 158, 11, 0.08)";
      ctx.lineWidth = 1;

      ctx.beginPath();
      for (let x = startX; x <= endX; x += gridSize) {
        ctx.moveTo(x, startY);
        ctx.lineTo(x, endY);
      }
      for (let y = startY; y <= endY; y += gridSize) {
        ctx.moveTo(startX, y);
        ctx.lineTo(endX, y);
      }
      ctx.stroke();

      // Arena boundary walls
      ctx.strokeStyle = "rgba(239, 68, 68, 0.6)";
      ctx.lineWidth = 4;
      ctx.strokeRect(20, 20, engine.arenaWidth - 40, engine.arenaHeight - 40);

      // 2. Draw Data Shards
      for (const shard of engine.shards) {
        ctx.save();
        ctx.translate(shard.x, shard.y);
        ctx.fillStyle = shard.type === "credit" ? "#f59e0b" : shard.type === "repair" ? "#10b981" : "#38bdf8";
        ctx.shadowColor = ctx.fillStyle;
        ctx.shadowBlur = 8;
        ctx.beginPath();
        ctx.arc(0, 0, 4, 0, Math.PI * 2);
        ctx.fill();
        ctx.restore();
      }

      // 3. Draw Particles
      for (const p of engine.particles) {
        const alpha = Math.max(0, p.life / p.maxLife);
        ctx.fillStyle = p.color;
        ctx.globalAlpha = alpha;
        ctx.beginPath();
        ctx.arc(p.x, p.y, p.size, 0, Math.PI * 2);
        ctx.fill();
      }
      ctx.globalAlpha = 1.0;

      // 4. Draw Telegraph Lines for Snipers & Chargers
      for (const enemy of engine.enemies) {
        if (enemy.telegraphTimer && enemy.telegraphTimer > 0) {
          ctx.strokeStyle = "rgba(244, 63, 94, 0.7)";
          ctx.lineWidth = 2;
          ctx.setLineDash([8, 8]);
          ctx.beginPath();
          ctx.moveTo(enemy.x, enemy.y);
          ctx.lineTo(enemy.x + Math.cos(enemy.angle) * 750, enemy.y + Math.sin(enemy.angle) * 750);
          ctx.stroke();
          ctx.setLineDash([]);
        }
      }

      // 5. Draw Enemies
      for (const enemy of engine.enemies) {
        ctx.save();
        ctx.translate(enemy.x, enemy.y);
        ctx.rotate(enemy.angle);

        ctx.fillStyle = enemy.color;
        ctx.shadowColor = enemy.color;
        ctx.shadowBlur = 10;

        if (enemy.type === "drone_scout") {
          // Sleek triangle drone
          ctx.beginPath();
          ctx.moveTo(enemy.radius, 0);
          ctx.lineTo(-enemy.radius, -enemy.radius * 0.7);
          ctx.lineTo(-enemy.radius * 0.5, 0);
          ctx.lineTo(-enemy.radius, enemy.radius * 0.7);
          ctx.closePath();
          ctx.fill();
        } else if (enemy.type === "dart_striker") {
          // Dart arrow shape
          ctx.beginPath();
          ctx.moveTo(enemy.radius * 1.3, 0);
          ctx.lineTo(-enemy.radius, -enemy.radius * 0.6);
          ctx.lineTo(-enemy.radius, enemy.radius * 0.6);
          ctx.closePath();
          ctx.fill();
        } else if (enemy.type === "heavy_shielder") {
          // Armored octagon with front shield arc
          ctx.beginPath();
          ctx.arc(0, 0, enemy.radius, 0, Math.PI * 2);
          ctx.fill();

          // Shield arc
          ctx.strokeStyle = "#38bdf8";
          ctx.lineWidth = 5;
          ctx.beginPath();
          ctx.arc(0, 0, enemy.radius + 6, -Math.PI * 0.35, Math.PI * 0.35);
          ctx.stroke();
        } else if (enemy.type === "void_sniper") {
          // Diamond spire
          ctx.beginPath();
          ctx.moveTo(enemy.radius * 1.5, 0);
          ctx.lineTo(0, -enemy.radius * 0.6);
          ctx.lineTo(-enemy.radius * 0.8, 0);
          ctx.lineTo(0, enemy.radius * 0.6);
          ctx.closePath();
          ctx.fill();
        } else if (enemy.type === "guardian_boss") {
          // Multi-layer rotating boss core
          ctx.beginPath();
          ctx.arc(0, 0, enemy.radius, 0, Math.PI * 2);
          ctx.fill();

          ctx.strokeStyle = "#ffffff";
          ctx.lineWidth = 4;
          ctx.beginPath();
          ctx.arc(0, 0, enemy.radius * 0.5, 0, Math.PI * 2);
          ctx.stroke();

          // Boss arms
          for (let a = 0; a < 6; a++) {
            const armAngle = (a * Math.PI * 2) / 6;
            ctx.fillStyle = "#f43f5e";
            ctx.fillRect(Math.cos(armAngle) * enemy.radius * 0.8, Math.sin(armAngle) * enemy.radius * 0.8, 12, 12);
          }
        }

        ctx.restore();

        // Enemy Health Bar for bosses / elites
        if (enemy.hp < enemy.maxHp || enemy.type === "guardian_boss") {
          const barWidth = enemy.radius * 2.2;
          const barHeight = 4;
          const barY = enemy.y - enemy.radius - 12;
          const pct = Math.max(0, enemy.hp / enemy.maxHp);

          ctx.fillStyle = "rgba(0,0,0,0.6)";
          ctx.fillRect(enemy.x - barWidth / 2, barY, barWidth, barHeight);
          ctx.fillStyle = enemy.type === "guardian_boss" ? "#f43f5e" : "#10b981";
          ctx.fillRect(enemy.x - barWidth / 2, barY, barWidth * pct, barHeight);
        }
      }

      // 6. Draw Orbiting Saws
      if (engine.player.upgrades.includes("orbital_saw_blades") || engine.player.upgrades.includes("evo_saw_storm")) {
        const count = engine.player.upgrades.includes("evo_saw_storm") ? 4 : 2;
        const sawRadius = 85;
        const sawAngle = (time / 1000) * (engine.player.upgrades.includes("evo_saw_storm") ? 6.5 : 4.0);

        for (let i = 0; i < count; i++) {
          const angle = sawAngle + (i * Math.PI * 2) / count;
          const sx = engine.player.x + Math.cos(angle) * sawRadius;
          const sy = engine.player.y + Math.sin(angle) * sawRadius;

          ctx.save();
          ctx.translate(sx, sy);
          ctx.rotate(angle * 3);
          ctx.fillStyle = "#38bdf8";
          ctx.shadowColor = "#38bdf8";
          ctx.shadowBlur = 12;

          // Saw star shape
          ctx.beginPath();
          for (let s = 0; s < 8; s++) {
            const rot = (s * Math.PI * 2) / 8;
            const r = s % 2 === 0 ? 14 : 7;
            const px = Math.cos(rot) * r;
            const py = Math.sin(rot) * r;
            if (s === 0) ctx.moveTo(px, py);
            else ctx.lineTo(px, py);
          }
          ctx.closePath();
          ctx.fill();
          ctx.restore();
        }
      }

      // 7. Draw Projectiles
      for (const p of engine.projectiles) {
        ctx.save();
        ctx.translate(p.x, p.y);
        ctx.fillStyle = p.color;
        ctx.shadowColor = p.color;
        ctx.shadowBlur = 10;

        if (p.isMissile) {
          // Guided Missile
          const angle = Math.atan2(p.vy, p.vx);
          ctx.rotate(angle);
          ctx.fillRect(-8, -3, 16, 6);
        } else {
          // Laser bead
          ctx.beginPath();
          ctx.arc(0, 0, p.radius, 0, Math.PI * 2);
          ctx.fill();
        }
        ctx.restore();
      }

      // 8. Draw Player Spaceship
      const p = engine.player;
      ctx.save();
      ctx.translate(p.x, p.y);
      ctx.rotate(p.angle);

      const ship = SHIP_CLASSES[p.shipClass];

      // Shield bubble when shield is active
      if (p.shield > 0) {
        ctx.strokeStyle = "rgba(56, 189, 248, " + (0.3 + (p.shield / p.maxShield) * 0.4) + ")";
        ctx.lineWidth = 2;
        ctx.shadowColor = "#38bdf8";
        ctx.shadowBlur = 8;
        ctx.beginPath();
        ctx.arc(0, 0, 26, 0, Math.PI * 2);
        ctx.stroke();
      }

      // Ship hull geometry
      ctx.fillStyle = ship.accentColor;
      ctx.shadowColor = ship.accentColor;
      ctx.shadowBlur = p.isDashing ? 20 : 12;

      ctx.beginPath();
      ctx.moveTo(22, 0); // Nose cone
      ctx.lineTo(-14, -15); // Left wing
      ctx.lineTo(-8, 0); // Center thruster notch
      ctx.lineTo(-14, 15); // Right wing
      ctx.closePath();
      ctx.fill();

      // Cockpit canopy glow
      ctx.fillStyle = "#ffffff";
      ctx.beginPath();
      ctx.arc(4, 0, 4, 0, Math.PI * 2);
      ctx.fill();

      ctx.restore();

      ctx.restore(); // Restore camera transformation

      animationFrameId = requestAnimationFrame(render);
    };

    animationFrameId = requestAnimationFrame(render);

    return () => {
      cancelAnimationFrame(animationFrameId);
      window.removeEventListener("resize", handleResize);
      window.removeEventListener("mousemove", handleMouseMove);
      window.removeEventListener("mousedown", handleMouseDown);
      window.removeEventListener("mouseup", handleMouseUp);
      window.removeEventListener("contextmenu", handleContextMenu);
      window.removeEventListener("keydown", handleKeyDown);
      window.removeEventListener("keyup", handleKeyUp);
    };
  }, [engine]);

  return <canvas ref={canvasRef} className="block w-full h-full cursor-crosshair" />;
};
