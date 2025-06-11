#ifndef GUI_USERIO_H_
#define GUI_USERIO_H_

#include "userio.h"

class GUIUserIO : public UserIO {
 public:
  void Print(const std::string& message) override;
};

#endif  // GUI_USERIO_H_