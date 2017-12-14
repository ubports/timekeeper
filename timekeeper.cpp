/*
 * Copyright (C) 2017 Marius Gripsgard <marius@ubports.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fstream>
#include <iostream>
#include <cstring>
#include <sys/time.h>

#define RTC_SYS_FILE "/sys/class/rtc/rtc0/since_epoch"
// TODO be able to set offset file using a envar (or argument)
#define OFFSET_FILE "/home/phablet/.cache/timekeeper_offset"

// Remove comment to enable debug
//#define DEBUG
#ifdef DEBUG
#define LOG_DEBUG(...) fprintf(stdout, __VA_ARGS__)
#else
#define LOG_DEBUG(...) do {} while (0)
#endif
#define LOG_ERR(...) fprintf(stderr, __VA_ARGS__)
#define ERR -1

using namespace std;

int _read(string file) {
  int sec = ERR;
  ifstream f(file);
  if (!f)
    return ERR;
  f >> sec;
  if (f.bad())
    return ERR;
  f.close();
  return sec;
}

int _write(string file, int sec) {
  ofstream f(file);
  if (!f)
    return ERR;
  f << sec;
  if (f.bad())
    return ERR;
  f.close();
  return 0;
}

int read_since_epoch() {
  return _read(RTC_SYS_FILE);
}

int write(int sec) {
  return _write(OFFSET_FILE, sec);
}

int read() {
  return _read(OFFSET_FILE);
}

int now() {
  time_t t;
  struct tm tm;

  time(&t);
  localtime_r(&t, &tm);
  return mktime(&tm);
}

int store() {
	int seconds = 0;
	int epoch_since = 0;
  int ret = ERR;

  seconds = now();

	if (seconds > 0) {
		epoch_since = read_since_epoch();
		if (epoch_since < 0) {
			LOG_ERR("Failed to read epoch_since\n");
		} else {
      LOG_DEBUG("Time now is %i\n", seconds);
      LOG_DEBUG("Epoch since is %i\n", epoch_since);
			seconds -= epoch_since;
      LOG_DEBUG("Offset is %i\n", seconds);
			ret = write(seconds);
			if (ret != ERR) {
        LOG_DEBUG("Offset file written successfully\n");
			} else {
				LOG_ERR("Failed to write offset file!\n");
			}
		}
	}

	return ret;
}

int restore() {
	struct timeval tv;
	int time_adjust = 0;
	int epoch_since = 0;
	int ret = ERR;

  time_adjust = read();

	epoch_since = read_since_epoch();
	if (epoch_since == ERR) {
		LOG_ERR("Failed to read epoch_since\n");
	} else {
    LOG_DEBUG("Offset is %i\n", time_adjust);
    LOG_DEBUG("Epoch since is %i\n", epoch_since);
    LOG_DEBUG("Final time is %i\n", epoch_since + time_adjust);
		tv.tv_sec = epoch_since + time_adjust;
		tv.tv_usec = 0;
		ret = settimeofday(&tv, NULL);
		if (ret != 0) {
			LOG_ERR("Failed to set time! are you root?\n");
		} else {
			LOG_DEBUG("Time restored!\n");
		}
	}

	return ret;
}

void usage(string file) {
  cerr << "Usage: " << file << " store|restore" << endl;
}

int main(int argc, char const *argv[]) {
  if (argc < 2) {
    usage(argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "restore") == 0)
    return restore();
  else if (strcmp(argv[1], "store") == 0)
    return store();

  usage(argv[0]);
  return 1;
}
