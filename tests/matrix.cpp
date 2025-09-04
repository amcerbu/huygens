// monitor_tui.cpp (C++17)
// Per-route gains + dynamic in/out channel counts (only when engine stopped) + separate device dropdowns.
// Build with your script: ./build monitor_tui.cpp

#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cmath>

#include <portaudio.h>
#include <ncurses.h>

#include "../src/audio.h"

using namespace soundmath;

static constexpr int BSIZE = 64;

// Gain limits/steps
static constexpr float GAIN_DB_MIN   = -60.0f;
static constexpr float GAIN_DB_MAX   = +12.0f;
static constexpr float GAIN_STEP_DB  =   1.0f;
static constexpr float GAIN_STEP2_DB =   6.0f;

static inline float db_to_lin(float db)  { return std::pow(10.0f, db / 20.0f); }
static inline float lin_to_db(float lin) { return 20.0f * std::log10(std::max(lin, 1e-12f)); }
static inline float clampf(float x, float lo, float hi) { return std::max(lo, std::min(hi, x)); }

// ---------------- Routing state (RT-safe snapshot via shared_ptr) ----------------
struct Routing {
    int inChans = 2;
    int outChans = 2;
    // routes[out * inChans + in] ∈ {0,1}
    std::vector<uint8_t> routes;
    // per-route linear gain (same shape as routes), default 0 dB = 1.0
    std::vector<float> gains;
};

// atomic ops for shared_ptr (C++17 libc++ friendly)
static std::shared_ptr<const Routing> gRouting;

static std::atomic<bool> gRunningEngine{false};
static std::atomic<bool> gKeepUI{true};
static int gInDevice = paNoDevice;
static int gOutDevice = paNoDevice;

// ---------------- Device enumeration (PortAudio) ----------------
struct DeviceList {
    std::vector<int> inputIds;
    std::vector<int> outputIds;
    std::vector<std::string> inputNames;
    std::vector<std::string> outputNames;
    std::vector<int> inputMaxCh;   // parallel to inputIds
    std::vector<int> outputMaxCh;  // parallel to outputIds
};

static DeviceList enumerateDevices() {
    DeviceList dl;
    if (Pa_Initialize() != paNoError) return dl;

    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (!info) continue;
        std::string name = info->name ? info->name : "unknown";
        if (info->maxInputChannels > 0) {
            dl.inputIds.push_back(i);
            dl.inputNames.push_back(name);
            dl.inputMaxCh.push_back(info->maxInputChannels);
        }
        if (info->maxOutputChannels > 0) {
            dl.outputIds.push_back(i);
            dl.outputNames.push_back(name);
            dl.outputMaxCh.push_back(info->maxOutputChannels);
        }
    }
    Pa_Terminate();
    return dl;
}

static int findIndex(const std::vector<int>& ids, int id) {
    auto it = std::find(ids.begin(), ids.end(), id);
    return it == ids.end() ? -1 : static_cast<int>(it - ids.begin());
}

static int maxInForDevice(const DeviceList& dl, int devId) {
    int idx = findIndex(dl.inputIds, devId);
    return idx >= 0 ? dl.inputMaxCh[idx] : 0;
}
static int maxOutForDevice(const DeviceList& dl, int devId) {
    int idx = findIndex(dl.outputIds, devId);
    return idx >= 0 ? dl.outputMaxCh[idx] : 0;
}

inline int process_cb(const float* in, float* out) {
    auto r = std::atomic_load_explicit(&gRouting, std::memory_order_acquire);
    if (!r) return 0;

    const int inC  = r->inChans;
    const int outC = r->outChans;

    // If host gives no input this cycle, output silence.
    if (!in) {
        std::fill(out, out + BSIZE * outC, 0.0f);
        return 0;
    }

    // --- 1) Copy input to scratch (prevents in/out aliasing issues) ---
    static thread_local std::vector<float> scratch;
    scratch.resize(static_cast<size_t>(BSIZE) * static_cast<size_t>(inC));
    std::memcpy(scratch.data(), in, scratch.size() * sizeof(float));

    // --- 2) Clear outputs ---
    std::fill(out, out + BSIZE * outC, 0.0f);

    // --- 3) Mix from scratch (uses per-route gains if you added them) ---
    for (int n = 0; n < BSIZE; ++n) {
        const float* inF  = scratch.data() + static_cast<size_t>(n) * inC;
        float*       outF = out            + static_cast<size_t>(n) * outC;

        for (int o = 0; o < outC; ++o) {
            float acc = 0.f;
            const uint8_t* rowR = &r->routes[o * inC];
            const float*   rowG = &r->gains [o * inC];   // if you have per-route gains
            for (int i = 0; i < inC; ++i) {
                if (rowR[i]) acc += rowG ? (rowG[i] * inF[i]) : inF[i];
            }
            outF[o] = acc;
        }
    }
    return 0;
}

// // ---------------- Audio callback ----------------
// inline int process_cb(const float* in, float* out) {
//     auto r = std::atomic_load_explicit(&gRouting, std::memory_order_acquire);
//     if (!r) return 0;

//     const int inC = r->inChans;
//     const int outC = r->outChans;

//     // zero outputs
//     for (int i = 0; i < BSIZE * outC; ++i) out[i] = 0.0f;

//     for (int n = 0; n < BSIZE; ++n) {
//         const float* inF  = in  + n * inC;
//         float*       outF = out + n * outC;
//         for (int o = 0; o < outC; ++o) {
//             float acc = 0.f;
//             const uint8_t* rowR = &r->routes[o * inC];
//             const float*   rowG = &r->gains [o * inC];
//             for (int i = 0; i < inC; ++i)
//                 if (rowR[i]) acc += rowG[i] * inF[i];
//             outF[o] = acc;
//         }
//     }
//     return 0;
// }

