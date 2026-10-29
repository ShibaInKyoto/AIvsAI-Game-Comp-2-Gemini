import express, { Request, Response } from "express";
import http from "http";
import path from "path";
import { fileURLToPath } from "url";
import { GoogleGenAI } from "@google/genai";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
const port = parseInt(process.env.PORT || "3000", 10);
const host = process.env.HOST || "0.0.0.0";

app.use(express.json());

// Initialize server-side Gemini AI client safely
const aiClient = process.env.GEMINI_API_KEY
  ? new GoogleGenAI({
      apiKey: process.env.GEMINI_API_KEY,
      httpOptions: {
        headers: {
          "User-Agent": "aistudio-build",
        },
      },
    })
  : null;

// Tactical AI Mission Generator & Boss Dialog API
app.post("/api/tactical-ai/directive", async (req: Request, res: Response) => {
  try {
    const { sector, score, shipClass, status } = req.body;

    if (!aiClient) {
      // High-quality procedural fallback directives
      const fallbacks = [
        "COMMAND OVERRIDE: Sector gravity lattice destabilizing. Prioritize energy siphon before perimeter collapse.",
        "ANOMALY DETECTED: Quantum core resonance at 184%. Divert excess power to directional thrusters and kinetic lances.",
        "TACTICAL NOTICE: Hostile dreadnought signals confirmed on sub-space sensors. Maintain orbital velocity.",
        "DEFENSE ADVISORY: Null-void breach imminent. Engage bullet-time overdrive when flanking elite interceptors.",
      ];
      const randomDirective = fallbacks[Math.floor(Math.random() * fallbacks.length)];
      return res.json({
        directive: randomDirective,
        threatLevel: sector > 3 ? "CRITICAL" : "ELEVATED",
        source: "Sub-Orbital Command (Heuristic Subroutine)",
      });
    }

    const prompt = `You are the Tactical Fleet AI Commander in a high-octane sci-fi space combat game titled 'ChronoStrider: Quantum Breach'.
The player is currently in Sector ${sector || 1}, score ${score || 0}, piloting a ${shipClass || "Vanguard Interceptor"}, status: ${status || "Normal"}.
Generate a short (1-2 sentences) immersive, badass military tactical broadcast or emergency warning. Return only the quote.`;

    const response = await aiClient.models.generateContent({
      model: "gemini-3.8-flash",
      contents: prompt,
    });

    res.json({
      directive: response.text?.trim() || "Hostile signatures spiking. Maintain optimal combat vector.",
      threatLevel: (sector || 1) > 3 ? "CRITICAL" : "ELEVATED",
      source: "Gemini Tactical Overwatch",
    });
  } catch (error: any) {
    console.error("Gemini directive error:", error.message);
    res.json({
      directive: "Sensors detecting quantum fluctuations. Brace for interceptor wave.",
      threatLevel: "MODERATE",
      source: "Local Avionics",
    });
  }
});

// High Score / Sector Records API (In-memory mock store)
interface RunRecord {
  id: string;
  pilotName: string;
  shipClass: string;
  score: number;
  sectorReached: number;
  timeSurvivedSeconds: number;
  enemiesDestroyed: number;
  date: string;
}

const leaderboard: RunRecord[] = [
  {
    id: "rec-1",
    pilotName: "Astra-09",
    shipClass: "Chrono-Stalker",
    score: 148500,
    sectorReached: 5,
    timeSurvivedSeconds: 432,
    enemiesDestroyed: 312,
    date: new Date().toISOString(),
  },
  {
    id: "rec-2",
    pilotName: "Vortex_Runner",
    shipClass: "Vanguard Interceptor",
    score: 98200,
    sectorReached: 4,
    timeSurvivedSeconds: 320,
    enemiesDestroyed: 220,
    date: new Date().toISOString(),
  },
  {
    id: "rec-3",
    pilotName: "Cipher_K",
    shipClass: "Titan Dread",
    score: 72400,
    sectorReached: 3,
    timeSurvivedSeconds: 245,
    enemiesDestroyed: 164,
    date: new Date().toISOString(),
  },
];

app.get("/api/leaderboard", (_req: Request, res: Response) => {
  res.json(leaderboard.sort((a, b) => b.score - a.score));
});

app.post("/api/leaderboard", (req: Request, res: Response) => {
  const { pilotName, shipClass, score, sectorReached, timeSurvivedSeconds, enemiesDestroyed } = req.body;
  const newRecord: RunRecord = {
    id: `rec-${Date.now()}`,
    pilotName: pilotName || "Rogue Strider",
    shipClass: shipClass || "Vanguard Interceptor",
    score: Number(score) || 0,
    sectorReached: Number(sectorReached) || 1,
    timeSurvivedSeconds: Number(timeSurvivedSeconds) || 0,
    enemiesDestroyed: Number(enemiesDestroyed) || 0,
    date: new Date().toISOString(),
  };
  leaderboard.push(newRecord);
  leaderboard.sort((a, b) => b.score - a.score);
  res.json({ success: true, rank: leaderboard.findIndex((r) => r.id === newRecord.id) + 1 });
});

// App Health
app.get("/api/health", (_req: Request, res: Response) => {
  res.json({
    status: "online",
    engine: "ChronoStrider Hybrid Engine (WebGL/WebAudio + Native C++ Windows)",
    port,
    timestamp: new Date().toISOString(),
  });
});

async function startServer() {
  const isDev = process.env.NODE_ENV !== "production";

  if (isDev) {
    const { createServer: createViteServer } = await import("vite");
    const vite = await createViteServer({
      server: { middlewareMode: true },
      appType: "spa",
    });
    app.use(vite.middlewares);
  } else {
    app.use(express.static(path.resolve(__dirname, "dist")));
    app.get("*", (_req, res) => {
      res.sendFile(path.resolve(__dirname, "dist", "index.html"));
    });
  }

  const server = http.createServer(app);
  server.listen(port, host, () => {
    console.log(`[ChronoStrider] Game dev server online at http://${host}:${port}`);
  });
}

startServer().catch((err) => {
  console.error("Failed to start server:", err);
  process.exit(1);
});
