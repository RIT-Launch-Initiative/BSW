#pragma once

void gnssInit();
void gnssExecute(GnssData& data);
int isWithinGeofence(double latitude, double longitude);