static Audio A(process_cb, BSIZE);

// ---------------- Engine control ----------------
static bool canStartWithCurrentConfig(const DeviceList& dl, const Routing& r,
                                      int inDev, int outDev, std::string& err) {
    int iMax = maxInForDevice(dl, inDev);
    int oMax = maxOutForDevice(dl, outDev);
    if (iMax == 0) { err = "Selected input device has 0 input channels."; return false; }
    if (oMax == 0) { err = "Selected output device has 0 output channels."; return false; }
    if (r.inChans > iMax) {
        err = "Requested input channels (" + std::to_string(r.inChans) +
              ") exceed device max (" + std::to_string(iMax) + ").";
        return false;
    }
    if (r.outChans > oMax) {
        err = "Requested output channels (" + std::to_string(r.outChans) +
              ") exceed device max (" + std::to_string(oMax) + ").";
        return false;
    }
    return true;
}

static bool startEngine(const DeviceList& dl) {
    if (gRunningEngine.load()) return true;
    auto r = std::atomic_load(&gRouting);
    if (!r) return false;

    std::string err;
    if (!canStartWithCurrentConfig(dl, *r, gInDevice, gOutDevice, err)) {
        mvprintw(2, 2, "Cannot start: %s", err.c_str());
        return false;
    }
    try {
        A.startup(r->inChans, r->outChans, false, gInDevice, gOutDevice);
        gRunningEngine.store(true);
        return true;
    } catch (...) {
        gRunningEngine.store(false);
        mvprintw(2, 2, "Failed to start engine (device busy/incompatible).");
        return false;
    }
}

static void stopEngine() {
    if (!gRunningEngine.load()) return;
    A.shutdown();
    gRunningEngine.store(false);
}

// ---------------- UI ----------------
struct UIState {
    int matrixTop = 5;
    int matrixLeft = 2;
    int cellW = 3;
    int cellH = 1;
    int cursorIn = 0;
    int cursorOut = 0;
    bool showInDropdown = false;
    bool showOutDropdown = false;
    int inDropIndex = 0;
    int outDropIndex = 0;
    std::string toast;
    std::chrono::steady_clock::time_point toastUntil{};
};

static void toast(UIState& ui, const std::string& s, int ms = 1800) {
    ui.toast = s;
    ui.toastUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
}

static void drawHeader(const Routing& r, const DeviceList& dl, const UIState& ui) {
    mvprintw(0, 2, "Audio Monitor  (q: quit)  (s: start/stop)  (d: devices)  "
                    "I/i: ±inputs  O/o: ±outputs  "
                    "-/+: ±1 dB  [/]: ±6 dB  0: reset dB") ;
    int iMax = maxInForDevice(dl, gInDevice);
    int oMax = maxOutForDevice(dl, gOutDevice);
    mvprintw(1, 2, "Inputs=%d (max %d)   Outputs=%d (max %d)   Engine: %s",
             r.inChans, iMax, r.outChans, oMax, gRunningEngine.load() ? "RUNNING" : "STOPPED");

    // Selected cell gain HUD
    int o = ui.cursorOut, i = ui.cursorIn;
    if (o >= 0 && o < r.outChans && i >= 0 && i < r.inChans) {
        size_t idx = static_cast<size_t>(o) * r.inChans + i;
        bool on = r.routes[idx] != 0;
        float lin = r.gains[idx];
        float db  = lin_to_db(lin);
        mvprintw(2, 2, "Selected: out %d <- in %d  |  %s  |  gain = %+5.1f dB",
                 o, i, on ? "ON " : "OFF", db);
    }
}

static void drawMatrix(const Routing& r, const UIState& ui) {
    int top = ui.matrixTop;
    int left = ui.matrixLeft;
    mvprintw(top, left + 6, "Routing Matrix (rows=outputs, cols=inputs)");

    for (int i = 0; i < r.inChans; ++i)
        mvprintw(top + 2, left + 6 + i * (ui.cellW + 1), "%2d", i);

    for (int o = 0; o < r.outChans; ++o) {
        mvprintw(top + 3 + o * (ui.cellH + 1), left, "out %2d →", o);
        for (int i = 0; i < r.inChans; ++i) {
            int y = top + 3 + o * (ui.cellH + 1);
            int x = left + 6 + i * (ui.cellW + 1);
            bool on = r.routes[o * r.inChans + i] != 0;
            bool cursorHere = (o == ui.cursorOut && i == ui.cursorIn);
            if (cursorHere) attron(A_REVERSE);
            mvprintw(y, x, "[%c]", on ? 'X' : ' ');
            if (cursorHere) attroff(A_REVERSE);
        }
    }
}

static void drawButtons(const DeviceList& dl, const UIState& ui) {
    int row = 3, col = 60;

    attron(A_BOLD);
    mvprintw(row, col, "%s", gRunningEngine.load() ? "[ Stop ]" : "[ Start ]");
    attroff(A_BOLD);

    auto findName = [&](bool input) -> std::string {
        const auto& ids   = input ? dl.inputIds   : dl.outputIds;
        const auto& names = input ? dl.inputNames : dl.outputNames;
        int sel = input ? gInDevice : gOutDevice;
        if (sel == paNoDevice) return "(none)";
        int idx = findIndex(ids, sel);
        if (idx < 0) return "(unknown)";
        return names[static_cast<size_t>(idx)];
    };
    auto findMax = [&](bool input) -> int {
        return input ? maxInForDevice(dl, gInDevice) : maxOutForDevice(dl, gOutDevice);
    };

    mvprintw(row+2, col, "Input: ");
    if (ui.showInDropdown) attron(A_REVERSE);
    mvprintw(row+2, col + 7, "%s  [max %d]", findName(true).c_str(), findMax(true));
    if (ui.showInDropdown) attroff(A_REVERSE);

    mvprintw(row+3, col, "Output:");
    if (ui.showOutDropdown) attron(A_REVERSE);
    mvprintw(row+3, col + 7, "%s  [max %d]", findName(false).c_str(), findMax(false));
    if (ui.showOutDropdown) attroff(A_REVERSE);
}

