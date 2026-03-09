#include "wifi_setup.h"

//parsen & Anzeigen
#define MAX_DEPARTURES 14 //Max departures requested from API
#define MAX_LINE_LEN   4 
#define MAX_DEST_LEN   8
struct Departure {
  char line[MAX_LINE_LEN];
  String destination;
  int minutes;
  int delay;
};
extern Departure departures[MAX_DEPARTURES];

void updateDepartures();