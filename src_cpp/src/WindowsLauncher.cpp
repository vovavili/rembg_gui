#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <shellapi.h>

#include <string>
#include <string_view>
#include <vector>

namespace {

std::wstring modulePath()
{
  std::wstring path(260, L'\0');
  for(;;) {
    const auto length =
      GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if(length == 0) {
      return {};
    }
    if(length < path.size()) {
      path.resize(length);
      return path;
    }
    path.resize(path.size() * 2);
  }
}

std::wstring parentDirectory(std::wstring_view path)
{
  const auto separator = path.find_last_of(L"\\/");
  if(separator == std::wstring_view::npos) {
    return L".";
  }
  return std::wstring(path.substr(0, separator));
}

std::wstring joinPath(std::wstring_view left, std::wstring_view right)
{
  if(left.empty()) {
    return std::wstring(right);
  }

  std::wstring path(left);
  if(path.back() != L'\\' && path.back() != L'/') {
    path.push_back(L'\\');
  }
  path.append(right);
  return path;
}

bool fileExists(std::wstring_view path)
{
  const auto attributes = GetFileAttributesW(std::wstring(path).c_str());
  return attributes != INVALID_FILE_ATTRIBUTES &&
         (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring quoteCommandLineArgument(std::wstring_view argument)
{
  if(!argument.empty() && argument.find_first_of(L" \t\n\v\"") == std::wstring_view::npos) {
    return std::wstring(argument);
  }

  std::wstring quoted;
  quoted.push_back(L'"');

  auto backslashes = std::size_t{0};
  for(const auto character : argument) {
    if(character == L'\\') {
      ++backslashes;
      continue;
    }

    if(character == L'"') {
      quoted.append((backslashes * 2) + 1, L'\\');
      quoted.push_back(character);
      backslashes = 0;
      continue;
    }

    quoted.append(backslashes, L'\\');
    backslashes = 0;
    quoted.push_back(character);
  }

  quoted.append(backslashes * 2, L'\\');
  quoted.push_back(L'"');
  return quoted;
}

std::wstring formatWindowsError(DWORD errorCode)
{
  wchar_t* rawMessage = nullptr;
  const auto length = FormatMessageW(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
    nullptr,
    errorCode,
    0,
    reinterpret_cast<wchar_t*>(&rawMessage),
    0,
    nullptr);

  std::wstring message =
    length == 0 ? L"Unknown Windows error." : std::wstring(rawMessage, length);
  if(rawMessage != nullptr) {
    LocalFree(rawMessage);
  }

  while(!message.empty() &&
        (message.back() == L'\r' || message.back() == L'\n' || message.back() == L' ')) {
    message.pop_back();
  }
  return message;
}

bool launcherMessageBoxesAreDisabled()
{
  const auto required =
    GetEnvironmentVariableW(L"REMBG_GUI_LAUNCHER_SILENT", nullptr, 0);
  if(required == 0) {
    return false;
  }

  std::wstring value(required, L'\0');
  const auto length = GetEnvironmentVariableW(
    L"REMBG_GUI_LAUNCHER_SILENT", value.data(), static_cast<DWORD>(value.size()));
  return length > 0 && value.front() == L'1';
}

int failWithMessage(std::wstring_view message)
{
  if(!launcherMessageBoxesAreDisabled()) {
    MessageBoxW(nullptr, std::wstring(message).c_str(), L"Rembg GUI", MB_ICONERROR | MB_OK);
  }
  return 1;
}

std::wstring buildChildCommandLine(std::wstring_view childPath)
{
  auto commandLine = quoteCommandLineArgument(childPath);

  auto argc = 0;
  const auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if(argv == nullptr) {
    return commandLine;
  }

  for(auto index = 1; index < argc; ++index) {
    commandLine.push_back(L' ');
    commandLine.append(quoteCommandLineArgument(argv[index]));
  }

  LocalFree(argv);
  return commandLine;
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
  const auto launcherPath = modulePath();
  if(launcherPath.empty()) {
    return failWithMessage(L"Rembg GUI could not find its install folder.");
  }

  const auto bundleRoot = parentDirectory(launcherPath);
  const auto childPath = joinPath(bundleRoot, L"bin\\rembg-gui-app.exe");

  if(!fileExists(childPath)) {
    return failWithMessage(
      L"Rembg GUI could not find bin\\rembg-gui-app.exe.\n\n"
      L"Extract the whole zip file first, then run rembg-gui.exe from the "
      L"extracted folder.\n\n"
      L"Do not move rembg-gui.exe away from the bin, Qt6, and translations folders.");
  }

  auto commandLine = buildChildCommandLine(childPath);
  std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
  mutableCommandLine.push_back(L'\0');

  STARTUPINFOW startupInfo{};
  startupInfo.cb = sizeof(startupInfo);
  PROCESS_INFORMATION processInfo{};

  const auto started = CreateProcessW(
    childPath.c_str(),
    mutableCommandLine.data(),
    nullptr,
    nullptr,
    FALSE,
    0,
    nullptr,
    bundleRoot.c_str(),
    &startupInfo,
    &processInfo);

  if(started == FALSE) {
    return failWithMessage(
      L"Rembg GUI could not start bin\\rembg-gui-app.exe.\n\n" +
      formatWindowsError(GetLastError()));
  }

  const auto waitResult = WaitForSingleObject(processInfo.hProcess, INFINITE);
  auto exitCode = DWORD{1};
  if(waitResult == WAIT_OBJECT_0) {
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
  } else {
    failWithMessage(L"Rembg GUI lost track of the app process.\n\n" +
                    formatWindowsError(GetLastError()));
  }

  CloseHandle(processInfo.hThread);
  CloseHandle(processInfo.hProcess);
  return static_cast<int>(exitCode);
}
