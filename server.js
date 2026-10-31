const http = require('http');
const fs = require('fs');
const path = require('path');
const { execFile } = require('child_process');

const PORT = 3000;
const PUBLIC_DIR = path.join(__dirname, 'public');
const RELEASES_DIR = path.join(__dirname, 'releases');
const SCREENSHOTS_DIR = path.join(__dirname, 'docs', 'screenshots');

const MIME_TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.js': 'application/javascript; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg',
  '.svg': 'image/svg+xml',
  '.zip': 'application/zip',
  '.gz': 'application/gzip',
  '.tar.gz': 'application/gzip',
  '.txt': 'text/plain; charset=utf-8'
};

function serveFile(res, filePath, contentType, isDownload = false, downloadName = '') {
  fs.stat(filePath, (err, stats) => {
    if (err || !stats.isFile()) {
      res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
      res.end('404 Not Found');
      return;
    }

    const headers = {
      'Content-Type': contentType,
      'Content-Length': stats.size,
      'Cache-Control': 'public, max-age=3600'
    };

    if (isDownload) {
      headers['Content-Disposition'] = `attachment; filename="${downloadName || path.basename(filePath)}"`;
    }

    res.writeHead(200, headers);
    fs.createReadStream(filePath).pipe(res);
  });
}

const server = http.createServer((req, res) => {
  const parsedUrl = new URL(req.url, `http://${req.headers.host || 'localhost'}`);
  let pathname = decodeURIComponent(parsedUrl.pathname);

  // CORS headers for local preview
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    res.writeHead(204);
    res.end();
    return;
  }

  // API endpoints
  if (pathname === '/api/status') {
    const winExeExists = fs.existsSync(path.join(RELEASES_DIR, 'Aethernaut-Windows-x64', 'Aethernaut.exe'));
    const winZipExists = fs.existsSync(path.join(RELEASES_DIR, 'Aethernaut-Windows-x64.zip'));
    const linuxBinExists =
      fs.existsSync(path.join(RELEASES_DIR, 'Aethernaut-Linux-x64', 'Aethernaut')) ||
      fs.existsSync(path.join('/tmp/build-linux/bin/Aethernaut'));
    
    let winZipSize = 0;
    if (winZipExists) {
      winZipSize = fs.statSync(path.join(RELEASES_DIR, 'Aethernaut-Windows-x64.zip')).size;
    }

    const payload = {
      game: 'AETHERNAUT: 3D Sky-Fleet Architect & Leviathan Hunter',
      engine: 'Native C++20 + Raylib 5.5 + GLSL 330 Shaders',
      status: 'Ready for Judging',
      builds: {
        windowsX64: {
          ready: winExeExists && winZipExists,
          binary: 'Aethernaut.exe (PE32+ executable for MS Windows x86-64)',
          archive: 'Aethernaut-Windows-x64.zip',
          sizeBytes: winZipSize,
          sizeMb: (winZipSize / (1024 * 1024)).toFixed(2)
        },
        linuxX64: {
          ready: linuxBinExists,
          binary: 'Aethernaut (ELF 64-bit LSB executable)',
          archive: 'Aethernaut-Linux-x64.tar.gz'
        }
      },
      metrics: {
        simulationRateHz: 120,
        voxelResolution: '15x11x23 (3,795 cells/vessel)',
        leviathanSegments: '18-30 segments across 6 Colossal Species + Boss Counterplay',
        starterBlueprints: 12,
        blockTypes: 22,
        leviathanSpecies: 6,
        skyWonders: 10,
        loreLogs: 12,
        skyEcosystemEntities: '36 Sky-Ray Boids + 24 Aether-Medusa Jellyfish',
        tradeCommodities: 4,
        specialistOfficers: 5,
        islandsCount: 14
      }
    };

    res.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8' });
    res.end(JSON.stringify(payload, null, 2));
    return;
  }

  if (pathname === '/api/run-tests') {
    const releaseTestBin = path.join(RELEASES_DIR, 'Aethernaut-Linux-x64', 'aethernaut_tests');
    const testBin = fs.existsSync(releaseTestBin) ? releaseTestBin : '/tmp/build-linux/bin/aethernaut_tests';
    if (!fs.existsSync(testBin)) {
      res.writeHead(500, { 'Content-Type': 'application/json; charset=utf-8' });
      res.end(JSON.stringify({ error: 'Test binary not found' }));
      return;
    }

    execFile(testBin, (error, stdout, stderr) => {
      res.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8' });
      res.end(JSON.stringify({
        success: !error,
        output: stdout.toString(),
        stderr: stderr.toString()
      }));
    });
    return;
  }

  if (pathname === '/api/run-benchmark') {
    const releaseBenchBin = path.join(RELEASES_DIR, 'Aethernaut-Linux-x64', 'aethernaut_benchmark');
    const benchBin = fs.existsSync(releaseBenchBin) ? releaseBenchBin : '/tmp/build-linux/bin/aethernaut_benchmark';
    if (!fs.existsSync(benchBin)) {
      res.writeHead(500, { 'Content-Type': 'application/json; charset=utf-8' });
      res.end(JSON.stringify({ error: 'Benchmark binary not found' }));
      return;
    }

    execFile(benchBin, (error, stdout, stderr) => {
      res.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8' });
      res.end(JSON.stringify({
        success: !error,
        output: stdout.toString(),
        stderr: stderr.toString()
      }));
    });
    return;
  }

  // Release downloads
  if (pathname.startsWith('/releases/')) {
    const relFile = pathname.replace('/releases/', '');
    const safeRelPath = path.join(RELEASES_DIR, path.basename(relFile));
    const ext = path.extname(safeRelPath).toLowerCase();
    serveFile(res, safeRelPath, MIME_TYPES[ext] || 'application/octet-stream', true, path.basename(safeRelPath));
    return;
  }

  // Screenshots
  if (pathname.startsWith('/docs/screenshots/')) {
    const shotFile = pathname.replace('/docs/screenshots/', '');
    const safeShotPath = path.join(SCREENSHOTS_DIR, path.basename(shotFile));
    const ext = path.extname(safeShotPath).toLowerCase();
    serveFile(res, safeShotPath, MIME_TYPES[ext] || 'image/png');
    return;
  }

  // Public static files
  let safePath = path.normalize(path.join(PUBLIC_DIR, pathname));
  if (!safePath.startsWith(PUBLIC_DIR)) {
    res.writeHead(403, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end('403 Forbidden');
    return;
  }

  if (pathname === '/' || pathname === '') {
    safePath = path.join(PUBLIC_DIR, 'index.html');
  }

  fs.stat(safePath, (err, stats) => {
    if (err) {
      // Fallback to index.html for SPA-style routing if needed
      safePath = path.join(PUBLIC_DIR, 'index.html');
    } else if (stats.isDirectory()) {
      safePath = path.join(safePath, 'index.html');
    }

    const ext = path.extname(safePath).toLowerCase();
    const contentType = MIME_TYPES[ext] || 'text/html; charset=utf-8';
    serveFile(res, safePath, contentType);
  });
});

server.listen(PORT, '0.0.0.0', () => {
  console.log(`[AETHERNAUT Mission Control] Server listening on http://0.0.0.0:${PORT}`);
});
