import React, { useState, useEffect } from "react";
import { UpgradeCard } from "../game/types.ts";
import { getRandomUpgradeCards } from "../game/upgrades.ts";
import { GameEngine } from "../game/engine.ts";
import { Zap, Shield, Sparkles, Tornado, Sun, CircleDot, Rocket, Disc } from "lucide-react";

interface UpgradeDraftModalProps {
  engine: GameEngine;
  onSelectUpgrade: (cardId: string) => void;
}

export const UpgradeDraftModal: React.FC<UpgradeDraftModalProps> = ({
  engine,
  onSelectUpgrade,
}) => {
  const [cards, setCards] = useState<UpgradeCard[]>([]);

  useEffect(() => {
    const rolled = getRandomUpgradeCards(3, engine.player.upgrades);
    setCards(rolled);
  }, []);

  const getRarityBadge = (rarity: string) => {
    switch (rarity) {
      case "legendary":
        return "bg-amber-500/20 text-amber-300 border-amber-500/60 shadow-[0_0_10px_rgba(245,158,11,0.5)]";
      case "epic":
        return "bg-purple-500/20 text-purple-300 border-purple-500/60 shadow-[0_0_10px_rgba(168,85,247,0.5)]";
      case "rare":
        return "bg-cyan-500/20 text-cyan-300 border-cyan-500/60";
      default:
        return "bg-slate-700/30 text-slate-300 border-slate-600";
    }
  };

  const getCardBorder = (rarity: string) => {
    switch (rarity) {
      case "legendary":
        return "border-amber-400 hover:border-amber-300 shadow-[0_0_20px_rgba(245,158,11,0.3)]";
      case "epic":
        return "border-purple-400 hover:border-purple-300 shadow-[0_0_15px_rgba(168,85,247,0.3)]";
      case "rare":
        return "border-cyan-400 hover:border-cyan-300 shadow-[0_0_12px_rgba(6,182,212,0.25)]";
      default:
        return "border-slate-700 hover:border-slate-500";
    }
  };

  const renderIcon = (card: UpgradeCard) => {
    if (card.category === "evolution") {
      if (card.id === "evo_saw_storm") return <Tornado className="w-8 h-8 text-amber-400" />;
      if (card.id === "evo_singularity_missiles") return <CircleDot className="w-8 h-8 text-amber-400" />;
      return <Sun className="w-8 h-8 text-amber-400" />;
    }
    if (card.category === "weapon") return <Zap className="w-8 h-8 text-cyan-400" />;
    if (card.category === "defense") return <Shield className="w-8 h-8 text-emerald-400" />;
    return <Sparkles className="w-8 h-8 text-purple-400" />;
  };

  return (
    <div className="absolute inset-0 bg-slate-950/85 backdrop-blur-lg flex items-center justify-center p-6 z-50 select-none">
      <div className="max-w-4xl w-full flex flex-col items-center">
        {/* Title */}
        <div className="text-center mb-8">
          <span className="text-xs font-mono font-bold tracking-widest text-cyan-400 uppercase">
            SYSTEM FIRMWARE LEVEL UP
          </span>
          <h2 className="text-3xl font-heading font-black text-white tracking-wider mt-1 text-glow-cyan">
            CHOOSE TACTICAL MODULE
          </h2>
          <p className="text-sm text-slate-400 mt-1">
            Install an offensive modification, passive defensive matrix, or unlock an evolution.
          </p>
        </div>

        {/* 3 Upgrade Cards Grid */}
        <div className="grid grid-cols-1 md:grid-cols-3 gap-6 w-full">
          {cards.map((card) => (
            <div
              key={card.id}
              onClick={() => onSelectUpgrade(card.id)}
              className={`bg-slate-900/90 rounded-2xl p-6 border-2 flex flex-col justify-between transition-all duration-200 hover:-translate-y-2 hover:scale-102 cursor-pointer group ${getCardBorder(
                card.rarity
              )}`}
            >
              <div>
                {/* Header with Icon and Rarity */}
                <div className="flex items-start justify-between mb-4">
                  <div className="p-3 bg-slate-950 rounded-xl border border-slate-800 group-hover:scale-110 transition-transform">
                    {renderIcon(card)}
                  </div>
                  <span
                    className={`text-[10px] font-mono font-bold uppercase px-2.5 py-0.5 rounded-full border ${getRarityBadge(
                      card.rarity
                    )}`}
                  >
                    {card.rarity}
                  </span>
                </div>

                {/* Card Title & Category */}
                <span className="text-[10px] font-mono text-slate-500 uppercase tracking-wider block mb-1">
                  {card.category}
                </span>
                <h3 className="font-heading font-bold text-lg text-white mb-2 leading-tight">
                  {card.name}
                </h3>

                {/* Description */}
                <p className="text-xs text-slate-300 leading-relaxed mb-4">
                  {card.description}
                </p>
              </div>

              {/* Stat Modifier & Install Action */}
              <div>
                <div className="bg-slate-950/80 rounded-lg p-2.5 border border-slate-800/80 mb-4">
                  <span className="text-[10px] font-mono text-slate-400 block mb-0.5 uppercase">
                    Modifier
                  </span>
                  <span className="text-xs font-mono font-bold text-cyan-300">
                    {card.statModifier}
                  </span>
                </div>

                <button className="w-full py-2.5 rounded-xl font-heading font-bold text-xs uppercase tracking-wider bg-cyan-600 group-hover:bg-cyan-500 text-white transition-colors shadow-lg cursor-pointer">
                  Install Upgrade
                </button>
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
};
