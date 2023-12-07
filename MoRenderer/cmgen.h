#ifndef CMGEN_H
#define CMGEN_H
#include <string>
#include <tchar.h>      
#pragma comment(lib, "Setupapi.lib")


inline void ExecuteProcess(const std::string& command_str)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	const LPTSTR command = _tcsdup(command_str.c_str());

	if (!CreateProcess(
		nullptr,		// No module name (use command line)
		command,		// Command line
		nullptr,		// Process handle not inheritable
		nullptr,		// Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		nullptr,        // Use parent's environment block
		nullptr,        // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)			// Pointer to PROCESS_INFORMATION structure
		)
	{
		std::cout << "Ö´ÐÐÃüÁîÊ§°Ü£º" + command_str << std::endl;
		exit(EXIT_FAILURE);
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

void GenerateCubeMap() {

	std::string cmgen_path = "C:/WorkSpace/MoRenderer/tools/cmgen.exe";
	std::string file = "C:/WorkSpace/MoRenderer/assets/spruit_sunrise/spruit_sunrise.hdr";
	std::string output_path = "C:/WorkSpace/MoRenderer/assets/spruit_sunrise";

	std::string irradiance_command = cmgen_path + " --format=hdr --ibl-irradiance=" + output_path + " " + file;
	std::string specular_command = cmgen_path + " --format=hdr --size=512 --ibl-ld=" + output_path + " " + file;
	std::string lut_command = cmgen_path + " --ibl-dfg=" + output_path + "/spruit_sunrise/brdf_lut.hdr";

	std::cout << lut_command << std::endl;

	ExecuteProcess(irradiance_command);
	ExecuteProcess(specular_command);
	ExecuteProcess(lut_command);

}

#endif // !CMGEN_H

