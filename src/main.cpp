// SmoothDash - high FPS + less stutter for Geometry Dash (Geode, Windows)
//
// What it does:
//   1. FPS unlock      - overrides GD's frame interval with your target FPS.
//   2. 1 ms timer      - timeBeginPeriod(1) so the frame limiter's sleeps are
//                        accurate (default Windows timer is ~15.6 ms, which
//                        makes frame times jitter = stutter).
//   3. No throttling   - opts GD out of Windows "EcoQoS" power throttling so
//                        it isn't parked on efficiency cores mid-level.
//   4. Priority        - optional ABOVE_NORMAL process priority.
//   5. Particle cap    - limits particles per emitter on heavy levels.
//   6. Low detail      - forces each level's Low Detail Mode on (if it has one).
//   7. No shake        - optionally turns off screen shake.
//
// Defaults are tuned for a Ryzen 5 7520U + Radeon 610M (4-core Zen 2,
// 2-CU integrated GPU sharing system RAM): GPU-bound, so we cut draw work.

#include <Geode/Geode.hpp>
#include <Geode/modify/CCApplication.hpp>
#include <Geode/modify/CCParticleSystem.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>

#ifdef GEODE_IS_WINDOWS
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <Windows.h>
    #include <timeapi.h>
#endif

using namespace geode::prelude;

namespace {

template <class T>
T setting(char const* key) {
    return Mod::get()->getSettingValue<T>(key);
}

double targetInterval() {
    auto fps = setting<int64_t>("target-fps");
    // 0 = uncapped; 1/10000 s is effectively no limit.
    if (fps <= 0) return 1.0 / 10000.0;
    return 1.0 / static_cast<double>(fps);
}

// Re-apply the frame interval. Our CCApplication hook below swaps in the
// target value when FPS unlock is on; when it's off we fall back to 60.
void refreshFrameInterval() {
    CCDirector::get()->setAnimationInterval(1.0 / 60.0);
}

#ifdef GEODE_IS_WINDOWS

bool g_timerRaised = false;

void applyTimerResolution(bool enable) {
    if (enable && !g_timerRaised) {
        g_timerRaised = timeBeginPeriod(1) == TIMERR_NOERROR;
    }
    else if (!enable && g_timerRaised) {
        timeEndPeriod(1);
        g_timerRaised = false;
    }
}

void applyPowerThrottling(bool disableThrottling) {
#ifdef PROCESS_POWER_THROTTLING_CURRENT_VERSION
    PROCESS_POWER_THROTTLING_STATE state{};
    state.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    if (disableThrottling) {
        // ControlMask set + StateMask cleared = "never throttle me".
        state.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
        state.StateMask = 0;
    }
    else {
        // Both cleared = let Windows decide (default behaviour).
        state.ControlMask = 0;
        state.StateMask = 0;
    }
    SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &state, sizeof(state));
#endif
}

void applyPriority(bool high) {
    SetPriorityClass(GetCurrentProcess(), high ? ABOVE_NORMAL_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
}

#endif

} // namespace

// ---------------------------------------------------------------------------
// 1. FPS unlock
// Every time GD (or cocos) sets the frame interval, replace it with ours.
// ---------------------------------------------------------------------------
class $modify(SDApplication, CCApplication) {
    void setAnimationInterval(double interval) {
        if (setting<bool>("fps-unlock")) {
            interval = targetInterval();
        }
        CCApplication::setAnimationInterval(interval);
    }
};

// ---------------------------------------------------------------------------
// 5. Particle cap
// Lowering m_uTotalParticles below what was allocated is always safe in
// cocos2d-x 2.x (it never reallocates downward). We also scale the emission
// rate so emitters don't instantly hit the cap and look "choppy".
// The check is a couple of integer compares, so running it per-update is free.
// ---------------------------------------------------------------------------
class $modify(SDParticles, CCParticleSystem) {
    void update(float dt) {
        static int64_t s_percent = 100;
        static int s_frame = 0;
        // Refresh the cached setting occasionally instead of every call.
        if ((s_frame++ & 255) == 0) {
            s_percent = setting<int64_t>("particle-percent");
        }

        if (s_percent < 100 && m_uAllocatedParticles > 1) {
            auto cap = std::max<unsigned int>(
                1u, static_cast<unsigned int>(m_uAllocatedParticles * s_percent / 100)
            );
            if (m_uTotalParticles > cap) {
                float scale = static_cast<float>(cap) / static_cast<float>(m_uTotalParticles);
                m_fEmissionRate *= scale;
                m_uTotalParticles = cap;
            }
        }

        CCParticleSystem::update(dt);
    }
};

// ---------------------------------------------------------------------------
// 6 + 7. Low Detail Mode / no shake
// GJGameLevel::m_lowDetailMode = the level offers LDM.
// m_lowDetailModeToggled / m_disableShakeToggled = the player's per-level
// choice, which PlayLayer reads while building the level, so set them first.
// ---------------------------------------------------------------------------
class $modify(SDPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (level) {
            if (setting<bool>("force-low-detail") && level->m_lowDetailMode) {
                level->m_lowDetailModeToggled = true;
            }
            if (setting<bool>("disable-shake")) {
                level->m_disableShakeToggled = true;
            }
        }
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }
};

// ---------------------------------------------------------------------------
// Startup + live setting changes
// ---------------------------------------------------------------------------
$on_mod(Loaded) {
#ifdef GEODE_IS_WINDOWS
    applyTimerResolution(setting<bool>("timer-resolution"));
    applyPowerThrottling(setting<bool>("no-power-throttling"));
    applyPriority(setting<bool>("high-priority"));

    listenForSettingChanges<bool>("timer-resolution", [](bool v) { applyTimerResolution(v); });
    listenForSettingChanges<bool>("no-power-throttling", [](bool v) { applyPowerThrottling(v); });
    listenForSettingChanges<bool>("high-priority", [](bool v) { applyPriority(v); });
#endif

    // The director may not be fully set up yet at load time, so apply the
    // FPS cap on the next frame of the main loop.
    Loader::get()->queueInMainThread([] { refreshFrameInterval(); });

    listenForSettingChanges<bool>("fps-unlock", [](bool) { refreshFrameInterval(); });
    listenForSettingChanges<int64_t>("target-fps", [](int64_t) { refreshFrameInterval(); });
}
