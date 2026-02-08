#pragma once

#include "types.h"

class IBackground {
public:
  virtual Color get_value(Ray ray) = 0;
};

class FuncBackground : public IBackground {
  using ColorFunc = std::function<Color(Ray)>;

public:
  FuncBackground(ColorFunc &&backgroud_function)
      : _backgroudFunction(std::move(backgroud_function)) {}
  FuncBackground() : FuncBackground(BlackBackground) {}

private:
  ColorFunc _backgroudFunction;

  // constants func
  static Color BlackBackground(Ray r) { return BLACK; };
};

// TODO HDRBackground