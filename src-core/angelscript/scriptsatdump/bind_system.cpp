#include "bind_satdump.h"
#include <string>

#ifdef _WIN32
#include <Windows.h>

#include <TlHelp32.h>
#endif

namespace satdump
{
    namespace script
    {
        // This function calls the system command, captures the stdout and return the status
        // Return of -1 indicates an error. Else the return code is the return status of the executed command
        // ref: https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
        int ExecSystemCmd(const std::string &str, std::string &out)
        {
#ifdef _WIN32
            // Convert the command to UTF16 to properly handle unicode path names
            wchar_t bufUTF16[10000];
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, bufUTF16, 10000);

            // Create a pipe to capture the stdout from the system command
            HANDLE pipeRead, pipeWrite;
            SECURITY_ATTRIBUTES secAttr = {0};
            secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
            secAttr.bInheritHandle = TRUE;
            secAttr.lpSecurityDescriptor = NULL;
            if (!CreatePipe(&pipeRead, &pipeWrite, &secAttr, 0))
                return -1;

            // Start the process for the system command, informing the pipe to
            // capture stdout, and also to skip showing the command window
            STARTUPINFOW si = {0};
            si.cb = sizeof(STARTUPINFOW);
            si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
            si.hStdOutput = pipeWrite;
            si.hStdError = pipeWrite;
            si.wShowWindow = SW_HIDE;
            PROCESS_INFORMATION pi = {0};
            BOOL success = CreateProcessW(NULL, bufUTF16, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
            if (!success)
            {
                CloseHandle(pipeWrite);
                CloseHandle(pipeRead);
                return -1;
            }

            // Run the command until the end, while capturing stdout
            for (;;)
            {
                // Wait for a while to allow the process to work
                DWORD ret = WaitForSingleObject(pi.hProcess, 50);

                // Read from the stdout if there is any data
                for (;;)
                {
                    char buf[1024];
                    DWORD readCount = 0;
                    DWORD availCount = 0;

                    if (!::PeekNamedPipe(pipeRead, NULL, 0, NULL, &availCount, NULL))
                        break;

                    if (availCount == 0)
                        break;

                    if (!::ReadFile(pipeRead, buf, sizeof(buf) - 1 < availCount ? sizeof(buf) - 1 : availCount, &readCount, NULL) || !readCount)
                        break;

                    buf[readCount] = 0;
                    out += buf;
                }

                // End the loop if the process finished
                if (ret == WAIT_OBJECT_0)
                    break;
            }

            // Get the return status from the process
            DWORD status = 0;
            GetExitCodeProcess(pi.hProcess, &status);

            CloseHandle(pipeRead);
            CloseHandle(pipeWrite);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            return status;
#else
            // TODO: Implement suppor for ExecSystemCmd(const string &, string&) on non-Windows platforms
            asIScriptContext *ctx = asGetActiveContext();
            if (ctx)
                ctx->SetException("Oops! This is not yet implemented on non-Windows platforms. Sorry!\n");
            return -1;
#endif
        }

        // This function simply calls the system command and returns the status
        // Return of -1 indicates an error. Else the return code is the return status of the executed command
        int ExecSystemCmd(const std::string &str)
        {
            // Check if the command line processor is available
            if (system(0) == 0)
            {
                asIScriptContext *ctx = asGetActiveContext();
                if (ctx)
                    ctx->SetException("Command interpreter not available\n");
                return -1;
            }

#ifdef _WIN32
            // Convert the command to UTF16 to properly handle unicode path names
            wchar_t bufUTF16[10000];
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, bufUTF16, 10000);
            return _wsystem(bufUTF16);
#else
            return system(str.c_str());
#endif
        }

        void registerSystem(asIScriptEngine *engine)
        {
            engine->RegisterGlobalFunction("int exec(const string &in)", asFUNCTIONPR(ExecSystemCmd, (const std::string &), int), asCALL_CDECL);
            engine->RegisterGlobalFunction("int exec(const string &in, string &out)", asFUNCTIONPR(ExecSystemCmd, (const std::string &, std::string &), int), asCALL_CDECL);
        }
    } // namespace script
} // namespace satdump