static void drawDropdown(const std::vector<std::string>& names,
                         const std::vector<int>& maxCh,
                         int startRow, int startCol, int highlightIndex) {
    int maxVisible = static_cast<int>(names.size());
    if (maxVisible > 12) maxVisible = 12;
    for (int i = 0; i < maxVisible; ++i) {
        if (i == highlightIndex) attron(A_REVERSE);
        mvprintw(startRow + i, startCol, "%s  [max %d]", names[i].c_str(), maxCh[i]);
        if (i == highlightIndex) attroff(A_REVERSE);
    }
}

static bool insideCell(const UIState& ui, const Routing& r,
                       int my, int mx, int& outRow, int& inCol) {
    int top = ui.matrixTop + 3;
    int left = ui.matrixLeft + 6;
    for (int o = 0; o < r.outChans; ++o) {
        for (int i = 0; i < r.inChans; ++i) {
            int y = top + o * (ui.cellH + 1);
            int x = left + i * (ui.cellW + 1);
            if (my == y && mx >= x && mx <= x + 2) {
                outRow = o; inCol = i; return true;
            }
        }
    }
    return false;
}

static void toggleRoute(std::shared_ptr<Routing>& wip, int o, int i) {
    if (!wip) return;
    size_t idx = static_cast<size_t>(o) * static_cast<size_t>(wip->inChans) + static_cast<size_t>(i);
    wip->routes[idx] = wip->routes[idx] ? 0 : 1;
    // if turning on and gain is "unset", ensure default 0 dB (1.0)
    if (wip->routes[idx] && wip->gains[idx] <= 0.0f) wip->gains[idx] = 1.0f;
}

// set/step gain (in dB) for a specific route
static void stepRouteGainDb(std::shared_ptr<Routing>& wip, int o, int i, float deltaDb) {
    size_t idx = static_cast<size_t>(o) * static_cast<size_t>(wip->inChans) + static_cast<size_t>(i);
    float curLin = wip->gains[idx];
    float curDb  = lin_to_db(curLin);
    float newDb  = clampf(curDb + deltaDb, GAIN_DB_MIN, GAIN_DB_MAX);
    wip->gains[idx] = db_to_lin(newDb);
}

static void resetRouteGain(std::shared_ptr<Routing>& wip, int o, int i) {
    size_t idx = static_cast<size_t>(o) * static_cast<size_t>(wip->inChans) + static_cast<size_t>(i);
    wip->gains[idx] = 1.0f; // 0 dB
}

// Resize routing (engine must be stopped). Preserves overlap of old routes & gains.
static void resizeRouting(int newIn, int newOut) {
    auto snap = std::atomic_load(&gRouting);
    std::shared_ptr<Routing> rnew(new Routing(*snap));
    rnew->inChans = newIn;
    rnew->outChans = newOut;
    rnew->routes.assign(static_cast<size_t>(newIn * newOut), 0);
    rnew->gains .assign(static_cast<size_t>(newIn * newOut), 1.0f);
    int copyIn = std::min(newIn, snap->inChans);
    int copyOut = std::min(newOut, snap->outChans);
    for (int o = 0; o < copyOut; ++o) {
        for (int i = 0; i < copyIn; ++i) {
            rnew->routes[o * newIn + i] = snap->routes[o * snap->inChans + i];
            rnew->gains [o * newIn + i] = snap->gains [o * snap->inChans + i];
        }
    }
    std::atomic_store_explicit(&gRouting, std::shared_ptr<const Routing>(rnew), std::memory_order_release);
}

static void maybeShowToast(UIState& ui) {
    if (!ui.toast.empty() && std::chrono::steady_clock::now() < ui.toastUntil) {
        mvprintw(4, 2, "%s", ui.toast.c_str());
    } else {
        ui.toast.clear();
    }
}

// ---------------- Main ----------------
static void sigint_handler(int) { gKeepUI.store(false); }

