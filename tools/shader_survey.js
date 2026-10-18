// Evaluated in the page by N64Recomp-browser's tools/record_shaders.mjs (tools/shader_plan.json): finds the shader
// survey struct of patches/shader_survey.c in RDRAM by its magic words, and offers surveyMap(map, exit, kong), which
// asks the game to go to a map (MAP_MAIN_MENU, 0x50, is the title and main menu; any other starts the game there, or
// warps there in play) and resolves once the game has loaded it, with the map the game is on; and surveyState(), the
// game's map and mode as the survey last copied them; and surveyMode(mode), which resolves once the game is in that
// mode (2 the DK Rap, 3 DK TV, 5 the main menu, 6 adventure), or after a minute. gameTime() is the game's own time in
// 60ths, for play_plan.json's clock: DK64 moves things by each frame's length in VIs (D_global_asm_80744478, its lag
// boost), so this adds that length for each frame the game counts (D_global_asm_8076A068, which the pause menu stops
// and a map load zeroes), polled every few ms; a frame counted between two polls takes the later one's length.
(() => {
  if (!window.__survey) {
    for (let a = 0x80801000; a < 0x81801000; a += 4) {
      if (n64r(a) === 0x53484452 && n64r(a + 4) === 0x53525659) { window.__survey = a; break; }
    }
  }
  const s = window.__survey;
  const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));
  window.surveyState = () => ({ map: n64r(s + 24) | 0, mode: n64r(s + 28) | 0, loaded: n64r(s + 8) });
  window.surveyMap = async (map, exit = 0, kong = -1) => {
    for (let attempt = 0; attempt < 3; attempt++) {
      const before = n64r(s + 8);
      n64w(s + 16, exit);
      n64w(s + 20, kong);
      n64w(s + 12, map);
      // The game takes it once the DK Rap has started (or later), then the map loads.
      for (let i = 0; i < 1200 && n64r(s + 8) === before; i++) await sleep(50);
      for (let i = 0; i < 600 && (n64r(s + 24) | 0) !== map; i++) await sleep(50);
      if ((n64r(s + 24) | 0) === map) return surveyState();
      n64w(s + 12, -1);
    }
    return surveyState();
  };
  window.surveyMode = async (mode) => {
    for (let i = 0; i < 1200 && (n64r(s + 28) | 0) !== mode; i++) await sleep(50);
    return surveyState();
  };
  if (!window.gameTime) {
    let frames = null, time = 0;
    window.gameTime = () => {
      const n = n64r(0x8076A068);
      // A map load zeroes the count; one place in the game adds 0x1869F to it and takes it off again in one frame.
      const d = (n >= frames && n - frames < 100) ? n - frames : (n < 100 ? n : 0);
      if (frames !== null) time += d * n64r(0x80744478);
      frames = n;
      return time;
    };
    setInterval(window.gameTime, 4);
  }
  return s ? 'survey at ' + s.toString(16) + ' ' + JSON.stringify(surveyState()) : 'no survey struct: a page built without patches/shader_survey.c';
})()
