/*
  Synopsis of MYOSA event handling
  Validates sensor indices, finite thresholds, and range conditions for BLE events.
  Commands retain the existing field order; actions are b (buzzer) and r (relay).

  Modifications
  11 September, 2026 by Pegasus Automation
  (as a part of MYOSA Initiative)

  Contact Team MYOSA for feedback or issues.
  Email: myosa.event@gmail.com
*/
#ifndef MYOSA_EVENTS_H
#define MYOSA_EVENTS_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

namespace myosa_detail
{
// Sensor services 0..4 retain their fields; service 5 accepts event commands.
static const uint8_t characteristicCounts[6] = {4, 2, 3, 3, 3, 0};
static const uint8_t parameterCounts[6][4] = {{3, 3, 3, 2}, {1, 1, 0, 0}, {2, 3, 1, 0},
                                             {1, 1, 3, 0}, {2, 1, 2, 0}, {0, 0, 0, 0}};
struct Event
{
    bool enabled = false;
    uint8_t service = 0, characteristic = 0, parameter = 0;
    double minimum = 0, maximum = 0;
    bool inside = false, boundary = false;
    char action = 0;
};

// Fields: service, characteristic, parameter, minimum, maximum, inside, boundary, action.
// Validate the complete command before replacing the caller's event.
inline bool parseEvent(const char *text, Event &output)
{
  if(!text || strlen(text) >= 159)
    return false;
  char copy[159];
  strcpy(copy, text);
  char *fields[8] = {copy};
  uint8_t count = 1;
  for(char *p = copy; *p; ++p)
    if(*p == ',')
    {
      if(count == 8)
        return false;
      *p = 0;
      fields[count++] = p + 1;
    }
  if(count != 8)
    return false;
  double numbers[7];
  for(uint8_t i = 0; i < 7; ++i)
  {
    char *end;
    errno = 0;
    numbers[i] = strtod(fields[i], &end);
    if(end == fields[i] || *end || errno == ERANGE || !isfinite(numbers[i]))
      return false;
  }
  if(numbers[0] < 0 || numbers[0] >= 6 || numbers[0] != floor(numbers[0]))
    return false;
  const uint8_t service = (uint8_t)numbers[0];
  if(numbers[1] < 0 || numbers[1] >= characteristicCounts[service] ||
     numbers[1] != floor(numbers[1]))
    return false;
  const uint8_t characteristic = (uint8_t)numbers[1];
  if(numbers[2] < 0 || numbers[2] >= parameterCounts[service][characteristic] ||
     numbers[2] != floor(numbers[2]))
    return false;
  if(numbers[3] > numbers[4])
    return false;
  if((numbers[5] != 0 && numbers[5] != 1) || (numbers[6] != 0 && numbers[6] != 1))
    return false;
  if(strlen(fields[7]) != 1 || (fields[7][0] != 'b' && fields[7][0] != 'r'))
    return false;
  Event next;
  next.enabled = true;
  next.service = service;
  next.characteristic = characteristic;
  next.parameter = (uint8_t)numbers[2];
  next.minimum = numbers[3];
  next.maximum = numbers[4];
  next.inside = numbers[5] != 0;
  next.boundary = numbers[6] != 0;
  next.action = fields[7][0];
  output = next;
  return true;
}

// Evaluate one finite reading against the event; disabled events never trigger.
inline bool matches(const Event &event, double value)
{
  if(!event.enabled || !isfinite(value))
    return false;
  // Preserve the protocol: the second flag includes the boundary when set.
  if(event.inside)
    return event.boundary ? value >= event.minimum && value <= event.maximum
                          : value > event.minimum && value < event.maximum;
  return event.boundary ? value <= event.minimum || value >= event.maximum
                        : value < event.minimum || value > event.maximum;
}
} // namespace myosa_detail
#endif
