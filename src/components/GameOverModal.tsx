import React, { useState } from "react";
import { GameEngine, SHIP_CLASSES } from "../game/engine.ts";
import { Trophy, RotateCcw, Home, Crosshair, Clock, Award } from "lucide-react";

interface GameOverModalProps {
  engine: GameEngine;
  onRestart: () => void;
  onHome: () => void;
}

export const GameOverModal: React.FC<GameOverModalProps> = ({ engine, onRestart, onHome }) => {
  const [pilotName, setPilotName] = useState("Vanguard-One");
  const [submitted, setSubmitted] = useState(false);

  const p = engine.player;
  const isVictory = engine.isVictory;
  const ship = SHIP_CLASSES[p.shipClass];

  const handleSubmitScore = async () => {
    if (submitted) return;
    try {
      await fetch("/api/leaderboard", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          pilotName: pilotName || "Anonymous Strider",
          shipClass: ship.name,
          score: engine.score,
          sectorReached: engine.sector,
          timeSurvivedSeconds: Math.round(engine.sectorTimer),
          enemiesDestroyed: p.killCount,
        }),
      });
      setSubmitted(true);
    } catch {
      setSubmitted(true);
    }
  };

  return (
    <div className="absolute inset-0 bg-slate-950/90 backdrop-blur-md flex items-center justify-center p-6 z-50 select-none">
      <div className="bg-slate-900 border border-slate-800 rounded-3xl max-w-lg w-full p-8 shadow-2xl flex flex-col items-center text-center">
        {/* Status Badge */}
        <div
          className={`px-4 py-1 rounded-full text-xs font-mono font-bold uppercase tracking-widest mb-3 ${
            isVictory
              ? "bg-emerald-500/20 text-emerald-300 border border-emerald-500/50 shadow-[0_0_15px_rgba(16,185,129,0.4)]"
              : "bg-rose-500/20 text-rose-300 border border-rose-500/50 shadow-[0_0_15px_rgba(244,63,94,0.4)]"
          }`}
        >
          {isVictory ? "MISSION COMPLETE: VOID CORE DESTROYED" : "HULL CRITICAL: VESSEL COMPROMISED"}
        </div>

        <h2 className="text-4xl font-heading font-black text-white tracking-wider mb-1">
          {isVictory ? "VICTORY" : "RUN TERMINATED"}
        </h2>
        <p className="text-xs text-slate-400 font-mono mb-6">
          {ship.name} • Sector 0{engine.sector}
        </p>

        {/* Big Score Display */}
        <div className="bg-slate-950/80 border border-slate-800 rounded-2xl p-4 w-full mb-6">
          <div className="text-[10px] font-mono text-slate-400 uppercase tracking-widest mb-0.5">
            Final Combat Score
          </div>
          <div className="font-heading font-black text-4xl text-glow-cyan text-cyan-300">
            {engine.score.toLocaleString()}
          </div>
        </div>

        {/* Stats Grid */}
        <div className="grid grid-cols-3 gap-3 w-full mb-6">
          <div className="bg-slate-950/60 p-3 rounded-xl border border-slate-800/80">
            <span className="text-[10px] font-mono text-slate-400 block uppercase">Hostiles Cleared</span>
            <span className="font-heading font-bold text-lg text-white">{p.killCount}</span>
          </div>
          <div className="bg-slate-950/60 p-3 rounded-xl border border-slate-800/80">
            <span className="text-[10px] font-mono text-slate-400 block uppercase">Max Combo</span>
            <span className="font-heading font-bold text-lg text-rose-400">x{p.combo}</span>
          </div>
          <div className="bg-slate-950/60 p-3 rounded-xl border border-slate-800/80">
            <span className="text-[10px] font-mono text-slate-400 block uppercase">Modules Drafted</span>
            <span className="font-heading font-bold text-lg text-purple-400">{p.upgrades.length}</span>
          </div>
        </div>

        {/* Call-sign Submission */}
        {!submitted ? (
          <div className="w-full mb-6">
            <div className="flex gap-2">
              <input
                type="text"
                maxLength={16}
                value={pilotName}
                onChange={(e) => setPilotName(e.target.value)}
                placeholder="Enter Pilot Call-sign"
                className="bg-slate-950 border border-slate-700 px-3 py-2 rounded-xl text-xs font-mono text-white flex-1 focus:outline-none focus:border-cyan-400"
              />
              <button
                onClick={handleSubmitScore}
                className="px-4 py-2 bg-cyan-600 hover:bg-cyan-500 text-white font-heading font-bold text-xs uppercase tracking-wider rounded-xl cursor-pointer transition-colors"
              >
                Submit Record
              </button>
            </div>
          </div>
        ) : (
          <div className="text-xs font-mono text-emerald-400 mb-6 flex items-center gap-1.5">
            <Award className="w-4 h-4" /> Telemetry uploaded to Flight Records.
          </div>
        )}

        {/* Action Buttons */}
        <div className="flex items-center gap-3 w-full">
          <button
            onClick={onRestart}
            className="flex-1 py-3 bg-gradient-to-r from-cyan-600 to-blue-600 hover:from-cyan-500 hover:to-blue-500 text-white font-heading font-bold text-xs uppercase tracking-wider rounded-xl shadow-lg flex items-center justify-center gap-2 cursor-pointer transition-all"
          >
            <RotateCcw className="w-4 h-4" /> Play Again
          </button>
          <button
            onClick={onHome}
            className="px-5 py-3 bg-slate-800 hover:bg-slate-700 text-slate-300 font-heading font-bold text-xs uppercase tracking-wider rounded-xl cursor-pointer transition-colors flex items-center gap-2"
          >
            <Home className="w-4 h-4" /> Title
          </button>
        </div>
      </div>
    </div>
  );
};
