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

#ifndef OPENALPR_UTILITY_H_
#define OPENALPR_UTILITY_H_

#include <iostream>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include <alprsupport/timing.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
#include <vector>

namespace alpr {
  double distanceBetweenPoints(cv::Point p1, cv::Point p2);
  cv::Rect expandRect(cv::Rect original, int expandXPixels, int expandYPixels, int maxX, int maxY);
}

#endif  // OPENALPR_UTILITY_H_
