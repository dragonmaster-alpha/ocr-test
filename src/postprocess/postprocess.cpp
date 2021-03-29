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

#include "postprocess.h"
#include <alprsupport/filesystem.h>
#include <alprlog.h>
#include <alprsupport/json.hpp>

using namespace std;

namespace alpr {

PostProcess::PostProcess(Config* config) {
  this->config = config;
  this->min_confidence = 0;
  stringstream filename;
  filename << config->getPostProcessRuntimeDir() << "/" << config->getCountry() << ".patterns";

  std::ifstream infile(filename.str().c_str());
  string region, pattern;
  while (infile >> region >> pattern) {

    // Check to see if Right-to-left language is enabled.  If so, we need to reverse the output for characters/numbers
    std::string ocr_config_path = config->getOCRPrefix() + config->ocrLanguage + "/ocr_config.json";

    if (!alprsupport::fileExists(ocr_config_path.c_str())) {
      ALPR_ERROR << "Unable to find OCR configuration: " << ocr_config_path;
      return;
    }

    // Read runtime constants from JSON
    // Check if right_to_left is enabled to perform the conversion on output
    std::ifstream ifs(ocr_config_path);
    nlohmann::json ocr_config = nlohmann::json::parse(ifs);
    if (ocr_config.find("right_to_left") != ocr_config.end()) {
      translate_to_right_to_left = ocr_config["right_to_left"];
    } else {
      translate_to_right_to_left = false;
    }
  }
}

PostProcess::~PostProcess() {
}

void PostProcess::setConfidenceThreshold(float min_confidence, float skip_level) {
  this->min_confidence = min_confidence;
  this->skip_level = skip_level;
}


void PostProcess::addLetter(string letter, int line_index, int char_position, float score) {
  if (score < min_confidence)
    return;

  insertLetter(letter, line_index, char_position, score);
  if (score < (skip_level)) {
    float adjustedScore = abs(skip_level - score) + min_confidence;
    insertLetter(SKIP_CHAR, line_index, char_position, adjustedScore);
  }
}

void PostProcess::insertLetter(string letter, int line_index, int char_position, float score) {
  score = score - min_confidence;
  int existingIndex = -1;
  if (letters.size() < char_position + 1) {
    for (int i = letters.size(); i < char_position + 1; i++) {
      vector<Letter> tmp;
      letters.push_back(tmp);
    }
  }

  for (int i = 0; i < letters[char_position].size(); i++) {
    if (letters[char_position][i].letter == letter &&
        letters[char_position][i].line_index == line_index &&
        letters[char_position][i].char_position == char_position) {
      existingIndex = i;
      break;
    }
  }

  if (existingIndex == -1) {
    Letter newLetter;
    newLetter.line_index = line_index;
    newLetter.char_position = char_position;
    newLetter.letter = letter;
    newLetter.occurrences = 1;
    newLetter.total_score = score;
    letters[char_position].push_back(newLetter);
  } else {
    letters[char_position][existingIndex].occurrences += 1;
    letters[char_position][existingIndex].total_score += score;
  }
}

void PostProcess::clear() {
  for (int i = 0; i < letters.size(); i++) {
    letters[i].clear();
  }
  letters.resize(0);
  unknown_char_positions.clear();
  unknown_char_positions.resize(0);
  all_possibilities.clear();
  all_possibilities_letters.clear();
  best_chars = "";
  matches_template = false;
}

void PostProcess::analyze(string templateregion, int topn) {
  timespec startTime;
  alprsupport::getTimeMonotonic(&startTime);

  // Get a list of missing positions
  for (int i = letters.size() -1; i >= 0; i--) {
    if (letters[i].size() == 0) {
      unknown_char_positions.push_back(i);
    }
  }

  if (letters.size() == 0)
    return;

  // Sort the letters as they are
  for (int i = 0; i < letters.size(); i++) {
    if (letters[i].size() > 0)
      std::stable_sort(letters[i].begin(), letters[i].end(), letterCompare);
  }

  if (this->config->debugPostProcess) {
    // Print all letters
    for (int i = 0; i < letters.size(); i++) {
      for (int j = 0; j < letters[i].size(); j++)
        cout << "PostProcess Line " << letters[i][j].line_index << " Letter: " << letters[i][j].char_position << " "
             << letters[i][j].letter << " -- score: " << letters[i][j].total_score << " -- occurrences: "
             << letters[i][j].occurrences << endl;
    }
  }

  timespec permutationStartTime;
  alprsupport::getTimeMonotonic(&permutationStartTime);
  findAllPermutations(templateregion, topn);
  if (config->debugTiming) {
    timespec permutationEndTime;
    alprsupport::getTimeMonotonic(&permutationEndTime);
    cout << " -- PostProcess Permutation Time: " << alprsupport::diffclock(permutationStartTime, permutationEndTime)
         << "ms." << endl;
  }

  if (all_possibilities.size() > 0) {
    best_chars = all_possibilities[0].letters;
    for (int z = 0; z < all_possibilities.size(); z++) {
      if (all_possibilities[z].matches_template) {
        best_chars = all_possibilities[z].letters;
        break;
      }
    }

    // Now adjust the confidence scores to a percentage value
    float maxPercentScore = calculateMaxConfidenceScore();
    float highestRelativeScore = static_cast<float>(all_possibilities[0].total_score);
    if (translate_to_right_to_left) {
      // Convert the characters to RTL (specifically for Arabic output in Egypt)
      // this code will only execute if "right_to_left: true" is set in the ocr_config.json

      // All arabic numbers 0-9 (utf-8 and latin/ascii)
      const std::vector<std::string> NUMBERS = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        u8"\u0660",u8"\u0661",u8"\u0662",u8"\u0663",u8"\u0664",u8"\u0665",u8"\u0666",u8"\u0667",u8"\u0668",u8"\u0669",};
      for (uint32_t i = 0; i < all_possibilities.size(); i++) {
        std::vector<Letter> arabic_letters;
        std::vector<Letter> arabic_numbers;
        for (uint32_t j = 0; j < all_possibilities[i].letter_details.size(); j++) {
          bool is_digit = (std::find(NUMBERS.begin(), NUMBERS.end(), all_possibilities[i].letter_details[j].letter) != NUMBERS.end());
          if (is_digit)
            arabic_numbers.push_back(all_possibilities[i].letter_details[j]);
          else
            arabic_letters.push_back(all_possibilities[i].letter_details[j]);
        }

        // Now reconstruct the order based on how the UTF-8 must appear
        // Reverse the letters (RTL) and update their charpositions
        // put the numbers afterwards and leave their char positions
        all_possibilities[i].letter_details.clear();
        for (int32_t z = arabic_letters.size() - 1; z >= 0; z--) {
          arabic_letters[z].char_position = arabic_letters.size() - 1 - z;
          all_possibilities[i].letter_details.push_back(arabic_letters[z]);
        }

        for (uint32_t z = 0; z < arabic_numbers.size(); z++) {
          arabic_numbers[z].char_position = arabic_letters.size() + z;
          all_possibilities[i].letter_details.push_back(arabic_numbers[z]);
        }

        all_possibilities[i].letters = "";
        for (uint32_t j = 0; j < all_possibilities[i].letter_details.size(); j++) {
          all_possibilities[i].letters += all_possibilities[i].letter_details[j].letter;
        }
      }
    }
    if (this->config->debugPostProcess) {
      // Print top words
      for (int i = 0; i < all_possibilities.size(); i++) {
        cout << "Top " << topn << " Possibilities: " << all_possibilities[i].letters
             << " :\t" << all_possibilities[i].total_score;
        if (all_possibilities[i].letters == best_chars)
          cout << " <--- ";
        cout << endl;
      }
      cout << all_possibilities.size() << " total permutations" << endl;
    }

    for (int i = 0; i < all_possibilities.size(); i++) {
      all_possibilities[i].total_score = maxPercentScore * (all_possibilities[i].total_score / highestRelativeScore);
    }
  }

