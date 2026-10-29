import React, { useState } from "react";
import { GameMode, ShipClassId } from "../game/types.ts";
import { SHIP_CLASSES } from "../game/engine.ts";
import { Play, Sparkles, Trophy, Settings, Terminal, Shield, Zap, Gauge, Heart } from "lucide-react";

interface TitleScreenProps {
  onStartGame: (shipClass: ShipClassId, mode: GameMode) => void;
  onOpenNativeHub: () => void;
  onOpenLeaderboard: () => void;
  onOpenSettings: () => void;
}

export const TitleScreen: React.FC<TitleScreenProps> = ({
  onStartGame,
  onOpenNativeHub,
  onOpenLeaderboard,
  onOpenSettings,
}) => {
  const [selectedShip, setSelectedShip] = useState<ShipClassId>("vanguard");
  const [selectedMode, setSelectedMode] = useState<GameMode>("campaign");

  const ship = SHIP_CLASSES[selectedShip];

  return (
    <div className="absolute inset-0 bg-slate-950/75 backdrop-blur-sm flex flex-col justify-between p-8 z-40 select-none">
      {/* Top Header */}
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-3">
          <div className="w-3 h-3 rounded-full bg-cyan-400 animate-ping" />
          <span className="text-xs font-mono font-bold tracking-widest text-cyan-400 uppercase">
            AIvsAI COMPETITION ROUND 2 — STANDALONE ENGINE
          </span>
        </div>

        <div className="flex items-center gap-3">
          <button
            onClick={onOpenLeaderboard}
            className="px-4 py-2 bg-slate-900/80 hover:bg-slate-800 border border-slate-700 rounded-xl text-xs font-mono font-semibold text-slate-300 flex items-center gap-2 cursor-pointer transition-colors"
          >
            <Trophy className="w-4 h-4 text-amber-400" />
            RECORDS & LEADERBOARD
          </button>
          <button
            onClick={onOpenSettings}
            className="p-2 bg-slate-900/80 hover:bg-slate-800 border border-slate-700 rounded-xl text-slate-300 cursor-pointer transition-colors"
          >
            <Settings className="w-4 h-4" />
          </button>
        </div>
      </div>

      {/* Hero Title & Ship Selection */}
      <div className="max-w-4xl mx-auto w-full text-center">
        <div className="inline-block px-4 py-1 rounded-full bg-cyan-950/70 border border-cyan-500/40 text-[11px] font-mono font-bold text-cyan-400 uppercase mb-3 shadow-[0_0_15px_rgba(6,182,212,0.3)]">
          KINETIC SCI-FI COMBAT SIMULATOR
        </div>
        <h1 className="text-5xl md:text-7xl font-heading font-black tracking-wider text-white text-glow-cyan mb-2">
          CHRONO<span className="text-cyan-400">STRIDER</span>
        </h1>
        <p className="text-sm md:text-base text-slate-400 font-sans tracking-wide max-w-2xl mx-auto mb-8">
          Master dynamic bullet-time physics, evade swarms of autonomous hostiles, and draft game-breaking tactical weapon evolutions across 5 deep-space sectors.
        </p>

        {/* Ship Class Selection Tabs */}
        <div className="bg-slate-900/90 border border-slate-800 rounded-2xl p-6 shadow-2xl max-w-3xl mx-auto mb-8">
          <div className="flex justify-center gap-2 mb-6 flex-wrap">
            {(Object.keys(SHIP_CLASSES) as ShipClassId[]).map((id) => {
              const sc = SHIP_CLASSES[id];
              const isSelected = selectedShip === id;
              return (
                <button
                  key={id}
                  onClick={() => setSelectedShip(id)}
                  className={`px-4 py-2.5 rounded-xl font-heading font-bold text-xs uppercase tracking-wider transition-all cursor-pointer border ${
                    isSelected
                      ? "bg-cyan-600 text-white border-cyan-400 shadow-[0_0_15px_rgba(6,182,212,0.5)]"
                      : "bg-slate-950 text-slate-400 border-slate-800 hover:border-slate-700"
                  }`}
                >
                  {sc.name}
                </button>
              );
            })}
          </div>

          {/* Selected Ship Stats Grid */}
          <div className="grid grid-cols-1 md:grid-cols-2 gap-6 text-left">
            <div>
              <h3 className="font-heading font-bold text-lg text-white mb-1">
                {ship.name}
              </h3>
              <p className="text-xs text-slate-400 mb-4 leading-relaxed">
                {ship.tagline}
              </p>
              <div className="bg-slate-950/90 rounded-xl p-3 border border-slate-800/80">
                <span className="text-[10px] font-mono text-cyan-400 uppercase font-semibold block mb-0.5">
                  Primary Armament
                </span>
                <span className="text-xs font-heading font-bold text-white">
                  {ship.primaryWeapon}
                </span>
                <div className="mt-2 pt-2 border-t border-slate-900">
                  <span className="text-[10px] font-mono text-purple-400 uppercase font-semibold block mb-0.5">
                    Tactical Override
                  </span>
                  <span className="text-xs text-slate-300">
                    {ship.specialAbility}
                  </span>
                </div>
              </div>
            </div>

            {/* Stat Bars */}
            <div className="space-y-3 flex flex-col justify-center">
              <div>
                <div className="flex justify-between text-xs font-mono text-slate-400 mb-1">
                  <span className="flex items-center gap-1"><Heart className="w-3.5 h-3.5 text-rose-400" /> Hull Integrity</span>
                  <span className="text-white font-bold">{ship.hull} HP</span>
                </div>
                <div className="w-full bg-slate-950 h-2 rounded-full overflow-hidden">
                  <div className="bg-rose-500 h-full" style={{ width: `${(ship.hull / 180) * 100}%` }} />
                </div>
              </div>

              <div>
                <div className="flex justify-between text-xs font-mono text-slate-400 mb-1">
                  <span className="flex items-center gap-1"><Shield className="w-3.5 h-3.5 text-cyan-400" /> Deflector Shield</span>
                  <span className="text-white font-bold">{ship.shield} SP</span>
                </div>
                <div className="w-full bg-slate-950 h-2 rounded-full overflow-hidden">
                  <div className="bg-cyan-400 h-full" style={{ width: `${(ship.shield / 100) * 100}%` }} />
                </div>
              </div>

              <div>
                <div className="flex justify-between text-xs font-mono text-slate-400 mb-1">
                  <span className="flex items-center gap-1"><Gauge className="w-3.5 h-3.5 text-amber-400" /> Sub-Light Speed</span>
                  <span className="text-white font-bold">{ship.speed} m/s</span>
                </div>
                <div className="w-full bg-slate-950 h-2 rounded-full overflow-hidden">
                  <div className="bg-amber-400 h-full" style={{ width: `${(ship.speed / 540) * 100}%` }} />
                </div>
              </div>

              <div>
                <div className="flex justify-between text-xs font-mono text-slate-400 mb-1">
                  <span className="flex items-center gap-1"><Zap className="w-3.5 h-3.5 text-purple-400" /> Dash Cooldown</span>
                  <span className="text-white font-bold">{ship.dashCooldown}s</span>
                </div>
                <div className="w-full bg-slate-950 h-2 rounded-full overflow-hidden">
                  <div className="bg-purple-500 h-full" style={{ width: `${((4 - ship.dashCooldown) / 2.5) * 100}%` }} />
                </div>
              </div>
            </div>
          </div>
        </div>

        {/* Game Mode Selector */}
        <div className="flex justify-center gap-4 mb-6">
          <button
            onClick={() => setSelectedMode("campaign")}
            className={`px-4 py-2 rounded-xl text-xs font-mono font-bold transition-all border cursor-pointer ${
              selectedMode === "campaign"
                ? "bg-cyan-950/80 text-cyan-300 border-cyan-400 shadow-[0_0_10px_rgba(6,182,212,0.4)]"
                : "bg-slate-950/60 text-slate-400 border-slate-800"
            }`}
          >
            CAMPAIGN (5 SECTORS)
          </button>
          <button
            onClick={() => setSelectedMode("endless")}
            className={`px-4 py-2 rounded-xl text-xs font-mono font-bold transition-all border cursor-pointer ${
              selectedMode === "endless"
                ? "bg-cyan-950/80 text-cyan-300 border-cyan-400 shadow-[0_0_10px_rgba(6,182,212,0.4)]"
                : "bg-slate-950/60 text-slate-400 border-slate-800"
            }`}
          >
            ENDLESS CORRUPTION
          </button>
          <button
            onClick={() => setSelectedMode("bossrush")}
            className={`px-4 py-2 rounded-xl text-xs font-mono font-bold transition-all border cursor-pointer ${
              selectedMode === "bossrush"
                ? "bg-cyan-950/80 text-cyan-300 border-cyan-400 shadow-[0_0_10px_rgba(6,182,212,0.4)]"
                : "bg-slate-950/60 text-slate-400 border-slate-800"
            }`}
          >
            BOSS RUSH
          </button>
        </div>

        {/* Launch Buttons */}
        <div className="flex justify-center items-center gap-4">
          <button
            onClick={() => onStartGame(selectedShip, selectedMode)}
            className="px-8 py-4 bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-white font-heading font-black text-lg uppercase tracking-wider rounded-2xl shadow-[0_0_25px_rgba(6,182,212,0.6)] flex items-center gap-3 transition-transform hover:scale-105 cursor-pointer"
          >
            <Play className="w-5 h-5 fill-white" />
            LAUNCH BREACH RUN
          </button>

          <button
            onClick={onOpenNativeHub}
            className="px-6 py-4 bg-slate-900/90 hover:bg-slate-800 border border-slate-700 text-slate-200 font-heading font-bold text-sm uppercase tracking-wider rounded-2xl shadow-xl flex items-center gap-2.5 transition-all cursor-pointer"
          >
            <Terminal className="w-4 h-4 text-cyan-400" />
            WINDOWS 10/11 C++ ENGINE
          </button>
        </div>
      </div>

      {/* Footer Controls & Attract Mode notice */}
      <div className="flex items-center justify-between text-xs font-mono text-slate-500">
        <div>
          ATTRACT MODE ACTIVE: AI Pilot is running background combat simulation.
        </div>
        <div className="flex items-center gap-4">
          <span>CONTROLS: WASD / ARROWS (Move)</span>
          <span>MOUSE / L-CLICK (Aim & Fire)</span>
          <span>SPACE / SHIFT (Dash)</span>
          <span>R-CLICK / Q (Bullet-Time Slomo)</span>
        </div>
      </div>
    </div>
  );
};
