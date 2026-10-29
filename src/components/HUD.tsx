import React, { useState, useEffect } from "react";
import { GameEngine, SHIP_CLASSES } from "../game/engine.ts";
import {
  Shield,
  Heart,
  Zap,
  Flame,
  Radio,
  Cpu,
  Settings as SettingsIcon,
  Pause,
  Play,
  Volume2,
  VolumeX,
  Sparkles,
} from "lucide-react";

interface HUDProps {
  engine: GameEngine;
  onOpenSettings: () => void;
  onOpenNativeHub: () => void;
  onOpenLeaderboard: () => void;
}

export const HUD: React.FC<HUDProps> = ({
  engine,
  onOpenSettings,
  onOpenNativeHub,
  onOpenLeaderboard,
}) => {
  const [, setTick] = useState(0);
  const [tacticalDirective, setTacticalDirective] = useState("Scanning hostile combat network...");
  const [aiSource, setAiSource] = useState("Tactical Overwatch");
  const [muted, setMuted] = useState(false);

  // Sync HUD state every 60ms
  useEffect(() => {
    const interval = setInterval(() => {
      setTick((t) => t + 1);
    }, 60);
    return () => clearInterval(interval);
  }, []);

  // Fetch dynamic AI tactical directive every 20 seconds
  useEffect(() => {
    const fetchDirective = async () => {
      try {
        const res = await fetch("/api/tactical-ai/directive", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            sector: engine.sector,
            score: engine.score,
            shipClass: SHIP_CLASSES[engine.player.shipClass].name,
            status: engine.player.hull < 35 ? "Hull Critical" : "Combat Active",
          }),
        });
        const data = await res.json();
        if (data.directive) {
          setTacticalDirective(data.directive);
          setAiSource(data.source || "Tactical Overwatch");
        }
      } catch (err) {
        // Fallback directive
        setTacticalDirective("Maintain orbital velocity. Eliminate elite interceptor nodes.");
      }
    };

    fetchDirective();
    const directiveInterval = setInterval(fetchDirective, 22000);
    return () => clearInterval(directiveInterval);
  }, [engine.sector]);

  const p = engine.player;
  const ship = SHIP_CLASSES[p.shipClass];

  const hullPct = Math.max(0, Math.min(100, (p.hull / p.maxHull) * 100));
  const shieldPct = Math.max(0, Math.min(100, (p.shield / p.maxShield) * 100));
  const energyPct = Math.max(0, Math.min(100, (p.chronoEnergy / p.maxChronoEnergy) * 100));

  const toggleSound = () => {
    if (muted) {
      engine.settings.soundVolume = 0.7;
      engine.settings.musicVolume = 0.5;
      setMuted(false);
    } else {
      engine.settings.soundVolume = 0;
      engine.settings.musicVolume = 0;
      setMuted(true);
    }
  };

  const toggleAiPilot = () => {
    engine.settings.aiPilotEnabled = !engine.settings.aiPilotEnabled;
  };

  return (
    <div className="absolute inset-0 pointer-events-none flex flex-col justify-between p-4 select-none">
      {/* Top Cockpit Header */}
      <div className="flex items-start justify-between">
        {/* Pilot Vitals */}
        <div className="pointer-events-auto bg-slate-900/85 backdrop-blur-md border border-cyan-500/30 rounded-xl p-3 shadow-xl w-72">
          <div className="flex items-center justify-between mb-2">
            <span className="font-heading font-bold text-xs tracking-wider text-cyan-400 uppercase">
              {ship.name}
            </span>
            <span className="text-xs font-mono font-semibold text-slate-400">
              LVL {p.level}
            </span>
          </div>

          {/* Shield Bar */}
          <div className="mb-2">
            <div className="flex justify-between text-xs font-mono mb-1">
              <span className="flex items-center gap-1 text-cyan-300">
                <Shield className="w-3.5 h-3.5" /> SHIELD
              </span>
              <span className="text-cyan-400 font-bold">
                {Math.round(p.shield)} / {p.maxShield}
              </span>
            </div>
            <div className="w-full bg-slate-950 h-2.5 rounded-full overflow-hidden border border-cyan-900/40">
              <div
                className="h-full bg-gradient-to-r from-cyan-500 to-sky-300 transition-all duration-100 shadow-[0_0_8px_rgba(6,182,212,0.8)]"
                style={{ width: `${shieldPct}%` }}
              />
            </div>
          </div>

          {/* Hull Integrity Bar */}
          <div className="mb-2">
            <div className="flex justify-between text-xs font-mono mb-1">
              <span className="flex items-center gap-1 text-rose-400">
                <Heart className="w-3.5 h-3.5" /> HULL
              </span>
              <span className="text-rose-400 font-bold">
                {Math.round(p.hull)} / {p.maxHull}
              </span>
            </div>
            <div className="w-full bg-slate-950 h-2.5 rounded-full overflow-hidden border border-rose-900/40">
              <div
                className="h-full bg-gradient-to-r from-rose-600 to-rose-400 transition-all duration-100 shadow-[0_0_8px_rgba(244,63,94,0.8)]"
                style={{ width: `${hullPct}%` }}
              />
            </div>
          </div>

          {/* Chrono-Drive (Bullet Time) Energy Bar */}
          <div>
            <div className="flex justify-between text-xs font-mono mb-1">
              <span className="flex items-center gap-1 text-purple-400">
                <Zap className="w-3.5 h-3.5" /> CHRONO-SLOMO [R-CLICK / Q]
              </span>
              <span className="text-purple-400 font-bold">
                {Math.round(p.chronoEnergy)}%
              </span>
            </div>
            <div className="w-full bg-slate-950 h-2 rounded-full overflow-hidden border border-purple-900/40">
              <div
                className="h-full bg-gradient-to-r from-purple-600 to-fuchsia-400 transition-all duration-100 shadow-[0_0_8px_rgba(168,85,247,0.8)]"
                style={{ width: `${energyPct}%` }}
              />
            </div>
          </div>
        </div>

        {/* Center Sector Progress & Score */}
        <div className="flex flex-col items-center">
          <div className="pointer-events-auto bg-slate-900/85 backdrop-blur-md border border-slate-700/50 rounded-2xl px-6 py-2 shadow-2xl flex items-center gap-6">
            <div className="text-center">
              <div className="text-[10px] font-mono tracking-wider text-slate-400 uppercase">
                Combat Score
              </div>
              <div className="font-heading font-black text-2xl text-glow-cyan text-cyan-300">
                {engine.score.toLocaleString()}
              </div>
            </div>

            <div className="h-8 w-px bg-slate-800" />

            <div className="text-center">
              <div className="text-[10px] font-mono tracking-wider text-slate-400 uppercase">
                Sector
              </div>
              <div className="font-heading font-black text-2xl text-amber-400">
                0{engine.sector} <span className="text-xs text-slate-500 font-normal">/ 05</span>
              </div>
            </div>

            <div className="h-8 w-px bg-slate-800" />

            {/* Combo Multiplier */}
            <div className="text-center">
              <div className="text-[10px] font-mono tracking-wider text-slate-400 uppercase">
                Chain Combo
              </div>
              <div className="font-heading font-black text-2xl text-glow-rose text-rose-400 flex items-center justify-center gap-1">
                <Flame className="w-5 h-5 text-rose-500 fill-rose-500 animate-pulse" />
                x{p.combo}
              </div>
            </div>
          </div>

          {/* Time Dilation Active Banner */}
          {engine.timeDilationActive && (
            <div className="mt-2 bg-purple-950/80 border border-purple-500/60 px-4 py-0.5 rounded-full text-xs font-heading font-bold text-purple-300 tracking-widest animate-pulse shadow-[0_0_15px_rgba(168,85,247,0.6)]">
              CHRONO-DRIVE OVERCLOCK ACTIVE (0.2x TIME)
            </div>
          )}

          {/* Guardian Boss Alarm Banner */}
          {engine.bossActive && (
            <div className="mt-2 bg-rose-950/90 border border-rose-500 px-6 py-1 rounded-full text-xs font-heading font-black text-rose-300 tracking-widest animate-bounce shadow-[0_0_20px_rgba(244,63,94,0.9)]">
              ⚠️ GUARDIAN DREADNOUGHT SIGNATURE DETECTED ⚠️
            </div>
          )}
        </div>

        {/* Top-Right Game Controls & Native Hub Button */}
        <div className="pointer-events-auto flex items-center gap-2">
          {/* Autopilot Button */}
          <button
            onClick={toggleAiPilot}
            className={`px-3 py-2 rounded-xl text-xs font-mono font-bold flex items-center gap-1.5 transition-all shadow-lg border ${
              engine.settings.aiPilotEnabled
                ? "bg-purple-600/90 hover:bg-purple-500 text-white border-purple-400 shadow-[0_0_12px_rgba(168,85,247,0.7)]"
                : "bg-slate-900/85 hover:bg-slate-800 text-slate-300 border-slate-700"
            }`}
            title="Toggle autonomous AI Pilot"
          >
            <Cpu className="w-4 h-4" />
            {engine.settings.aiPilotEnabled ? "AI PILOT: ON" : "AI PILOT"}
          </button>

          {/* Windows 10/11 Native Engine Hub */}
          <button
            onClick={onOpenNativeHub}
            className="px-3 py-2 rounded-xl text-xs font-mono font-bold flex items-center gap-1.5 bg-gradient-to-r from-cyan-600 to-blue-600 hover:from-cyan-500 hover:to-blue-500 text-white border border-cyan-400 shadow-[0_0_15px_rgba(6,182,212,0.4)] transition-all cursor-pointer"
            title="Open Windows 10/11 C++ Source, CMake, and Release Hub"
          >
            <Sparkles className="w-4 h-4" />
            WIN 10/11 C++ HUB
          </button>

          {/* Sound Toggle */}
          <button
            onClick={toggleSound}
            className="p-2.5 rounded-xl bg-slate-900/85 hover:bg-slate-800 border border-slate-700 text-slate-300 transition-all cursor-pointer"
            title="Toggle Audio"
          >
            {muted ? <VolumeX className="w-4 h-4 text-rose-400" /> : <Volume2 className="w-4 h-4 text-cyan-400" />}
          </button>

          {/* Pause */}
          <button
            onClick={() => (engine.isPaused = !engine.isPaused)}
            className="p-2.5 rounded-xl bg-slate-900/85 hover:bg-slate-800 border border-slate-700 text-slate-300 transition-all cursor-pointer"
            title="Pause Game"
          >
            {engine.isPaused ? <Play className="w-4 h-4 text-emerald-400" /> : <Pause className="w-4 h-4 text-amber-400" />}
          </button>

          {/* Settings */}
          <button
            onClick={onOpenSettings}
            className="p-2.5 rounded-xl bg-slate-900/85 hover:bg-slate-800 border border-slate-700 text-slate-300 transition-all cursor-pointer"
            title="Settings & Audio"
          >
            <SettingsIcon className="w-4 h-4" />
          </button>
        </div>
      </div>

      {/* Bottom HUD: Tactical AI Overwatch & Upgrades Bar */}
      <div className="flex items-end justify-between">
        {/* Tactical Overwatch Box (Gemini powered server-side) */}
        <div className="pointer-events-auto bg-slate-900/85 backdrop-blur-md border border-cyan-500/20 rounded-xl p-3 max-w-md shadow-xl flex items-start gap-2.5">
          <div className="p-1.5 bg-cyan-950/80 border border-cyan-500/40 rounded-lg text-cyan-400 shrink-0">
            <Radio className="w-4 h-4 animate-pulse" />
          </div>
          <div className="flex-1">
            <div className="flex items-center justify-between mb-0.5">
              <span className="text-[10px] font-mono tracking-wider text-cyan-400 uppercase font-semibold">
                {aiSource}
              </span>
              <span className="text-[9px] font-mono text-slate-500">LIVE FEED</span>
            </div>
            <p className="text-xs text-slate-200 leading-snug font-sans italic">
              "{tacticalDirective}"
            </p>
          </div>
        </div>

        {/* Active Upgrades Strip */}
        <div className="pointer-events-auto flex items-center gap-1.5 bg-slate-900/85 backdrop-blur-md border border-slate-800 rounded-xl px-3 py-2">
          <span className="text-[10px] font-mono text-slate-400 uppercase mr-1">
            MODULES ({p.upgrades.length}):
          </span>
          {p.upgrades.length === 0 ? (
            <span className="text-xs text-slate-500 italic">None installed</span>
          ) : (
            p.upgrades.map((upg, idx) => (
              <span
                key={idx}
                className="px-2 py-0.5 bg-cyan-950/70 border border-cyan-500/30 rounded text-[11px] font-mono text-cyan-300 font-semibold"
              >
                {upg.replace(/_/g, " ")}
              </span>
            ))
          )}
        </div>
      </div>
    </div>
  );
};
