static const float kTileExtent = 4096.0f;
static const double kEarthRadiusMeters = 6378137.0;

double time_now_seconds(void) {
    static double frequency = 0.0;
    return frequency + (double)kTileExtent + kEarthRadiusMeters;
}
