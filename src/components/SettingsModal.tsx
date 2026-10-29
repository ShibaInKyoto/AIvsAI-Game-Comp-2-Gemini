import React from "react";
import { GameEngine } from "../game/engine.ts";
import { sound } from "../game/audio.ts";
import { Settings as SettingsIcon, Volume2, Sparkles, Monitor, Activity } from "lucide-react";

interface SettingsModalProps {
  engine: GameEngine;
  onClose: () => void;
}

export const SettingsModal: React.FC<SettingsModalProps> = ({ engine, onClose }) => {
  const settings = engine.settings;

  const handleSoundChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const val = parseFloat(e.target.value);
    settings.soundVolume = val;
    sound.setVolumes(val, settings.musicVolume);
  };

  const handleMusicChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const val = parseFloat(e.target.value);
    settings.musicVolume = val;
    sound.setVolumes(settings.soundVolume, val);
  };

  return (
    <div className="absolute inset-0 bg-slate-950/85 backdrop-blur-md flex items-center justify-center p-6 z-50 select-none">
      <div className="bg-slate-900 border border-slate-700/80 rounded-2xl max-w-lg w-full p-6 shadow-2xl flex flex-col space-y-6">
        <div className="flex items-center justify-between pb-4 border-b border-slate-800">
          <div className="flex items-center gap-2.5 text-white">
            <SettingsIcon className="w-5 h-5 text-cyan-400" />
            <h3 className="font-heading font-black text-lg">AVIONICS & SIMULATION SETTINGS</h3>
          </div>
          <button
            onClick={onClose}
            className="px-4 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-xl text-xs font-mono font-semibold transition-colors cursor-pointer"
          >
            CLOSE
          </button>
        </div>

        {/* Audio Controls */}
        <div className="space-y-4">
          <div className="text-xs font-mono font-bold text-slate-400 uppercase tracking-wider flex items-center gap-1.5">
            <Volume2 className="w-4 h-4 text-cyan-400" /> Audio Synthesizer
          </div>

          <div>
            <div className="flex justify-between text-xs font-mono text-slate-300 mb-1">
              <span>Sound Effects</span>
              <span>{Math.round(settings.soundVolume * 100)}%</span>
            </div>
            <input
              type="range"
              min="0"
              max="1"
              step="0.05"
              value={settings.soundVolume}
              onChange={handleSoundChange}
              className="w-full accent-cyan-400 cursor-pointer"
            />
          </div>

          <div>
            <div className="flex justify-between text-xs font-mono text-slate-300 mb-1">
              <span>Dynamic Synthwave Music</span>
              <span>{Math.round(settings.musicVolume * 100)}%</span>
            </div>
            <input
              type="range"
              min="0"
              max="1"
              step="0.05"
              value={settings.musicVolume}
              onChange={handleMusicChange}
              className="w-full accent-cyan-400 cursor-pointer"
            />
          </div>
        </div>

        {/* Graphics & Effects */}
        <div className="space-y-3 pt-4 border-t border-slate-800">
          <div className="text-xs font-mono font-bold text-slate-400 uppercase tracking-wider flex items-center gap-1.5">
            <Sparkles className="w-4 h-4 text-cyan-400" /> Visual Dynamics
          </div>

          <label className="flex items-center justify-between p-3 bg-slate-950 rounded-xl border border-slate-800 cursor-pointer">
            <span className="text-xs font-mono text-slate-300">Screen Shake & Impact Force</span>
            <input
              type="checkbox"
              checked={settings.screenShake}
              onChange={(e) => (settings.screenShake = e.target.checked)}
              className="accent-cyan-400 w-4 h-4 rounded cursor-pointer"
            />
          </label>

          <label className="flex items-center justify-between p-3 bg-slate-950 rounded-xl border border-slate-800 cursor-pointer">
            <span className="text-xs font-mono text-slate-300">CRT Retro Scanline Overlay</span>
            <input
              type="checkbox"
              checked={settings.crtScanlines}
              onChange={(e) => (settings.crtScanlines = e.target.checked)}
              className="accent-cyan-400 w-4 h-4 rounded cursor-pointer"
            />
          </label>

          <label className="flex items-center justify-between p-3 bg-slate-950 rounded-xl border border-slate-800 cursor-pointer">
            <span className="text-xs font-mono text-slate-300">Reduced Flashing (Accessibility)</span>
            <input
              type="checkbox"
              checked={settings.reducedFlashing}
              onChange={(e) => (settings.reducedFlashing = e.target.checked)}
              className="accent-cyan-400 w-4 h-4 rounded cursor-pointer"
            />
          </label>
        </div>
      </div>
    </div>
  );
};
