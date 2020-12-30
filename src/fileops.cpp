#define WIN32_LEAN_AND_MEAN // Disables inclusion of many large headers
#define _WIN32_WINNT 0x0601 // Defines the version of Windows for <windows.h>
#define UNICODE

// clang-format off
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
// clang-format on

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Shell32.lib")

/**
 * Print the CLI usage info
 */
void printUsage() {
  std::cout << "\n"
            << "usage: (action is one of: copy, move, delete)" << std::endl;
  std::cout << "  FileOps.exe <action> --from <sourcePath> [sourcePath]* --to "
               "<directoryPath>"
            << std::endl;
  std::cout << "  FileOps.exe <action> --from <sourcePath> [sourcePath]* --to "
               "<destPath> [destPath]*"
            << std::endl;
}

/**
 * Check the given inputs and print an error if they're not valid
 */
bool inputIsValid(const std::string &action,
                  const std::vector<std::string> &srcPaths,
                  const std::vector<std::string> &destPaths) {
  if (action == "") {
    std::cout << "error: action is required" << std::endl;
    printUsage();
    return false;
  }

  if (action != "copy" && action != "move" && action != "delete") {
    std::cout << "error: action must be one of: copy, move, delete"
              << std::endl;
    printUsage();
    return false;
  }

  if (srcPaths.size() == 0) {
    std::cout << "at least one source path is required" << std::endl;
    printUsage();
    return false;
  }

  if (action == "delete") {
    if (destPaths.size() > 0) {
      std::cout
          << "error: cannot specify destination path when action is delete"
          << std::endl;
      printUsage();
      return false;
    }
  } else {
    if (destPaths.size() == 0) {
      std::cout << "error: at least one destination path is required when "
                   "action is not delete"
                << std::endl;
      printUsage();
      return false;
    }
  }

  if (destPaths.size() > srcPaths.size()) {
    std::cout << "error: number of destination paths cannot be more than "
                 "number of source paths"
              << std::endl;
    printUsage();
    return false;
  }

  if (srcPaths.size() > 1 && destPaths.size() > 1 &&
      srcPaths.size() != destPaths.size()) {
    std::cout << "error: number of source and destination paths must match "
                 "when more than one destination path is specified"
              << std::endl;
    printUsage();
    return false;
  }

  return true;
}

/**
 * Convert an std::string to LPCWSTR
 * See https://stackoverflow.com/a/27296
 */
LPWSTR stringToLpwstr(const std::string &str) {
  int stringLength = (int)str.length() + 1;

  // The first call with a 0 target string returns the buffer length needed
  int bufferLength = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 0, 0);

  wchar_t *buffer = new wchar_t[bufferLength];

  // The second call actually does the conversion
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer, bufferLength);

  return buffer;
}

/**
 * Get the error string for the given Windows error code
 */
std::string getErrorAsString(DWORD errorCode) {
  { // The errors below are specific to SHFileOperation, and override
    // errors in winerror.h, which are handled below
    std::map<DWORD, std::string> errors;

    errors[0x71] = "The source and destination files are the same file.";
    errors[0x72] = "Multiple file paths were specified in the source buffer, "
                   "but only one destination file path.";
    errors[0x73] = "Rename operation was specified but the destination path is "
                   "a different directory. Use the move operation instead.";
    errors[0x74] =
        "The source is a root directory, which cannot be moved or renamed.";
    errors[0x75] =
        "The operation was canceled by the user, or silently canceled if the "
        "appropriate flags were supplied to SHFileOperation.";
    errors[0x76] = "The destination is a subtree of the source.";
    errors[0x78] = "Security settings denied access to the source.";
    errors[0x79] =
        "The source or destination path exceeded or would exceed MAX_PATH.";
    errors[0x7A] = "The operation involved multiple destination paths, which "
                   "can fail in the case of a move operation.";
    errors[0x7C] = "The path in the source or destination or both was invalid.";
    errors[0x7D] = "The source and destination have the same parent folder.";
    errors[0x7E] = "The destination path is an existing file.";
    errors[0x80] = "The destination path is an existing folder.";
    errors[0x81] = "The name of the file exceeds MAX_PATH.";
    errors[0x82] =
        "The destination is a read-only CD-ROM, possibly unformatted.";
    errors[0x83] = "The destination is a read-only DVD, possibly unformatted.";
    errors[0x84] =
        "The destination is a writable CD-ROM, possibly unformatted.";
    errors[0x85] = "The file involved in the operation is too large for the "
                   "destination media or file system.";
    errors[0x86] = "The source is a read-only CD-ROM, possibly unformatted.";
    errors[0x87] = "The source is a read-only DVD, possibly unformatted.";
    errors[0x88] = "The source is a writable CD-ROM, possibly unformatted.";
    errors[0xB7] = "MAX_PATH was exceeded during the operation.";
    errors[0x402] = "An unknown error occurred. This is typically due to an "
                    "invalid path in the source or destination. This error "
                    "does not occur on Windows Vista and later.";
    errors[0x10000] = "An unspecified error occurred on the destination.";
    errors[0x10074] = "Destination is a root directory and cannot be renamed.";

    if (errors.find(errorCode) != errors.end()) {
      return errors[errorCode];
    }
  }

  LPSTR messageBuffer = NULL;

  size_t size = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPSTR)&messageBuffer, 0, NULL);

  std::string message(messageBuffer, size);

  LocalFree(messageBuffer);

  return message;
}