int main() {
    // Initial routing config
    std::shared_ptr<Routing> r(new Routing());
    r->inChans = 2;
    r->outChans = 2;
    r->routes.assign(static_cast<size_t>(r->inChans * r->outChans), 0);
    r->gains .assign(static_cast<size_t>(r->inChans * r->outChans), 1.0f); // 0 dB
    std::atomic_store_explicit(&gRouting, std::shared_ptr<const Routing>(r), std::memory_order_release);

    // Devices
    DeviceList devices = enumerateDevices();
    if (!devices.inputIds.empty())  gInDevice  = devices.inputIds[0];
    if (!devices.outputIds.empty()) gOutDevice = devices.outputIds[0];

    // ncurses init
    initscr();
    raw();
    noecho();
    keypad(stdscr, true);
    nodelay(stdscr, true);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, 0);
    curs_set(0);

    signal(SIGINT, sigint_handler);

    UIState ui;
    bool redraw = true;

    while (gKeepUI.load()) {
        if (redraw) {
            erase();
            auto snap = std::atomic_load_explicit(&gRouting, std::memory_order_acquire);
            drawHeader(*snap, devices, ui);
            drawMatrix(*snap, ui);
            drawButtons(devices, ui);

            if (ui.showInDropdown) {
                mvprintw(12, 60, "Select Input Device:");
                drawDropdown(devices.inputNames, devices.inputMaxCh, 13, 60, ui.inDropIndex);
            } else if (ui.showOutDropdown) {
                mvprintw(12, 60, "Select Output Device:");
                drawDropdown(devices.outputNames, devices.outputMaxCh, 13, 60, ui.outDropIndex);
            }
            maybeShowToast(ui);
            refresh();
            redraw = false;
        }

        int ch = getch();
        if (ch == ERR) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        auto snap = std::atomic_load(&gRouting);
        std::shared_ptr<Routing> working(new Routing(*snap));

        switch (ch) {
            case 'q':
                gKeepUI.store(false);
                break;

            case 's':
                if (gRunningEngine.load()) stopEngine();
                else startEngine(devices);
                redraw = true;
                break;

            case 'd':
                if (!ui.showInDropdown && !ui.showOutDropdown) ui.showInDropdown = true;
                else if (ui.showInDropdown) { ui.showInDropdown = false; ui.showOutDropdown = true; }
                else ui.showOutDropdown = false;
                redraw = true;
                break;

            // ------- Dynamic channel changes (only when stopped) -------
            case 'I': { // +input
                if (gRunningEngine.load()) { toast(ui, "Stop engine to change channel counts."); break; }
                int iMax = maxInForDevice(devices, gInDevice);
                if (iMax <= 0) { toast(ui, "Selected input device has no inputs."); break; }
                int newIn = std::min(working->inChans + 1, iMax);
                if (newIn != working->inChans) { resizeRouting(newIn, working->outChans); redraw = true; }
                break;
            }
            case 'i': { // -input
                if (gRunningEngine.load()) { toast(ui, "Stop engine to change channel counts."); break; }
                int newIn = std::max(1, working->inChans - 1);
                if (newIn != working->inChans) { resizeRouting(newIn, working->outChans); redraw = true; }
                break;
            }
            case 'O': { // +output
                if (gRunningEngine.load()) { toast(ui, "Stop engine to change channel counts."); break; }
                int oMax = maxOutForDevice(devices, gOutDevice);
                if (oMax <= 0) { toast(ui, "Selected output device has no outputs."); break; }
                int newOut = std::min(working->outChans + 1, oMax);
                if (newOut != working->outChans) { resizeRouting(working->inChans, newOut); redraw = true; }
                break;
            }
            case 'o': { // -output
                if (gRunningEngine.load()) { toast(ui, "Stop engine to change channel counts."); break; }
                int newOut = std::max(1, working->outChans - 1);
                if (newOut != working->outChans) { resizeRouting(working->inChans, newOut); redraw = true; }
                break;
            }
            // -----------------------------------------------------------

            // ------- Per-route gain controls (selected cell) -----------
            case '-': // -1 dB
                stepRouteGainDb(working, ui.cursorOut, ui.cursorIn, -GAIN_STEP_DB);
                std::atomic_store_explicit(&gRouting, std::shared_ptr<const Routing>(working), std::memory_order_release);
                redraw = true;
                break;
            case '+': // +1 dB
            case '=':
                stepRouteGainDb(working, ui.cursorOut, ui.cursorIn, +GAIN_STEP_DB);
                std::atomic_store_explicit(&gRouting, std::shared_ptr<const Routing>(working), std::memory_order_release);
                redraw = true;
                break;
            case '[': // -6 dB
                stepRouteGainDb(working, ui.cursorOut, ui.cursorIn, -GAIN_STEP2_DB);
                std::atomic_store_explicit(&gRouting, std::shared_ptr<const Routing>(working), std::memory_order_release);
                redraw = true;
                break;
            case ']': // +6 dB
                stepRouteGainDb(working, ui.cursorOut, ui.cursorIn, +GAIN_STEP2_DB);
                std::atomic_store_explicit(&gRouting, std::shared_ptr<const Routing>(working), std::memory_order_release);
                redraw = true;
                break;
            case '0': // reset gain
                resetRouteGain(working, ui.cursorOut, ui.cursorIn);
                std::atomic_store_explicit(&gRouting, std::shared_ptr<const Routing>(working), std::memory_order_release);
                redraw = true;
                break;
            // -----------------------------------------------------------

            case KEY_UP:
                if (ui.showInDropdown && ui.inDropIndex > 0) { ui.inDropIndex--; redraw = true; }
                else if (ui.showOutDropdown && ui.outDropIndex > 0) { ui.outDropIndex--; redraw = true; }
                else if (!ui.showInDropdown && !ui.showOutDropdown && ui.cursorOut > 0) { ui.cursorOut--; redraw = true; }
                break;

            case KEY_DOWN:
                if (ui.showInDropdown && ui.inDropIndex + 1 < (int)devices.inputNames.size()) { ui.inDropIndex++; redraw = true; }
                else if (ui.showOutDropdown && ui.outDropIndex + 1 < (int)devices.outputNames.size()) { ui.outDropIndex++; redraw = true; }
                else if (!ui.showInDropdown && !ui.showOutDropdown && ui.cursorOut + 1 < working->outChans) { ui.cursorOut++; redraw = true; }
                break;

            case KEY_LEFT:
                if (!ui.showInDropdown && !ui.showOutDropdown && ui.cursorIn > 0) { ui.cursorIn--; redraw = true; }
                break;

            case KEY_RIGHT:
                if (!ui.showInDropdown && !ui.showOutDropdown && ui.cursorIn + 1 < working->inChans) { ui.cursorIn++; redraw = true; }
                break;

            case ' ':
            case '\n':
            case KEY_ENTER:
                if (ui.showInDropdown && !devices.inputIds.empty()) {
                    gInDevice = devices.inputIds[ui.inDropIndex];
                    ui.showInDropdown = false;
                    redraw = true;
                } else if (ui.showOutDropdown && !devices.outputIds.empty()) {
                    gOutDevice = devices.outputIds[ui.outDropIndex];
                    ui.showOutDropdown = false;
                    redraw = true;
                } else {
                    // toggle route
                    toggleRoute(working, ui.cursorOut, ui.cursorIn);
                    std::atomic_store_explicit(&gRouting, std::shared_ptr<const Routing>(working), std::memory_order_release);
                    redraw = true;
                }
                break;

            case KEY_MOUSE: {
                MEVENT me;
                if (getmouse(&me) == OK) {
                    // Start/Stop button area (row 3, cols ~60..68)
                    if (me.y == 3 && me.x >= 60 && me.x <= 68) {
                        if (gRunningEngine.load()) stopEngine();
                        else startEngine(devices);
                        redraw = true;
                        break;
                    }
                    // Click on device labels to open dropdowns
                    if (me.y == 5 && me.x >= 67 && me.x <= 90) {
                        ui.showInDropdown = !ui.showInDropdown; ui.showOutDropdown = false; redraw = true; break;
                    }
                    if (me.y == 6 && me.x >= 67 && me.x <= 90) {
                        ui.showOutDropdown = !ui.showOutDropdown; ui.showInDropdown = false; redraw = true; break;
                    }
                    int o=-1, i=-1;
                    if (insideCell(ui, *working, me.y, me.x, o, i)) {
                        toggleRoute(working, o, i);
                        std::atomic_store_explicit(&gRouting, std::shared_ptr<const Routing>(working), std::memory_order_release);
                        ui.cursorOut = o; ui.cursorIn = i;
                        redraw = true;
                    }
                }
                break;
            }

            default: break;
        }
    }

    stopEngine();
    endwin();
    return 0;
}


