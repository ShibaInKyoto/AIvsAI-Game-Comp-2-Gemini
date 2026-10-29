import React, { useState, useEffect } from "react";
import { Trophy, Clock, Skull, Crosshair, Award } from "lucide-react";

interface RecordItem {
  id: string;
  pilotName: string;
  shipClass: string;
  score: number;
  sectorReached: number;
  timeSurvivedSeconds: number;
  enemiesDestroyed: number;
  date: string;
}

interface LeaderboardModalProps {
  onClose: () => void;
}

export const LeaderboardModal: React.FC<LeaderboardModalProps> = ({ onClose }) => {
  const [records, setRecords] = useState<RecordItem[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    fetch("/api/leaderboard")
      .then((res) => res.json())
      .then((data) => {
        setRecords(data);
        setLoading(false);
      })
      .catch(() => {
        setLoading(false);
      });
  }, []);

  return (
    <div className="absolute inset-0 bg-slate-950/85 backdrop-blur-md flex items-center justify-center p-6 z-50 select-none">
      <div className="bg-slate-900 border border-slate-700/80 rounded-2xl max-w-2xl w-full p-6 shadow-2xl flex flex-col max-h-[85vh]">
        <div className="flex items-center justify-between pb-4 border-b border-slate-800">
          <div className="flex items-center gap-3">
            <Trophy className="w-6 h-6 text-amber-400" />
            <div>
              <h3 className="font-heading font-black text-xl text-white">
                HALL OF CHRONO-STRIDERS
              </h3>
              <p className="text-xs text-slate-400 font-mono">
                Top Flight Simulation Records & Sector High Scores
              </p>
            </div>
          </div>
          <button
            onClick={onClose}
            className="px-4 py-2 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-xl text-xs font-mono font-semibold transition-colors cursor-pointer"
          >
            CLOSE
          </button>
        </div>

        <div className="py-4 overflow-y-auto space-y-3 flex-1">
          {loading ? (
            <div className="text-center py-12 text-slate-500 font-mono text-xs">
              Retrieving telemetry records from flight recorder...
            </div>
          ) : (
            records.map((rec, idx) => (
              <div
                key={rec.id}
                className="bg-slate-950/80 border border-slate-800 rounded-xl p-3.5 flex items-center justify-between hover:border-slate-700 transition-colors"
              >
                <div className="flex items-center gap-4">
                  <div
                    className={`w-8 h-8 rounded-lg flex items-center justify-center font-heading font-black text-sm ${
                      idx === 0
                        ? "bg-amber-500/20 text-amber-400 border border-amber-500/50"
                        : idx === 1
                        ? "bg-slate-300/20 text-slate-300 border border-slate-400/50"
                        : idx === 2
                        ? "bg-amber-700/20 text-amber-600 border border-amber-700/50"
                        : "bg-slate-900 text-slate-500"
                    }`}
                  >
                    #{idx + 1}
                  </div>
                  <div>
                    <div className="font-heading font-bold text-white text-sm">
                      {rec.pilotName}
                    </div>
                    <div className="text-[11px] font-mono text-cyan-400">
                      {rec.shipClass} • Sector 0{rec.sectorReached}
                    </div>
                  </div>
                </div>

                <div className="flex items-center gap-6 text-right">
                  <div className="text-xs font-mono text-slate-400 flex items-center gap-1">
                    <Crosshair className="w-3.5 h-3.5 text-rose-400" />
                    {rec.enemiesDestroyed} kills
                  </div>
                  <div>
                    <div className="font-heading font-black text-lg text-glow-cyan text-cyan-300">
                      {rec.score.toLocaleString()}
                    </div>
                    <div className="text-[10px] font-mono text-slate-500">
                      {Math.floor(rec.timeSurvivedSeconds / 60)}m {rec.timeSurvivedSeconds % 60}s
                    </div>
                  </div>
                </div>
              </div>
            ))
          )}
        </div>
      </div>
    </div>
  );
};
