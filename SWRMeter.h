//
//  REQUIRED: numeric limits for new SWR hardware sensor
//
const int BufferLength = 24;

// I/O buffer
char ioBuffer[BufferLength];

// define power limits (W); this is the power reading corresponding
//    to a full-scale A/D reading; to report the power as a percentage
//    of full scale, set this to 100.0.
#define FULL_SCALE_FORWARD (20.0)
#define FULL_SCALE_REFLECTED (20.0)

// MINIMUM A/D for SWR - this is the minimum A/D reading that will produce an
//   SWR != 1.0; this helps to prevent unnecessarily high SWR readings at low
//   power levels where there is insufficient A/D resolution to make accurate
//   SWR calculations.  This value is the RAW value used as a minimum FORWARD
//   power limit.  Below this RAW value, the SWR will be reported as 1.0.
#define MIN_POWER (5)

// Maximum Arduino A/D reading; used only to scale the power readings.
const int ARDUINO_AD_MAX = 1023;

// maximum auto-poll interval (msec)
const int MaxAutoPoll = 1000;

// compute scaling constants for power readings
const float FORWARD_SCALE = (float)FULL_SCALE_FORWARD / (float)ARDUINO_AD_MAX;
const float REFLECT_SCALE = (float)FULL_SCALE_REFLECTED / (float)ARDUINO_AD_MAX;