// // monitor_tui.cpp (C++17)
// // Dynamic in/out channel counts (when engine stopped) + separate input/output device dropdowns.
// // Build with your script: ./build monitor_tui.cpp

// #include <atomic>
// #include <csignal>
// #include <cstdint>
// #include <cstdlib>
// #include <memory>
// #include <string>
// #include <vector>
// #include <algorithm>
// #include <chrono>
// #include <thread>

// #include <portaudio.h>
// #include <ncurses.h>

// #include "../src/audio.h"

// using namespace soundmath;

// static constexpr int BSIZE = 64;

// // ---------------- Routing state (RT-safe snapshot via shared_ptr) ----------------
// struct Routing {
//     int inChans = 2;
//     int outChans = 2;
//     // routes[out * inChans + in] ∈ {0,1}
//     std::vector<uint8_t> routes;
// };

// // atomic ops for shared_ptr (libc++ C++17-friendly)
// static std::shared_ptr<const Routing> gRouting;

// static std::atomic<bool> gRunningEngine{false};
// static std::atomic<bool> gKeepUI{true};
// static int gInDevice = paNoDevice;
// static int gOutDevice = paNoDevice;

// // ---------------- Device enumeration (PortAudio) ----------------
// struct DeviceList {
//     std::vector<int> inputIds;
//     std::vector<int> outputIds;
//     std::vector<std::string> inputNames;
//     std::vector<std::string> outputNames;
//     std::vector<int> inputMaxCh;   // parallel to inputIds
//     std::vector<int> outputMaxCh;  // parallel to outputIds
// };

// static DeviceList enumerateDevices() {
//     DeviceList dl;
//     if (Pa_Initialize() != paNoError) return dl;

//     int count = Pa_GetDeviceCount();
//     for (int i = 0; i < count; ++i) {
//         const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
//         if (!info) continue;
//         std::string name = info->name ? info->name : "unknown";
//         if (info->maxInputChannels > 0) {
//             dl.inputIds.push_back(i);
//             dl.inputNames.push_back(name);
//             dl.inputMaxCh.push_back(info->maxInputChannels);
//         }
//         if (info->maxOutputChannels > 0) {
//             dl.outputIds.push_back(i);
//             dl.outputNames.push_back(name);
//             dl.outputMaxCh.push_back(info->maxOutputChannels);
//         }
//     }
//     Pa_Terminate();
//     return dl;
// }

// static int findIndex(const std::vector<int>& ids, int id) {
//     auto it = std::find(ids.begin(), ids.end(), id);
//     return it == ids.end() ? -1 : static_cast<int>(it - ids.begin());
// }

// static int maxInForDevice(const DeviceList& dl, int devId) {
//     int idx = findIndex(dl.inputIds, devId);
//     return idx >= 0 ? dl.inputMaxCh[idx] : 0;
// }
// static int maxOutForDevice(const DeviceList& dl, int devId) {
//     int idx = findIndex(dl.outputIds, devId);
//     return idx >= 0 ? dl.outputMaxCh[idx] : 0;
// }

// // ---------------- Audio callback ----------------
// inline int process_cb(const float* in, float* out) {
//     auto r = std::atomic_load_explicit(&gRouting, std::memory_order_acquire);
//     if (!r) return 0;

//     const int inC = r->inChans;
//     const int outC = r->outChans;

//     // zero outputs
//     for (int i = 0; i < BSIZE * outC; ++i) out[i] = 0.0f;

