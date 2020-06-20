#pragma once

#include "libvig_access.h"
#include "constraint.h"
#include "tokens.h"

#include <vector>

namespace ParallelSynthesizer {

class Parser {
  
private:
  std::vector<LibvigAccess>  accesses;
  std::vector<RawConstraint> raw_constraints;

private:
  LibvigAccess& get_or_push_unique_access(const LibvigAccess& access);
  void push_unique_raw_constraint(const RawConstraint& raw_constraint);

  std::istringstream consume_token(std::string& line, const std::string& token);
  void parse_access(std::vector<std::string>& state_content);
  void parse_constraint(std::vector<std::string>& state_content);

public:
  const std::vector<LibvigAccess>& get_accesses() const { return accesses; }

  const std::vector<RawConstraint>&
    get_raw_constraints() const { return raw_constraints; }

  void parse(std::string filepath);
};

}