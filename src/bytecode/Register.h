#pragma once

#include <iostream>
#include <limits>
#include <stdint.h>

class Register {
public:
  static Register forLocal(uint32_t);
  static Register forParameter(uint32_t);
  static Register invalid();

  int32_t offset() const {
      ASSERT(isValid(), "Trying to get offset of invalid register");
      return m_offset;
  }

  bool isLocal() const { return m_offset < 0; }
  bool isValid() const { return m_offset != s_invalidOffset; }

  bool operator==(Register);

  void dump(std::ostream& out, unsigned = 0) const
  {
      if (!isValid())
          out << "<invalid>";
      else if (isLocal())
          out << "loc" << -m_offset;
      else
          out << "arg" << m_offset - 1;
  }

  friend std::ostream& operator<<(std::ostream& out, const Register& reg)
  {
      reg.dump(out);
      return out;
  }

private:
  static constexpr int32_t s_invalidOffset = std::numeric_limits<int32_t>::max();

  explicit Register(int32_t);

  int32_t m_offset;
};