//     for (int n = 0; n < BSIZE; ++n) {
//         const float* inF  = in  + n * inC;
//         float*       outF = out + n * outC;
//         for (int o = 0; o < outC; ++o) {
//             float acc = 0.f;
//             const uint8_t* row = &r->routes[o * inC];
//             for (int i = 0; i < inC; ++i)
//                 if (row[i]) acc += inF[i];
//             outF[o] = acc; // consider per-route gain if summing many inputs
//         }
//     }
//     return 0;
// }

// static Audio A(process_cb, BSIZE);

// // ---------------- Engine control ----------------
// static bool canStartWithCurrentConfig(const DeviceList& dl, const Routing& r,
//                                       int inDev, int outDev, std::string& err) {
//     int iMax = maxInForDevice(dl, inDev);
//     int oMax = maxOutForDevice(dl, outDev);
//     if (iMax == 0) { err = "Selected input device has 0 input channels."; return false; }
//     if (oMax == 0) { err = "Selected output device has 0 output channels."; return false; }
//     if (r.inChans > iMax) {
//         err = "Requested input channels (" + std::to_string(r.inChans) +
//               ") exceed device max (" + std::to_string(iMax) + ").";
//         return false;
//     }
//     if (r.outChans > oMax) {
//         err = "Requested output channels (" + std::to_string(r.outChans) +
//               ") exceed device max (" + std::to_string(oMax) + ").";
//         return false;
//     }
//     return true;
// }

// static bool startEngine(const DeviceList& dl) {
//     if (gRunningEngine.load()) return true;
//     auto r = std::atomic_load(&gRouting);
//     if (!r) return false;

//     std::string err;
//     if (!canStartWithCurrentConfig(dl, *r, gInDevice, gOutDevice, err)) {
//         mvprintw(2, 2, "Cannot start: %s", err.c_str());
//         return false;
//     }
//     try {
//         A.startup(r->inChans, r->outChans, false, gInDevice, gOutDevice);
//         gRunningEngine.store(true);
//         return true;
//     } catch (...) {
//         gRunningEngine.store(false);
//         mvprintw(2, 2, "Failed to start engine (device busy/incompatible).");
//         return false;
//     }
// }

// static void stopEngine() {
//     if (!gRunningEngine.load()) return;
//     A.shutdown();
//     gRunningEngine.store(false);
// }

// // ---------------- UI ----------------
// struct UIState {
//     int matrixTop = 4;
//     int matrixLeft = 2;
//     int cellW = 3;
//     int cellH = 1;
//     int cursorIn = 0;
//     int cursorOut = 0;
//     bool showInDropdown = false;
//     bool showOutDropdown = false;
//     int inDropIndex = 0;
//     int outDropIndex = 0;
//     std::string toast;
//     std::chrono::steady_clock::time_point toastUntil{};
// };

// static void toast(UIState& ui, const std::string& s, int ms = 1800) {
//     ui.toast = s;
//     ui.toastUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
// }

// static void drawHeader(const Routing& r, const DeviceList& dl) {
//     mvprintw(0, 2, "Audio Monitor  (q: quit)  (s: start/stop)  (d: devices)  "
//                     "I/i: +/- inputs  O/o: +/- outputs");
//     int iMax = maxInForDevice(dl, gInDevice);
//     int oMax = maxOutForDevice(dl, gOutDevice);
//     mvprintw(1, 2, "Inputs=%d (max %d)   Outputs=%d (max %d)   Engine: %s",
//              r.inChans, iMax, r.outChans, oMax, gRunningEngine.load() ? "RUNNING" : "STOPPED");
// }

// static void drawMatrix(const Routing& r, const UIState& ui) {
//     int top = ui.matrixTop;
//     int left = ui.matrixLeft;
//     mvprintw(top, left + 6, "Routing Matrix (rows=outputs, cols=inputs)");

//     for (int i = 0; i < r.inChans; ++i)
//         mvprintw(top + 2, left + 6 + i * (ui.cellW + 1), "%2d", i);

//     for (int o = 0; o < r.outChans; ++o) {
//         mvprintw(top + 3 + o * (ui.cellH + 1), left, "out %2d →", o);
//         for (int i = 0; i < r.inChans; ++i) {
//             int y = top + 3 + o * (ui.cellH + 1);
//             int x = left + 6 + i * (ui.cellW + 1);
//             bool on = r.routes[o * r.inChans + i] != 0;
//             bool cursorHere = (o == ui.cursorOut && i == ui.cursorIn);
//             if (cursorHere) attron(A_REVERSE);
//             mvprintw(y, x, "[%c]", on ? 'X' : ' ');
//             if (cursorHere) attroff(A_REVERSE);
//         }
//     }
// }

// static void drawButtons(const DeviceList& dl, const UIState& ui) {
//     int row = 2, col = 60;

//     attron(A_BOLD);
//     mvprintw(row, col, "%s", gRunningEngine.load() ? "[ Stop ]" : "[ Start ]");
//     attroff(A_BOLD);

//     auto findName = [&](bool input) -> std::string {
//         const auto& ids   = input ? dl.inputIds   : dl.outputIds;
//         const auto& names = input ? dl.inputNames : dl.outputNames;
//         int sel = input ? gInDevice : gOutDevice;
//         if (sel == paNoDevice) return "(none)";
//         int idx = findIndex(ids, sel);
//         if (idx < 0) return "(unknown)";
//         return names[static_cast<size_t>(idx)];
//     };
//     auto findMax = [&](bool input) -> int {
//         return input ? maxInForDevice(dl, gInDevice) : maxOutForDevice(dl, gOutDevice);
//     };