  if (this->config->debugPostProcess) {
    // Print top words
    for (int i = 0; i < all_possibilities.size(); i++) {
      cout << "Top " << topn << " Possibilities: " << all_possibilities[i].letters << " :\t"
           << all_possibilities[i].total_score;
      if (all_possibilities[i].letters == best_chars)
        cout << " <--- ";
      cout << endl;
    }
    cout << all_possibilities.size() << " total permutations" << endl;
  }

  if (config->debugTiming) {
    timespec endTime;
    alprsupport::getTimeMonotonic(&endTime);
    cout << "PostProcess Time: " << alprsupport::diffclock(startTime, endTime) << "ms." << endl;
  }

  if (this->config->debugPostProcess)
    cout << "PostProcess Analysis Complete: " << best_chars << " -- MATCH: " << matches_template << endl;
}

bool PostProcess::regionIsValid(std::string templateregion) {
  return true;
}

float PostProcess::calculateMaxConfidenceScore() {
  // Take the best score for each char position and average it.
  float total_score = 0;
  int numScores = 0;
  // Get a list of missing positions
  for (int i = 0; i < letters.size(); i++) {
    if (letters[i].size() > 0) {
      total_score += (letters[i][0].total_score / letters[i][0].occurrences) + min_confidence;
      numScores++;
    }
  }

  if (numScores == 0)
    return 0;

  return total_score / static_cast<float>(numScores);
}

