#include "clouds/wind_system.h"

namespace Clouds {

static GlobalWind g_wind;

GlobalWind& getGlobalWind() {
    return g_wind;
}

void updateWind(float /*dt*/) {
    // Ветер может медленно меняться — пока константный
}

} // namespace Clouds