//     mvprintw(row+2, col, "Input: ");
//     if (ui.showInDropdown) attron(A_REVERSE);
//     mvprintw(row+2, col + 7, "%s  [max %d]", findName(true).c_str(), findMax(true));
//     if (ui.showInDropdown) attroff(A_REVERSE);

//     mvprintw(row+3, col, "Output:");
//     if (ui.showOutDropdown) attron(A_REVERSE);
//     mvprintw(row+3, col + 7, "%s  [max %d]", findName(false).c_str(), findMax(false));
//     if (ui.showOutDropdown) attroff(A_REVERSE);
// }

// static void drawDropdown(const std::vector<std::string>& names,
//                          const std::vector<int>& maxCh,
//                          int startRow, int startCol, int highlightIndex) {
//     int maxVisible = static_cast<int>(names.size());
//     if (maxVisible > 12) maxVisible = 12;
//     for (int i = 0; i < maxVisible; ++i) {
//         if (i == highlightIndex) attron(A_REVERSE);
//         mvprintw(startRow + i, startCol, "%s  [max %d]", names[i].c_str(), maxCh[i]);
//         if (i == highlightIndex) attroff(A_REVERSE);
//     }
// }

// static bool insideCell(const UIState& ui, const Routing& r,
//                        int my, int mx, int& outRow, int& inCol) {
//     int top = ui.matrixTop + 3;
//     int left = ui.matrixLeft + 6;
//     for (int o = 0; o < r.outChans; ++o) {
//         for (int i = 0; i < r.inChans; ++i) {
//             int y = top + o * (ui.cellH + 1);
//             int x = left + i * (ui.cellW + 1);
//             if (my == y && mx >= x && mx <= x + 2) {
//                 outRow = o; inCol = i; return true;
//             }
//         }
//     }
//     return false;
// }

// static void toggleRoute(std::shared_ptr<Routing>& wip, int o, int i) {
//     if (!wip) return;
//     size_t idx = static_cast<size_t>(o) * static_cast<size_t>(wip->inChans) + static_cast<size_t>(i);
//     wip->routes[idx] = wip->routes[idx] ? 0 : 1;
// }

// // Resize routing (engine must be stopped). Preserves overlap of old routes.
// static void resizeRouting(int newIn, int newOut) {
//     auto snap = std::atomic_load(&gRouting);
//     std::shared_ptr<Routing> rnew(new Routing(*snap));
//     rnew->inChans = newIn;
//     rnew->outChans = newOut;
//     rnew->routes.assign(static_cast<size_t>(newIn * newOut), 0);
//     int copyIn = std::min(newIn, snap->inChans);
//     int copyOut = std::min(newOut, snap->outChans);
//     for (int o = 0; o < copyOut; ++o) {
//         for (int i = 0; i < copyIn; ++i) {
//             rnew->routes[o * newIn + i] = snap->routes[o * snap->inChans + i];
//         }
//     }
//     std::atomic_store_explicit(&gRouting, std::shared_ptr<const Routing>(rnew), std::memory_order_release);
// }

// static void maybeShowToast(UIState& ui) {
//     if (!ui.toast.empty() && std::chrono::steady_clock::now() < ui.toastUntil) {
//         mvprintw(3, 2, "%s", ui.toast.c_str());
//     } else {
//         ui.toast.clear();
//     }
// }

// // ---------------- Main ----------------
// static void sigint_handler(int) { gKeepUI.store(false); }

// int main() {
//     // Initial routing config
//     std::shared_ptr<Routing> r(new Routing());
//     r->inChans = 2;
//     r->outChans = 2;
//     r->routes.assign(static_cast<size_t>(r->inChans * r->outChans), 0);
//     std::atomic_store_explicit(&gRouting, std::shared_ptr<const Routing>(r), std::memory_order_release);

//     // Devices
//     DeviceList devices = enumerateDevices();
//     if (!devices.inputIds.empty())  gInDevice  = devices.inputIds[0];
//     if (!devices.outputIds.empty()) gOutDevice = devices.outputIds[0];

//     // ncurses init
//     initscr();
//     raw();
//     noecho();
//     keypad(stdscr, true);
//     nodelay(stdscr, true);
//     mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, 0);
//     curs_set(0);

//     signal(SIGINT, sigint_handler);

//     UIState ui;
//     bool redraw = true;

//     while (gKeepUI.load()) {
//         if (redraw) {
//             erase();
//             auto snap = std::atomic_load_explicit(&gRouting, std::memory_order_acquire);
//             drawHeader(*snap, devices);
//             drawMatrix(*snap, ui);
//             drawButtons(devices, ui);

//             if (ui.showInDropdown) {
//                 mvprintw(10, 60, "Select Input Device:");
//                 drawDropdown(devices.inputNames, devices.inputMaxCh, 11, 60, ui.inDropIndex);
//             } else if (ui.showOutDropdown) {
//                 mvprintw(10, 60, "Select Output Device:");
//                 drawDropdown(devices.outputNames, devices.outputMaxCh, 11, 60, ui.outDropIndex);
//             }
//             maybeShowToast(ui);
//             refresh();
//             redraw = false;
//         }

//         int ch = getch();
//         if (ch == ERR) {
//             std::this_thread::sleep_for(std::chrono::milliseconds(10));
//             continue;
//         }

//         auto snap = std::atomic_load(&gRouting);
//         std::shared_ptr<Routing> working(new Routing(*snap));

//         switch (ch) {
//             case 'q':
//                 gKeepUI.store(false);
//                 break;

//             case 's':
//                 if (gRunningEngine.load()) stopEngine();
//                 else startEngine(devices);
//                 redraw = true;
//                 break;

