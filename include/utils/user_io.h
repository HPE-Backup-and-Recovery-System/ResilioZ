#ifndef USER_IO_H_
#define USER_IO_H_

#include <string>
#include <vector>

namespace UserIO {

std::string GetHiddenInput();

std::string DisplayTitle(const std::string& message,
                         bool print = true);  // <<< Title >>>

std::string DisplayMinTitle(const std::string& message,
                            bool print = true);  // --- MinTitle ---
                            
std::string DisplayMaxTitle(const std::string& message,
                            bool print = true);  // === MaxTitle ===

void DisplayMenu(const std::string& menu_header,
                 const std::vector<std::string>& menu_options,
                 const std::string& menu_footer);

int HandleMenuWithInput(const std::string& menu_header,
                        const std::vector<std::string>& menu_options =
                            std::vector<std::string>(1, "Back..."),
                        const std::string& menu_footer = "Choice");

int HandleMenuWithSelect(const std::string& menu_header,
                         const std::vector<std::string>& menu_options =
                             std::vector<std::string>(1, "Back..."),
                         const std::string& menu_footer = "");

}  // namespace UserIO

#endif  // USER_IO_H_