const vector<PPResult> PostProcess::getResults() {
  return this->all_possibilities;
}

struct PermutationCompare {
  bool operator() (pair<float, vector<int>> &a, pair<float, vector<int>> &b) {
    return (a.first < b.first);
  }
};

void PostProcess::findAllPermutations(string templateregion, int topn) {
  // use a priority queue to process permutations in highest scoring order
  priority_queue<pair<float, vector<int>>, vector<pair<float, vector<int>>>, PermutationCompare> permutations;
  set<float> permutationHashes;

  // push the first word onto the queue
  float total_score = 0;
  for (int i=0; i < letters.size(); i++) {
    if (letters[i].size() > 0)
      total_score += letters[i][0].total_score;
  }
  vector<int> v(letters.size());
  permutations.push(make_pair(total_score, v));

  int consecutiveNonMatches = 0;
  while (permutations.size() > 0) {
    // get the top permutation and analyze
    pair<float, vector<int>> topPermutation = permutations.top();
    if (analyzePermutation(topPermutation.second, templateregion, topn) == true)
      consecutiveNonMatches = 0;
    else
      consecutiveNonMatches += 1;
    permutations.pop();

    if (all_possibilities.size() >= topn || consecutiveNonMatches >= (topn*2))
      break;

    // add child permutations to queue
    for (int i = 0; i < letters.size(); i++) {
      // no more permutations with this letter
      if (topPermutation.second[i]+1 >= letters[i].size())
        continue;

      pair<float, vector<int>> childPermutation = topPermutation;
      childPermutation.first -= letters[i][topPermutation.second[i]].total_score
                                 - letters[i][topPermutation.second[i] + 1].total_score;
      childPermutation.second[i] += 1;

      // ignore permutations that have already been visited (assume that score is a good hash for permutation)
      if (permutationHashes.end() != permutationHashes.find(childPermutation.first))
        continue;

      permutations.push(childPermutation);
      permutationHashes.insert(childPermutation.first);
    }
  }
}

bool PostProcess::analyzePermutation(vector<int> letterIndices, string templateregion, int topn) {
  PPResult possibility;
  possibility.letters = "";
  possibility.total_score = 0;
  possibility.matches_template = false;
  int plate_char_length = 0;

  int last_line = 0;
  for (int i = 0; i < letters.size(); i++) {
    if (letters[i].size() == 0)
      continue;

    Letter letter = letters[i][letterIndices[i]];
    // Add a "\n" on new lines
    if (letter.line_index != last_line) {
      possibility.letters = possibility.letters + "\n";
    }
    last_line = letter.line_index;
    if (letter.letter != SKIP_CHAR) {
      possibility.letters = possibility.letters + letter.letter;
      possibility.letter_details.push_back(letter);
      plate_char_length += 1;
    }
    possibility.total_score = possibility.total_score + letter.total_score;
  }

  // ignore plates that don't fit the length requirements
  if (plate_char_length < config->postProcessMinCharacters ||
    plate_char_length > config->postProcessMaxCharacters)
    return false;

  // ignore duplicate words
  if (all_possibilities_letters.end() != all_possibilities_letters.find(possibility.letters))
    return false;

  // If mustMatchPattern is toggled in the config and a template is provided,
  // only include this result if there is a pattern match
  if (!config->mustMatchPattern || templateregion.size() == 0 ||
      (config->mustMatchPattern && possibility.matches_template)) {
    all_possibilities.push_back(possibility);
    all_possibilities_letters.insert(possibility.letters);
    return true;
  }
  return false;
}

std::vector<string> PostProcess::getPatterns() {
  vector<string> v;
  return v;
}

bool letterCompare(const Letter &left, const Letter &right) {
  if (left.total_score < right.total_score)
    return false;
  return true;
}
}  // namespace alpr
