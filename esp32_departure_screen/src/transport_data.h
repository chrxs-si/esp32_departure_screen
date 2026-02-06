#include "wifi_setup.h"

//parsen & Anzeigen
#define MAX_DEPARTURES 8 //Max departures requested from API
#define MAX_LINE_LEN   4
#define MAX_DEST_LEN   9
struct Departure {
  char line[MAX_LINE_LEN];
  char destination[MAX_DEST_LEN];
  int minutes;
  int delay;
};
extern Departure departures[MAX_DEPARTURES];

void updateDepartures();