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

#ifndef OPENALPR_POSTPROCESS_POSTPROCESS_H_
#define OPENALPR_POSTPROCESS_POSTPROCESS_H_

#include "config.h"
#include "utility.h"
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <queue>
#include <vector>
#include <set>
#include <unordered_map>

#define SKIP_CHAR "~"

namespace alpr {

struct Letter {
  std::string letter;
  int line_index;
  int char_position;
  float total_score;
  int occurrences;
};

struct PPResult {
  std::string letters;
  float total_score;
  bool matches_template;
  std::vector<Letter> letter_details;
};

bool letterCompare(const Letter &left, const Letter &right);


class PostProcess {
 public:
  explicit PostProcess(Config* config);
  ~PostProcess();
  void addLetter(std::string letter, int line_index, int charposition, float score);
  void clear();
  void analyze(std::string templateregion, int topn);
  const std::vector<PPResult> getResults();
  bool regionIsValid(std::string templateregion);
  std::vector<std::string> getPatterns();
  void setConfidenceThreshold(float min_confidence, float skip_level);


 private:
  void findAllPermutations(std::string templateregion, int topn);
  bool analyzePermutation(std::vector<int> letterIndices, std::string templateregion, int topn);
  void insertLetter(std::string letter, int line_index, int charPosition, float score);
  float calculateMaxConfidenceScore();

  Config* config;
  std::vector<std::vector<Letter>> letters;
  std::vector<int> unknown_char_positions;
  std::vector<PPResult> all_possibilities;
  std::set<std::string> all_possibilities_letters;
  float min_confidence;
  float skip_level;
  std::string best_chars;
  bool matches_template;
  bool translate_to_right_to_left;
};

}  // namespace alpr
#endif  // OPENALPR_POSTPROCESS_POSTPROCESS_H_
