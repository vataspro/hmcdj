#pragma once
#include <yaml-cpp/yaml.h>

#include <format>
#include <functional>
#include <iostream>
#include <string>

/* HMCDJ ParameterBase *

  A base class that can be a component of a
  vector of Parameters. As Parameter is a
  termplate class it needs to interface with
  vector through this class.

*/
class ParameterBase {
 public:
  std::string name;

  ParameterBase(std::string nm) : name(nm) {}
  virtual ~ParameterBase() = default;

  virtual void readFromYAML(const YAML::Node& node) = 0;
  virtual std::string toString();
};

/* HMCDJ Parameter */
template <typename T>
class djParameter : public ParameterBase {
 public:
  T value;

  djParameter(std::string nm) : ParameterBase(nm) {}

  // Read a parameter of a template defined type from the YAML file
  void readFromYAML(const YAML::Node& node) override {
    if (node[name]) {
      value = node[name].template as<T>();
    } else {
      std::cerr << "Requested parameter " << name << " not in YAML keys"
                << std::endl;
      exit(-1);
    }
  }
  operator T() const { return value; };

  std::string toString() { return std::format("{}{}", name, value); }
};

// Custom type definition for djParameterList
typedef std::vector<std::reference_wrapper<ParameterBase> > djParameterList;
