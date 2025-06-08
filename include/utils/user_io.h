#ifndef USER_IO_H_
#define USER_IO_H_

#include <string>
#include <vector>

namespace UserIO {

void ClearTerminal();

char ReadRawKey();

std::string ReadHiddenInput();

std::string DisplayTitle(const std::string& message,
                         bool print = true);  // <<< Title >>>

std::string DisplayMinTitle(const std::string& message,
                            bool print = true);  // --- MinTitle ---

std::string DisplayMaxTitle(const std::string& message,
                            bool print = true);  // === MaxTitle ===

void DisplayMenu(const std::string& header,
                 const std::vector<std::string>& options,
                 const std::string& footer = "", bool index_mode = true,
                 int active_option = 0);

int HandleMenuWithInput(const std::string& header,
                        const std::vector<std::string>& options =
                            std::vector<std::string>(1, "Back..."),
                        const std::string& footer = "Choice");

int HandleMenuWithSelect(const std::string& header,
                         const std::vector<std::string>& options =
                             std::vector<std::string>(1, "Back..."),
                         const std::string& footer = "");

void ClearPreviousMenuLines(int count);

int CountMenuLines(const std::string& header,
                   const std::vector<std::string>& options,
                   const std::string& footer);

}  // namespace UserIO

#endif  // USER_IO_H_
