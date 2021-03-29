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

#ifndef OPENALPR_POSTPROCESS_REGEXRULE_H_
#define OPENALPR_POSTPROCESS_REGEXRULE_H_

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <memory>
#include <alprsupport/re2.h>
#include <alprsupport/utf8.h>

namespace alpr {
class RegexRule {
 public:
  RegexRule(std::string region, std::string pattern, std::string letters_regex, std::string numbers_regex);
  virtual ~RegexRule();
  bool match(std::string text);

 private:
  bool valid;

  int numchars;
  std::unique_ptr<re2::RE2> re2_regex;
  std::string original;
  std::string regex;
  std::string region;
};
}  // namespace alpr
#endif  // OPENALPR_POSTPROCESS_REGEXRULE_H_
