/*
 * Copyright (c) 2015 OpenALPR Technology, Inc.
 * Open source Automated License Plate Recognition [http://www.openalpr.com]
 *
 * This file is part of OpenALPR.
 *
 * OpenALPR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License
 * version 3 as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <opencv2/core/core.hpp>
#include <functional>
#include <cctype>

#include "utility.h"

using cv::Rect;
using cv::Point;
using std::min;
using std::max;

namespace alpr {

Rect expandRect(Rect original, int expandXPixels, int expandYPixels, int maxX, int maxY) {
  Rect expandedRegion = Rect(original);
  float halfX = round(static_cast<float>(expandXPixels) / 2.0);
  float halfY = round(static_cast<float>(expandYPixels) / 2.0);
  expandedRegion.x = expandedRegion.x - halfX;
  expandedRegion.width = expandedRegion.width + expandXPixels;
  expandedRegion.y = expandedRegion.y - halfY;
  expandedRegion.height = expandedRegion.height + expandYPixels;
  expandedRegion.x = std::min(std::max(expandedRegion.x, 0), maxX);
  expandedRegion.y = std::min(std::max(expandedRegion.y, 0), maxY);
  if (expandedRegion.x + expandedRegion.width > maxX) {
    expandedRegion.width = maxX - expandedRegion.x;
  }
  if (expandedRegion.y + expandedRegion.height > maxY) {
    expandedRegion.height = maxY - expandedRegion.y;
  }
  return expandedRegion;
}

double distanceBetweenPoints(Point p1, Point p2) {
  float asquared = (p2.x - p1.x)* (p2.x - p1.x);
  float bsquared = (p2.y - p1.y)*(p2.y - p1.y);

  return sqrt(asquared + bsquared);
}
}