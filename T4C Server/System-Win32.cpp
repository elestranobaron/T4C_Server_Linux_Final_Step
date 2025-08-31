#include "System.h"
#include <unistd.h>
#include <limits.h>
#include <sys/utsname.h>
#include <filesystem>
#include <cstdlib>

std::string System::GetMachineName()
{
        char machine_name[256]; // Adjusted from MAX_COMPUTERNAME_LENGTH
        if (gethostname(machine_name, sizeof(machine_name)) == 0)
                        return machine_name;
                else
                        return "Unamed Machine";
}

std::string System::GetProgramPath()
{
        char path[PATH_MAX];
        char resolved_path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", path, PATH_MAX - 1);
        if (len != -1) {
                path[len] = '\0';
                if (realpath(path, resolved_path)) {
                        std::filesystem::path p(resolved_path);
                        return p.parent_path().string() + "/";
                }
        }
        return "./"; // Fallback
}

std::string System::MakePath(const char *path)
{
        int path_length = strlen(path);
        std::string pathstr(GetProgramPath());
        char absPath[PATH_MAX];
        if (realpath(path, absPath))
        {
                pathstr = absPath;
        }
        if (pathstr[pathstr.length() -1] != '/')
                pathstr += '/';
        return pathstr;
}