/**
 * Handle the exit status of the file operation based on the given input
 */
void handleStatus(int opReturnCode, bool wasAborted, std::string action,
                  bool showErrorDialog) {
  // Handle user cancellation of the operation
  if (wasAborted || opReturnCode == ERROR_CANCELLED) {
    std::cout << "cancelled" << std::endl;
    return;
  }

  // Handle successful operation
  if (opReturnCode == 0) {
    std::cout << "ok" << std::endl;
    return;
  }

  // Convert the error code to hex format
  std::stringstream sstream;
  sstream << "0x" << std::hex << opReturnCode;
  std::string errorHex = sstream.str();

  // Get the error message string
  std::string errorMessage = getErrorAsString(opReturnCode);

  // Show an error dialog if allowed
  if (showErrorDialog) {
    std::string caption =
        "Unable to " + action + " files (ERR " + errorHex + ")";

    LPWSTR lpCaption = stringToLpwstr(caption);
    LPWSTR lpText = stringToLpwstr(errorMessage);

    if (action == "copy") {
      MessageBox(0, lpText, lpCaption, MB_ICONWARNING);
    } else if (action == "move") {
      MessageBox(0, lpText, lpCaption, MB_ICONWARNING);
    } else if (action == "delete") {
      MessageBox(0, lpText, lpCaption, MB_ICONWARNING);
    }

    delete[] lpCaption;
    delete[] lpText;
  }

  // Print the error
  std::cout << "error " << errorHex << ": " << errorMessage << std::endl;
}

/**
 * Combine the given file names into a single LPWSTR string, with a null
 * terminator character used as separator, with double null terminators at the
 * end of the string.
 * All this to create a string for pFrom and pTo in the SHFILEOPSTRUCT:
 *   https://docs.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-shfileopstructw#members
 */
LPWSTR combileFileNames(const std::vector<std::string> &files) {
  // Combine the file names into an std::string with '\t' as separator
  std::string combinedStr = "";
  for (std::string file : files) {
    combinedStr += file + "\t";
  }

  // Convert the combined string to LPWSTR
  LPWSTR combined = stringToLpwstr(combinedStr);

  // Replace each '\t' separator with a null terminator '\0'
  int length = wcslen(combined);
  for (int i = 0; i < length; i++) {
    if (combined[i] == L'\t') {
      combined[i] = L'\0';
    }
  }

  return combined;
}

/**
 * Perform the file operation with the given input
 */
int performFileOperation(const std::string &action,
                         const std::vector<std::string> &srcPaths,
                         const std::vector<std::string> &destPaths,
                         bool showErrorDialog) {
  SHFILEOPSTRUCTW op;

  // Set the file flags
  op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR | FOF_WANTNUKEWARNING;
  if (destPaths.size() > 1) {
    op.fFlags = op.fFlags | FOF_MULTIDESTFILES;
  }

  // Set the source
  LPWSTR pFrom = combileFileNames(srcPaths);
  op.pFrom = pFrom;

  // Set the destination
  LPWSTR pTo = combileFileNames(destPaths);
  op.pTo = pTo;

  // Set the action
  if (action == "copy") {
    op.wFunc = FO_COPY;
  } else if (action == "move") {
    op.wFunc = FO_MOVE;
  } else if (action == "delete") {
    op.wFunc = FO_DELETE;
  }

  int status = SHFileOperationW(&op);

  // Handle any possible errors
  handleStatus(status, op.fAnyOperationsAborted, action, showErrorDialog);

  delete[] pFrom;
  delete[] pTo;

  return status;
}

/**
 * The CLI entry point
 */
int main(int argc, char *argv[]) {
  bool showErrorDialog = false;
  std::string action = "";
  std::vector<std::string> srcPaths;
  std::vector<std::string> destPaths;

  std::string currentlyProcessing = "action";

  for (int i = 1; i < argc; i++) {
    std::string arg(argv[i]);

    if (arg == "--from") {
      currentlyProcessing = "from";
      continue;
    } else if (arg == "--to") {
      currentlyProcessing = "to";
      continue;
    } else if (arg == "--show-errors") {
      showErrorDialog = true;
      continue;
    } else if (arg.rfind("--", 0) == 0) {
      // An unknown arg starting with --, ignore
      continue;
    }

    if (currentlyProcessing == "action") {
      action = arg;
    } else if (currentlyProcessing == "from") {
      srcPaths.push_back(std::string(argv[i]));
      continue;
    } else if (currentlyProcessing == "to") {
      destPaths.push_back(std::string(argv[i]));
      continue;
    }
  }

  if (!inputIsValid(action, srcPaths, destPaths)) {
    return 1;
  }

  return performFileOperation(action, srcPaths, destPaths, showErrorDialog);
}
