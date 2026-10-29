import React, { useState, useEffect, useRef } from "react";
import { GameEngine } from "./game/engine.ts";
import { AIPilot } from "./game/aiPilot.ts";
import { GameCanvas } from "./components/GameCanvas.tsx";
import { HUD } from "./components/HUD.tsx";
import { TitleScreen } from "./components/TitleScreen.tsx";
import { UpgradeDraftModal } from "./components/UpgradeDraftModal.tsx";
import { GameOverModal } from "./components/GameOverModal.tsx";
import { NativeWindowsHub } from "./components/NativeWindowsHub.tsx";
import { LeaderboardModal } from "./components/LeaderboardModal.tsx";
import { SettingsModal } from "./components/SettingsModal.tsx";
import { ShipClassId, GameMode } from "./game/types.ts";

export default function App() {
  const [inGame, setInGame] = useState(false);
  const [showNativeHub, setShowNativeHub] = useState(false);
  const [showLeaderboard, setShowLeaderboard] = useState(false);
  const [showSettings, setShowSettings] = useState(false);
  const [, setTrigger] = useState(0);

  // Single persistent game engine & AI pilot
  const engineRef = useRef<GameEngine | null>(null);
  const aiPilotRef = useRef<AIPilot | null>(null);

  if (!engineRef.current) {
    const eng = new GameEngine("vanguard", "campaign");
    engineRef.current = eng;
    aiPilotRef.current = new AIPilot(eng);
  }

  const engine = engineRef.current;
  const aiPilot = aiPilotRef.current;

  // Global loop for AI pilot when enabled or on Title Screen (Attract mode)
  useEffect(() => {
    let frameId: number;
    let lastTime = performance.now();

    const loop = (t: number) => {
      const dt = Math.min(0.05, (t - lastTime) / 1000);
      lastTime = t;

      if (!inGame || engine.settings.aiPilotEnabled) {
        aiPilot?.update(dt);
      }

      setTrigger((prev) => prev + 1);
      frameId = requestAnimationFrame(loop);
    };

    frameId = requestAnimationFrame(loop);
    return () => cancelAnimationFrame(frameId);
  }, [inGame]);

  const handleStartGame = (shipClass: ShipClassId, mode: GameMode) => {
    engine.reset(shipClass, mode);
    setInGame(true);
  };

  const handleSelectUpgrade = (cardId: string) => {
    engine.applyUpgrade(cardId);
  };

  const handleRestart = () => {
    engine.reset(engine.player.shipClass, engine.gameMode);
  };

  const handleHome = () => {
    engine.reset("vanguard", "campaign");
    setInGame(false);
  };

  return (
    <div className="relative w-screen h-screen bg-slate-950 overflow-hidden font-sans select-none">
      {/* 60 FPS Game Simulation Canvas */}
      <GameCanvas engine={engine} />

      {/* CRT Scanline overlay if enabled */}
      {engine.settings.crtScanlines && (
        <div className="absolute inset-0 scanlines pointer-events-none z-30" />
      )}

      {/* Title Screen Overlay (Attract Mode running beneath) */}
      {!inGame && (
        <TitleScreen
          onStartGame={handleStartGame}
          onOpenNativeHub={() => setShowNativeHub(true)}
          onOpenLeaderboard={() => setShowLeaderboard(true)}
          onOpenSettings={() => setShowSettings(true)}
        />
      )}

      {/* In-Game Cockpit HUD */}
      {inGame && (
        <HUD
          engine={engine}
          onOpenSettings={() => setShowSettings(true)}
          onOpenNativeHub={() => setShowNativeHub(true)}
          onOpenLeaderboard={() => setShowLeaderboard(true)}
        />
      )}

      {/* Upgrade Level-Up Draft Modal */}
      {inGame && engine.isLevelUpPending && (
        <UpgradeDraftModal engine={engine} onSelectUpgrade={handleSelectUpgrade} />
      )}

      {/* Game Over or Victory Modal */}
      {inGame && (engine.isGameOver || engine.isVictory) && (
        <GameOverModal engine={engine} onRestart={handleRestart} onHome={handleHome} />
      )}

      {/* Windows 10/11 Native Hub Modal */}
      {showNativeHub && <NativeWindowsHub onClose={() => setShowNativeHub(false)} />}

      {/* Leaderboard Modal */}
      {showLeaderboard && <LeaderboardModal onClose={() => setShowLeaderboard(false)} />}

      {/* Settings Modal */}
      {showSettings && <SettingsModal engine={engine} onClose={() => setShowSettings(false)} />}
    </div>
  );
}
