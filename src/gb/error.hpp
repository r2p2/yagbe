#pragma once

#include <string>

class Error
{
public:
  enum class Code {
    None,
    RomNotSupported,
  };

  Error() = default;
  Error(Error const&) = default;
  Error(Error&&) = default;
  
  explicit Error(Code code)
    : _code(code)
  {}

  ~Error() = default;

  Error& operator=(Error const&) = default;
  Error& operator=(Error&&) = default;

  static Error NoError()
  {
    static Error error;
    return error;
  }

  bool is_set() const
  {
    return _code != Code::None;
  }

  std::string text() const
  {
    switch (_code) {
    case Code::None:
      return "No error";
    case Code::RomNotSupported: 
      return "Rom is not supported.";
    default:
      return "No error text specified.";
    }
  }

private:
  Code const _code = Code::None;
};