//             case 'd':
//                 if (!ui.showInDropdown && !ui.showOutDropdown) ui.showInDropdown = true;
//                 else if (ui.showInDropdown) { ui.showInDropdown = false; ui.showOutDropdown = true; }
//                 else ui.showOutDropdown = false;
//                 redraw = true;
//                 break;

//             // ------- Dynamic channel changes (only when stopped) -------
//             case 'I': { // +input
//                 if (gRunningEngine.load()) { toast(ui, "Stop engine to change channel counts."); break; }
//                 int iMax = maxInForDevice(devices, gInDevice);
//                 if (iMax <= 0) { toast(ui, "Selected input device has no inputs."); break; }
//                 int newIn = std::min(working->inChans + 1, iMax);
//                 if (newIn != working->inChans) { resizeRouting(newIn, working->outChans); redraw = true; }
//                 break;
//             }
//             case 'i': { // -input
//                 if (gRunningEngine.load()) { toast(ui, "Stop engine to change channel counts."); break; }
//                 int newIn = std::max(1, working->inChans - 1);
//                 if (newIn != working->inChans) { resizeRouting(newIn, working->outChans); redraw = true; }
//                 break;
//             }
//             case 'O': { // +output
//                 if (gRunningEngine.load()) { toast(ui, "Stop engine to change channel counts."); break; }
//                 int oMax = maxOutForDevice(devices, gOutDevice);
//                 if (oMax <= 0) { toast(ui, "Selected output device has no outputs."); break; }
//                 int newOut = std::min(working->outChans + 1, oMax);
//                 if (newOut != working->outChans) { resizeRouting(working->inChans, newOut); redraw = true; }
//                 break;
//             }
//             case 'o': { // -output
//                 if (gRunningEngine.load()) { toast(ui, "Stop engine to change channel counts."); break; }
//                 int newOut = std::max(1, working->outChans - 1);
//                 if (newOut != working->outChans) { resizeRouting(working->inChans, newOut); redraw = true; }
//                 break;
//             }
//             // -----------------------------------------------------------

//             case KEY_UP:
//                 if (ui.showInDropdown && ui.inDropIndex > 0) { ui.inDropIndex--; redraw = true; }
//                 else if (ui.showOutDropdown && ui.outDropIndex > 0) { ui.outDropIndex--; redraw = true; }
//                 else if (!ui.showInDropdown && !ui.showOutDropdown && ui.cursorOut > 0) { ui.cursorOut--; redraw = true; }
//                 break;

//             case KEY_DOWN:
//                 if (ui.showInDropdown && ui.inDropIndex + 1 < (int)devices.inputNames.size()) { ui.inDropIndex++; redraw = true; }
//                 else if (ui.showOutDropdown && ui.outDropIndex + 1 < (int)devices.outputNames.size()) { ui.outDropIndex++; redraw = true; }
//                 else if (!ui.showInDropdown && !ui.showOutDropdown && ui.cursorOut + 1 < working->outChans) { ui.cursorOut++; redraw = true; }
//                 break;

//             case KEY_LEFT:
//                 if (!ui.showInDropdown && !ui.showOutDropdown && ui.cursorIn > 0) { ui.cursorIn--; redraw = true; }
//                 break;

//             case KEY_RIGHT:
//                 if (!ui.showInDropdown && !ui.showOutDropdown && ui.cursorIn + 1 < working->inChans) { ui.cursorIn++; redraw = true; }
//                 break;

//             case ' ':
//             case '\n':
//             case KEY_ENTER:
//                 if (ui.showInDropdown && !devices.inputIds.empty()) {
//                     gInDevice = devices.inputIds[ui.inDropIndex];
//                     ui.showInDropdown = false;
//                     // if running, change applies on next start; if stopped, counts still user-controlled
//                     redraw = true;
//                 } else if (ui.showOutDropdown && !devices.outputIds.empty()) {
//                     gOutDevice = devices.outputIds[ui.outDropIndex];
//                     ui.showOutDropdown = false;
//                     redraw = true;
//                 } else {
//                     // toggle route only; allowed while running (purely mixing)
//                     toggleRoute(working, ui.cursorOut, ui.cursorIn);
//                     std::atomic_store_explicit(&gRouting, std::shared_ptr<const Routing>(working), std::memory_order_release);
//                     redraw = true;
//                 }
//                 break;

//             case KEY_MOUSE: {
//                 MEVENT me;
//                 if (getmouse(&me) == OK) {
//                     // Start/Stop button area (row 2, cols ~60..68)
//                     if (me.y == 2 && me.x >= 60 && me.x <= 68) {
//                         if (gRunningEngine.load()) stopEngine();
//                         else startEngine(devices);
//                         redraw = true;
//                         break;
//                     }
//                     // Click on device labels to open dropdowns
//                     if (me.y == 4 && me.x >= 67 && me.x <= 90) {
//                         ui.showInDropdown = !ui.showInDropdown; ui.showOutDropdown = false; redraw = true; break;
//                     }
//                     if (me.y == 5 && me.x >= 67 && me.x <= 90) {
//                         ui.showOutDropdown = !ui.showOutDropdown; ui.showInDropdown = false; redraw = true; break;
//                     }
//                     int o=-1, i=-1;
//                     if (insideCell(ui, *working, me.y, me.x, o, i)) {
//                         toggleRoute(working, o, i);
//                         std::atomic_store_explicit(&gRouting, std::shared_ptr<const Routing>(working), std::memory_order_release);
//                         ui.cursorOut = o; ui.cursorIn = i;
//                         redraw = true;
//                     }
//                 }
//                 break;
//             }

//             default: break;
//         }
//     }

//     stopEngine();
//     endwin();
//     return 0;
// }
